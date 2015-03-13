#include "queue.h"

void pushmessage(struct qmessage in, int qnum){
int i;	
	pthread_mutex_lock(&mutex[qnum]);
		
	qm[qnum]=realloc(qm[qnum],(++qc[qnum])*sizeof(struct qmessage));
//	memcpy(qm[qnum][qc[qnum]-1].text,in.text,sizeof(in.text)); //Установка текста сообщения
//      qm[qnum][qc[qnum]-1].delay=in.delay; //Установка задержки вывода
	
	qm[qnum][qc[qnum]-1]=in;	
	
	
#ifdef DEBUG            
	 for(i=0;i<qc[qnum]; i++){
               printf("put>>> Q: %d",qnum);
               printf(" Message: %s Delay: %d\n",qm[qnum][i].text,qm[qnum][i].delay);
         }
         fflush(stdout);

#endif //Отладочная печать для проверки наполнения очереди

	sem_post(&semid[qnum]);
 	pthread_mutex_unlock(&mutex[qnum]);
}
