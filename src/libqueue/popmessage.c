#include "queue.h"

int popmessage(queue_t * queues, int qnum, void * data, size_t size)
{
	queue_t * queue   = &queues[qnum];

	sem_wait(&queue->semid);               //Ожидаем появления сообщения
	pthread_mutex_lock(&queue->mutex);     //Блокируем
	
	if (queue->head == NULL) return -1;
	
	memset( data, 0, size );
	memcpy( data, queue->head->data, size < queue->head->data_size ? size : queue->head->data_size );      //Получаем голову очереди
	
	deletehead(queue);		      //Удаляем голову из очереди
	
	pthread_mutex_unlock(&queue->mutex);   //Разблокируем

	return  0;
}
