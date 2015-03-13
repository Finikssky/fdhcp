#include "queue.h"

void printmessage(struct qmessage in, int qnum)
{
	int i;	

	usleep(in.delay);
	printf("get>>> QNUM: %d Message: %s iface: %s\n", qnum, in.text, in.iface);
        fflush(stdout);                 //Печать сообщения из очереди
}
