#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h> 
#include <stdlib.h>
#include <time.h>
#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "builtins.h"
#include "tokenizer.h"
#include "redirection.h"
#include "color.h"
#include "job.h"
#include "signals.h"

char line[MAX_LINE];    // #define MAX_LINE 1024

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

void write_history(const char *command) {
    FILE *history_file = fopen(".myshell_history", "a");
    if (history_file == NULL) {
        perror("fopen");
        return;
    }
    struct tm *cur_time;
    time_t raw_time;
    time(&raw_time);
    cur_time = localtime(&raw_time);
    char buffer[80];
    strftime(buffer, 80, "%Y年%m月%d日 %H:%M:%S", cur_time);
    fprintf(history_file, "%s %s", buffer, command);
    fclose(history_file);
}

void shell_loop(void) {
    
    char work_line[MAX_LINE];

    struct Command cmd;     // 存解析后的命令参数（argc + argv参数）

    // 2.1 - 保存历史命令
    char history[MAX_HISTORY][MAX_LINE];
    int history_count = 0;

    while (1) {
        init_signal_handlers();
        // 4.0
        struct Token tokens[MAX_TOKENS];
        int token_count;
        
        char cwd[MAX_LINE];
        printf(COLOR_GREEN STYLE_BOLD "MiniShell:" STYLE_RESET);
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf(COLOR_BBLUE STYLE_BOLD "~%s", cwd);
            printf(STYLE_RESET "$ ");
        }

        fflush(stdout); // 强制刷新标准输出缓冲区（把缓冲区未显示的内容立刻打印）

        // 1.2 - 读取整行输入
        // fgets() 会读取末尾回车\n
        if (fgets(line, sizeof(line), stdin) == NULL) { // fgets 读取一整行直到换行/越界
            printf("\n");
            break;  // Ctrl + D, fgets() == NULL, break 跳出循环
        }

        write_history(line);

        // 1.2 - 去除结尾换行
        line[strcspn(line, "\n")] = '\0';
        // strcspn() 首个 \n 所在位置下标 将 line[] 替换为 \0

        //printf("%d\n", str_hash(line));
        if(find_alias(line) != NULL){
            strcpy(line, find_alias(line));
        }

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

        if(cmd.argc == 1 && find_alias(cmd.argv[0]) != NULL){
            strcmp(line, find_alias(cmd.argv[0]));
            continue;
        }

        // 1.4 - 不是内置命令，就按外部命令执行
        execute_command(&cmd);  // fork + execvp 执行外部命令

    }
}