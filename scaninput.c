#include"main.h"
#include <errno.h>
char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
						"set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
						"exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help","jobs","bg","fg","clear", NULL};
char *external[155];
stopped_process *head = NULL;

int si_no = 1;
int no = 1;
int pid = 0;
int status = 0;
extern volatile sig_atomic_t fg_pid;
void execute_ext(char *argv[])
{
    int pipe_count=0;
    for(int i=0;argv[i]!=NULL;i++)
    {
        if(strcmp(argv[i],"|")==0)
            pipe_count++;
    }
    if(pipe_count==0)
    {
        execvp(argv[0],argv);
        perror("execvp");
        exit(1);
    }
    int pipes[pipe_count][2];
    for(int i=0;i<pipe_count;i++)
    {
        if(pipe(pipes[i])==-1)
        {
            perror("pipe");
            exit(1);
        }
    }
    int cmd_start=0;
    int cmd_no=0;
    for(int i=0;;i++)
    {
        int end =(argv[i]==NULL);
        if(end || strcmp(argv[i],"|")==0)
        {
            if(!end)
                argv[i]=NULL; 
            pid_t pid=fork();
            if(pid==0)
            {
                if(cmd_no > 0)
                    dup2(pipes[cmd_no - 1][0],STDIN_FILENO);
                if(cmd_no<pipe_count)
                    dup2(pipes[cmd_no][1],STDOUT_FILENO);
                for(int j=0;j<pipe_count;j++)
                {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                execvp(argv[cmd_start],&argv[cmd_start]);
                perror("execvp");
                exit(1);
            }
            cmd_no++;
            cmd_start=i + 1;
        }
        if(end)
            break;
    }
    for(int i=0;i<pipe_count;i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for(int i=0;i<pipe_count + 1;i++)
        wait(NULL);

    exit(0);
}
void scan_input(char *prompt,char *cmd)
{
    extract_external_commands(external);
    while(1)
    {
        printf("%s",prompt);


        if(fgets(cmd,100,stdin)==NULL)
        {
            clearerr(stdin);
            continue;
        }

        cmd[strcspn(cmd,"\n")]='\0';
        if(!strncmp(cmd,"PS1=",4))
        {
            char *ptr=strchr(&cmd[4],' ');
            if(ptr)
            {
                printf("%s: command not found\n",ptr+1);
                continue;
            }
            else
                strcpy(prompt,&cmd[4]);
        }
        char *str=get_command(cmd);

        int type=check_command_type(str);

        if(type==BUILTIN)
            execute_internal_commands(cmd);
        else if(type==NO_COMMAND)
        {
            printf("%s: command not found\n",cmd);
            status = 127 << 8;
        }
        else if(type==EXTERNAL)
        {
            int pid=fork();
            if(!pid)
            {
                signal(SIGINT,SIG_DFL);
                signal(SIGTSTP,SIG_DFL);

                char *argv[20];
                split_string(cmd,argv);
                execute_ext(argv);
            }
            else if(pid>0)
            {
                fg_pid=pid;

                waitpid(pid, &status, WUNTRACED);
                fg_pid=0;
                if(WIFSTOPPED(status))
                {
                    stopped_process *new = malloc(sizeof(stopped_process));
                
                    new->pid = pid;
                    new->state = STOPPED;
                    strcpy(new->cmd, cmd);
                
                    new->link = head;
                    head = new;
                
                    printf("[%d]+ Stopped\t%s\n", no++, cmd);
                }
            }
        }
    }
}
char *get_command(char *cmd)
{
    static char ret[20];
    int i=0;
    while(cmd[i]!=' '&& cmd[i]!='\0')
    {
        ret[i]=cmd[i];
        i++;
    }
    ret[i]='\0';
    return ret;
}
int split_string(char *str,char *argv[])
{
    int i=0,j,k=0;
    while(str[i]!='\0')
    {
        while(str[i]==' ')
            i++;

        if(str[i]=='\0')
            break;

        argv[k]=malloc(50);  // allocate space

        j=0;
        while(str[i]!=' ' && str[i]!='\0')
            argv[k][j++]=str[i++];

        argv[k][j]='\0';
        k++;
    }

    argv[k]=NULL;

    return k;
}
void extract_external_commands(char **external)
{
    FILE*fptr=fopen("extern.txt","r");
    if(fptr==NULL)
    {
        printf("Fileopen failed\n");
        return;
    }
    char temp[25];
    int i=0;

    while(fgets(temp,sizeof(temp),fptr)!=NULL)
    {
        temp[strcspn(temp,"\r\n")]='\0';

        external[i]=malloc(strlen(temp) + 1);
        strcpy(external[i],temp);

        i++;
    }
    fclose(fptr);
}
int check_command_type(char *command)
{
    int i=0;
    
    while(builtins[i])
    {
        if(!strcmp(command,builtins[i]))
        {
            return BUILTIN;
        }
        i++;
    }
    
    i=0;
    while(external[i])
    {
        if(!strcmp(command,external[i]))
        {
            return EXTERNAL;
        }
        i++;
    }
    return NO_COMMAND;
}
void execute_internal_commands(char *input_string)
{
    if(!strncmp(input_string,"pwd",3))
    {
        char path[100];

        if(getcwd(path,sizeof(path)) != NULL)
        {
            printf("%s\n",path);
            status = 0;
        }
        else
        {
            perror("pwd");
            status = 1;
        }
    }
    else if(!strncmp(input_string,"exit",4))
    {
        exit(1);
    }
    else if(!strncmp(input_string,"cd ",3))
    {
        if(chdir(&input_string[3])!=0)
        {
            perror("chdir");
        }
    }
    else if(!strncmp(input_string,"echo",4))
    {
        if(!strcmp("$SHELL",&input_string[5]))
        {
            printf("%s\n",getenv("SHELL"));
        }
        else if(!strcmp("$$",&input_string[5]))
        {
            printf("%d\n",getpid());
        }
        else if(strcmp(input_string+5, "$?") == 0)
        {
            if (WIFEXITED(status))
            {
                printf("%d\n", WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status))
            {
                printf("%d\n", 128 + WTERMSIG(status));
            }
            else if (WIFSTOPPED(status))
            {
                printf("%d\n", 128 + WSTOPSIG(status));
            }
        }
    }
     else if(strcmp(input_string, "clear") == 0)
    {
        system("clear");
    }
    else if(strcmp(input_string, "jobs") == 0)
    {
        stopped_process *temp = head;
        int n = 1;
    
        while (temp)
        {
            printf("[%d]\t", n++);
    
            if (temp->state == STOPPED)
                printf("Stopped\t");
            else
                printf("Running\t");
    
            printf("%s\n", temp->cmd);
    
            temp = temp->link;
        }
    }

    else if (strcmp(input_string, "fg") == 0)
    {
        if (head != NULL)
        {
            stopped_process *temp = head;
    
            printf("%s\n", temp->cmd);
    
            kill(temp->pid, SIGCONT);
    
            temp->state = RUNNING;
    
            waitpid(temp->pid, &status, WUNTRACED);
    
            if (WIFSTOPPED(status))
            {
                temp->state = STOPPED;
            }
            else
            {
                head = temp->link;
                free(temp);
            }
        }
    }

    else if (strcmp(input_string, "bg") == 0)
    {
        int job_no = 1;
        stopped_process *temp = head;

        while (temp && temp->state == RUNNING)
        {
            temp = temp->link;
            job_no++;
        }

        if (temp)
        {
            kill(temp->pid, SIGCONT);
            temp->state = RUNNING;

            printf("[%d] %d\n", job_no, temp->pid);
        }
    }
         
}
