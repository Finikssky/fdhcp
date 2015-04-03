CC = gcc
IDIR = $(PWD)/include

AFLAGS += -Wall -O3 -fPIC
IFLAGS += -I$(IDIR)
CFLAGS += $(AFLAGS) $(IFLAGS) $(UFLAGS)
LFLAGS += -L./libs

dcc_LDADD = -ldyndctp -ldyndhcp -ldynqueue -ldyncommon -lpthread
dsc_LDADD = -ldyndctp -ldyndhcp -ldynqueue -ldyntimer -ldyncommon -lpthread
sender_LDADD = -ldyndctp -ldyncommon
ntest_LDADD = -ldyndhcp -ldynqueue -ldyncommon -lpthread

all: 
	make CC="$(CC)" CFLAGS="$(CFLAGS)" -C src/ all
	$(CC) $(CFLAGS) -o dcc dcc.c $(LFLAGS) $(dcc_LDADD)
	$(CC) $(CFLAGS) -o dsc dsc.c $(LFLAGS) $(dsc_LDADD)

debug: clean
	make UFLAGS='-g' all

utils:
	$(CC) $(CFLAGS) -o sender sender.c $(LFLAGS) $(sender_LDADD)
	$(CC) $(CFLAGS) -o test_nagr dnagr.c $(LFLAGS) $(ntest_LDADD)

clean: 
	make -C src/ clean
	rm -f *.o
	rm -f *.txt
	rm -f *.lease
	rm -f *~
	rm -f dcc
	rm -f dsc
	rm -f sender
	rm -f libs/*
	rm -Rf install
