#include "queue.h"

struct qmessage popmessage(int qnum)
{
	struct qmessage ret;

	sem_wait(&semid[qnum]);               //Ожидаем появления сообщения
	pthread_mutex_lock(&mutex[qnum]);     //Блокируем
	
	if (queues[qnum].head == NULL) return ret;
	
	ret = queues[qnum].head->data;      //Получаем голову очереди
	
	deletehead(qnum);		      //Удаляем голову из очереди
	
	pthread_mutex_unlock(&mutex[qnum]);   //Разблокируем

	return ret;
}
