#include "app1.h"

void *transfer(void *arg){
int i;
struct dest trans=*((struct dest*)arg);  //Получаем номер очередей - концов тоннеля

	while(1)
	     pushmessage(reversemessage(popmessage(trans.in)),trans.out);
				
}

