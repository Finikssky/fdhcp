headers=common.h
dsources=common.c
dobj=$(dsources:.c=.o)

all: $(dobj) 
	ar cr libcommon.a $(dobj)
	$(CC) -shared -o libdyncommon.so $(dobj) -L../../libs 
	cp libdyncommon.so ../../libs/libdyncommon.so
	cp libcommon.a ../../libs/libcommon.a

%.o:%.c $(headers)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

