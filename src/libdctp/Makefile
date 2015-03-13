headers=dctp.h	
dsources=dctp.c 
dobj=$(dsources:.c=.o)
 
CC=gcc
CFLAGS  = -I../../include/libdctp -I../../include/libdhcp -I../../include/libqueue

all: $(dobj) 
	ar cr libdctp.a $(dobj)
	$(CC) -shared -o libdyndctp.so $(dobj) 
	cp libdyndctp.so ../../libs/libdyndctp.so
	cp libdctp.a ../../libs/libdctp.a

%.o:%.c $(headers)
	$(CC) $(CFLAGS) -fPIC -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

