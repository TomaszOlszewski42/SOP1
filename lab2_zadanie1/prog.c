#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

int MAX_PID_LENGTH = 15;
volatile sig_atomic_t sig_count = 0;

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

void sighandler()
{
    sig_count++;
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void child_work(int n, sigset_t oldmask)
{
    srand(time(NULL) * getpid());
    int diff;
    int count;
    int written_count = 0;
    int s = 10 + rand() % 91;
    int fd;
    char* name = malloc(MAX_PID_LENGTH + 1);
    snprintf(name,MAX_PID_LENGTH+1,"%d",getpid());
    if ((fd = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");
    char* buf = malloc(s*1000);
    memset(buf,'0' + n,s*1000);
    sethandler(sighandler,SIGUSR1);
    struct timespec t = {1, 100000000};
    sigprocmask(SIG_SETMASK,&oldmask, NULL);
    while(0 != nanosleep(&t,&t))
    {
        //printf("%ld\n",t.tv_nsec);
        diff = sig_count - written_count;
        for(int i = 0; i < diff; i++)
            if( (count = bulk_write(fd,buf,s*1000)) < 0)
                ERR("bulk_write");
        written_count += diff;    
    }
    diff = sig_count - written_count;
    for(int i = 0; i < diff; i++)
        if( (count = bulk_write(fd,buf,s*1000)) < 0)
            ERR("bulk_write");

    if (TEMP_FAILURE_RETRY(close(fd)))
        ERR("close");
    free(buf);
    free(name);
}

void parent_work()
{
    struct timespec t = {0, 10000000};
    struct timespec tt;
    for (int i = 0; i < 100; i++)
    {
        tt = t;
        nanosleep(&tt, NULL);
        kill(0, SIGUSR1);
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n... \n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if(argc < 2)
        usage(argv[0]);
    
    sethandler(SIG_IGN,SIGUSR1);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    pid_t pid;
    int x;
    for(int i = 1; i <= argc - 1; i++)
    {
        if((pid = fork()) < 0)
            ERR("fork");
        if(pid == 0)
        {
            x = atoi(argv[i]);
            child_work(x, oldmask);
            return EXIT_SUCCESS;
        }
    }
    parent_work();
    while (wait(NULL) > 0);
    return EXIT_SUCCESS;
}