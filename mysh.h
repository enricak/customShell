#include <stdio.h>
#include <stdlib.h>

//functiondef
int bareNameCheck(char *argv);
void checkFileInDirectory(const char *filename, char *path);
int execute_command(char *argv[], int inputFD, int outputFD);
int execute_pipe_command(char *args1[], char *args2[], int inputFD, int outputFD);
void expand_wildcards(char *arg, char **argv, int *argc);
int parse_and_execute(char *command);

void interactive_mode();
void batch_mode(const char *filename);