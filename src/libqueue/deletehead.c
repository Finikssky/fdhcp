#include "queue.h"

int deletehead(queue_t * queue)
{
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
	
	free(p->data);
	free(p);
	
	return 0;
}
