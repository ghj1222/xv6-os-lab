#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *whoami;

void panic(char *info)
{
    fprintf(2, "%s : %s\n", whoami, info);
    exit(1);
}

int fork1()
{
    int pid = fork();
    if (pid < 0) panic("cannot fork");
    return pid;
}

int getcmd(char *buf, int buflen)
{
    gets(buf, buflen);
    if (buf[0] == 0) return -1;
    return 0;
}

void runcmd(int argc, char *argv[], char *buf)
{
 //   printf("runcmd %d %s\n", argc, buf);
    char *nargv[256];
    int nargc = argc - 1;
    for (int i = 1; i < argc; i++)
        nargv[i - 1] = argv[i];
    for (char *p = buf; *p != 0; p++)
    {
   //     printf("Pos = %d\n", p - buf);
        if (*p == ' ' || *p == '\n') *p = 0;
        if (*p != 0 && (p == buf || *(p - 1) == 0))
        {
         //   printf("Zhaodaole yige %d\n", p - buf);
            nargv[nargc++] = p;
            if (nargc >= sizeof(nargv)) panic("too many arguments");
        }
    }
    nargv[nargc] = 0;
    //for (int i = 0; i < nargc; i++)
    {
    //    printf("Args[%d] = %p\n", i, nargv[i]);
    }
    exec(nargv[0], nargv);
}

int main(int argc, char *argv[])
{
    whoami = argv[0];
    char buf[1145];
    while (getcmd(buf, sizeof(buf)) == 0)
    {
        if (fork1() == 0)
        {
            runcmd(argc, argv, buf);
        }
        wait((int*)0);
    }
    exit(0);
}
