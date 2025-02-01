CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lssl -lcrypto

all: server run


server: main.o server.o websocket.o utils.o
	$(CC) $(CFLAGS) -o server main.o server.o websocket.o utils.o $(LIBS)

run:
	./server

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o server


