#include "key_attempts.h"

size_t buffer_length = 0;
struct termios original_termios;

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    clear_line();
}

void enable_raw_mode()
{
    // Get the terminal attributes
    tcgetattr(STDIN_FILENO, &original_termios);

    // Set the terminal attributes to raw mode
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    atexit(disable_raw_mode);
}

void clear_line()
{
    printf("\r\033[K");
}

void print_prompt(char prompt[1024], char input_buffer[1024])
{
    printf("%s%s", prompt, input_buffer);
    fflush(stdout);
}

int handle_arrow_key(char seq[2], char prompt[1024], char input_buffer[1024], char prev_comms[20][1024], int curr_index, int comm_count)
{
    if(comm_count == 0) {
        return curr_index;
    }
    
    clear_line();
    if (seq[1] == 'A' && curr_index < comm_count - 1)
    { // Up arrow
        curr_index++;
    }
    else if (seq[1] == 'B' && curr_index > 0)
    { // Down arrow
        curr_index--;
    }
    else if (seq[1] == 'C' || seq[1] == 'D')
    { 
        return curr_index;
    }

    // Copy selected string to buffer
    strncpy(input_buffer, prev_comms[curr_index], 1023);
    input_buffer[1023] = '\0'; // Ensure null-terminated
    buffer_length = strlen(input_buffer);
    print_prompt(prompt, input_buffer);

    return curr_index;
}

void handle_character_input(char c, char prompt[1024], char input_buffer[1024])
{
    if (buffer_length < 1023)
    {
        input_buffer[buffer_length++] = c;
        input_buffer[buffer_length] = '\0';
    }
    clear_line();
    print_prompt(prompt, input_buffer);
}

int get_command(char prompt[1024], char input_buffer[1024], char prev_comms[20][1024], int comms_count)
{
    buffer_length = 0;
    
    enable_raw_mode(&original_termios);

    memset(input_buffer, 0, 1024);      // Initialize the input buffer
    print_prompt(prompt, input_buffer); // Print the initial prompt

    int curr_index = -1;

    while (1)
    {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1)
        {
            perror("read");
            return -1;
        }

        if (c == '\x1b')
        { // Escape sequence
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == -1 || read(STDIN_FILENO, &seq[1], 1) == -1)
            {
                perror("read");
                exit(EXIT_FAILURE);
            }

            if (seq[0] == '[')
            {
                curr_index = handle_arrow_key(seq, prompt, input_buffer, prev_comms, curr_index, comms_count);
            }
        }
        else if (c == 127 || c == '\b')
        { // Handle backspace
            if (buffer_length > 0)
            {
                input_buffer[--buffer_length] = '\0';
            }
            curr_index = -1;
            clear_line();
            print_prompt(prompt, input_buffer);
        }
        else
        {
            if (c == '\n' || c == '\r')
            {
                break;
            }

            // Reset index to 0 for any key other than up/down arrow keys
            curr_index = -1;
            clear_line();
            print_prompt(prompt, input_buffer);
            handle_character_input(c, prompt, input_buffer);
        }
    }

    printf("\n");
    disable_raw_mode();

    return 0;
}