#include "queue.h"

void init_sync(int count)
{
	int i;
	srand(time(NULL)); //Инициируем рандом
	
	mutex = malloc(count * sizeof(pthread_mutex_t)); //Инициализация мьютексов
	semid = malloc(count * sizeof(sem_t)); //Инициализация семафоров

	for ( i = 0; i < count; i++ )
	{
		pthread_mutex_init(&mutex[i], NULL); //Инициируем мьютексы
		sem_init(&semid[i], 0, 0); //Инициируем семафоры
	}
}

void uninit_sync(int count)
{
	int i;
	
	for ( i = 0; i < count; i++ )
	{	
		pthread_mutex_destroy(&mutex[i]); //Уничтожаем мьютексы
		sem_destroy(&semid[i]); //Уничтожаем семафоры
	}
	
	free(mutex);
	free(semid);
}

void init_res(int count)
{
	int i;
	queues = malloc(count * sizeof(queue_t)); //Инициализация массива очередей

	srand(time(NULL)); //Инициируем рандом

	for ( i = 0; i < count; i++ )
	{
		queues[i].head     = NULL;
		queues[i].tail     = NULL;
		queues[i].elements = 0;
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

void uninit_res(int count)
{
	int i;

	for ( i = 0; i < count; i++ )
	{
		free_element(queues[i].head);
	}
	
	free(queues);
}

void init_queues(int count)
{
	init_sync(count);
	init_res(count);
}

void uninit_queues(int count)
{
	uninit_res(count);
	uninit_sync(count);
}