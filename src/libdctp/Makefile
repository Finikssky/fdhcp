headers=dctp.h	
dsources=dctp.c 
dobj=$(dsources:.c=.o)

all: $(dobj) 
	ar cr libdctp.a $(dobj)
	$(CC) -shared -o libdyndctp.so $(dobj) 
	cp libdyndctp.so ../../libs/libdyndctp.so
	cp libdctp.a ../../libs/libdctp.a

%.o:%.c $(headers)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

