/*
 * Nikoleta Xenaki
 * AM:csd4968
 * Assignment1
 * 2023
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "declarations.h"

char path[BUFFER_SIZE]; /*global vars*/
int error = 0;

int main(int argc, char **argv) {
  char *username, cwd[BUFFER_SIZE], commands[BUFFER_SIZE];
  size_t len = 0;

  while (1) {
    if (getcwd(cwd, 256) ==NULL) { /*get the name of current working directory*/
      perror("getcwd");
    }

    username = getenv("USER"); /*get the current username*/

    if (username == NULL) {
      perror("Unable to get the user's name");
    } else {
      printf("csd4968-hy345sh@%s:%s$ ", username, cwd);
    }

    if (fgets(commands, sizeof(commands), stdin) == NULL) { /*gia to ctrl+D*/
      break;
    }

    len = strlen(commands); /*length of all commands in total*/
    if (len > 0 && commands[len - 1] == '\n') {
      commands[len - 1] = '\0';
    }

    execute_command(commands, len);
  }

  return 0;
}

void execute_command(char *commands, int length) {
  char *sub_command[BUFFER_SIZE];
  int i = 0;

  strcpy(path, "/bin/"); /*path for the commands to be executed*/

  if (check_for_continues_commands(commands, length) ==  1) { /*for continues commands*/

    sub_command[i] = strtok(commands, ";");

    while (sub_command[i] != NULL) {
      i++;
      sub_command[i] = strtok(NULL, ";");
    }

    i = 0;
    while (sub_command[i] != NULL) { /*for each sub commnad*/

      if (check_for_pipes(sub_command[i], strlen(sub_command[i])) == 1) {
        
         execute_pipe_command(sub_command[i]);

      } else if (check_for_redirection_output_or_append_command(sub_command[i], strlen(sub_command[i])) != 0 && check_for_redirection_input_command( sub_command[i], strlen(sub_command[i])) == 1) {
        
         execute_redirection_input_and_output_command(sub_command[i],strlen(sub_command[i]));

      } else if (check_for_redirection_input_command( sub_command[i], strlen(sub_command[i]) == 1)) {
      
         execute_redirection_input_command(sub_command[i],strlen(sub_command[i]));

      } else if (check_for_redirection_output_or_append_command(sub_command[i], strlen(sub_command[i])) != 0) {
         
         execute_redirection_output_command(sub_command[i],strlen(sub_command[i]));

      } else {

        if (error == -1) {  /*an yparxei kapoio syntax error kai mpei sto else na
                              mhn mpei kan sto kopo na kalestei h synarthsh*/
          error = 0;
          return;

        }

        execute_ordinary_command(sub_command[i]);
      }

      i++;
    }

  } else if (check_for_pipes(commands, length) == 1) { /*pipe command*/

    execute_pipe_command(commands);

  } else { /*just a command alone*/

    if (check_for_redirection_output_or_append_command(commands, length) != 0 && check_for_redirection_input_command(commands, length) == 1) {
     
      execute_redirection_input_and_output_command(commands, length);

    } else if (check_for_redirection_input_command(commands, length) == 1) {
       
       execute_redirection_input_command(commands, length);

    } else if (check_for_redirection_output_or_append_command(commands,length) != 0) {
      
      execute_redirection_output_command(commands, length);

    } else {

      if (error == -1) { /*an yparxei kapoio syntax error kai mpei sto else na
                            mhn mpei kan sto kopo na kalestei h synarthsh*/
        error = 0;
        return;
      }

      execute_ordinary_command(commands);
      error = 0; /*epanafora flag*/
    }
  }
}

void parse_command(char *commands,
  char *dest[BUFFER_SIZE]) { /*parse command according to the " " between the arguments*/
  char *replica;
  int i = 0;

  replica = malloc(sizeof(char) * BUFFER_SIZE);
  strcpy(replica, commands);

  dest[i] = strtok(replica, " ");

  while (dest[i] != NULL) {
    i++;
    dest[i] = strtok(NULL, " ");
  }
}

