
CFLAGS = -g -Wall -Og -std=c99 
LDFLAGS = 
CC = $(CROSS_COMPILE)gcc

all: aesdsocket
 
aesdsocket: aesdsocket.o
	$(CC) $< -o $@

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.bin 
