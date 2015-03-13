CC = gcc
CFLAGS = -I./include/libqueue -I./include/libdhcp -I./include/libdctp -I./include/libtimer -I./include -Wall -O3
 
all: 
	make -C src/
	$(CC) $(CFLAGS) -o dcc dcc.c -L./libs -ldyndctp -ldyndhcp -ldynqueue -lpthread
	$(CC) $(CFLAGS) -g -o dsc dsc.c -L./libs -ldyndctp -ldyndhcp -ldynqueue -ldyntimer -lpthread

utils:
	$(CC) $(CFLAGS) -o sender sender.c -L./libs -ldyndctp

clean: 
	make -C src/ clean
	rm -f *.o
	rm -f *~
	rm -f dcc
	rm -f dsc
	rm -f sender
	rm -f libs/*
	rm -Rf install
