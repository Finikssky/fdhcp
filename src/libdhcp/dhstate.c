#include "core.h"
#include "libdhcp/dhstate.h"
#include "libdhcp/dhioctl.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

//Функция поиска записи с заданный идентификатором
cl_session_t * search_sid(int xid, queue_t * sessions)
{
	add_log("Searshing sid...");

	qelement_t * iter;
	for (iter = sessions->head; iter != NULL; iter = iter->next)
	{
		cl_session_t * ses = (cl_session_t *) iter->data;
		if (ses->sid == xid) return ses;
	}

	add_log("No sid( ");
	return NULL;
}

//Функция получения типа сообщения
int get_stype(int stat, struct dhcp_packet * dhc)
{
	int type = -1;
	get_option(dhc, 53, &type, sizeof(type));
	printf("<%s> state: %d dtype: %s\n", __FUNCTION__, stat, stringize_dtype(type));
	
	add_log("Start validation and get signal...");
	
	if (stat == START)
	{
		if (type == DHCPDISCOVER) return DHCPDISCOVER;
		if (type == DHCPREQUEST)  return DHCPREQUEST;
	}
	if (stat == OFFER)
	{
		if (type == DHCPREQUEST)  return DHCPREQUEST;
	}
	if (stat == ANSWER)
	{
		if (type == DHCPREQUEST)  return DHCPREQUEST;
		if (type == DHCPDECLINE)  return DHCPDECLINE;
		if (type == DHCPRELEASE )  return DHCPRELEASE;
	}

	add_log("Validation fail! Unknown signal!");
	return UNKNOWN;
}

void get_need_info(request_t * info, frame_t * frame)
{
	memset(info, 0, sizeof(request_t));
	
	struct dhcp_packet * dhc = &frame->p_dhc;
	info->xid  = dhc->xid;
	
	get_option(dhc, 53, &info->type, sizeof(info->type));	
	if (-1 == get_option(dhc, 50, &info->req_address, sizeof(info->req_address))) 
		printf("can't get 50 opt\n");	
	else
	{
		printf("requested ");
		printip(info->req_address);
	}
	
	if (-1 == get_option(dhc, 54, &info->req_server_address, sizeof(info->req_server_address))) 
		printf("can't get 54 opt\n");	
	else
	{
		printf("requested server ");
		printip(info->req_address);
	}
	memcpy(info->mac, dhc->chaddr, sizeof(info->mac));
	
	free(frame);
}
//Функция смены состояний, возвращает номер записи с которой мы будем работать
cl_session_t * change_state(frame_t * request, queue_t * sessions, void * interface)
{
	int i;
	struct timeval now;
	cl_session_t * ses;

	add_log("Changing state...");

	//Получаем нужный контекст клиента
	ses = search_sid(request->p_dhc.xid, sessions);

	if (ses == NULL)
	{
		//Если клиента нет в списке добавляем новую запись
		cl_session_t temp;
		temp.sid   = request->p_dhc.xid;
		temp.state = START;
		
		push_queue(sessions, 0, &temp, sizeof(temp));
		ses = (cl_session_t *)sessions->tail->data; //hack, refact it
		printf("created new session\n");
	}
	printf("sessions count now: %d\n", sessions->elements);
	
	gettimeofday(&now, NULL);

//Проводим валидацию сообщения с учетом текущего состояния
	int signal = get_stype(ses->state, &request->p_dhc);
	printf("signal %d\n", signal);

//В зависимости от текущего состояния и результата валидации совершаем переход
//по конечному автомату в следующее состояние

	for (i = 0; i < ptable_count; i++)
	{
		if (ses->state == ptable[i].currstate && signal == ptable[i].in)
		{
			printf("change state %d to %d\n", ptable[i].currstate, ptable[i].nextstate);
			ses->state       = ptable[i].nextstate; //Меняем состояние
			ses->ctime       = now.tv_sec; 		 //Устанавливаем время последней смены состояния
			get_need_info(&ses->info, request);
			break;
		}	
	}

	//Взависимости от текущего состояния автомата выполняем соответствующую функцию
	for (i = 0; i < ptable_count; i++)
	{
		if (ses->state == ptable[i].currstate) 
		{
			printf("state %d\n", ses->state);
			long status = ptable[i].fun(&ses->info, interface);
			if ( -1 == status ) return NULL;
			ses->ltime = status;
			add_log("State changed!");
			break;
		}
	}
	
	return ses;
}

void clear_context(queue_t * sessions, void * interface)
{
	int i;
	struct timeval now;

	int response_time = MAX_RESPONSE - (sessions->elements / 200);
	if ( response_time < MIN_RESPONSE ) response_time = MIN_RESPONSE;

	//Копируем все действительные контексты во временный массив
	qelement_t * iter;
	for (iter = sessions->head; iter != NULL; )
	{
		cl_session_t * ses = (cl_session_t *)iter->data;
		if (ses->state == CLOSE) 
		{
			qelement_t * next = iter->next;
			delete_ptr(sessions, iter);
			iter = next;
		}
		else
		{
			gettimeofday(&now,NULL);
			
			switch (ses->state)
			{
				case OFFER:
					if (ses->ctime + response_time < now.tv_sec) ses->state = CLOSE;
					break;
					
				case ANSWER:
					if (ses->ctime + ses->ltime + response_time < now.tv_sec) 
					{
						ses->state = NAK;
						
						for (i = 0; i < ptable_count; i++)
						{
							if (ses->state == ptable[i].currstate) 
							{
								ptable[i].fun(&ses->info, interface);
								break;
							}
						}
					}
					break;
					
				case NAK:
					ses->state = CLOSE;
					break;
					
				default:
					break;
			}
			
			iter = iter->next;
		}
	}
}


