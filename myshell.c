#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>

#define MAX_ARG 20

void initialize_history_commands(char history_commands[20][1024]) {
    for (int i = 0; i < 20; i++) {
        strcpy(history_commands[i], "");
    }
}

// add a command to the history of commands limit 20 commands
void add_to_history_commands(char *command, char history_commands[20][1024]) {
    for (int i = 19; i > 0; i--) {
        strcpy(history_commands[i], history_commands[i - 1]);
    }
    strcpy(history_commands[0], command);
}

// return the last command from the history of commands
char *get_last_command(char history_commands[20][1024]) {
    return history_commands[0];
}

void handle_sigint(int sig) {
    printf("\nYou typed Control-C!\n");
}


int main() {
char command[1024];
char history_commands[20][1024];
initialize_history_commands(history_commands);
char *token;
char *outfile;
int i, fd, amper, redirect, retid, status, argc1;
int redirect_error, redirect_create;
char *argv[MAX_ARG], *argv2[MAX_ARG];
char prompt[1024] = "hello: ";
int fildes[2];
int piping;

// handle ctrl+c
signal(SIGINT, handle_sigint);

while (1)
{   
    piping = 0;
    // get the command from the user
    printf("%s", prompt);
    fgets(command, 1024, stdin);
    command[strlen(command) - 1] = '\0';

    // Check for prompt change command 
    if (strncmp(command, "prompt = ", 9) == 0) {
        strcpy(prompt, command + 9);
        strcat(prompt, " ");
        continue;
    }

    // check for !! command
    if (strcmp(command, "!!") == 0) {
        
        char* last_command = get_last_command(history_commands);
        
        // if no commands in history
        if (strcmp(last_command, "") == 0) {
            printf("No commands in history\n");
            continue;
        } else { // execute the last command
            strcpy(command, last_command);
        }
    } else { 
        add_to_history_commands(command, history_commands);
    }

    /* parse command line */
    i = 0;
    token = strtok (command," ");
    while (token != NULL && i < MAX_ARG)
    {
        argv[i] = token;
        token = strtok (NULL, " ");
        i++;
        if (token && ! strcmp(token, "|")) {
            piping = 1;
            break;
        }
    }
    argv[i] = NULL;

    // check for cd command 
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            printf("No directory argument\n");
        } else {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        continue;
    }

    /* Is command empty */
    if (argv[0] == NULL)
        continue;

    /* Does command contain pipe */
    if (piping) {
        int index = 0;
        while (token != NULL)
        {
            token = strtok (NULL, " ");
            argv2[index] = token;
            index++;
        }
        argv2[index] = NULL;
    }

    /* Does command line end with & */ 
    if (i > 0 && ! strcmp(argv[i - 1], "&")) {
        amper = 1;
        argv[i - 1] = NULL;
    }
    else 
        amper = 0; 

    // check for output redirection
    if (i >= 2 && ! strcmp(argv[i - 2], ">")) {
        redirect = 1;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else 
        redirect = 0; 

    // check for error redirection
    if (i >= 2 && ! strcmp(argv[i - 2], "2>")) {
        redirect_error = 1;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else 
        redirect_error = 0;

    // check for output redirection
    if (i >= 2 && ! strcmp(argv[i - 2], ">>")) {
        redirect_create = 1;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else 
        redirect_create = 0;

    // print all arguments
    if (! strcmp(argv[0], "echo")) {
        for (int j = 1; j < 9 && argv[j] != NULL; j++) {
            // check for $? and replace with status
            if(! strcmp(argv[j], "$?")) {
                printf("%d ", status);
            }
            else
                printf("%s ", argv[j]);
        }
        printf("\n");
        continue;
    }

    // exit the shell 
    if (! strcmp(argv[0], "quit")) {
        exit(0);
        break;
    }

    if (fork() == 0) { 
        
        if (redirect) {
            fd = creat(outfile, 0660); 
            close (STDOUT_FILENO); 
            dup(fd); 
            close(fd); 
            
        } 

        if (redirect_error) {
            fd = creat(outfile, 0660); 
            close (STDERR_FILENO); 
            dup(fd); 
            close(fd); 
        }

        if (redirect_create) {
            fd = open(outfile, O_WRONLY | O_APPEND | O_CREAT, 0660); 
            close (STDOUT_FILENO); 
            dup(fd); 
            close(fd); 
        }

        if (piping) {
            pipe (fildes);
            if (fork() == 0) { 
                /* first component of command line */ 
                close(STDOUT_FILENO); 
                dup(fildes[1]); 
                close(fildes[1]); 
                close(fildes[0]); 
                /* stdout now goes to pipe */ 
                /* child process does command */ 
                execvp(argv[0], argv);
            } 
            /* 2nd command component of command line */ 
            close(STDIN_FILENO);
            dup(fildes[0]);
            close(fildes[0]); 
            close(fildes[1]); 
            /* standard input now comes from pipe */ 
            execvp(argv2[0], argv2);
        } 
        else
            execvp(argv[0], argv);
    }
    /* parent continues here */
    if (amper == 0)
        retid = wait(&status);
    }
}

