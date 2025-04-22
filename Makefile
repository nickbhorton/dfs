CC=gcc
CFLAGS=-Wall -Werror -Iinclude -g3

all: dfs dfc testcli

dfs: dfs.o md5.o tcp.o filestuff.o protocol.o
	$(CC) -o $@ $^ $(CFLAGS)

dfc: dfc.o md5.o tcp.o filestuff.o protocol.o
	$(CC) -o $@ $^ $(CFLAGS)

testcli: testcli.o md5.o tcp.o filestuff.o protocol.o
	$(CC) -o $@ $^ $(CFLAGS)


dfs.o: src/dfs.c include/macros.h
	$(CC) $(CFLAGS) -c $< -o $@

dfc.o: src/dfc.c include/macros.h
	$(CC) $(CFLAGS) -c $< -o $@

testcli.o: src/testcli.c include/macros.h
	$(CC) $(CFLAGS) -c $< -o $@
	
md5.o: src/md5.c include/md5.h
	$(CC) $(CFLAGS) -c $< -o $@

tcp.o: src/tcp.c include/tcp.h
	$(CC) $(CFLAGS) -c $< -o $@

filestuff.o: src/filestuff.c include/filestuff.h
	$(CC) $(CFLAGS) -c $< -o $@

protocol.o: src/protocol.c include/protocol.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f dfs dfc testcli
	rm -rf dfs1
	rm -rf .dir1 .dir2 .dir3 .dir4

.PHONY: all clean
