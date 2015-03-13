headers=queue.h
libqsources:=getmesstext.c pushmessage.c popmessage.c printmessage.c initsync.c deletehead.c genrandmessage.c reversemessage.c
libqobj:=$(libqsources:.c=.o) 

CC=gcc
CFLAGS = -I../../include/libqueue 

all: $(libqobj) 
	ar cr libqueue.a $(libqobj)
	$(CC) $(CFLAGS) -shared -o libdynqueue.so $(libqobj)
	cp libdynqueue.so ../../libs/libdynqueue.so
	cp libqueue.a ../../libs/libqueue.a
	
%.o:%.c $(headers)
	$(CC) $(CFLAGS) -fPIC -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

