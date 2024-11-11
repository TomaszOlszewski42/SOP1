#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXFD 20
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define MAX 256

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void show_stage2(const char *const path, const struct stat *const stat_buf)
{
    if(S_ISREG(stat_buf->st_mode))
    {
        printf("Size: %ld\n",stat_buf->st_size);
        char buf[stat_buf->st_size+1];
        FILE* f;
        f = fopen(path,"r");
        bulk_read(f->_fileno,buf,stat_buf->st_size);
        buf[stat_buf->st_size] = '\0';
        printf("%s\n",buf);
        fclose(f);
        return;
    }
    if(S_ISDIR(stat_buf->st_mode))
    {
        chdir(path);
        DIR *dirp;
        struct dirent *dp;
        struct stat filestat;
        if(NULL == (dirp = opendir(".")))
            ERR("opendir");
        do
        {
            errno = 0;
            if ((dp = readdir(dirp)) != NULL)
            {
                if (lstat(dp->d_name, &filestat))
                    ERR(dp->d_name);
                if (S_ISREG(filestat.st_mode))
                    printf("%s\n", dp->d_name);
            }
        } while (dp != NULL);
        closedir(dirp);
        return;
    }
    printf("Unknown file type\n");
}

void write_stage3(const char *const path, const struct stat *const stat_buf)
{
    if(!S_ISREG(stat_buf->st_mode))
    {
        printf("Not a file!\n");
        return;
    }
    const int fd = open(path, O_APPEND | O_WRONLY);
    if (-1 == fd)
        ERR("open");
    char* input = NULL;
    for(;;)
    {
        size_t len = 1;
        if(-1 == getline(&input, &len, stdin))
        {
            if(input)
                free(input);
            close(fd);
            break;
        }
        if (input[0] == '\n')
        {
            if(input)
                free(input);
            close(fd);
            break;
        }
        if (bulk_write(fd, input, strlen(input)) == -1)
            ERR("bulk_write");
    }
    close(fd);
}

int walk(const char *name, const struct stat *s, int type, struct FTW *f)
{
    //char* actual_name = &name[f->base];
    printf("%s",&name[f->base]);
    switch (type)
    {
        case FTW_D:
        case FTW_DNR:
            printf(" is directory\n");
            break;
        case FTW_F:
            printf(" is a file\n");
            break;
        default:
            printf(" is of unknown type\n");
            break;
    }
    return 0;
}

void walk_stage4(const char *const path, const struct stat *const stat_buf)
{
    if (nftw(path,walk,MAXFD,FTW_PHYS) != 0)
        printf("brak dostepu\n");
}

int interface_stage1() 
{
    printf("1. show\n2. write\n3. walk\n4. exit\n");
    char option[256];
    if(fgets(option,256,stdin) == NULL)
        ERR("fgets");
    int opt = atoi(option);
    if(opt != 1 && opt != 2 && opt != 3 && opt != 4)
    {
        printf("Wrong option\n");
        return 1;
    }
    if(opt == 4)
        return 0;

    char path[MAX];
    if(fgets(path,256,stdin) == NULL)
        ERR("fgets");

    struct stat filestat;
    path[strlen(path)-1] ='\0';
    if((-1 == stat(path,&filestat)))
    {
        printf("wrong file\n");
        return 1;
    }
    switch(opt)
    {
        case 1: show_stage2(path, &filestat); break;
        case 2: write_stage3(path, &filestat); break;
        case 3: walk_stage4(path, &filestat); break; 
    }

    return 1;
}

int main()
{
    while (interface_stage1())
        ;
    return EXIT_SUCCESS;
}
