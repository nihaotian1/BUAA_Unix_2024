all:
	gcc myshell.c -o MyShell
	make run

run: MyShell
	./MyShell