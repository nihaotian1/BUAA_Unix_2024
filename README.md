#Unix
___
# PRE
作业需求：编写简单的shell
1. 可以运行不带参数的外部命令
2. 支持标准IO重定向
3. 可以使用管道连接两个外部命令
4. 300行以内代码

# 功能实现展示
## CLI界面设计（仿Linux）
![[Pasted image 20240507182121.png]]

## 含参外部命令
![[Pasted image 20240507181848.png]]

## 少量内部命令实现
```sh
cd
exit
```
![[Pasted image 20240507182327.png]]

![[Pasted image 20240507182343.png]]

# 标准IO重定向
![[Pasted image 20240507183401.png]]

## 管道
![[Pasted image 20240507183821.png]]

# 报错信息提醒
包括
```
ERROR:
错误cd地址
空指令(包括管道前后检查)
重定向空路径
无法读文件
无法写文件
无法追加文件内容
无效/未知指令

WARNING:
同时出现管道和重定向抢占IO，按照Linux标准是重定向优先，此情况下给出提示
```
![[Pasted image 20240507185445.png]]
![[Pasted image 20240507185522.png]]
![[Pasted image 20240507185538.png]]
## 支持
支持多空白符，包含空格和`\t`的输入
![[Pasted image 20240507185735.png]]
___
# 实现
## 整体实现流程
```c
// main
while(1) {
	print_cur_dir();
	if (fgets(buf, BUF_SIZE, stdin) != NULL) {
		buf_index = 0;
		Cmd cmd = parse();
		f = exec_my(cmd);
		cmd_free(cmd);
		if (f == -1) {
			exit_my();
			break;
		}
	}
}
```
每次接受到指令后，对指令进行解析parse()，返回一个Cmd的结构体，并根据解析过程，给出报错信息
```c
typedef struct command {
char *args[MAX_ARGS_NUM]; // 参数表
int len;                  // 参数表长度
int valid;                // 指令是否有效
char *from_file;          // 重定向 <
char *add_to_flie;        // 重定向 >>
char *write_to_file;      // 重定向 >
struct command * next;    // 下一个指令
} *Cmd;
```
![[Pasted image 20240507193459.png]]

调用exec_my()执行指令
![[Pasted image 20240507194452.png]]

## 带参数的外部命令
使用到的系统调用：
```c
fork()
execlp()
execvp()
waitpid()
exit()
```
```c
/* single out cmd without params */
void exc_out_single(Cmd cmd) {
	pid_t pid = fork();
	if (pid < 0) {
		panic("exc_out_single fork err");
	} else if (pid == 0) { // son exec
		if (execlp(cmd->args[0], cmd->args[0], NULL) == -1) {
			panic("ERROR: Invalid Command");
		}
		exit(0);
	} else {
		int status;
		waitpid(pid, &status, 0);
	}
}
```
如何实现不含参数外部命令？
1. `fork()`一个子进程，把这个命令交给子进程执行
2. 使用`execlp()`/`execvp()`函数实现外部命令执行
3. 在子进程未执行完之前，父进程需要等待`waitpid()`
4. 子进程执行后，及时使用`exit()`退出

但是，为了方便后续内容的测试，决定实现可以含参数的外部命令，此时，为了动态参数长度便于处理，改为`execvp()`：
```c
if (execvp(cmd->args[0], cmd->args) == -1) {
	panic("ERROR: Invalid Outer Command");
}
```

## 标准IO重定向
只考虑实现标准IO重定向，因此需要在解析以下字符模式
```c
<
>
>>
```
IO 重定向主要涉及到文件相关的系统调用
```c
dup2()
open()
close()
```
实现流程：
1. 在解析阶段准备好需要重定向的文件
2. 对文件执行open()操作，并配置好对应的权限位，以及对于可能得新创建文件的权限
```c
#define READ_PERM O_RDONLY
#define WRITE_PERM O_RDWR | O_CREAT | O_TRUNC
#define APPEND_PERM O_WRONLY | O_CREAT | O_APPEND

fd = open(cmd->write_to_file, WRITE_PERM, 0755)；
fd = open(cmd->add_to_file, APPEND_PERM, 0755)；
```
3. 使用dup2将需要重定向的文件表中表项复制到标准IO
```c
dup2(in, STDIN_FILENO);
dup2(out, STDOUT_FILENO);
dup2(fd, STDIN_FILENO);
dup2(fd, STDOUT_FILENO);
```

## 两个外部命令的管道
主要是依赖管道制定新的文件描述符，实现进程间通信
涉及到的系统调用包括
```c
pipe()
close()
```
实现流程：
1. 使用管道连接两个文件描述符
2. 依此对管道前后命令建立子进程执行，并修改对应的IO
3. 按顺序关闭文件
```c
Cmd cmd1 = cmd;
Cmd cmd2 = cmd->next;
int fd[2];
pipe(fd);
exec_a_cmd(cmd1, -1, fd[1]);
close(fd[1]);
exec_a_cmd(cmd2, fd[0], -1);
close(fd[0]);
```

