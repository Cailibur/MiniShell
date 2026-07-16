#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>   // 3.1 - mkdir
#include <fcntl.h>      // 3.3 - open() / O_WRONLY / O_CREAT
#include <utime.h>      // 3.3 - utime()
#include <dirent.h>     // 3.4 - DIR / struct dirent / opendir() / readdir() / closedir()
#include "builtins.h"
#include "job.h"

// 2.4 - 'cd -' 记录上一次工作目录
static char oldpwd[MAX_LINE] = "";

// 2.6 - export
extern char **environ;

// 2.8 - alias 哈希表
/*
static struct AliasNode *alias_table[ALIAS_TABLE_SIZE] = {NULL};

unsigned int has_alias(const char *str) {
    unsigned int hash = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        hash = hash * 31 + (unsigned char)str[i];
    }

    return hash % ALIAS_TABLE_SIZE;
}

struct AliasNode *find_alias_node(const char *name) {
    unsigned int index = hash_alias(name);

    for (struct AliasNode *cur = alias_table[index]; cur != NULL; cur = cur->next) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
    }

    return NULL;
}

void set_alias(const char *name, const char *value) {
    unsigned int index = hash_alias(name);
    struct AliasNode *node = find_alias_node(name);
}
*/

int is_command(struct Command *cmd, const char *target_name) {
    // 空指针保护
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return 0;
    }
    return strcmp(cmd->argv[0], target_name) == 0;
}

int is_builtin(struct Command *cmd) {
    return is_command(cmd, "pwd") ||
           is_command(cmd, "clear") ||
           is_command(cmd, "history") ||
           is_command(cmd, "echo") ||
           is_command(cmd, "sleep") ||
           is_command(cmd, "cd") ||
           is_command(cmd, "exit") ||
           is_command(cmd, "export") ||
           is_command(cmd, "unset") ||
           is_command(cmd, "mkdir") ||
           is_command(cmd, "rm") ||
           is_command(cmd, "touch") ||
           is_command(cmd, "ls") ||
           is_command(cmd, "jobs");
}

