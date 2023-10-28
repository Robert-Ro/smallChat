all: smallchat

smallchat: smallchat.c
	$(CC) smallchat.c -o smallchat.o -O2 -Wall -W

clean:
	rm -f smallchat.o
