#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<limits.h>
/*
    22371325-倪浩天-MyShell
    运行不带参数的外部命令
    支持标准IO重定向
    可以使用管道连接两个外部命令
*/
void printCurDir();
int main() {
    while(1) {
        printCurDir();
        int n;
        scanf("%d", &n);
    }
}

void printCurDir() {
    char p[PATH_MAX];
    getcwd(p, sizeof(p));
    // printf("\033[字背景颜色;字体颜色m字符串\033[0m" );
    printf("\033[40;32mMyShell:\033[0m \033[40;34m%s\033[0m \033[40;33m$\033[0m ", p);
}