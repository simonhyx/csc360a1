CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lreadline -lhistory

.phony all:
all: PMan.o inf.o

PMan.o: PMan.c
	$(CC) PMan.c $(CFLAGS) $(LDFLAGS) -o PMan.o

inf.o: inf.c
	$(CC) inf.c $(CFLAGS) -o inf.o

.PHONY clean:
clean:
	rm -f *.o
