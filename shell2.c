#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>

int main() {
char command[1024];
char *token;
char *outfile;
int i, fd, amper, redirect, retid, status;
int redirect_error, redirect_create;
char *argv[10];
char prompt[1024] = "hello: ";

while (1)
{
    printf("%s", prompt);
    fgets(command, 1024, stdin);
    command[strlen(command) - 1] = '\0';

    // Check for prompt change command 
    if (strncmp(command, "prompt = ", 9) == 0) {
        strcpy(prompt, command + 9);
        strcat(prompt, " ");
        continue;
    }

    /* parse command line */
    i = 0;
    token = strtok (command," ");
    while (token != NULL)
    {
        argv[i] = token;
        token = strtok (NULL, " ");
        i++;
    }
    argv[i] = NULL;

    /* Is command empty */
    if (argv[0] == NULL)
        continue;

    /* Does command line end with & */ 
    if (! strcmp(argv[i - 1], "&")) {
        amper = 1;
        argv[i - 1] = NULL;
    }
    else 
        amper = 0; 

    // check for output redirection
    if (! strcmp(argv[i - 2], ">")) {
        redirect = 1;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else 
        redirect = 0; 

    // check for error redirection
    if (! strcmp(argv[i - 2], "2>")) {
        redirect_error = 1;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
        }
    else 
        redirect_error = 0;

    // check for output redirection
    if (! strcmp(argv[i - 2], ">>")) {
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
        
        execvp(argv[0], argv);
    }
    /* parent continues here */
    if (amper == 0)
        retid = wait(&status);
}
}
