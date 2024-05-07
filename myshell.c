#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<sys/wait.h>
#include<fcntl.h>
/*
    22371325-倪浩天-MyShell
    运行不带参数的外部命令
    支持标准IO重定向
    可以使用管道连接两个外部命令
*/
#define BUF_SIZE 128
#define MAX_ARGS_NUM 100
#define MAX_ARG_LEN 20
#define READ_PERM O_RDONLY
#define WRITE_PERM O_RDWR | O_CREAT | O_TRUNC
#define APPEND_PERM O_WRONLY | O_CREAT | O_APPEND

typedef struct command {
    char *args[MAX_ARGS_NUM];
    int len;
    int valid;
    char *from_file; // redirect
    char *add_to_flie;   // redirect
    char *write_to_file; // redirect
    struct command * next; // tunnel
} *Cmd;

char buf[BUF_SIZE];
int buf_index;

// system
void welcome();
void print_cur_dir();
void exit_my();

// cmd_utils
Cmd parse();
Cmd cmd_init();
void cmd_add_arg(Cmd cmd, char *arg);
void cmd_free(Cmd cmd);
void parse_a_filepath(Cmd cmd);

// exec
int exec_my(Cmd cmd);
void exec_a_cmd(Cmd cmd, int in, int out);

// inner_cmd
void cd(Cmd cmd);
void echo(Cmd cmd);
void help(Cmd cmd);

// debug_info
void panic(char *s);
void warn(char *s);
void check(Cmd cmd);

