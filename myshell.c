#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<sys/wait.h>
/*
    22371325-倪浩天-MyShell
    运行不带参数的外部命令
    支持标准IO重定向
    可以使用管道连接两个外部命令
*/
#define BUF_SIZE 128
#define MAX_ARGS_NUM 100

typedef struct command {
    char *args[MAX_ARGS_NUM];
    int len;
} *Cmd;

char buf[BUF_SIZE];

// system
void welcome();
void print_cur_dir();
void exit_my();

// cmd_utils
Cmd parse();
Cmd cmd_init();
void cmd_add_arg(Cmd cmd, char *arg);
void cmd_free(Cmd cmd);
int is_inner_cmd(Cmd cmd); /* 1-cd 2-echo 3-help */

// exec
void exc_out_single(Cmd cmd);

// inner_cmd
void cd(Cmd cmd);
void echo(Cmd cmd);
void help(Cmd cmd);

// debug
void panic(char *s);
void check_cmd(Cmd cmd);

int main() {
    welcome();
    int f = 0;
    while(1) {
        print_cur_dir();
        if (fgets(buf, BUF_SIZE, stdin) != NULL) {
            Cmd cmd = parse();
            // check_cmd(cmd);
            int id;
            if ((id = is_inner_cmd(cmd)) != 0) {
                if (id == 1) { // cd
                    cd(cmd);
                } else if (id == 2) { // echo
                    echo(cmd);
                } else if (id == 3) { // help
                    help(cmd);
                } else if (id == -1) { // exit
                    exit_my();
                    f = 1;
                }
            } else {
                exc_out_single(cmd);
            }
            cmd_free(cmd);
            if (f == 1) {
                break;
            }
        }
    }
    return 0;
}

Cmd cmd_init() {
    Cmd cmd = (Cmd) malloc(sizeof(struct command));
    cmd->len = 0;
    for (int i = 0; i < MAX_ARGS_NUM; i++) {
        cmd->args[i] = NULL;
    }
    return cmd;
}

Cmd parse() {
    Cmd cmd = cmd_init();
    char arg[BUF_SIZE] = "";
    int index = 0;
    int arg_i = 0;
    while(1) {
        // splitter \t, space, \0, \n
        if (buf[index] == '\t' || buf[index] == ' ' || buf[index] == '\n' || buf[index] == '\0') {
            arg[arg_i] = '\0';
            arg_i = 0;
            cmd_add_arg(cmd, arg);
            if (buf[index] == '\n' || buf[index] == '\0') {
                break;
            } else {
                index++;
                continue;
            }
        }

        arg[arg_i] = buf[index];
        index++;
        arg_i++;
    }
    return cmd;
}

void cmd_add_arg(Cmd cmd, char *arg) {
    cmd->args[cmd->len] = (char *) malloc(strlen(arg) * sizeof(char) + 1);
    strcpy(cmd->args[cmd->len], arg);
    cmd->len++;
}

void cmd_free(Cmd cmd) {
    for(int i = 0; i < cmd->len; i++) {
        free(cmd->args[i]);
    }
    free(cmd);
}

/* single out cmd without params */
void exc_out_single(Cmd cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        panic("exc_out_single fork err");
    } else if (pid == 0) { // son exec
        if (execvp(cmd->args[0], cmd->args) == -1) {
            panic("ERROR: Invalid Outer Command");
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        // wait(NULL);
    }
}

int is_inner_cmd(Cmd cmd) {
    char *arg = cmd->args[0];
    if (strcmp(arg, "cd") == 0) {
        return 1;
    } else if (strcmp(arg, "echo") == 0) {
        return 2;
    } else if (strcmp(arg, "help") == 0) {
        return 3;
    } else if (strcmp(arg, "exit") == 0) {
        return -1;
    }
    return 0;
}

void welcome() {
    printf("\033[1;30;36m#####################################################\033[0m\n");
    printf("\033[1;30;36m#                                                   #\033[0m\n");
    printf("\033[1;30;36m#          Welcome to Sebastian's Shell!            #\033[0m\n");
    printf("\033[1;30;36m#          BUAA_UNIX Homework of 22371325           #\033[0m\n");
    printf("\033[1;30;36m#                                                   #\033[0m\n");
    printf("\033[1;30;36m#####################################################\033[0m\n");
}

void exit_my() {
    printf("\033[1;30;36m#####################################################\033[0m\n");
    printf("\033[1;30;36m#                                                   #\033[0m\n");
    printf("\033[1;30;36m#          Exit Sebastian's Shell!                  #\033[0m\n");
    printf("\033[1;30;36m#          BUAA_UNIX Homework of 22371325           #\033[0m\n");
    printf("\033[1;30;36m#                                                   #\033[0m\n");
    printf("\033[1;30;36m#####################################################\033[0m\n");
}

void print_cur_dir() {
    char p[PATH_MAX];
    getcwd(p, sizeof(p));
    // printf("\033[字背景颜色;字体颜色m字符串\033[0m" );
    printf("\033[40;32mMyShell:\033[0m \033[40;34m%s\033[0m \033[40;33m$\033[0m ", p);
}

void panic(char *s) {
    printf("\033[1;40;31m%s\033[0m\n", s);
}

void check_cmd(Cmd cmd) {
        printf("argc: %d\n", cmd->len);
        for (int i = 0; i < cmd->len; i++) {
            printf("args[%d]: len %d", i, strlen(cmd->args[i]));
            panic(cmd->args[i]);
        }
}

void cd(Cmd cmd) {
    if (chdir(cmd->args[1]) == -1) {
        panic("ERROR: Target Directory Doesn't Exists");
    }
    return;
}

void echo(Cmd cmd) {
    char s[BUF_SIZE];
    strcpy(s, buf + 5);
    printf("%s", s);
}

void help(Cmd cmd) {
    return;
}
