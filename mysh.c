#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "mysh.h"

#define MAX_COMMAND_LENGTH 2097152 
#define MAX_ARGS 100
#define MAX_PATH_LENGTH 256

int bareNameCheck(char *argv) {
    char ch = '/'; 
    char *result = strchr(argv, ch); 
    // int index = (int) (result - argv);

    if (result != NULL) {
        // printf("'%c' found at index %d in the string: %s\n", ch, index, argv);
        return 1; 
    } else {
        // printf("'%c' not found in the string: %s\n", ch, argv); //is a bare name 
        return 0; 
    }

}

void checkFileInDirectory(const char *filename, char *path) { //this method works for paths through directories in workingdir as well as filenames 
    const char *directories[] = { "/usr/local/bin/", "/usr/bin/", "/bin/"}; 
    const int numDirectories = sizeof(directories) / sizeof(directories[0]);

    //clear path array before loop?

    for (int i = 0; i < numDirectories; ++i) {
        snprintf(path, MAX_PATH_LENGTH, "%s%s", directories[i], filename);

        // Check if the file exists
        if (access(path, F_OK) == 0) {
            // printf("File '%s' found at path: %s\n", filename, path);
            return; // File exists
        }
        
    }

    printf("File '%s' not found in the specified directories.\n", filename);
    path = NULL;
    return; // File does not exist
}

int execute_command(char *argv[], int inputFD, int outputFD) {

    if (strcmp(argv[0], "exit") == 0) {
        printf("Exiting mysh...\n");
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        // cd command
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "cd: Error Number of Parameters\n");
        } else if (chdir(argv[1]) != 0) {
            perror("cd");
        }

        //printf("Changed directory to %s\n", argv[1]);

    } else if (strcmp(argv[0], "which") == 0) {
        //which command
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "which: Error Number of Parameters\n");
        }
    
        char path[MAX_PATH_LENGTH];
        checkFileInDirectory(argv[1], path);
        if (path == NULL) {
            perror("which");
        }

    } else if (strcmp(argv[0], "pwd") == 0) {
        // pwd command
        char cwd[MAX_COMMAND_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } 
        else {
            perror("pwd");
        }

    } else { // external command
        if (bareNameCheck(argv[0]) == 0){
            char fullPath[MAX_PATH_LENGTH];
            checkFileInDirectory(argv[0], fullPath);
            free(argv[0]);
            argv[0] = malloc(MAX_PATH_LENGTH);
            strcpy(argv[0], fullPath);
        }

        
        pid_t pid = fork();
        if (pid == -1) {

            perror("fork");
        } else if (pid == 0) { 
            // child progress
            if (inputFD != STDIN_FILENO) {
                dup2(inputFD, STDIN_FILENO);
                close(inputFD);
            }
            if (outputFD != STDOUT_FILENO) {
                dup2(outputFD, STDOUT_FILENO);
                close(outputFD);
            }

            if (execv(argv[0], argv) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            // father progress
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status); // Return the exit status of the child
            } else {
                return -1; // Indicate an error if the child didn't exit normally
}
        }
    }
    return 0;
}

