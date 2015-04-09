#include "libqueue/queue.h"
#include <string.h>

int pop_queue(queue_t * queues, int qnum, void * data, size_t size)
{
	queue_t * queue   = &queues[qnum];

	if (queue->mode == Q_TRANSPORT_MODE)
		sem_wait(&queue->semid);               //Ожидаем появления сообщения
	pthread_mutex_lock(&queue->mutex);     //Блокируем
	
	if (queue->head == NULL || data == NULL || size <= 0) 
	{
		pthread_mutex_unlock(&queue->mutex);
		return -1;
	}
	
	memset( data, 0, size );
	memcpy( data, queue->head->data, size < queue->head->data_size ? size : queue->head->data_size );      //Получаем голову очереди
	
	delete_ptr(queue, queue->head);		      //Удаляем голову из очереди
	
	pthread_mutex_unlock(&queue->mutex);   //Разблокируем

	return  0;
}
