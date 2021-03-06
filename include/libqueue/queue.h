#ifndef QUEUE_H
#define QUEUE_H

#define DEBUG

#include <pthread.h>
#include <semaphore.h>

#define Q_TRANSPORT_MODE 0
#define Q_STANDART_MODE  1


struct qelement 
{ 
	struct qelement * next;
	struct qelement * prev;
	size_t            data_size;
	void            * data;
};

typedef struct qelement qelement_t;

typedef struct queue
{
	int              mode;
	int              elements;
	qelement_t     * head;
	qelement_t     * tail;
	sem_t            semid;
	pthread_mutex_t  mutex;
} queue_t;

void * push_queue(queue_t * queues, int qnum, void * data, size_t size); //Функция добавления сообщения в очередь с номером qnum
int pop_queue (queue_t * queues, int qnum, void * data, size_t size); //Вытаскивает сообщение из очереди с номером qnum
int delete_ptr (queue_t * queue, qelement_t * p); //Функция удаления первого элемента очереди

queue_t * init_queues(int count, int mode);
void uninit_queues(queue_t * queues, int count);

#define Q_FOREACH(__type__, __point__, __queue__, __body__) \
     { \
        qelement_t * Q_ITER; \
        for ( Q_ITER = (__queue__)->head; Q_ITER != NULL; Q_ITER = Q_ITER->next) \
        { \
            __type__ __point__ = (__type__) (Q_ITER -> data); \
            __body__ \
        }    \
    }

#endif
