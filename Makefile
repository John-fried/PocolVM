all: pocol

pocol:
	gcc --std=c11 -Os -s -o pocol main.c pocolvm.c
clean:
	rm pocol
