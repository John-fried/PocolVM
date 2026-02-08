all: pocol

pocol:
	gcc -o pocol main.c pocolvm.c
