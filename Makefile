CC=gcc
CFLAGS=-Wall -Werror -Iinclude -g3

all: md5.o filestuff.o tcp.o

md5.o: src/md5.c include/md5.h
	$(CC) $(CFLAGS) -c $< -o $@

tcp.o: src/tcp.c include/tcp.h
	$(CC) $(CFLAGS) -c $< -o $@

filestuff.o: src/filestuff.c include/filestuff.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o

.PHONY: all clean
