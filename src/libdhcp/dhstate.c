#include "dhstate.h"
#include "core.h"

//Функция поиска записи с заданный идентификатором
int search_sid(int xid, int scount, struct session **ses)
{
	int i;
	add_log("Searshing sid...");

	for( i = 0; i < scount; i++)
	{
		if ((*ses)[i].sid == xid) 
		{
			add_log("Have sid!"); 
			return i;
		}
	}

	add_log("No sid( ");
	return -1;
}

//Функция получения типа сообщения
int get_stype(int stat, struct qmessage mess)
{
	struct dhcp_packet *dhc = (struct dhcp_packet*)(mess.text+FULLHEAD_LEN);

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


//Функция смены состояний, возвращает номер записи с которой мы будем работать
int change_state(int xid, int dtype, struct qmessage mess, struct session **ses, int *scount, void * interface)
{
	int num;
	int i;
	struct timeval now;

	add_log("Changing state...");

	//Получаем нужный контекст клиента
	printf("SCOUNT %d\n", *scount);
	num = search_sid(xid, *scount, ses);

	if (num == -1)
	{
		//Если клиента нет в списке добавляем новую запись
		*ses = realloc(*ses,(++(*scount)) * sizeof(struct session));
		if (*ses == NULL) perror("realloc in changing state");

		num = *scount - 1;
		memset(&((*ses)[num]), 0, sizeof(struct session));
		(*ses)[num].sid   = xid;
		(*ses)[num].state = START;
	}

	gettimeofday(&now, NULL);

//Проводим валидацию сообщения с учетом текущего состояния
	int signal = get_stype((*ses)[num].state, mess);
	printf("signal %d\n", signal);

//В зависимости от текущего состояния и результата валидации совершаем переход
//по конечному автомату в следующее состояние

	for (i = 0; i < ptable_count; i++)
	{
		if ((*ses)[num].state == ptable[i].currstate && signal == ptable[i].in)
		{
			printf("change state %d to %d\n", ptable[i].currstate, ptable[i].nextstate);
			(*ses)[num].state = ptable[i].nextstate; //Меняем состояние
			(*ses)[num].ctime = now.tv_sec; 		 //Устанавливаем время последней смены состояния
			mess.delay 		  = (*ses)[num].state;   //В сообщение добавляем текущее состояние для TX
			(*ses)[num].mess  = mess; 				 //Запоминаем последнее сообщение
			break;
		}	
	}

	//Взависимости от текущего состояния автомата выполняем соответствующую функцию
	for (i = 0; i < ptable_count; i++)
	{
		if ((*ses)[num].state == ptable[i].currstate) 
		{
			printf("state %d\n", (*ses)[num].state);
			long status = ptable[i].fun((*ses)[num].mess.text, interface);
			if ( -1 == status ) return -1;
			(*ses)[num].ltime = status;
			add_log("State changed!");
			break;
		}
	}
	
	return num;
}

void clear_context(struct session **ses, int * scount, void * interface)
{
	struct session * new = NULL;
	int i,j;
	int newscount = 0;
	struct timeval now;

	//Копируем все действительные контексты во временный массив
	for (i = 0; i < *scount;i++)
	{
		if ((*ses)[i].state != CLOSE) 
		{
			new = realloc(new, (++newscount) * sizeof(struct session));
			new[newscount-1] = (*ses)[i];
		}
	}
	
	//Очищаем старый массив и копируем временный в него
	*scount = newscount;
	*ses    = realloc(*ses,(*scount)*sizeof(struct session));
	memset(*ses, 0, (*scount) * sizeof(struct session));
	memcpy(*ses, new, (*scount) * sizeof(struct session));

	//Если время ожидания перехода истекло
	for (i = 0; i < *scount; i++)
	{
		gettimeofday(&now,NULL);
		switch ((*ses)[i].state)
		{
			case OFFER:
				if ((*ses)[i].ctime + TIMEPAUSE < now.tv_sec) (*ses)[i].state = CLOSE;
				break;
				
			case ANSWER:
				if ((*ses)[i].ctime + (*ses)[i].ltime + TIMEPAUSE < now.tv_sec) 
				{
					(*ses)[i].state = NAK;
					(*ses)[i].mess.delay = NAK;
					
					for (j = 0; j < ptable_count; j++)
					{
						if ((*ses)[i].state == ptable[j].currstate) 
						{
							ptable[j].fun((*ses)[i].mess.text, interface);
							break;
						}
					}
					dserver_interface_t * ifs = (dserver_interface_t *)interface;
					pushmessage((*ses)[i].mess, ifs->c_idx * 2 + 1);
				}
				break;
				
			case NAK:
				(*ses)[i].state = CLOSE;
				break;
				
			default:
				break;
		}
	}

	if (new != NULL) free(new);
}


