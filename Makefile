CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c23 -pthread -g

all: threads

threads: threads.c
	$(CC) $(CFLAGS) -o threads threads.c

test: threads
	./threads

clean:
	rm -f threads
