#ifndef STATE
#define STATE

#include "dhcp.h"
#include "core.h"

#define START 0
#define OFFER 1
#define ANSWER 2
#define NAK 3
#define CLOSE 4
#define UNKNOWN 255

#define STEP 0
#define TIME 7
#define DISABLE 1

#define TIMEPAUSE 10

struct session 
{ 
	int        sid; 					//Идентификатор сессии
	int        state; 					//Идентификатор состояния
	int        ctime;  				//Время последней смены состояния
	int        ltime;					//Время аренды
	qmessage_t mess;		//Последнее принятое сообщение
};

struct pass 
{
	int currstate;
	int in;
	int nextstate;
	int(*fun)(void *, void *);
} *ptable;

int ptable_count;

int  search_sid(int xid, int scount, struct session **ses);
int  get_stype(int stat, qmessage_t mess);
int  change_state(int xid, int dtype, qmessage_t mess, struct session **ses, int * scount, void * interface);
void clear_context(struct session **ses, int *scount, void * arg);

#endif
