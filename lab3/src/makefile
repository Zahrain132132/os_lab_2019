CC=gcc
CFLAGS=-I.

all: sequential_min_max parallel_min_max fork_exec

sequential_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

fork_exec: fork_exec.c sequential_min_max
	$(CC) -o fork_exec fork_exec.c $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max fork_exec
