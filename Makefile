CC=gcc
CFLAGS=-Wall -Werror -Iinclude -g3

all: md5.o

md5.o: src/md5.c include/md5.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o

.PHONY: all clean
