#include "queue.h"

struct qmessage genrandmessage(){
int i;
struct qmessage ret;

        i=rand()%10;
        strcpy(ret.text,getmesstext(i));  //Установка текста сообщения
        ret.delay=rand()%200000+400000;
	strcpy(ret.iface,"IFACE");	

return ret;
}

