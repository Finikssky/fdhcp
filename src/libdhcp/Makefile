headers=dhcp.h dleases.h dhconn.h dhioctl.h dhstate.h 
dsources=dcrpacket.c dhcp_leases.c dhconn.c dhioctl.c dhstate.c
dobj=$(dsources:.c=.o)
 
CC=gcc
CFLAGS  = -fPIC -I../../include/libdhcp -I../../include/libqueue -I../../include -I../../include/libcommon

all: $(dobj) 
	ar cr libdhcp.a $(dobj)
	$(CC) -shared -o libdyndhcp.so $(dobj) -L../../libs -ldynqueue -ldyntimer -ldyncommon
	cp libdyndhcp.so ../../libs/libdyndhcp.so
	cp libdhcp.a ../../libs/libdhcp.a

%.o:%.c $(headers)
	$(CC) $(CFLAGS) -fPIC -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

