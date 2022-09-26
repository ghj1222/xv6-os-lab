#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *whoami;

#define false 0
#define true 1
#define bool char

bool cmpname(char *path, char *filename)
{
    char *p = path + strlen(path);
    char *s = filename + strlen(filename);
    while (p >= path && *p != '/')
    {
        if (*p != *s) return false;
        p--, s--;
    }
    return true;
}

void find(char *path, char *filename)
{
//    printf("info: find %s in %s\n", filename, path);
    char buf[512], *p;
    int fd;
    struct dirent dir;
    struct stat st;
    if (stat(path, &st) < 0)
    {
        fprintf(2, "%s: 获取不了 %s 的状态\n", whoami, path);
        return;
    }
    switch (st.type)
    {
        case T_FILE:
            if (cmpname(path, filename))
            {
                printf("%s\n", path);
            }
            break;
        case T_DIR:
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
            {
                fprintf(2, "%s: 路径太长\n", whoami);
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *(p++) = '/';
            if ((fd = open(path, 0)) < 0)
            {
                fprintf(2, "%s: 打不开 %s\n", whoami, path);
            }
            while (read(fd, &dir, sizeof(dir)) == sizeof(dir))
            {
                if (dir.inum == 0) continue;
                if (strcmp(dir.name, ".") == 0 || strcmp(dir.name, "..") == 0) continue;
//                printf("info: open file %s\n", dir.name);
                memmove(p, dir.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, filename);
            }
    }
}

int main(int argc, char *argv[])
{
    whoami = argv[0];
    if (argc < 2) fprintf(2, "%s: 参数的数量不够\n", whoami);
    else find(argv[1], argv[2]);
    exit(0);
}
