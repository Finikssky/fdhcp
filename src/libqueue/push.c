#include "libqueue/queue.h"
#include <string.h>

void * push_queue(queue_t * queues, int qnum, void * data, size_t size)
{
	queue_t * queue   = &queues[qnum];
	
	pthread_mutex_lock(&queue->mutex);
	
	qelement_t * temp = malloc(sizeof(qelement_t));
	
	if (temp == NULL || data == NULL || size <= 0)
	{
		pthread_mutex_unlock(&queue->mutex);
		return NULL;
	}
	
	temp->next     = NULL;
	temp->prev     = NULL;
	temp->data     = malloc(size); 
	temp->data_size = size;
	memcpy(temp->data, data, size);
	
	if (queue->head == NULL)
	{
		queue->head = temp;
		queue->tail = queue->head;
	}
	else
	{
		queue->tail->next = temp;
		temp->prev       = queue->tail;
		queue->tail       = temp;
	}
	
	queue->elements++;
	
	if (queue->mode == Q_TRANSPORT_MODE) 
		sem_post(&queue->semid);
	
 	pthread_mutex_unlock(&queue->mutex);
	
	return temp->data;
}
