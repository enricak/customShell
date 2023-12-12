CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g -fsanitize=address

all: mysh

mysh: mysh.c  mysh.h
	$(CC) $(CFLAGS) -o mysh mysh.c 

clean:
	rm -f mysh *.o *.out 

.PHONY: all clean