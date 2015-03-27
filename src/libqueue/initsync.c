#include "queue.h"

void init_queues(int count)
{
	int i;
	queues = malloc(count * sizeof(queue_t)); //Инициализация массива очередей

	srand(time(NULL)); //Инициируем рандом

	for ( i = 0; i < count; i++ )
	{
		queues[i].head     = NULL;
		queues[i].tail     = NULL;
		queues[i].elements = 0;
		sem_init(&(queues[i].semid), 0, 0);
		pthread_mutex_init(&(queues[i].mutex), NULL);
	}
}

int free_element(qelement_t * element)
{
	if (element == NULL) return 0;
	if (element->next != NULL) 
		free_element(element->next);
	
	free(element);
	return 0;
}

void uninit_queues(int count)
{
	int i;

	for ( i = 0; i < count; i++ )
	{
		free_element(queues[i].head);
		pthread_mutex_destroy(&(queues[i].mutex));
		sem_destroy(&(queues[i].semid));
	}
	
	free(queues);
}