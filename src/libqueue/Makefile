headers := queue.h
libqsources := push.c pop.c init.c delete.c
libqobj := $(libqsources:.c=.o) 

all: $(libqobj) 
	ar cr libqueue.a $(libqobj)
	$(CC) $(CFLAGS) -shared -o libdynqueue.so $(libqobj)
	cp libdynqueue.so ../../libs/libdynqueue.so
	cp libqueue.a ../../libs/libqueue.a
	
%.o:%.c $(headers)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

