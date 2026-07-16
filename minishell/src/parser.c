#include <string.h>
#include "parser.h"


int parse_tokens(struct Token tokens[], int token_count, struct Command *cmd) {
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = 0;
    cmd->background = 0;

    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_WORD) {
            if (cmd->argc < MAX_ARGS - 1) {
                cmd->argv[cmd->argc] = tokens[i].text;
                cmd->argc++;
            }
            continue;
        }

        if (tokens[i].type == TOK_IN) {
            if (i + 1 >= token_count || tokens[i+1].type != TOK_WORD) return 0;
            cmd->input_file = tokens[i+1].text;
            i++;
            continue;
        }
        

        if (tokens[i].type == TOK_OUT) {
            if (i + 1 >= token_count || tokens[i+1].type != TOK_WORD) return 0;
            cmd->output_file = tokens[i+1].text;
            cmd->append_output = 0;
            i++;
            continue;
        }

        if (tokens[i].type == TOK_APPEND) {
            if (i + 1 >= token_count || tokens[i+1].type != TOK_WORD) return 0;
            cmd->output_file = tokens[i+1].text;
            cmd->append_output = 1;
            i++;
            continue;
        }

        if (tokens[i].type == TOK_AMP && i == token_count - 1) {
            cmd->background = 1;
            token_count--;
            continue;
        }
    }

    cmd->argv[cmd->argc] = NULL;

    if (cmd->argc == 0) {
        return 0;
    }

    return 1;
}








/*
int parse_line(char *line, struct Command *cmd) {

    char *token;    // token/临时指针，每次指向切割出来的单个参数（子字符串）
    
    // 1.3 每次解析前把参数计数清零
    cmd->argc = 0;

    // 4.1 初始化重定向字段
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = 0;
    cmd->background = 0;

    // 1.3 第一次按空格切分，返回第一个参数字段于 token
    token = strtok(line, " ");

    // 1.3 持续切分直至无参数/达到上限
    while (token != NULL && cmd->argc < MAX_ARGS - 1) {
        // 4.1
        if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");

            if (token == NULL) {
                return 0;
            }

            cmd->output_file = token;
            cmd->append_output = 0;
        }
        else {
            cmd->argv[cmd->argc] = token;   // 把token字段存入当前argc位置的argv数组成员
            cmd->argc++;                // 参数计数器加1
        }

        token = strtok(NULL, " ");  // 继续以空格分割
    } 

    // 1.3 
    cmd->argv[cmd->argc] = NULL;    // Shell编程硬性要求，给参数数组补 NULL

    // 1.3 空命令返回 0，无效输入
    if (cmd->argc == 0) {
        return 0;
    }

    return 1;
}

*/