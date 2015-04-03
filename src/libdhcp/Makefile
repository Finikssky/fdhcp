headers=dhcp.h dleases.h dhconn.h dhioctl.h dhstate.h 
dsources=dcrpacket.c dhcp_leases.c dhconn.c dhioctl.c dhstate.c
dobj=$(dsources:.c=.o)

LDADD = -ldynqueue -ldyntimer -ldyncommon

all: $(dobj) 
	ar cr libdhcp.a $(dobj)
	$(CC) -shared -o libdyndhcp.so $(dobj) -L../../libs $(LDADD)
	cp libdyndhcp.so ../../libs/libdyndhcp.so
	cp libdhcp.a ../../libs/libdhcp.a

%.o:%.c $(headers)
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean: 
	rm -f *.o
	rm -f lib*
	rm -f *~

