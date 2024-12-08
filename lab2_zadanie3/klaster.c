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

volatile sig_atomic_t last_sig = 0;
volatile sig_atomic_t terminate = 1;

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_handler(int sig) { last_sig = sig; }

void sig_handler_C(int sig) {terminate = 0; last_sig = sig; }

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

void child_work(int i, sigset_t oldmask)
{
    srand(time(NULL)*getpid());
    int interval;
    sig_atomic_t counter = 0;
    struct timespec wait_time = {0, 0};
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    while(terminate)
    {
        while(last_sig == SIGUSR1)
        {
            interval = 100 + rand() % 101;
            wait_time.tv_nsec = interval * 1000000;
            nanosleep(&wait_time, NULL);
            counter++;
            printf("{%d}: {%d}\n", getpid(), counter);
        }
        if(terminate)
            sigsuspend(&oldmask);
    }
    int fd;
    char name[256];
    snprintf(name,256,"%d.txt",getpid());
    if ((fd = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");

    char buf[256];
    snprintf(buf,256,"%d",counter);
    bulk_write(fd,buf,strlen(buf));
    if (TEMP_FAILURE_RETRY(close(fd)))
        ERR("close");
}

void parent_work(pid_t* pid, int n, sigset_t oldmask)
{
    sigaddset(&oldmask,SIGUSR2);
    kill(pid[0],SIGUSR1);
    int i = 0;
    while(terminate)
    {
        sigsuspend(&oldmask);
        if(last_sig == SIGUSR1)
        {
            kill(pid[i],SIGUSR2);
            i++;
            if(i >= n) i = 0;
            kill(pid[i],SIGUSR1);
        }
    }
    kill(0,SIGINT);
}

int main(int argc, char **argv)
{
    if(argc != 2)
        ERR("arguments");
    
    int n = atoi(argv[1]);

    sigset_t mask, oldmask;
    sethandler(sig_handler, SIGUSR1);
    sethandler(sig_handler, SIGUSR2);
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    sethandler(sig_handler_C,SIGINT);

    pid_t pid[n];

    for(int i = 0; i < n; i++)
    {
        pid[i] = fork();
        if(pid[i] < 0)
            ERR("fork");
        if(pid[i] == 0)
        {
            child_work(i, oldmask);
            return EXIT_SUCCESS;
        }
    }
    
    parent_work(pid, n, oldmask);

    for(int i = 0; i < n; i++)
    {
        wait(NULL);
    }
    return EXIT_SUCCESS;
}