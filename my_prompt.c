/////////////////////////////////////////////////////////
//                                                     //
//               Trabalho I: Mini-Shell                //
//                                                     //
// Compilação: gcc my_prompt.c -lreadline -o my_prompt //
// Utilização: ./my_prompt                             //
//                                                     //
/////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>


#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1


typedef struct command {
    char *cmd;              // string with the command
    int argc;               // number of arguments
    char *argv[MAXARGS+1];  // argument vector from the command
    struct command *next;   // pointer for next command
} COMMAND;


// global variables
char* inputfile = NULL;	    // name of file
char* outputfile = NULL;    // name of file
int background_exec = 0;    // indication of concurrent execution with the mini-shell (0/1)


// function declaration
COMMAND* parse(char *);
void print_parse(COMMAND *);
void execute_commands(COMMAND *);
void free_commlist(COMMAND *);
void filter(COMMAND *commlist);


// include of parser
#include "parser.c"


int main(int argc, char* argv[]) {
    char *linha;
    COMMAND *com;

    while (1) {
        if ((linha = readline("my_prompt$ ")) == NULL)
            exit(0);
        if (strlen(linha) != 0) {
            add_history(linha);
            com = parse(linha);
            if (com) {
                //print_parse(com);
                if(strcmp(com->cmd, "filter") == 0) {
                    filter(com);
                }
                else
                    execute_commands(com);
                free_commlist(com);
            }
        }
        free(linha);
    }
}


void print_parse(COMMAND* commlist) {
    int n, i;

    printf("---------------------------------------------------------\n");
    printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
    n = 1;
    while (commlist != NULL) {
        printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
        i = 0;
        while (commlist->argv[i] != NULL) {
            printf("%s,", commlist->argv[i]);
            i++;
        }
        printf("%s'\n", commlist->argv[i]);
        commlist = commlist->next;
        n++;
    }
    printf("---------------------------------------------------------\n");
}


void free_commlist(COMMAND *commlist){
    COMMAND *tmp;
    do {
        tmp = commlist->next;
        free(commlist);
        commlist = tmp;
    } while(commlist != NULL);
}


void execute_commands(COMMAND *commlist) {
    pid_t pid;
    if(strcmp("exit", commlist->cmd) == 0)
        exit(0);
    
    COMMAND* com = commlist;
    int n = 0;
    while(com != NULL) {
        com = com->next;
        n++;
    }
    n--;

    int i = 0, fd[n][2], fd_in, fd_out, status;
    while(commlist != NULL) {
        if(i < n) {
            if(pipe(fd[i]) < 0) {
                printf("Pipe error\n");
                exit(1);
            }
        }
        if ((pid = fork()) < 0) {
            printf("Fork error");
            exit(1);
        }
        else if(pid == 0) { // child
            if (inputfile != NULL && i == 0) {
                if((fd_in = open(inputfile, O_RDONLY)) < 0) {
                    printf("Open error\n");
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            if (outputfile != NULL && i == n) {
                if((fd_out = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
                    printf("Open error\n");
                    exit(1);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            if(i > 0) { // previous pipe
                dup2(fd[i-1][0], STDIN_FILENO);
                close(fd[i-1][0]);
            }
            if(i < n) { // next pipe
                dup2(fd[i][1], STDOUT_FILENO);
            }
            if (execvp(commlist->cmd, commlist->argv) < 0) {
                    printf("Exec error\n");
                    exit(1);
            }
        }
        else { // parent
            if(background_exec == 0) {
                //wait(NULL);
                waitpid(pid, &status, WUNTRACED | WCONTINUED);
            }
            if(i < n) {
                close(fd[i][1]);
            }
        }

        commlist = commlist->next;
        i++;
    } 
} 


void filter(COMMAND *commlist) {
    int fd[2];
    char *argv1[3];
    char *argv2[3];
    if(pipe(fd) < 0) {
        printf("Pipe error\n");
        exit(1);
    }
    if(fork() == 0) {
        close(STDOUT_FILENO);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        argv1[0] = "cat";
        argv1[1] = commlist->argv[1];
        argv1[2] = NULL;
        if (execvp("cat", argv1) < 0) {
             printf("Exec error\n");
             exit(1);
        }
    }
    if(fork() == 0) {
        close(STDIN_FILENO);
        dup(fd[0]);
        close(fd[1]);
        close(fd[0]);
        argv2[0] = "grep";
        argv2[1] = commlist->argv[3];
        argv2[2] = NULL;
        if((fd[0] = open(commlist->argv[2], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
            printf("Open error\n");
            exit(1);
        }
        dup2(fd[0], STDOUT_FILENO);
        close(fd[0]);
        if (execvp("grep", argv2) < 0) {
             printf("Exec error\n");
             exit(1);
        }
    }
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
}

