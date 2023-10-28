all: maplechat smallchat
smallchat: smallchat.c
	$(CC) smallchat.c -o smallchat.o -O2 -Wall -W
maplechat: maplechat.c
	$(CC) maplechat.c -o maplechat.o -O2 -Wall -W

clean:
	rm -f smallchat.o
	rm -f maplechat.o