int check_for_continues_commands(char *commands, int length) {
  int i = 0, spaces = 0; /*spaces in the begging of the whole command*/

  while (i < length) {

    if (commands[i] == ' ') {
      
      spaces++;

    } else if (commands[i] == ';') {

      if ((i < length - 1 && commands[i + 1] == ';') || i == spaces) {
        perror("SYNTAX ERROR");
        error = -1;
        return 9;
      }

      return 1;

    } else { /* an vrethei opoios allos character ektos ; kanei restart to
                spaces*/
      spaces = 0;

    }

    i++;
  }

  return 0;
}

int check_for_pipes(char *commands, int length) {
  int i = 0, spaces = 0;

  while (i < length) {

    if (commands[i] == ' ') {

      spaces++;

    } else if (commands[i] == '|') {

      if ((i < length - 1 && commands[i + 1] == '|') || spaces == i ||commands[length - 1] == '|') {
        perror("SYNTAX ERROR");
        error = -1;
        return 9;
      }

      return 1;

    } else {
      spaces = 0;
    }

    i++;
  }

  return 0;
}

void execute_pipe_command(char *command) {
  char *arguments[BUFFER_SIZE], *sub_command[BUFFER_SIZE],*command_arguments[BUFFER_SIZE], *redirection_args[BUFFER_SIZE],*inner_redirectional_command[BUFFER_SIZE];
  int i = 0, fd[2], prev_fd[2], pid, num_of_pipe_commands = 0, end, input_file,output_file, count, return_value = 0;

  sub_command[i] = strtok(command, "|");

  while (sub_command[i] != NULL) {
    i++;
    sub_command[i] = strtok(NULL, "|");
  }

  num_of_pipe_commands = i;

  i = 0;
  while (sub_command[i] != NULL) { /*for each sub command between the pipe*/

    if (i == 0 && check_for_redirection_input_command(sub_command[i], strlen(sub_command[i])) == 1) {
      count = 0;
      inner_redirectional_command[count] = strtok(sub_command[count], "<");

      while (inner_redirectional_command[count] != NULL) {
        count++;
        inner_redirectional_command[count] = strtok(NULL, "<");
      }

      parse_command(inner_redirectional_command[0],command_arguments); /*the parsed command is the actual command to be executed*/
      parse_command(inner_redirectional_command[1],redirection_args); /*the parsed command is the INPUT file ofthe command*/

      input_file = open(redirection_args[0], O_RDONLY);

      if (input_file == -1) {
        perror("\nFILE ERROR");
        return;
      }

      count = 0; /*combining all the arguments toghether*/
      while (command_arguments[count] != NULL) {
        arguments[count] = command_arguments[count];
        count++;
      }

      end = count;
      count = 0;
      while (redirection_args[count] != NULL) {
        arguments[end] = redirection_args[count];
        count++;
        end++;
      }

      arguments[end] = NULL;

    } else if (i == num_of_pipe_commands - 1 &&check_for_redirection_output_or_append_command(sub_command[i], strlen(sub_command[i])) != 0) {
        
      return_value = check_for_redirection_output_or_append_command(sub_command[i], strlen(sub_command[i]));

      count = 0;
      if (return_value == 1) { /*an h entolh einai apla gia write arxeiou*/

        inner_redirectional_command[count] = strtok(sub_command[i], ">");

        while (inner_redirectional_command[count] != NULL) {
          count++;
          inner_redirectional_command[count] = strtok(NULL, ">");
        }

        parse_command(inner_redirectional_command[0],command_arguments); /*the parsed command is the actual command to be executed*/
        parse_command(inner_redirectional_command[1],redirection_args); /*the parsed command is the OUTPUT file of the command*/

        output_file = open(redirection_args[0], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

      } else { /*an h entolh einai apla gia append arxeiou*/

        inner_redirectional_command[count] = strtok(sub_command[i], ">>");

        while (inner_redirectional_command[count] != NULL) {
          count++;
          inner_redirectional_command[count] = strtok(NULL, ">>");
        }

        parse_command(inner_redirectional_command[0],command_arguments); /*the parsed command is the actual command to be executed*/
        parse_command(inner_redirectional_command[1], redirection_args); /*the parsed command is the OUTPUT file of the command*/

        output_file = open(redirection_args[0], O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
      }

      if (output_file == -1) {
        perror("\nFILE ERROR");
        return;
      }

      count = 0; /*combining all the arguments toghether*/
      while (command_arguments[count] != NULL) {
        arguments[count] = command_arguments[count];
        count++;
      }

      end = count;
      count = 1; /*gia na mhn pairnei ws input to output file*/
      while (redirection_args[count] != NULL) {
        arguments[end] = redirection_args[count];
        count++;
        end++;
      }

      arguments[end] = NULL;

    } else {
      parse_command(sub_command[i],arguments); /*the parsed command is stored in arguments*/
    }

    strcpy(path, "/bin/");
    strcat(path,arguments[0]); /*gia na dhmiourgei to path gia to ektelesimo arxeio*/

    if (i < num_of_pipe_commands-1) { /*create pipes for all commands except the last one*/

      if (pipe(fd) == -1) { /*creating the pipe*/
        perror("ERROR IN PIPE");
        exit(1);
      }
    }

    pid = fork();

    if (pid == 0) { /*Im in the 1st child process*/

      if (i == 0 && check_for_redirection_input_command(sub_command[i], strlen(sub_command[i])) == 1) {            /*ama exw redirectional input command*/
        dup2(input_file, STDIN_FILENO); /*REDIRECT STANDARD INPUT*/
        close(input_file);
      }

      if (i > 0) {
        close(prev_fd[1]);
        dup2(prev_fd[0],STDIN_FILENO); /* Redirect stdin to the read end of the pipe*/
        close(prev_fd[0]);
      }

      if ((i == num_of_pipe_commands - 1) && return_value != 0) {
        dup2(output_file, STDOUT_FILENO); /*REDIRECT STANDARD OUTPUT*/
        close(output_file);
      }

      if (i < num_of_pipe_commands - 1) { /*oles ektos ths teleutaias*/

        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO); /* Redirect stdout to the write end of the
                                       new pipe*/
        close(fd[1]);
      }

      if (execv(path, arguments) == -1) {
        perror("\nERROR IN EXECV() 11\n");
        exit(0);
      }

    } else if (pid == -1) {
      perror("ERROR WITH FORK 1");
      return;
    }

    /*PARENT HERE*/

    if (i > 0) { /*release the old file discriptors*/
      close(prev_fd[0]);
      close(prev_fd[1]);
    }

    prev_fd[0] = fd[0];
    prev_fd[1] = fd[1];

    wait(NULL);

    i++;
  }
}

int check_for_redirection_input_command(char *command, int length) {
  int i = 0, flag = 0;

  while (i < length) {

    if (command[i] == '<') {

      if (flag == 1) {
        perror("SYNTAX ERROR");
        error = -1;
        return 0;
      }

      flag = 1;
    }
    i++;
  }

  return flag;
}

int check_for_redirection_output_or_append_command(char *command, int length) {
  int i = 0, flag = 0;

  while (i < length) {

    if (command[i] == '>') {

      if (flag == 2) {
        perror("SYNTAX ERROR");
        error = -1;
        return 0;
      }

      if (flag == 1) {
        flag = 2;
      } else {
        flag = 1;
      }

    }
    i++;
  }

  return flag;
}

void execute_ordinary_command(char *command) {
  char *arguments[BUFFER_SIZE];
  int id;

  parse_command(command,
                arguments); /*the parsed command is stored in arguments*/

  if (arguments[0] != NULL) { /*for quit and chdir*/

    if (strcmp(arguments[0], "quit") == 0) {
        exit(0);
    } else if (strcmp(arguments[0], "chdir") == 0) {

      if (arguments[1] == NULL) {
        chdir(getenv("HOME"));
      } else {
        if (chdir(arguments[1]) == -1) { /*ean den vrei to arxeio*/
          error = -1;
          perror("chdir");
        }
      }

      return;
    }

    id = fork();

    if (id == -1) {
      perror("\nERROR WITH FORK");
      return;
    }

    if (id == 0) { /*Im in the child process*/

      strcat(path,arguments[0]); /*gia na dhmiourgei to path gia to ektelesimo arxeio*/

      if (execv(path, arguments) == -1) {
        perror("\nERROR IN EXECV()");
        exit(0);
      }
    } else {
      wait(NULL);
    }
  }
}

void execute_redirection_input_and_output_command(char *commands, int length) {
  char *sub_command[BUFFER_SIZE], *inner_sub_command[BUFFER_SIZE];
  char *arguments[BUFFER_SIZE], *all_arguments[BUFFER_SIZE], *redirection_input__args[BUFFER_SIZE],*redirection_output__args[BUFFER_SIZE];
  int id, i = 0, end;
  int input_file, output_file, return_value;

  return_value =check_for_redirection_output_or_append_command(commands, length);
  i = 0;

  sub_command[i] = strtok(commands, "<");

  while (sub_command[i] !=NULL) { /*sub_command[0] is the command BEFORE < (THE ACTUAL COMMAND)*/
         i++; /*sub_command[1] is the command AFTER > (the path for the file and the command with > or >>)*/
        sub_command[i] = strtok(NULL, "<");
    }

  parse_command(sub_command[0], arguments);

  i = 0;
  if (return_value == 1) {
    inner_sub_command[i] = strtok(sub_command[1], ">");

    while (inner_sub_command[i] != NULL) {
      i++;
      inner_sub_command[i] = strtok(NULL, ">");
    }

    parse_command(inner_sub_command[1],redirection_output__args); /*the parsed command is the OUTPUT file of the command*/

    output_file = open(redirection_output__args[0], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

  } else {

    inner_sub_command[i] = strtok(sub_command[1], ">>");

    while (inner_sub_command[i] != NULL) {
      i++;
      inner_sub_command[i] = strtok(NULL, ">>");
    }

    parse_command(inner_sub_command[1], redirection_output__args); /*the parsed command is the OUTPUT file of the command*/

    output_file = open(redirection_output__args[0], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  }

  parse_command(inner_sub_command[0],redirection_input__args); /*the parsed command is the INPUT file of the command*/

  input_file = open(redirection_input__args[0], O_RDONLY);

  if (input_file == -1) {
    perror("\nFILE ERROR");
    return;
  }

  i = 0; /*combining all the arguments toghether*/
  while (arguments[i] != NULL) {
    all_arguments[i] = arguments[i];
    i++;
  }

  end = i;
  i = 0;
  while (redirection_input__args[i] != NULL) {
    all_arguments[end] = redirection_input__args[i];
    i++;
    end++;
  }

  i = 1;
  while (redirection_output__args[i] != NULL) {
    all_arguments[end] = redirection_output__args[i];
    i++;
    end++;
  }

  all_arguments[end] = NULL;

  id = fork();

  if (id == -1) {
    perror("ERROR WITH FORK");
    return;
  }

  if (id == 0) {
    dup2(output_file, STDOUT_FILENO); /*REDIRECT STANDARD OUTPUT*/
    close(output_file);

    dup2(input_file, STDIN_FILENO); /*REDIRECT STANDARD INPUT*/
    close(input_file);

    strcpy(path, "/bin/");
    strcat(path, arguments[0]);

    if (execv(path, all_arguments) == -1) {
      perror("\nERROR IN EXECV()");
      exit(0);
    }
  } else {
    wait(NULL);
  }
}

void execute_redirection_input_command(char *commands, int length) {
  char *sub_command[BUFFER_SIZE];
  char *arguments[BUFFER_SIZE], *redirection_args[BUFFER_SIZE],*all_arguments[BUFFER_SIZE];
  int id, i = 0, end;
  int input_file;

  i = 0;
  sub_command[i] = strtok(commands, "<");

  while (sub_command[i] != NULL) {
    i++;
    sub_command[i] = strtok(NULL, "<");
  }

  parse_command(sub_command[0],arguments); /*the parsed command is the actual command to be executed*/
  parse_command(sub_command[1],redirection_args); /*the parsed command is the INPUT file of the command*/

  input_file = open(redirection_args[0], O_RDONLY);

  if (input_file == -1) {
    perror("\nFILE ERROR");
    return;
  }

  i = 0; /*combining all the arguments toghether*/
  while (arguments[i] != NULL) {
    all_arguments[i] = arguments[i];
    i++;
  }

  end = i;
  i = 0;
  while (redirection_args[i] != NULL) {
    all_arguments[end] = redirection_args[i];
    i++;
    end++;
  }

  all_arguments[end] = NULL;

  id = fork();

  if (id == -1) {
    perror("ERROR WITH FORK");
    return;
  }

  if (id == 0) {
    dup2(input_file, STDIN_FILENO); /*REDIRECT STANDARD INPUT*/
    close(input_file);

    strcpy(path, "/bin/");
    strcat(path, arguments[0]);

    if (execv(path, all_arguments) == -1) {
      perror("\nERROR IN EXECV()");
      exit(0);
    }

  } else {
    wait(NULL);
  }
}

void execute_redirection_output_command(char *commands, int length) {
  char *sub_command[BUFFER_SIZE];
  char *arguments[BUFFER_SIZE], *redirection_args[BUFFER_SIZE],*all_arguments[BUFFER_SIZE];
  int id, i = 0, end;
  int output_file, return_value;

  return_value = check_for_redirection_output_or_append_command(commands, length);
  i = 0;

  if (return_value == 1) { /*an h entolh einai apla gia write arxeiou*/
    sub_command[i] = strtok(commands, ">");

    while (sub_command[i] != NULL) {
      i++;
      sub_command[i] = strtok(NULL, ">");
    }

    parse_command(sub_command[0], arguments); /*the parsed command is the actual command to be executed*/
    parse_command(sub_command[1], redirection_args); /*the parsed command is the OUTPUT file of the command*/

    output_file = open(redirection_args[0], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

  } else { /*an h entolh einai apla gia append arxeiou*/

    sub_command[i] = strtok(commands, ">>");

    while (sub_command[i] != NULL) {
      i++;
      sub_command[i] = strtok(NULL, ">>");
    }

    parse_command( sub_command[0], arguments); /*the parsed command is the actual command to be executed*/
    parse_command( sub_command[1], redirection_args); /*the parsed command is the OUTPUT file of the command*/

    output_file = open(redirection_args[0], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  }

  if (output_file == -1) {
    perror("\nFILE ERROR");
    return;
  }

  i = 0; /*combining all the arguments toghether*/
  while (arguments[i] != NULL) {
    all_arguments[i] = arguments[i];
    i++;
  }

  end = i;
  i = 1; /*gia na mhn pairnei ws parametro to output file*/
  while (redirection_args[i] != NULL) {
    all_arguments[end] = redirection_args[i];
    i++;
    end++;
  }

  all_arguments[end] = NULL;
  id = fork();

  if (id == -1) {
    perror("\nERROR WITH FORK");
    return;
  }

  if (id == 0) {
    dup2(output_file, STDOUT_FILENO); /*REDIRECT STANDARD OUTPUT*/
    close(output_file);

    strcpy(path, "/bin/"); /*kalou kakou*/
    strcat(path, arguments[0]);

    if (execv(path, all_arguments) == -1) {
      perror("\nERROR IN EXECV()");
      exit(0);
    }

  } else {
    wait(NULL);
  }
}