#include "queue.h"

int deletehead(int qnum)
{
	queue_t * queue = &queues[qnum];
	qelement_t * p = queue->head;
	
	if (p == NULL) return -1;
	
	if (queue->head != queue->tail)
	{
		queue->head = p->next;
	}
	else 
	{
		queue->head = NULL;
		queue->tail = NULL;
	}
	
	free(p);
	
	return 0;
}
