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

pid_t children[20];

typedef struct {
    char *name;
    char *value;
} variable, *Pvariable;

variable *variables = NULL;
int variables_count = 0;

int last_command_status = 0;


void add_child_procces(pid_t children[20], pid_t child) {
    for (int i = 19; i > 0; i--) {
        children[i] = children[i - 1];
    }
    children[0] = child;
}

char* get_variable_value(char *name) {
    for (int i = 0; i < variables_count; i++) {
        if (! strcmp(variables[i].name, name)) {
            return variables[i].value;
        }
    }
    return NULL;
}

void echo_func(char *command) {
    char *pcommand = command + 5;
    if (! strcmp(pcommand, "$?")) {
        printf("%d\n", last_command_status);
    } else {
        char cpy_command[1024];
        strcpy(cpy_command, pcommand); // copy the command to not change the original command
        char *token = strtok(cpy_command, " "); 
        while (token != NULL) { // print all the arguments
            if (token[0] == '$') { // check if the argument is a variable
                char* value = get_variable_value(token + 1);
                if (value != NULL) {
                    printf("%s ", value);
                }
            } else {
                printf("%s ", token);
            }
            token = strtok(NULL, " ");
        }
        printf("\n");
    }
}

void remove_variable(char *name) {
    for (int i = 0; i < variables_count; i++) {
        // check if the variable exists
        if (! strcmp(variables[i].name, name)) {
            free(variables[i].name);
            free(variables[i].value);
            for (int j = i; j < variables_count - 1; j++) {
                // shift the variables to the left instead of the removed variable
                variables[j] = variables[j + 1];
            }
            variables_count--;
            variables = realloc(variables, variables_count * sizeof(variable));
            return;
        }
    }
}

void set_variable_value(char *name, char *value) {
    // check if the variable already exists and remove it
    if (get_variable_value(name) != NULL) {
        remove_variable(name); 
    }

    // add the variable to the variables array
    variables = realloc(variables, (variables_count + 1) * sizeof(variable));
    variables[variables_count].name =  strdup(name);
    variables[variables_count].value = strdup(value);
    variables_count++;
}

void free_variables() {
    for (int i = 0; i < variables_count; i++) {
        free(variables[i].name);
        free(variables[i].value);
    }
    free(variables);
}

// removing spaces
char* no_spaces(char* str) {
    int len = strlen(str);
    char *new_str = malloc(len + 1);
    char *p = new_str;
    for (int i = 0; i < len; i++) {
        if (str[i] != ' ') {
            *p = str[i];
            p++;
        }
    }
    *p = '\0';
    return new_str;
}

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
    for (int i = 0; i < 20; i++) {
        if (children[i] != 0) {
            kill(children[i], SIGKILL);
        }
    }
}

void execute(char* command) {
    char *token;
    char *outfile;
    int i, fd, amper, redirect, retid, status;
    int redirect_error, redirect_create, redirect_oposite;
    char *argv[MAX_ARG];

    /* parse command line */
    i = 0;
    token = strtok (command," ");
    while (token != NULL && i < MAX_ARG)
    {
        argv[i] = token;
        token = strtok (NULL, " ");
        i++;
    }
    argv[i] = NULL;

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
        redirect_oposite = 0;
        redirect_create = 0;
        redirect_error = 0;
        argv[i - 2] = NULL;
        outfile = argv[i - 1];
    } else {

        // check for error redirection
        if (i >= 2 && ! strcmp(argv[i - 2], "2>")) {
            redirect_error = 1;
            redirect = 0;
            redirect_create = 0;
            redirect_oposite = 0;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
        } else {
            
            // check for output redirection
            if (i >= 2 && ! strcmp(argv[i - 2], ">>")) {
                printf("enter >>\n");
                redirect_create = 1;
                redirect = 0;
                redirect_error = 0;
                redirect_oposite = 0;
                argv[i - 2] = NULL;
                outfile = argv[i - 1];
            } else {

                if (i >= 2 && ! strcmp(argv[i - 2], "<")) {
                    redirect_oposite = 1;
                    redirect = 0;
                    redirect_create = 0;
                    redirect_error = 0;
                    argv[i - 2] = NULL;
                    outfile = argv[i - 1];
                } else {
                    redirect_create = 0;
                    redirect = 0;
                    redirect_error = 0;
                    redirect_oposite = 0;
                }
            }
        }
    }
    
    // exit the shell 
    if (! strcmp(argv[0], "quit")) {
        exit(0);
    }

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
        printf("open file\n");
        fd = open(outfile, O_WRONLY | O_APPEND | O_CREAT, 0660); 
        close (STDOUT_FILENO); 
        dup(fd); 
        close(fd); 
    }

    if (redirect_oposite) {
        fd = open(outfile, O_RDONLY); 
        close (STDIN_FILENO); 
        dup(fd); 
        close(fd); 
    }
    
    execvp(argv[0], argv);

    if (amper == 0)
        retid = wait(&status);
}

int evaluate_condition(char *condition) {
    char command[1024];
    snprintf(command, sizeof(command), "%s > /dev/null 2>&1", condition);
    int status = system(command);
    return WEXITSTATUS(status);
}

