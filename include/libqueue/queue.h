#ifndef QUEUE_H
#define QUEUE_H

#define DEBUG

#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

struct qmessage { unsigned char text[2048]; unsigned char iface[50]; int delay;} **qm; //Очередь сообщений;
int *qc;         //Счетчики количества сообщений в очереди

sem_t *semid; //Семафоры
pthread_mutex_t *mutex; //Мьютексы

char *getmesstext(int arg); //Функция возврата текста сообщения по его номеру
void pushmessage(struct qmessage in, int qnum); //Функция добавления сообщения в очередь с номером qnum
struct qmessage popmessage(int qnum); //Вытаскивает сообщение из очереди
struct qmessage genrandmessage(); //Генерирует случайное сообщение
struct qmessage reversemessage(struct qmessage in); //Разворот текста сообщения
void printmessage(struct qmessage in, int qnum); //Функция распечатки сообщения и удаления его из очереди
void deletehead(int qnum); //Функция удаления первого элемента очереди


void initsync(int count); //Инициализация и закрытие  средств синхронизации
void uninitsync(int count);
void initres(int count); //Инициализация и закрыти ресурсов
void uninitres(int count); 


#endif
