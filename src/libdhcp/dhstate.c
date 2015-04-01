#include "dhstate.h"
#include "core.h"

//Функция поиска записи с заданный идентификатором
cl_session_t * search_sid(int xid, queue_t * sessions)
{
	int i;
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
int get_stype(int stat, qmessage_t * mess)
{
	struct dhcp_packet * dhc = (struct dhcp_packet *)(mess->packet + FULLHEAD_LEN);

	add_log("Start validation and get signal...");
	
	if (stat == START)
	{
		if (dhc->options[6] == DHCPDISCOVER) return DHCPDISCOVER;
		if (dhc->options[6] == DHCPREQUEST)  return DHCPREQUEST;
	}
	if (stat == OFFER)
	{
		if (dhc->options[6] == DHCPREQUEST)  return DHCPREQUEST;
	}
	if (stat == ANSWER)
	{
		if (dhc->options[6] == DHCPREQUEST)  return DHCPREQUEST;
		if (dhc->options[6] == DHCPDECLINE)  return DHCPDECLINE;
	}

	add_log("Validation fail! Unknown signal!");
	return UNKNOWN;
}

void get_need_info(request_t * info, qmessage_t * mess)
{
	memset(info, 0, sizeof(request_t));
	
	struct dhcp_packet * dhc = (struct dhcp_packet *)(mess->packet + FULLHEAD_LEN); //TODO  мб нет с все заголовки пихать в пакет
	info->xid  = dhc->xid;
	
	get_option(dhc, 53, &info->type, sizeof(info->type));	
	get_option(dhc, 50, &info->req_address, sizeof(info->req_address));	
	memcpy(info->mac, dhc->chaddr, sizeof(info->mac));
	
	free(mess->packet);
}
//Функция смены состояний, возвращает номер записи с которой мы будем работать
cl_session_t * change_state(int xid, int dtype, qmessage_t * request, queue_t * sessions, void * interface)
{
	int num;
	int i;
	struct timeval now;
	cl_session_t * ses;

	add_log("Changing state...");

	//Получаем нужный контекст клиента
	ses = search_sid(xid, sessions);

	if (ses == NULL)
	{
		//Если клиента нет в списке добавляем новую запись
		cl_session_t temp;
		temp.sid   = xid;
		temp.state = START;
		
		push_queue(sessions, 0, &temp, sizeof(temp));
		ses = (cl_session_t *)sessions->tail->data; //hack, refact it
		printf("created new session\n");
	}
	printf("sessions count now: %d\n", sessions->elements);
	
	gettimeofday(&now, NULL);

//Проводим валидацию сообщения с учетом текущего состояния
	int signal = get_stype(ses->state, request);
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
						dserver_interface_t * ifs = (dserver_interface_t *)interface;
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


