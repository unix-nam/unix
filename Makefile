CC = gcc
CFLAGS = -Wall -g

all: server client

server: server.c battleship.c
	$(CC) $(CFLAGS) -o server server.c battleship.c

client: client.c battleship.c
	$(CC) $(CFLAGS) -o client client.c battleship.c

clean:
	rm -f server client
