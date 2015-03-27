#include "queue.h"

struct qmessage popmessage(int qnum)
{
	struct qmessage ret;
	queue_t * queue   = &queues[qnum];

	sem_wait(&queue->semid);               //Ожидаем появления сообщения
	pthread_mutex_lock(&queue->mutex);     //Блокируем
	
	if (queue->head == NULL) return ret;
	
	ret = queue->head->data;      //Получаем голову очереди
	
	deletehead(qnum);		      //Удаляем голову из очереди
	
	pthread_mutex_unlock(&queue->mutex);   //Разблокируем

	return ret;
}
