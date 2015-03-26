#include "queue.h"

int pushmessage(struct qmessage in, int qnum)
{
	int i;	
	
	pthread_mutex_lock(&mutex[qnum]);
		
	queue_t * queue   = &queues[qnum];
	qelement_t * temp = malloc(sizeof(qelement_t));
	
	if (temp == NULL) return -1;
	
	temp->next = NULL;
	temp->data = in; 
	
	if (queue->head == NULL)
	{
		queue->head = temp;
		queue->tail = queue->head;
	}
	else
	{
		queue->tail->next = temp;
		queue->tail = temp;
	}
	
	queue->elements++;
	
	sem_post(&semid[qnum]);
	
 	pthread_mutex_unlock(&mutex[qnum]);
	
	return 0;
}