int handle_builtin(struct Command *cmd, char history[][MAX_LINE], int history_count) {

    // 1.5 - 内置 pwd
    if (is_command(cmd, "pwd")) {
        char cwd[MAX_LINE];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("pwd");
        } else {
            printf("%s\n", cwd);
        }

        return 1;
    }

    // 1.5 - 内置 clear
    if (is_command(cmd, "clear")) {
        printf("\033[2J\033[H");    // ANSI 转义序列，清屏并把光标移回左上角
        
        return 1;
    }

    // 2.1 - history 输出历史命令
    if (is_command(cmd, "history")) {
        for (int i = 0; i < history_count; i++) {
            printf("%d %s\n", i + 1, history[i]);
        }

        return 1;
    }

    // 2.2 - echo / echo -n 打印命令
    if (is_command(cmd, "echo")) {
        int start = 1;
        int need_newline = 1;

        //如果发现 -n，那就从第 2 个参数开始打印，换行标记为 0
        if (cmd->argc > 1 && strcmp(cmd->argv[1], "-n") == 0) {
            start = 2;
            need_newline = 0;
        }

        // 每个参数之间补一个空格
        for (int i = start; i < cmd->argc; i++) {
            printf("%s", cmd->argv[i]);
            if (i != cmd->argc - 1) {
                printf(" ");
            }
        }

        // 默认最后换行（标记为 1），如果用了 -n（标记为 0），最后不换行
        if (need_newline) {
            printf("\n");
        }

        return 1;
    }

    // 2.3 - sleep 休眠并检查非法参数
    if (is_command(cmd, "sleep")) {
        // 1.没传参数，报错
        if (cmd->argc < 2) {
            fprintf(stderr, "sleep: missing argument\n");
        }
        // 2.有参数，先校验格式，再执行睡眠
        else {
            char *endptr;   // 标记转换停止位置，用于判断是否是完整数字
            long seconds = strtol(cmd->argv[1], &endptr, 10);   // 字符串转成 long

            // 3.校验：有非法字符 或 秒数为负，报错
            if (*endptr != '\0' || seconds < 0) {
                fprintf(stderr, "sleep: invalid time interval '%s'\n", cmd->argv[1]);
            }
            // 4.参数合法，执行睡眠
            else {
                sleep((unsigned int)seconds);
            }
        }

        return 1;
    }

    // 2.4 - cd / cd ~ / cd - / cd path
    if (is_command(cmd, "cd")) {
        char current_dir[MAX_LINE];
        char *target = NULL;

        // 切换前先保存当前目录，供 cd - 使用
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("cd");

            return 1;
        }

        // cd 或 cd ~： 回到家目录
        if (cmd->argc == 1 || strcmp(cmd->argv[1], "~") == 0) {
            target = getenv("HOME");
            if (target == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        } 
        // cd -：回到上一次目录
        else if (strcmp(cmd->argv[1], "-") == 0) {
            if (oldpwd[0] == '\0') {
                fprintf(stderr, "cd: OLDPWD not set\n");
                return 1;
            }
            target = oldpwd;
            printf("%s\n", target);
        } 
        // 普通路径：cd path
        else {
            target = cmd->argv[1];
        }

        if (chdir(target) != 0) {
            perror("cd");
        } 
        // 切换成功后，把原目录保存为 oldpwd
        else {
            strcpy(oldpwd, current_dir);
        }

        return 1;
    }

    // 2.5 - exit / exit [status]
    if (is_command(cmd, "exit")) {
        if (cmd->argc == 1) {
            exit(0);
        } else {
            char *endptr;
            long status = strtol(cmd->argv[1], &endptr, 10);

            if (*endptr != '\0') {
                fprintf(stderr, "exit: numeric argument required\n");
                exit(1);
            }

            exit((int)status);
        }
    }

    // 2.6 - export 查看或设置环境变量
    if (is_command(cmd, "export")) {
        if (cmd->argc == 1) {

            // 不带参数：输出全部环境变量
            for (int i = 0;environ[i] != NULL; i++) {
                printf("%s\n", environ[i]);
            }
        }
        else {
            // strchr(const char *str, int ch);
            // str：要搜索的字符串 ch：要找的字符 返回值：所在地址/NULL
            char *equal_sign = strchr(cmd->argv[1], '=');

            // 没有等号，格式不对，报错
            if (equal_sign == NULL) {
                fprintf(stderr, "export: invalid format\n");
            }
            else {
                char name[MAX_LINE];
                char *value;
                size_t name_len = equal_sign - cmd->argv[1];   // 指针减法，size_t更规范

                // 提取变量名
                strncpy(name, cmd->argv[1], name_len);
                name[name_len] = '\0';

                // 提取变量值
                value = equal_sign + 1;

                // 第三个参数 1 表示已存在就覆盖
                if (setenv(name, value, 1) != 0) {
                    perror("export");
                }
            }
        }

        return 1;
    }

    // 2.7 - unset
    if (is_command(cmd, "unset")) {
        if (cmd->argc < 2) {
            fprintf(stderr, "unset: missing argument\n");
        }
        else {
            if (unsetenv(cmd->argv[1]) != 0) {
                perror("unset");
            }
        }

        return 1;
    }


    // 3.1 - mkdir 创建目录，支持多参数
    if (is_command(cmd, "mkdir")) {
        if (cmd->argc < 2) {    // 无目录名，报错
            fprintf(stderr, "mkdir: missing operand\n");
        } else {
            for (int i = 1; i < cmd->argc; i++) {
                if (mkdir(cmd->argv[i], 0755) != 0) {   // 创建目录直至无参数
                    perror("mkdir");
                }
            }
        }

        return 1;
    }

    // 3.2 - rm 删除普通文件，支持多参数
    if (is_command(cmd, "rm")) {
        if (cmd->argc < 2) {    // 无文件名，报错
            fprintf(stderr, "rm: missing operand\n");
        } else {
            for (int i = 1; i < cmd->argc; i++) {   // 删除目录直至无参数
                if (unlink(cmd->argv[i]) != 0) {
                    perror("rm");
                }
            }
        }

        return 1;
    }

    // 3.3 - touch 创建文件或更新时间戳
    if (is_command(cmd, "touch")) {
        if (cmd->argc < 2) {
            fprintf(stderr, "touch: missing file operand\n");
        } else {
            for (int i = 1; i < cmd->argc; i++) {
                // 1.尝试打开/创建文件
                // O_WRONLY：只写模式打开
                // O_CREAT：文件不存在就创建它
                // 0644：新建文件的默认权限（所有者读写，其他人只读，普通文件标准权限）
                int fd = open(cmd->argv[i], O_WRONLY | O_CREAT, 0644);

                if (fd < 0) {
                    perror("touch");    // 创建/打开失败，报错
                } else {
                    close(fd);  // 创建文件完毕，关闭

                    // 2.更新文件时间戳为当前时间
                    // NULL 代表设置为当前系统时间
                    if (utime(cmd->argv[i], NULL) != 0) {
                        perror("touch");
                    }
                }
            }
        }
        return 1;
    }

    // 3.4 - 简化版 ls ，只列出文件名
    if (is_command(cmd, "ls")) {
        char *path = ".";   // . 是 Linux 系统的约定，代表当前目录本身
        DIR *dir;   // 目录流指针
        struct dirent *entry;   // 目录条目结构体，每读取一次目录就返回一个

        if (cmd->argc > 2) {    // 一次最多接受一个目录路径参数
            fprintf(stderr, "ls: too many arguments\n");
            return 1;
        }

        if (cmd->argc == 2) {
            path = cmd->argv[1];
        }

        dir = opendir(path);    // 返回 DIR* 指针，失败返回 NULL
        if (dir == NULL) {
            perror("ls");
            return 1;
        }

        for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
            // 过滤掉 . 和 .. 两个特殊目录 - . 当前目录 .. 上一级目录
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {  // d_name 文件名
                printf("%s\n", entry->d_name);
            }
        }

        printf("\n");
        closedir(dir);  // 关闭目录流，释放内存
        return 1;
    }

    if(is_command(cmd, "jobs")){
        if(cmd->argc > 2){
            fprintf(stderr, "jobs: too many arguments\n");
            return 1;
        }
        check_job();
        if(cmd->argc == 1){
            job_query();
        }
        else if(cmd->argc == 2){
            if(strcmp(cmd->argv[1], "-l") == 0){
                job_query();
            }   
            else if(strcmp(cmd->argv[1], "-r") == 0){
                job_query_on_status(JOB_RUNNING);
            }
            else if(strcmp(cmd->argv[1], "-s") == 0){
                job_query_on_status(JOB_STOPPED);
            }
            else{
                fprintf(stderr, "jobs: invalid option: %s\n", cmd->argv[1]);
            }
        }
    }

    // 不是内置命令，交给外部命令执行逻辑
    return 0;
}