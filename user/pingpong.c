#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[])
{
	int pipe1[2], pipe2[2];
	char buf[233];
	pipe(pipe1);
	pipe(pipe2);
	if (fork() == 0) // child process
	{
		close(pipe1[1]);
		read(pipe1[0], buf, 4);
//		close(pipe1[1]);
		close(pipe1[0]);
		printf("%d: received %s\n", getpid(), buf);
		close(pipe2[0]);
		write(pipe2[1], "pong", 4);
//		close(pipe2[0]);
		close(pipe2[1]);
	}
	else
	{
		close(pipe1[0]);
		write(pipe1[1], "ping", 4);
//		close(pipe1[0]);
		close(pipe1[1]);
		close(pipe2[1]);
		read(pipe2[0], buf, 4);
		printf("%d: received %s\n", getpid(), buf);
//		close(pipe2[1]);
		close(pipe2[0]);
		wait((int*)0);
	}
	exit(0);
}
