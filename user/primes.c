#include "kernel/types.h"
#include "user.h"

void gao(int pr)
{
	int tmp;
	if (read(pr, &tmp, sizeof(tmp)))
	{
		int prime = tmp;
		printf("prime %d\n", prime);
		int p[2];
		pipe(p);
		if (fork() == 0)
		{
			close(p[1]);
			gao(p[0]);
			close(p[0]);
		}
		else
		{
			close(p[0]);
			while (read(pr, &tmp, sizeof(tmp)))
			{
				if (tmp % prime != 0)
				{
					write(p[1], &tmp, sizeof(tmp));
				}
			}
			close(p[1]);
			wait((int*)0);
		}
	}
}

int main(int argc, char *argv[])
{
	int p[2];
	pipe(p);

	if (fork() == 0)
	{
		close(p[1]);
		gao(p[0]);
		close(p[0]);
	}
	else
	{
		close(p[0]);
		for (int i = 2; i <= 35; i++)
		{
			write(p[1], &i, sizeof(i)); // little endian
		}
		close(p[1]);
		wait((int*)0);
	}

	exit(0);
}
