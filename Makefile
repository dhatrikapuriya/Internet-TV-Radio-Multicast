# Dhruvil Dave AU1841003

.PHONY: all

all:
	gcc -o server.out server/server.c -lpthread
	gcc -o client.out client/client.c -lpthread
