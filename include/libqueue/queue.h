#ifndef QUEUE_H
#define QUEUE_H

#define DEBUG

#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

typedef struct qmessage
{
	unsigned char  text[2048];
	unsigned char  iface[50];  //Убрать попозже
	int            delay;
} qmessage_t;

struct qelement 
{ 
	struct qelement * next;
	struct qmessage   data;
};

typedef struct qelement qelement_t;

typedef struct queue
{
	qelement_t * head;
	qelement_t * tail;
	int          elements;
} queue_t;

queue_t * queues;

sem_t * semid; //Семафоры
pthread_mutex_t * mutex; //Мьютексы

int pushmessage(struct qmessage in, int qnum); //Функция добавления сообщения в очередь с номером qnum
struct qmessage popmessage(int qnum); //Вытаскивает сообщение из очереди с номером qnum
int deletehead(int qnum); //Функция удаления первого элемента очереди

void init_queues(int count);
void uninit_queues(int count);


#endif
