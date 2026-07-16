#ifndef SHELL_H /* if not defined */
#define SHELL_H 

#define MAX_LINE 1024   // 1.1
#define MAX_ARGS 64     // 1.1
#define MAX_HISTORY 100 // 2.1
#define ALIAS_TABLE_SIZE 53    // 2.8

// 1.3 保存命令解析后的结果
struct Command {
    char *argv[MAX_ARGS];   // 参数数组
    int argc;               // 参数个数

    // 4
    char *input_file;   // 4.3 - 输入重定向 <
    char *output_file;  // 4.1 / 4.2 - 输出重定向 > / >>
    int append_output;  // 4.2 - 0 表示 > | 1 表示 >>
    int background;     // 5.1 - 后台执行 &
};

struct AliasNode {
    char name[MAX_LINE];
    char value[MAX_LINE];
    struct AliasNode *next;
};

extern char line[MAX_LINE];    // #define MAX_LINE 1024

// 1.2 Shell 主循环
void shell_loop(void);

#endif  /* 与#ifndef配对 */