char* handle_if_else(char *command) {
    char *condition = strstr(command, "if ");
    if (!condition) return NULL;

    condition += 3; // skip past "if "

    char *then_part = strstr(condition, " then ");
    if (!then_part) return NULL;
    *then_part = '\0'; // terminate condition part
    then_part += 6; // skip past " then "

    char *else_part = strstr(then_part, " else ");
    char *end_if = strstr(then_part, " fi");
    if (!end_if) return NULL;
    *end_if = '\0'; // terminate the then part
    end_if += 3; // skip past " fi"

    if (else_part) {
        *else_part = '\0'; // terminate then part before else
        else_part += 6; // skip past " else "
    }

    // Evaluate condition
    int condition_result = evaluate_condition(condition);

    if (condition_result == 0) {
        return then_part;
    } else if (else_part) {
        return else_part;
    } else {
        return NULL;
    }
}

int main() {
    char command[1024];
    char history_commands[20][1024];
    initialize_history_commands(history_commands);
    char prompt[1024] = "hello: ";
    int pipe_count;
    int status;

    // handle ctrl+c
    signal(SIGINT, handle_sigint);

    while (1)
    {   
        pipe_count = 0;
        // get the command from the user
        printf("%s", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';

        if (! strcmp(command, "quit")) {
            break;
        }

        // check for if/else command
        if (strncmp(command, "if ", 3) == 0) {
            char* cond_command = handle_if_else(command);
            if (cond_command != NULL) 
                strcpy(command, cond_command);
            else
                continue;
        }

        // adding the variables to the shell
        if (command[0] == '$') {
            char cpy_command[1024];
            strcpy(cpy_command, command);
            char *variable_name = strtok(cpy_command + 1, "=");
            if (variable_name != NULL) {
                char *variable_value = strtok(NULL, " ");
                if (variable_value != NULL) {
                    
                    char *name_without_spaces = no_spaces(variable_name);
                    char *value_without_spaces = no_spaces(variable_value);
                    
                    set_variable_value(name_without_spaces, value_without_spaces);

                    free(name_without_spaces);
                    free(value_without_spaces);
                }
            }
            continue;
        }

        // check for pipe
        for (int i = 0; i < strlen(command); i++) {
            if (command[i] == '|') {
                pipe_count++;
            }
        }

        // Check for prompt change command 
        if (strncmp(command, "prompt = ", 9) == 0) {
            strcpy(prompt, command + 9);
            strcat(prompt, " ");
            continue;
        }

        // Check for read command
        if (strncmp(command, "read ", 5) == 0) {
            char *variable_name = command + 5;
            
            char value[1024];
            fgets(value, 1024, stdin);
            value[strlen(value) - 1] = '\0';
            set_variable_value(variable_name, value);
            
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

        // check for cd command 
        if (strncmp(command, "cd ", 3) == 0) {
            char* directory = command + 3;
            if (directory == NULL) {
                printf("No directory specified\n");
            } else {
                if (chdir(directory) == -1) {
                    perror("chdir");
                }
            }
            continue;
        }

         // print all arguments echo command
        if (strncmp(command, "echo ", 5) == 0) {
            echo_func(command);
            continue;
        }

        // check for pipe
        if (pipe_count > 0) {
            char *token = strtok(command, "|");
            char *argv[pipe_count + 1];
            int i = 0;
            while (token != NULL)
            {
                argv[i] = token;
                token = strtok (NULL, "|");
                i++;
            }
            argv[i] = NULL;

            // create pipes
            int pipefd[pipe_count][2];
            for (int i = 0; i < pipe_count; i++) {
                if (pipe(pipefd[i]) == -1) {
                    perror("pipe");
                    exit(1);
                
                }
            }

            int pid;
            // create child processes
            for (int i = 0; i < pipe_count + 1; i++) {
                
                pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(1);
                } else if (pid == 0) {

                    signal(SIGINT, handle_sigint);
                    
                    if (i > 0) {
                        if(dup2(pipefd[i - 1][0], STDIN_FILENO) == -1 ) {
                            perror("dup2");
                            exit(1);
                        }
                    }

                    if (i < pipe_count) {
                        if(dup2(pipefd[i][1], STDOUT_FILENO) == -1 ) {
                            perror("dup2");
                            exit(1);
                        }
                    }

                    for (int j = 0; j < pipe_count; j++) {
                        if (i > 0 && j == i - 1) {
                            close(pipefd[j][1]);
                        } else if (i < pipe_count && j == i) {
                            close(pipefd[j][0]);
                        } else {
                            close(pipefd[j][0]);
                            close(pipefd[j][1]);
                        }
                    }

                    execute(argv[i]);
                }
            }

            for (int i = 0; i < pipe_count; i++) {
                close(pipefd[i][0]);
                close(pipefd[i][1]);
            }

            int status_pipe;
            for (int i = 0; i < pipe_count + 1; i++) {
                wait(&status_pipe);
            }

        } else { // execute the command normally
            int pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                signal(SIGINT, handle_sigint);
                execute(command);
            } else {
                add_child_procces(children, pid);
                wait(&status);
                if (WIFEXITED(status)) {
                    last_command_status = WEXITSTATUS(status);
                }
            }
        }
    }
    free_variables();
    return 0;
}

