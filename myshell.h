#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
#include "key_attempts.h"

#define MAX_ARG 20

typedef struct {
    char *name;
    char *value;
} variable, *Pvariable;

void add_child_procces(pid_t children[20], pid_t child);

char* get_variable_value(char *name);

void echo_func(char *command);

void remove_variable(char *name);

void set_variable_value(char *name, char *value);

void free_variables();

char* no_spaces(char* str);

void initialize_history_commands(char history_commands[20][1024]);

void add_to_history_commands(char *command, char history_commands[20][1024]);

char *get_last_command(char history_commands[20][1024]);

void handle_sigint(int sig);

void execute(char* command);

int evaluate_condition(char *condition);

void handle_if_else(char *command);