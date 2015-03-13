#ifndef TIMER_H
#define TIMER_H

struct timer 
{ 
	int  timeout; 
	void *(*fun)(void *); 
	void *args;  
};

void *timer(void *arg);

#endif

