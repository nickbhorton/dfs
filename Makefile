CC=gcc
CFLAGS=-Wall -Werror -Iinclude -g3

all: dfs

dfs: dfs.o md5.o tcp.o filestuff.o
	$(CC) -o $@ $^ $(CFLAGS)


dfs.o: src/dfs.c include/macros.h
	$(CC) $(CFLAGS) -c $< -o $@
	
md5.o: src/md5.c include/md5.h
	$(CC) $(CFLAGS) -c $< -o $@

tcp.o: src/tcp.c include/tcp.h
	$(CC) $(CFLAGS) -c $< -o $@

filestuff.o: src/filestuff.c include/filestuff.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o

.PHONY: all clean