int main() {
    welcome();
    int f = 0;
    while(1) {
        print_cur_dir();
        if (fgets(buf, BUF_SIZE, stdin) != NULL) {
            buf_index = 0;
            Cmd cmd = parse();
            // check(cmd);
            f = exec_my(cmd);
            cmd_free(cmd);
            if (f == -1) break;
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
    cmd->valid = 1;
    cmd->from_file = NULL;
    cmd->add_to_flie = NULL;
    cmd->write_to_file = NULL;
    cmd->next = NULL;
    return cmd;
}

Cmd parse() {
    Cmd cmd = cmd_init();
    char arg[BUF_SIZE] = "";
    int arg_i = 0;
    while(1) {
        while(buf[buf_index] == ' ' || buf[buf_index] == '\t') {
            buf_index++;
        } // pass all blank
        if (buf[buf_index] == '\n') {
            if (cmd->args[0] == NULL) {
                cmd->valid = 0;
                panic("Empty Command!");
            }
            return cmd;
        } else if (buf[buf_index] == '|') {
            buf_index++;
            cmd->next = parse();
            return cmd;
        } else if (buf[buf_index] == '<' || buf[buf_index] == '>') {
            parse_a_filepath(cmd);
        } else {
            while(buf[buf_index] != '\n' && buf[buf_index] != ' ' && buf[buf_index] != '\t') {
                arg[arg_i++] = buf[buf_index++];
            }
            arg[arg_i] = '\0';
            arg_i = 0;
            cmd_add_arg(cmd, arg);
        }
    }
}

void parse_a_filepath(Cmd cmd) { // now points to a < > >>
    int re = 2;
    if (buf[buf_index] == '<') {
        re = 1; 
    } else if (buf[buf_index] == '>' && buf[buf_index + 1] == '>') {
        re = 3;
        buf_index++;
    }
    buf_index++;
    while(buf[buf_index] == ' '|| buf[buf_index] == '\t') {
        buf_index++;
    }
    if (buf[buf_index] == '<' || buf[buf_index] == '>' || buf[buf_index] == '|' || buf[buf_index] == '\n') {
        cmd->valid = 0;
        panic("ERROR: Can't Find Redirect File Path!\nusage: command [<, >, >>] file");
    } else {
        char arg[MAX_ARG_LEN];
        int i = 0;
        while(buf[buf_index] != ' ' && buf[buf_index] != '\t' && buf[buf_index] != '\n') {
            arg[i++] = buf[buf_index++];
        }
        arg[i] = '\0';
        char *tmp = (char *)malloc(strlen(arg) + 1);
        strcpy(tmp, arg);
        if (re == 1) {
            cmd->from_file = tmp;
        } else if (re == 2) {
            cmd->write_to_file = tmp;
        } else {
            cmd->add_to_flie = tmp;
        }
    }
}

void cmd_add_arg(Cmd cmd, char *arg) {
    cmd->args[cmd->len] = (char *) malloc(strlen(arg) + 1);
    strcpy(cmd->args[cmd->len], arg);
    cmd->len++;
}

void cmd_free(Cmd cmd) {
    for(int i = 0; i < cmd->len; i++) {
        free(cmd->args[i]);
    }
    if (cmd->write_to_file != NULL) free(cmd->write_to_file);
    if (cmd->add_to_flie != NULL) free(cmd->add_to_flie);
    if (cmd->from_file != NULL) free(cmd->from_file);
    if (cmd->next != NULL) cmd_free(cmd->next);
    free(cmd);
}

/* single out cmd with fd_in fd_out */
void exec_a_cmd(Cmd cmd, int in, int out) {
    pid_t pid = fork();
    if (pid < 0) {
        panic("RunTime Exception: exec_a_cmd fork err");
    } else if (pid == 0) { // son exec
        int fd;
        if (cmd->from_file != NULL) {
            if (in != -1) {
                warn("WARNING: Conflict Between Redirect & Pipe, Redirect Has Priority");
            }
            if ((fd = open(cmd->from_file, READ_PERM)) == -1) {
                panic("ERROR: Can't Read From The Target File");
            } else {
                dup2(fd, STDIN_FILENO);
            }
        } else if (in != -1) {
            dup2(in, STDIN_FILENO);
        }
        if (cmd->write_to_file != NULL) {
            if (out != -1) {
                warn("WARNING: Conflict Between Redirect & Pipe, Redirect Has Priority");
            }
            if ((fd = open(cmd->write_to_file, WRITE_PERM, 0755)) == -1) {
                panic("ERROR: Can't Write To The Target File");
            } else {
                dup2(fd, STDOUT_FILENO);
            }
        } else if (cmd->add_to_flie != NULL) {
            if (out != -1) {
                warn("WARNING: Conflict Between Redirect & Pipe, Redirect Has Priority");
            }
            if ((fd = open(cmd->add_to_flie, APPEND_PERM, 0755)) == -1) {
                panic("ERROR: Can't Append The Target File");
            } else {
                dup2(fd, STDOUT_FILENO);
            }
        } else if (out != -1) {
            dup2(out, STDOUT_FILENO);
        }
        if (execvp(cmd->args[0], cmd->args) == -1) {
            panic("ERROR: Invalid Command");
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
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
    printf("\033[40;32mMyShell:\033[0m \033[40;34m%s\033[0m \033[40;33m$\033[0m ", p);
}

void cd(Cmd cmd) {
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

void panic(char *s) {
    printf("\033[1;40;31m%s\033[0m\n", s);
}

void warn(char *s) {
    printf("\033[1;40;35m%s\033[0m\n", s);
}

int exec_my(Cmd cmd) {
    if (cmd->valid == 0 || cmd->next->valid == 0) {
        return 0;
    } else if (strcmp(cmd->args[0], "cd") == 0 || strcmp(cmd->args[0], "exit") == 0) {
        if (strcmp(cmd->args[0], "cd") == 0) {
            if (chdir(cmd->args[1]) == -1) {
                panic("ERROR: Target Directory Not Found!");
            }
        } else {
            return -1;
        }
    } else if (cmd->next == NULL) {
        exec_a_cmd(cmd, -1, -1);
    } else {
        Cmd cmd1 = cmd;
        Cmd cmd2 = cmd->next;
        int fd[2];
        pipe(fd);
        exec_a_cmd(cmd1, -1, fd[1]);
        close(fd[1]);
        exec_a_cmd(cmd2, fd[0], -1);
        close(fd[0]);
    }
    return 0;
}

void check(Cmd cmd) {
    for (int i = 0; i < cmd->len; i++) {
        warn(cmd->args[i]);
    }
    
    if (cmd->add_to_flie != NULL) warn(cmd->add_to_flie);
    if (cmd->from_file != NULL) warn(cmd->from_file);
    if (cmd->write_to_file != NULL) warn(cmd->write_to_file);
}