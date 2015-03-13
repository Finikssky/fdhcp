#include "queue.h"

void initsync(int count){
int i;
	srand(time(NULL)); //Инициируем рандом
	
	mutex=malloc(count*sizeof(pthread_mutex_t)); //Инициализация мьютексов
	semid=malloc(count*sizeof(sem_t)); //Инициализация семафоров

        for(i=0;i<count;i++){
                pthread_mutex_init(&mutex[i],NULL); //Инициируем мьютексы
                sem_init(&semid[i],0,0); //Инициируем семафоры
        }
}

void uninitsync(int count){
int i;
	for(i=0;i<count;i++){	
		pthread_mutex_destroy(&mutex[i]); //Уничтожаем мьютексы
                sem_destroy(&semid[i]); //Уничтожаем семафоры
        }
	
	free(mutex);
	free(semid);
}

void initres(int count){
int i;
	qm=malloc(count*sizeof(struct qmessage *)); //Инициализация массива очередей
        qc=malloc(count*sizeof(int)); //Инициализация массива счетчиков длин очередей

	srand(time(NULL)); //Инициируем рандом

	for(i=0;i<count;i++){
                qc[i]=0;     //Устанавливаем нулевое число сообщений в каждой очереди
                qm[i]=NULL;
	}
}

void uninitres(int count){
int i;

	for(i=0;i<count;i++)
               free(qm[i]);

	free(qm);
	free(qc);
}

