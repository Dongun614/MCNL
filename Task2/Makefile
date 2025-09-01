CC = gcc
CFLAGS = -Wall -Wextra -O2
LDLIBS =

TARGETS = client server

all: $(TARGETS)

client: client.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

server: server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

.PHONY: all clean

clean:
	rm -f $(TARGETS)