int execute_pipe_command(char *args1[], char *args2[], int inputFD, int outputFD) {
    int pipe_fds[2];
    pid_t pid1, pid2;
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 == 0) {
        // first child
        close(pipe_fds[0]); 
        dup2(pipe_fds[1], outputFD);   //Redirects standard input to the read side of the pipe
        close(pipe_fds[1]);

        execvp(args1[0], args1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        // second child
        close(pipe_fds[1]); 
        dup2(pipe_fds[0], inputFD);  //Redirects standard input to the read side of the pipe
        close(pipe_fds[0]);

        execvp(args2[0], args2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // father
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    if (WIFEXITED(status2) ) {
        return WEXITSTATUS(status2); // Return the exit status of the last child
    } 
    return -1; // Indicate an error if the last child didn't exit normally
    
}

void expand_wildcards(char *arg, char **argv, int *argc) {
    glob_t glob_result;
    size_t i;
    // GLOB_NOCHECK means that `glob` will return to the original pattern if no matching file is found.
    // GLOB_TILDE allows the wave symbol (`~`) to be used in the pattern to expand to the user's home directory.
    if (glob(arg, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
        for (i = 0; i < glob_result.gl_pathc && *argc < MAX_ARGS - 1; i++) {
            argv[(*argc)++] = strdup(glob_result.gl_pathv[i]);
        }
    }
    globfree(&glob_result);
}

int parse_and_execute(char *command) {
    char *args[MAX_ARGS];           
    char *args_pipe[MAX_ARGS];      
    int argc = 0, argc_pipe = 0;    
    int inputFD = STDIN_FILENO;  
    int outputFD = STDOUT_FILENO; 
    int pipe_found = 0;            
    int status = 0;

    char *token = strtok(command, " ");

    while (token != NULL) {
        if (strcmp(token, "|") == 0) {
            pipe_found = 1;
            args[argc] = NULL; // End the first command's arguments
            // Process the second part of the command after the pipe
            token = strtok(NULL, " ");
            while (token != NULL && argc_pipe < MAX_ARGS - 1) {
                if (strcmp(token, ">") == 0) {
                    // Handle output redirection for the second command
                    char* redirectedOutput = strtok(NULL, " ");

                    outputFD = open(redirectedOutput, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (outputFD == -1) {
                        perror("Error opening output file");
                        exit(EXIT_FAILURE);
                    }
                } else if (strcmp(token, "<") == 0) {  
                    char* redirectedOutput = strtok(NULL, " ");

                    inputFD = open(redirectedOutput, O_RDONLY, 0666);
                    if (inputFD == -1) {
                        perror("Error opening input file");
                        exit(EXIT_FAILURE);
                    }
                } else if (strchr(token, '*')) {
                    // If a wildcard is found, expand it
                    expand_wildcards(token, args, &argc);
                } else {
                    args_pipe[argc_pipe++] = strdup(token);
                }
                token = strtok(NULL, " ");
            }
            args_pipe[argc_pipe] = NULL;
            break;
        } else if (strcmp(token, ">") == 0) {
            // Handle output redirection for the second command
            char* redirectedOutput = strtok(NULL, " ");

            outputFD = open(redirectedOutput, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outputFD == -1) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(token, "<") == 0) {  
            char* redirectedOutput = strtok(NULL, " ");

            inputFD = open(redirectedOutput, O_RDONLY, 0666);
            if (inputFD == -1) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
        } else if (strchr(token, '*')) {
            // If a wildcard is found, expand it
            expand_wildcards(token, args, &argc);
        } else {
            args[argc++] = strdup(token);
        }
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

    if (pipe_found) {
        status = execute_pipe_command(args, args_pipe, inputFD, outputFD);
        // Free arguments
        for (int i = 0; i < argc; i++) free(args[i]);
        for (int i = 0; i < argc_pipe; i++) free(args_pipe[i]);
    } else {
        status = execute_command(args, inputFD, outputFD);
        // Free arguments
        for (int i = 0; i < argc; i++) free(args[i]);
    }

    // Close file descriptors if they were opened for redirection
    if (inputFD != STDIN_FILENO) close(inputFD);
    if (outputFD != STDOUT_FILENO) close(outputFD);
    
    return status;
}

void interactive_mode() {
    char command[MAX_COMMAND_LENGTH];
    int lastCommandStatus = 0;

    printf("Welcome to the shell!\n");
    while (1) {
        printf("mysh> ");
        if (!fgets(command, MAX_COMMAND_LENGTH, stdin)) {
            break; // Exit loop on read error
        }

        command[strcspn(command, "\n")] = 0; // Remove newline character

        if (strcmp(command, "exit") == 0) {
            printf("mysh: exiting\n");
            break;
        }

        // Check for 'then' or 'else' at the start of the command
        if (strncmp(command, "then ", 5) == 0 || strncmp(command, "else ", 5) == 0) {
            int isThen = strncmp(command, "then ", 5) == 0;
            int shouldExecute = (isThen && lastCommandStatus == 0) || (!isThen && lastCommandStatus != 0);

            if (!shouldExecute) {
                lastCommandStatus = 1; // Set status to failure if condition is not met
                continue;
            }

            // Shift the command string to remove 'then ' or 'else '
            memmove(command, command + 5, strlen(command) - 4);
        }

        lastCommandStatus = parse_and_execute(command);
    }
}

void batch_mode(const char *filename) {
    char command[MAX_COMMAND_LENGTH];
    int lastCommandStatus = 0; // Initialize the last command status

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("open file error");
        exit(EXIT_FAILURE);
    }

    while (fgets(command, MAX_COMMAND_LENGTH, file)) {
        // Remove newline character
        command[strcspn(command, "\n")] = 0;

        if (command[0] == '#') {
            // Skip comment lines
            continue;
        }
        // Check for 'then' or 'else' at the start of the command
        if (strncmp(command, "then ", 5) == 0 || strncmp(command, "else ", 5) == 0) {
            int isThen = strncmp(command, "then ", 5) == 0;
            int shouldExecute = (isThen && lastCommandStatus == 0) || (!isThen && lastCommandStatus != 0);

            // Shift the command string to remove 'then ' or 'else '
            memmove(command, command + 5, strlen(command) - 4);

            if (!shouldExecute || command[0] == '\0') {
                // Skip execution if condition not met or if there's no command following 'then'/'else'
                lastCommandStatus = 1; // Set status to failure
                continue;
            }
        }

        // Execute the command and store the status
        lastCommandStatus = parse_and_execute(command);
    }

    fclose(file);
}


int main(int argc, char *argv[]) {
    if (argc == 2) {
        // batch_mode
        batch_mode(argv[1]);
    } else {
        // interactive_mode first
        interactive_mode();
    }
    return 0;
}