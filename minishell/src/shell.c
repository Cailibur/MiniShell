#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h> 
#include <stdlib.h>
#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "tokenizer.h"
#include "redirection.h"
#include "color.h"
#include "job.h"

// 4.1
int redirect_output(struct Command *cmd) {
    int saved_stdout = -1;

    if (cmd->output_file != NULL) {
        int fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            perror("open");
            return -1;
        }

        saved_stdout = dup(STDOUT_FILENO);  // 备份终端输出
        if (saved_stdout < 0) {
            perror("dup");
            close(fd);
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {  // 把标准输出改到文件
            perror("dup2");
            close(fd);
            close(saved_stdout);
            return - 1;
        }

        close (fd);
    }

    return saved_stdout;    // 返回终端输出备份
}

// 4.1 
void restore_output(int saved_stdout) { // 不影响提示符输出 MiniShell> 
    if (saved_stdout >= 0) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
}


void shell_loop(void) {
    char line[MAX_LINE];    // #define MAX_LINE 1024
    char work_line[MAX_LINE];

    struct Command cmd;     // 存解析后的命令参数（argc + argv参数）

    // 2.1 - 保存历史命令
    char history[MAX_HISTORY][MAX_LINE];
    int history_count = 0;

    while (1) {
        // 4.0
        struct Token tokens[MAX_TOKENS];
        int token_count;
        
        // 1.2 - 显示提示符 "MiniShell> ""
        printf(COLOR_GREEN STYLE_BOLD "MiniShell> " STYLE_RESET);
        fflush(stdout); // 强制刷新标准输出缓冲区（把缓冲区未显示的内容立刻打印）

        // 1.2 - 读取整行输入
        // fgets() 会读取末尾回车\n
        if (fgets(line, sizeof(line), stdin) == NULL) { // fgets 读取一整行直到换行/越界
            printf("\n");
            break;  // Ctrl + D, fgets() == NULL, break 跳出循环
        }

        // 1.2 - 去除结尾换行
        line[strcspn(line, "\n")] = '\0';
        // strcspn() 首个 \n 所在位置下标 将 line[] 替换为 \0

        strcpy(work_line, line);

        // 跳过空行
        if (line[0] == '\0') {
            continue;
        }

        // 2.1 - 每输入一条非空命令，先存进 history
        if (history_count < MAX_HISTORY) {
            strcpy(history[history_count], line);
            history_count++;
        }

        // 1.3 - 解析命令
        /*
        if (!parse_line(line, &cmd)) {  // 跳过无效参数
            continue;
        }
        */

        // 4.0
        if (!tokenize_line(work_line, tokens, &token_count)) {
            continue;
        }

        // 管道
        int has_pipe = 0;
        for (int i = 0; i < token_count; i++) {
            if (tokens[i].type == TOK_PIPE) {
                has_pipe = 1;
                break;
            }
        }
        // if 有管道
        if (has_pipe) {
            execute_pipeline(tokens, token_count, history, history_count);
            continue;
        }

        if (!parse_tokens(tokens, token_count, &cmd)) {
            fprintf(stderr, "parse error\n");
            continue;
        }


        // 1.5 - 先判断是不是内置命令
        if (is_builtin(&cmd)) {
            struct RedirectBackup backup;

            if  (apply_redirection(&cmd, &backup) != 0) {
                continue;
            }
            if(cmd.background == 1){
                pid_t pid = fork();
                if(pid < 0){
                    perror("fork");
                }
                else if(pid == 0){
                    handle_builtin(&cmd, history, history_count);
                    exit(1);
                }
                else{
                    add_job(pid, line);
                }
            }
            else{
                handle_builtin(&cmd, history, history_count);
            }
            restore_redirection(&backup);
            continue;
        }

        // 1.4 - 不是内置命令，就按外部命令执行
        execute_command(&cmd);  // fork + execvp 执行外部命令

        // 1.1.5 调试
        /*
        printf("你输入的是：%s\n", line); 
        */

        // 1.2.4 调试
        /* printf("argc = %d\n", cmd.argc);    // 打印总参数个数 argc
        for (int i = 0; i < cmd.argc; i++) {    // 循环打印每个参数 argv[i] 内容
            printf("argv[%d] = %s\n", i, cmd.argv[i]);
        }
        */
    }
}