CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LIBS = -lpthread

all: server client

server: server.c helpers.c helpers.h
	$(CC) $(CFLAGS) -o $@ server.c helpers.c $(LIBS)

client: client.c helpers.c helpers.h
	$(CC) $(CFLAGS) -o $@ client.c helpers.c $(LIBS)

clean:
	rm -f server client
