#include <stdio.h>      // perror 打印错误
#include <unistd.h>     // 提供 fork execvp 系统调用
#include <sys/wait.h>   // 提供 waitpid 等待子进程
#include <fcntl.h>      // open() ... 
#include <signal.h>
#include "parser.h"
#include "builtins.h"
#include "executor.h"   // 声明头文件
#include "redirection.h"

void execute_command(struct Command *cmd) {
    // 1.4
    // 创建子进程 + 程序替换
    pid_t pid = fork();  // pid_t 为 Linux 中专门存放进程 ID 的类型，本质 int
    // 复制一个子进程

    // fork 失败
    if (pid < 0) {  // usually pid = -1
        perror("fork");
        return;
    }

    // 子进程：执行真正的外部命令
    if (pid == 0) { // 当前在子进程中
        struct RedirectBackup backup;
        if (apply_redirection(cmd, &backup) != 0) {
            _exit(1);
        }
        execvp(cmd->argv[0], cmd->argv);    // 成功，后面代码不运行
        perror("execvp");   // execvp 失败才到这里
        _exit(1);
        
    } else {    
        // 父进程；值是子进程 PID
        waitpid(pid, NULL, 0);  // 等子进程执行完后再显示提示符
    }
}

void execute_pipeline(struct Token tokens[], int token_count,
                 char history[][MAX_LINE], int history_count) {
    struct Command cmds[16];
    int cmd_count = 0;
    int start = 0;

    // 1. tokens by '|' -> each call parse_tokens
    for (int i = 0; i <= token_count; i++) {
        if (i == token_count || tokens[i].type == TOK_PIPE) {
            if (!parse_tokens(&tokens[start], i - start, &cmds[cmd_count])) {
                fprintf(stderr, "parse error\n");
                return;
            }
            cmd_count++;
            start = i + 1;
        }
    }

    // 2. create pipe
    int pipes[15][2];
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    // 3. each cmd match a fork
    for (int i = 0; i < cmd_count; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {

            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }


            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }


            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }


            struct RedirectBackup backup;
            if (apply_redirection(&cmds[i], &backup) != 0) {
                _exit(1);
            }


            if (is_builtin(&cmds[i])) {
                handle_builtin(&cmds[i], history, history_count);
                _exit(0);
            }

            execvp(cmds[i].argv[0], cmds[i].argv);
            perror("execvp");
            _exit(1);
        }
    }

    // 4. parent close all pipe
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // 5. waitpid
    for (int i = 0; i < cmd_count; i++) {
        wait(NULL);
    }
}