#include "app1.h"

void *generation(void *arg){
int i;
int qnum=0; //Номер очереди сообщений
int border=*((int*)arg);

	while(1){
	    	usleep(rand()%500000+150000); //Время через которое генерируется сообщение
		
		pushmessage(genrandmessage(),qnum);   //Добавление сообщения в очередь 
		
		qnum=(qnum+1)%border; //Выбор номера следующей очереди
	}
}
