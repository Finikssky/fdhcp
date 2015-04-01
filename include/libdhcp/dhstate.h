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

#define MAX_RESPONSE 10
#define MIN_RESPONSE 3

typedef struct request
{
	unsigned char type;
	int           xid;
	u_int32_t     req_address;
	unsigned char mac[16];
} request_t;

typedef struct cl_session
{ 
	int        sid; 					//Идентификатор сессии
	int        state; 					//Идентификатор состояния
	int        ctime;  				    //Время последней смены состояния
	int        ltime;					//Время аренды
	request_t  info;
} cl_session_t;

struct pass 
{
	int currstate;
	int in;
	int nextstate;
	int(*fun)(void *, void *);
} *ptable;

int ptable_count;

cl_session_t * search_sid(int xid, queue_t * sessions);
cl_session_t * change_state(int xid, int dtype, qmessage_t * request, queue_t * sessions, void * interface);
int  get_stype(int stat, qmessage_t * mess);
void clear_context(queue_t * sessions, void * interface);

#endif
