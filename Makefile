CC = gcc
CFLAGS = -I./include/libqueue -I./include/libdhcp -I./include/libdctp -I./include/libtimer -I./include -I./include/libcommon -Wall -O3
 
all: 
	make -C src/
	$(CC) $(CFLAGS) -o dcc dcc.c -L./libs -ldyndctp -ldyndhcp -ldynqueue -ldyncommon -lpthread
	$(CC) $(CFLAGS) -o dsc dsc.c -L./libs -ldyndctp -ldyndhcp -ldynqueue -ldyntimer -ldyncommon -lpthread

debug:
	make CFLAGS="$(CFLAGS) -g" all


utils:
	$(CC) $(CFLAGS) -o sender sender.c -L./libs -ldyndctp -ldyncommon
	$(CC) $(CFLAGS) -o test_nagr dnagr.c -L./libs -ldyndhcp -ldynqueue -ldyncommon -pthread

clean: 
	make -C src/ clean
	rm -f *.o
	rm -f *~
	rm -f dcc
	rm -f dsc
	rm -f sender
	rm -f libs/*
	rm -Rf install
