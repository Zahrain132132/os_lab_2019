
CC = gcc
CFLAGS = -Wall -Wextra

PROGRAMS = tcpclient tcpserver udpclient udpserver

.PHONY: all clean

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)
