
all:
	make CC="$(CC)" CFLAGS="$(CFLAGS)" -C libtimer
	make CC="$(CC)" CFLAGS="$(CFLAGS)" -C libcommon
	make CC="$(CC)" CFLAGS="$(CFLAGS)" -C libqueue
	make CC="$(CC)" CFLAGS="$(CFLAGS)" -C libdhcp
	make CC="$(CC)" CFLAGS="$(CFLAGS)" -C libdctp
	
clean:
	make -C libtimer clean
	make -C libqueue clean
	make -C libdhcp clean
	make -C libdctp clean
	make -C libcommon clean
