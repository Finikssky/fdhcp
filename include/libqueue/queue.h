#ifndef QUEUE_H
#define QUEUE_H

#define DEBUG

#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

struct qelement 
{ 
	struct qelement * next;
	void            * data;
	size_t            data_size;
};

typedef struct qelement qelement_t;

typedef struct queue
{
	qelement_t     * head;
	qelement_t     * tail;
	sem_t            semid;
	pthread_mutex_t  mutex;
	int              elements;
} queue_t;

int pushmessage(queue_t * queues, int qnum, void * data, size_t size); //Функция добавления сообщения в очередь с номером qnum
int popmessage (queue_t * queues, int qnum, void * data, size_t size); //Вытаскивает сообщение из очереди с номером qnum
int deletehead(queue_t * queue); //Функция удаления первого элемента очереди

queue_t * init_queues(int count);
void uninit_queues(queue_t * queues, int count);


#endif
