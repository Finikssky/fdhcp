#include "libqueue/queue.h"
#include <stdlib.h>

int delete_ptr(queue_t * queue, qelement_t * p)
{
	if (p == NULL) return -1;
	if (queue->mode == Q_TRANSPORT_MODE && p != queue->head) return -1;
	
	qelement_t * prev = p->prev;
	qelement_t * next = p->next;
	
	if (queue->head != queue->tail)
	{
		if (p == queue->head)
		{
			queue->head = next;
			if (next)
				queue->head->prev = NULL;
		}
		else if (p == queue->tail)
		{
			queue->tail = prev;
			if (prev)
				queue->tail->next = NULL;
		}
		else
		{	
			if (prev)
				prev->next = p->next;
			if (next)
				next->prev = p->prev;
		}
	}
	else 
	{
		queue->head = NULL;
		queue->tail = NULL;
	}
	
	queue->elements--;
	free(p->data);
	free(p);
	
	return 0;
}


