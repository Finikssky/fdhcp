headers  = timer.h
dsources = timer.c
dobj = $(dsources:.c=.o)

all: $(dobj) 
	ar cr libtimer.a $(dobj)
	$(CC) -shared -o libdyntimer.so $(dobj) 
	cp libdyntimer.so ../../libs/libdyntimer.so
	cp libtimer.a ../../libs/libtimer.a

%.o:%.c $(headers)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

