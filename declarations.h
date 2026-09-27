
/*
* Nikoleta Xenaki
* AM:csd4968
* Assignment1
* 2023
*/

#define BUFFER_SIZE 256

void execute_command( char* command,int length);

void execute_ordinary_command(char* command);

void parse_command(char* commands,char*dest[BUFFER_SIZE]);

int check_for_continues_commands(char*commands,int length);

int check_for_pipes(char*commands,int length);

void execute_pipe_command(char* command);

int check_for_redirection_input_command(char* command,int length);

int check_for_redirection_output_or_append_command(char* command,int length);

void execute_redirection_input_and_output_command(char* commands,int length);

void execute_redirection_input_command(char* commands,int length);

void execute_redirection_output_command(char* commands,int length);