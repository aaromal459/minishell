#include "main.h"

char prompt_str[30]=ANSI_COLOR_GREEN"MINISHELL"ANSI_COLOR_RESET":";
volatile sig_atomic_t fg_pid=0;
volatile sig_atomic_t sigint_received=0;
extern stopped_process *head;
void signal_handler(int signo)
{
    if (fg_pid == 0)
    {
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    if (signo == SIGINT)
    {
        kill(fg_pid, SIGINT);
    }
    else if (signo == SIGTSTP)
    {
        kill(fg_pid, SIGTSTP);
    }
}
void sigchld_handler(int sig)
{
    pid_t pid;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        stopped_process *temp = head;
        stopped_process *prev = NULL;

        while (temp)
        {
            if (temp->pid == pid)
            {
                if (prev)
                    prev->link = temp->link;
                else
                    head = temp->link;
                break;
            }

            prev = temp;
            temp = temp->link;
        }
    }
}
int main()
{
    system("clear");
    
    char path[100];
    getcwd(path,100);
    

    struct sigaction sa;

    sa.sa_handler=signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags= SA_RESTART;
    struct sigaction sa_chld;
    
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    
    sigaction(SIGCHLD, &sa_chld, NULL);

    sigaction(SIGINT, &sa, NULL);   
    sigaction(SIGTSTP, &sa, NULL);
    char *cmd=malloc(100);

    scan_input(prompt_str,cmd);
    
}
