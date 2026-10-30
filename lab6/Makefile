CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -pthread

all: client server

src/common.o: src/common.c src/common.h
	$(CC) $(CFLAGS) -c src/common.c -o src/common.o

libcommon.a: src/common.o
	$(AR) rcs libcommon.a src/common.o

client: src/client.c src/common.h libcommon.a
	$(CC) $(CFLAGS) src/client.c libcommon.a -o client

server: src/server.c src/common.h libcommon.a
	$(CC) $(CFLAGS) src/server.c libcommon.a -o server

clean:
	rm -f client server src/common.o libcommon.a

.PHONY: all clean