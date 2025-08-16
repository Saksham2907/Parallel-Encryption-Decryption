CC=gcc
CFLAGS=-O2 -Wall -pthread
LDFLAGS=-pthread

all: parallel_crypto

parallel_crypto: main.o chunk_queue.o
	$(CC) $(CFLAGS) -o parallel_crypto main.o chunk_queue.o $(LDFLAGS)

main.o: main.c chunk_queue.h
	$(CC) $(CFLAGS) -c main.c

chunk_queue.o: chunk_queue.c chunk_queue.h
	$(CC) $(CFLAGS) -c chunk_queue.c

clean:
	rm -f *.o parallel_crypto
