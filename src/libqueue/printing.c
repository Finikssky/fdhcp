#include "app1.h"

void *printing(void *arg){
int i;
int qnum=*((int*)arg); //Получаем номер очереди из аргумента потока

	while(1) 
		printmessage(popmessage(qnum),qnum); //Печатаем сообщение из очереди qnum
				
}

