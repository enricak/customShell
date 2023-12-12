**mysh** is a basic simple command line shell implemented in C. The provided Makefile can be used to generate the mysh executable file.
It can run in either batch or interactive mode. It enters through the main function and checks the command-line arguments. If a file is provided, it runs in batch mode; otherwise, it runs in interactive mode.

Then it runs the following functions:

bareNameCheck function checks if a given command (argv) contains a '/' character, indicating an absolute or relative path and is used in the execute_command function. If a '/' is found, it prints its index in the string; otherwise, it declares the command as a bare name.

checkFileInDirectory searches for a file (filename) in predefined directories (/usr/local/bin/, /usr/bin/, /bin/). If the file is found, it returns the full path; otherwise, it prints an error message and returns NULL.

execute_command executes a single command specified by the argument vector argv. It handles built-in commands such as exit, cd, which, and pwd. For external commands, it forks a new process and uses execvp to execute the command.

execute_pipe_command executes a command involving a pipe (|) by creating two child processes connected by a pipe. It redirects the standard input/output for each child accordingly and waits for their completion.

expand_wildcards expands wildcard characters (*) in a command argument using the glob function, which searches for files matching the pattern and replaces the argument with the matching file names.

parse_and_execute parses a given command, tokenizes it, and executes the corresponding command or commands. Handles input/output redirection, wildcards, and pipes.




**Testing**

In order to test this program, a variety of tests were used.

To test batch mode, a file called test.sh and test2.sh were used and the results were compared to normal bash console output. To test interactive mode, the commands in the previously mentioned files were inputed one by one. The tests were designed to check all four possible combinations of "then" and "else" such as "true command then false command", "true command then true command", "true command else false command", and "false command else false command". Piping was tested through the tests in test3.sh. Each test was run separately in normal bash mode to see the intended input and then in interactive mode. In addition, we created nested subdirectories to test the pwd, cd, and exit commands. 
