#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

void disable_raw_mode();

void enable_raw_mode();

void clear_line();

void print_prompt(char prompt[1024], char input_buffer[1024]);

int handle_arrow_key(char seq[2], char prompt[1024], char input_buffer[1024], char prev_comms[20][1024], int curr_index, int comm_count);

void handle_character_input(char c, char prompt[1024], char input_buffer[1024]);

int get_command(char prompt[1024], char input_buffer[1024], char prev_comms[20][1024], int comms_count);