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

volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t sick = 0;
volatile sig_atomic_t terminated = 0;
volatile sig_atomic_t alarmed = 0;
volatile sig_atomic_t pid_of_sender = 0;

void sethandler2(void (*f)(int,siginfo_t *, void*), int sigNo)
{
    last_signal = sigNo;
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sender_info_handler(int sig, siginfo_t* info, void * context)
{
    pid_of_sender = info->si_pid;
}

void sig_handler_terminated(int sig)
{
    terminated = 1;
}

void sig_handler_parent(int sig)
{
    last_signal = sig;
}

void sig_handler_alarmed(int sig)
{
    alarmed = 1;
}
//void alarm_handler(int sig)
//{
//    kill(0,SIGTERM);
//}

int child_work(sigset_t oldmask, int p, int i, int k)
{
    if(i == 0)
        sick = 1;
    srand(time(NULL) * getpid());
    sig_atomic_t coughs = 0;
    //int t = 300 + rand() % 701;
    //nanosleep(&tt,NULL);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGTERM);
    sigaddset(&mask,SIGALRM);
    sigprocmask(SIG_UNBLOCK,&mask,NULL);
    //fprintf(stderr,"DUPA\n");
    sethandler(sig_handler_terminated,SIGTERM);
    sethandler2(sender_info_handler,SIGUSR1);
    sethandler(sig_handler_alarmed,SIGALRM);
    int interval, random;
    while (1)
    {
        if(alarmed == 1)
        {
            printf("[%d] has been picked by parents!\n",getpid());
            return coughs;
        }
        if(terminated == 1)
            break;
        if(sick == 0)
        {
            sigsuspend(&oldmask);
        }       
        if(last_signal == SIGUSR1 && sick == 0)
        {
            random = 1 + (rand() % 100);
            fprintf(stdout,"[%d]: %d cought on me\n",getpid(),pid_of_sender);
            if( random <= p)
            {
                fprintf(stdout,"[%d] got sick!\n",getppid());
                sick = 1;
                alarm(k);
            }
        }
        else if(sick == 1)
        {
            interval = 100 + rand() % 151;
            struct timespec t = {0, interval*1000000};
            //printf("sranie: %d\n", interval);
            while(0 != nanosleep(&t,&t)) {};
            fprintf(stdout,"[%d] is coughing\n",getpid());
            kill(0,SIGUSR1);
            coughs++;
        }
    }
    //fprintf(stderr,"[%d] I caught: %d\n",getpid(), coughs);
    return coughs;
}

void parent_work(int t, sigset_t oldmask)
{
    sigaddset(&oldmask,SIGUSR1);
    alarm(t);
    while(last_signal != SIGALRM)
    {
        if(last_signal == SIGCHLD)
        {
            fprintf(stdout,"[%d] has been picked up!\n",pid_of_sender);
        }
        sigsuspend(&oldmask);
    }
    kill(0,SIGTERM);
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s t k n p \n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if(argc != 5)
        usage(argv[0]);
    int t, k, n, p;
    t = atoi(argv[1]);
    k = atoi(argv[2]);
    n = atoi(argv[3]);
    p = atoi(argv[4]);

    if(t < 1 || t > 100 || k < 1 || k > 100 || n < 1 || n > 30 || p < 1 || p > 100)
        usage(argv[0]);
    
    sethandler2(sender_info_handler,SIGCHLD);
    sethandler(SIG_IGN, SIGUSR1);
    sethandler(SIG_IGN, SIGTERM);
    sethandler(sig_handler_parent,SIGALRM);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGCHLD);
    sigprocmask(SIG_UNBLOCK,&mask, NULL);
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    pid_t pid;
    for(int i = 0; i < n; i++)
    {
        pid = fork();
        if(pid < 0)
            ERR("fork");
        if(pid == 0)
        {
            int coughs = child_work(oldmask, p, i, k);
            return coughs;
        }
    }
    parent_work(t,oldmask);
    int status;
    for(int i = 0; i < n; i++)
    {
        pid = wait(&status);
        int return_value = WEXITSTATUS(status);
        printf("[%d] has caught: %d times\n", pid, return_value);
    }

    return EXIT_SUCCESS;
}