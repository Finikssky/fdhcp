#include "dhcp.h"
#include "dctp.h"
#include "dhconn.h"
#include "dhioctl.h"
#include "dhstate.h"
#include "dleases.h"
#include "timer.h"
#include <pthread.h>
#include <limits.h>
#include <net/if.h>

#include "core.h"
#include "common.h"

#define PTABLE_COUNT   8
#define S_CONFIG_FILE "dsc.conf"

void * iface_loop (void * iface);
void * manipulate (void * server);
void * s_recvDHCP(void * arg);
void * s_replyDHCP(void * arg);
void * sm(void * arg);

dserver_subnet_t * search_subnet(dserver_interface_t * interface, char * args);
dserver_subnet_t * add_subnet_to_interface(dserver_interface_t * interface, char * args);
int add_pool_to_subnet(dserver_subnet_t * subnet, char * range);
int add_dns_to_subnet(dserver_subnet_t * subnet, char * address);
int add_router_to_subnet(dserver_subnet_t * subnet, char * address);
int set_host_name_on_subnet(dserver_subnet_t * subnet, char * args);
int set_domain_name_on_subnet(dserver_subnet_t * subnet, char * args);

long get_one_num(char * args);

int save_config(DSERVER * server, char * c_file)
{
	FILE * fd = fopen(c_file, "w");
	if (fd == NULL) return -1;
	int i;
	
	for (i = 0; i < MAX_INTERFACES; i++ )
	{
		dserver_interface_t * interface = &server->interfaces[i];
		if (strlen(interface->name) != 0)
		{
			fprintf(fd, "interface: %s\n", interface->name);
			
			fprintf(fd, "    %s\n", interface->enable == 1 ? "enable" : "disable");
			
			dserver_if_settings_t * settings = &interface->settings;
			
			dserver_subnet_t * subnet = settings->subnets;
			while (subnet != NULL)
			{
				char subnet_mask[INET_ADDRSTRLEN];
				char subnet_addr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &subnet->address, subnet_addr, sizeof(subnet_addr));
				inet_ntop(AF_INET, &subnet->netmask, subnet_mask, sizeof(subnet_mask));
				
				fprintf(fd, "    %s: %s %s\n", "subnet", subnet_addr, subnet_mask);
				
				if (subnet->lease_time != 0) 
					fprintf(fd, "        %s: %ld\n", "lease_time", subnet->lease_time);
				
				if (strlen(subnet->host_name) != 0)
					fprintf(fd, "        %s: %s\n", "host_name", subnet->host_name);
				
				if (strlen(subnet->domain_name) != 0)
					fprintf(fd, "        %s: %s\n", "domain_name", subnet->domain_name);
				
				dserver_pool_t * pool = subnet->pools;
				while (pool != NULL)
				{   
					char start_address[INET_ADDRSTRLEN];
					char end_address[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &pool->range.start_address, start_address, sizeof(start_address));
					inet_ntop(AF_INET, &pool->range.start_address, end_address, sizeof(end_address));
					fprintf(fd, "        %s: %s-%s\n", "range", start_address, end_address);
					pool = pool->next;
				}
				
				dserver_dns_t * dns = subnet->dns_servers;
				while (dns != NULL)
				{
					char dns_address[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &dns->address, dns_address, sizeof(dns_address));
					fprintf(fd, "        %s: %s\n", "dns-server", dns_address);
					dns = dns->next;
				}
				
				dserver_router_t * router = subnet->routers;
				while (router != NULL)
				{
					char router_address[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &router->address, router_address, sizeof(router_address));
					fprintf(fd, "        %s: %s\n", "router", router_address);
					router = router->next;
				}
				fprintf(fd, "    %s\n", "end_subnet");
				
				subnet = subnet->next;
			}
			fprintf(fd, "end_interface\n");
		}
	}
		
	fclose(fd);
	return 0;
}

int load_subnet(FILE * fd, dserver_subnet_t * subnet)
{
	char com[128]  = "";
	char args[128] = "";
	while (EOF != t_gets(fd, ':', com, sizeof(com), 0))
	{
		if (0 == strcmp(com, "range"))
		{
			if (EOF == t_gets(fd, 0, args, sizeof(args), 0)) 
				return -1;
			if (-1 == add_pool_to_subnet(subnet, args))	
				printf("parse config error: add range\n");
		}
		else if (0 == strcmp(com, "dns-server"))
		{
			if (EOF == t_gets(fd, 0, args, sizeof(args), 0)) 
				return -1;
			if (-1 == add_dns_to_subnet(subnet, args))	
				printf("parse config error: add dns-server\n");
		}
		else if (0 == strcmp(com, "router"))
		{
			if (EOF == t_gets(fd, 0, args, sizeof(args), 0)) 
				return -1;
			if (-1 == add_router_to_subnet(subnet, args))	
				printf("parse config error: add router\n");
		}
		else if (0 == strcmp(com, "lease_time"))
		{
			if (EOF == t_gets(fd, 0, args, sizeof(args), 0)) 
				return -1;
				
			long ltime = get_one_num(args);
			if (ltime == -1) 
				printf("parse config error: set lease time\n");
			else 
				subnet->lease_time = ltime;
		}
		else if (0 == strcmp(com, "host_name"))
		{
			if (EOF == t_gets(fd, 0, args, sizeof(args), 0)) 
				return -1;
			
			if (-1 == set_host_name_on_subnet(subnet, args))
				printf("parse config error: set host name\n");
		}
		else if (0 == strcmp(com, "domain_name"))
		{
			if (EOF == t_gets(fd, 0, args, sizeof(args), 0)) 
				return -1;
			
			if (-1 == set_domain_name_on_subnet(subnet, args))
				printf("parse config error: set domain name\n");
		}
		else if (0 == strcmp(com, "end_subnet"))
		{
			return 0;
		}
		memset(com, 0, sizeof(com));
		memset(args, 0, sizeof(args));
	}
	
	return -1;
}

int load_interface (FILE * fd, dserver_interface_t * interface)
{
	char com[128]  = "";
	char args[128] = "";
	while (EOF != t_gets(fd, ':', com, sizeof(com), 0))
	{
		if (0 == strcmp(com, "enable"))
		{
			interface->enable = 1;
		}
		else if (0 == strcmp(com, "disable"))
		{
			interface->enable = 0;
		}
		else if (0 == strcmp(com, "subnet"))
		{
			t_gets(fd, 0, args, sizeof(args), 0);
			dserver_subnet_t * subnet = add_subnet_to_interface(interface, args);
			if (subnet != NULL ) 
			{
				if (-1 == load_subnet(fd, subnet))
				{
					printf("parse config error: can't reach end of subnet\n");
					return -1;
				}
			}
			else 
				printf("parse config error: subnet add\n");
		}
		else if (0 == strcmp(com, "end_interface"))
		{
			return 0;
		}
		memset(com, 0, sizeof(com));
		memset(args, 0, sizeof(args));
	}
	
	return -1;
}

int load_config(DSERVER * server, char * c_file)
{
	FILE * fd = fopen(c_file, "r");
	if (fd == NULL) return -1;
	int i;
	char com[128] = "";
		
	for (i = 0; i < MAX_INTERFACES; i++ )
	{
		dserver_interface_t * interface = &server->interfaces[i];
		
		while (EOF != t_gets(fd, ':', com, sizeof(com), 0))
		{
			if (0 == strcmp(com, "interface"))
			{
				memset(interface, 0 ,sizeof(*interface));
				t_gets(fd, 0, interface->name, sizeof(interface->name), 0);
				if (-1 == load_interface(fd, interface)) 
				{
					printf("parse config error: can't reach end of interface");
					return -1;
				}
				break;
			}
			memset(com, 0, sizeof(com));
		}
	}
		
	fclose(fd);
	return 0;
}

int send_offer(void * buffer, void * arg)
{
	struct dhcp_packet * dhc = (struct dhcp_packet*) (buffer + FULLHEAD_LEN );
	dserver_interface_t * interface = (dserver_interface_t *) arg;
	unsigned char macs[6];

	add_log("Sending DHCPOFFER..");

	set_my_mac(interface->name, macs);	
	
	create_ethheader(buffer, macs, dhc->chaddr, ETH_P_IP);
	if (-1 == create_packet(interface->name, buffer, 2, DHCPOFFER, (void *)interface)) return -1;	
	create_ipheader(buffer, get_iface_ip(interface->name), dhc->yiaddr.s_addr);
	create_udpheader(buffer, DHCP_SERVER_PORT, DHCP_CLIENT_PORT);

	add_log("DHCPOFFER sended!");
	return 0;
}

int send_nak(void * buffer, void * arg)
{
	unsigned char macs[6];
	unsigned char macb[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	dserver_interface_t * interface = (dserver_interface_t *) arg;
	
	add_log("Sending DHCPNAK..");

	set_my_mac(interface->name, macs);

	create_ethheader(buffer, macs, macb, ETH_P_IP);
	create_packet(interface->name, buffer, 2, DHCPNAK, (void *)interface);
	create_ipheader(buffer, get_iface_ip(interface->name), INADDR_BROADCAST);
	create_udpheader(buffer, DHCP_SERVER_PORT, DHCP_CLIENT_PORT);

	add_log("DHCPNAK sended!");
	return 0;
}

int send_answer(void * buffer, void * arg)
{
	struct dhcp_packet * dhc = (struct dhcp_packet * ) (buffer + FULLHEAD_LEN);
	dserver_interface_t * interface = (dserver_interface_t *) arg;

	unsigned char macs[6];
	unsigned char macb[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	add_log("Sending DHCPACK/DHCPNAK..");

	set_my_mac(interface->name, macs);
	
	//Проверяем доступен ли адрес
	int result = get_proof(dhc, &dhc->yiaddr.s_addr);
	if (result > 0) 
	{	
		//В случае положительного ответа отправляем АСК
		create_ethheader(buffer, macs, dhc->chaddr, ETH_P_IP);		
		if (-1 == create_packet(interface->name, buffer, 2, DHCPACK, (void *)interface)) return -1;
		create_ipheader(buffer, get_iface_ip(interface->name), dhc->yiaddr.s_addr);
		s_add_lease(dhc->yiaddr.s_addr, get_lease_time(), dhc->chaddr, NULL); //TODO refactoring lease_time
	}
	else if (result == 0)  
	{	
		//В случае отрицательного ответа отправляем NAK
		create_ethheader(buffer, macs, macb, ETH_P_IP);
		create_packet(interface->name, buffer, 2, DHCPNAK, (void *) interface);
		create_ipheader(buffer, get_iface_ip(interface->name), INADDR_BROADCAST);
	}
	else if (result == -1) return -1;

	create_udpheader(buffer, DHCP_SERVER_PORT, DHCP_CLIENT_PORT);

	add_log("DHCPACK/DHCPNAK sended!");
	return 0;
}

void * s_recvDHCP(void * arg)
{

	dserver_interface_t * interface = (dserver_interface_t *)arg;
	int                   bytes;
	struct ethheader    * eth;
	struct dhcp_packet  * dhc;
	struct qmessage       mess;
	char                  buffer[2048];

	add_log("Start server receiving thread!");

	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		bytes = recvfrom(interface->listen_sock, buffer, 1518, 0, NULL, 0);
		if (bytes == -1)
		{
			perror("<s_recvDHCP> recvfrom");
		}
		if (bytes < DHCP_FIXED_NON_UDP || bytes > DHCP_MAX_OPTION_LEN) continue;

		eth = (struct ethheader*) buffer;
		printf("\nrecv %d bytes\n", bytes); fflush(stdout);
		printf("source  "); printmac(eth->smac);
		printf("dest  ");   printmac(eth->dmac);

		dhc = (struct dhcp_packet*) (buffer + FULLHEAD_LEN);
		if (dhc->op == 1) 
		{
			printf("I think this is dhcp-request\n");
			memcpy(mess.text, buffer, sizeof(buffer));
			mess.delay = STEP;
			pushmessage(mess, 2 * interface->c_idx);				//Помещаем в очередь
		}
	}
}

void * s_replyDHCP(void * arg) 
{
	dserver_interface_t * interface = (dserver_interface_t *)arg;
	struct qmessage mess;

	add_log("Start server replying thread");

	while(1)
	{
		add_log("SEND_SERVER_REPLY");
		mess = popmessage(2 * interface->c_idx + 1);
		sendDHCP(interface->send_sock, interface->name, (void*)mess.text, 0);
	}
}

void * s_fsmDHCP(void * arg)
{
	add_log("Start fsm");

	dserver_interface_t * interface = (dserver_interface_t *)arg;
	int scount  = 0;
	int i = 0;
	struct qmessage mess;
	struct session *ses;

	struct timer t = { interface->cci, sm, (void *)interface };
	int result = pthread_create(&interface->fsm_timer, NULL, timer, (void *)&t); //Создание потока таймера
	if (result != 0)
	{
		perror("Ошибка создания потока таймера");
		pthread_exit(NULL);
	}
	
	while(1)
	{
		mess = popmessage(2 * interface->c_idx);
		
		if (mess.delay == STEP) 
		{
			struct dhcp_packet *dhc = (struct dhcp_packet *) (mess.text + FULLHEAD_LEN);
			//Делаем шаг автомата
			int num = change_state(dhc->xid, dhc->options[6], mess, &ses, &scount, (void *) interface);
			if (num == -1) continue;
			//Пересылаем сообщение
			if (ses[num].state != CLOSE) pushmessage(ses[num].mess, 2 * interface->c_idx + 1);
		}
		if (mess.delay == TIME)  
		{	
			//printf("\n\ntime to change, scount=%d\n", scount);
			clear_context(&ses, &scount, (void *) interface);		   //Очищаем базу сессий
			//printf("cleared sessions, new scount =%d\n",scount);
			if ( ++i % (60 / interface->cci) == 0) 
			{ 
				clear_lease(); 
				i = 0;
			} 
		}
		if (mess.delay == DISABLE)
		{
			printf("disabling fsm...\n");
			if (ses != NULL) 
			{
				free(ses);
			}
			pthread_exit(PTHREAD_CANCELED);
		}
	}

}

int enable_interface(dserver_interface_t * interface, int idx)
{
	interface->enable = 1;
	interface->c_idx  = idx;
	interface->listen_sock = init_packet_sock(interface->name, ETH_P_ALL); //? ETH_P_ALL
	interface->send_sock   = init_packet_sock(interface->name, ETH_P_IP); 
	
	if (interface->listen_sock == -1 || interface->send_sock == -1) return -1;
	
	int result;
	
	result = pthread_create(&interface->listen, NULL, s_recvDHCP, (void *)interface); //Создание потока приема
	if (result != 0)
	{
		perror("Ошибка создания потока приема");
		pthread_exit(NULL);
	}
	
	result = pthread_create(&interface->fsm, NULL, s_fsmDHCP, (void *)interface); //Создание потока контекстов
	if (result != 0)
	{
		perror("Ошибка создания потока FSM");
		pthread_exit(NULL);
	}

	result = pthread_create(&interface->sender, NULL, s_replyDHCP, (void *)interface); //Создание потока ответа
	if (result != 0)
	{
		perror("Ошибка создания потока ответа");
		pthread_exit(NULL);
	}
	
	printf("IFACE %s ENABLED!\n", interface->name);
	
	return 0;
}

int disable_interface(dserver_interface_t * interface, int idx)
{
	if (0 == pthread_cancel(interface->listen)) 
	{ 
		void *res;
		if (0 != pthread_join(interface->listen, &res)) return -1;
		if (res == PTHREAD_CANCELED)
			printf("iface %s: listen thread was canceled\n", interface->name);
		else
			printf("iface %s: listen thread wasn't canceled (shouldn't happen!)\n", interface->name);
				//Запилить бы сборщик мусорных  ниток
	}
	
	if (0 == pthread_cancel(interface->sender)) 
	{ 
		void *res;
		if (0 != pthread_join(interface->sender, &res)) return -1;
		if (res == PTHREAD_CANCELED)
			printf("iface %s: sender thread was canceled\n", interface->name);
		else
			printf("iface %s: sender thread wasn't canceled (shouldn't happen!)\n", interface->name);
				//Запилить бы сборщик мусорных  ниток
	}
	
	if (0 == pthread_cancel(interface->fsm_timer)) 
	{ 
		void *res;
		if (0 != pthread_join(interface->fsm_timer, &res)) return -1;
		if (res == PTHREAD_CANCELED)
			printf("iface %s: timer thread was canceled\n", interface->name);
		else
			printf("iface %s: timer thread wasn't canceled (shouldn't happen!)\n", interface->name);
				//Запилить бы сборщик мусорных  ниток
	}
	
	struct qmessage mess;
	mess.delay = DISABLE;
	pushmessage(mess, 2 * interface->c_idx);
	
	void * res;
	if (0 != pthread_join(interface->fsm, &res)) return -1;
	if (res == PTHREAD_CANCELED)
		printf("iface %s: fsm thread was canceled\n", interface->name);
	else
		printf("iface %s: fsm thread wasn't canceled (shouldn't happen!)\n", interface->name);
			//Запилить бы сборщик мусорных  ниток
	
	
	interface->enable = 0;
	close(interface->listen_sock);
	close(interface->send_sock);
		
	printf("IFACE %s DISABLED!\n", interface->name);
	return 0;
}

void init_ptable(int size)
{
	ptable_count = size;
	ptable = realloc(ptable, size * sizeof(struct  pass));

	ptable[0].currstate = START;  ptable[0].in = DHCPDISCOVER; ptable[0].nextstate = OFFER;  ptable[0].fun = NULL;
	ptable[1].currstate = START;  ptable[1].in = DHCPREQUEST;  ptable[1].nextstate = ANSWER; ptable[1].fun = NULL;
	ptable[2].currstate = START;  ptable[2].in = UNKNOWN;      ptable[2].nextstate = CLOSE;  ptable[2].fun = NULL;

	ptable[3].currstate = OFFER;  ptable[3].in = DHCPREQUEST;  ptable[3].nextstate = ANSWER; ptable[3].fun = send_offer;
	ptable[4].currstate = OFFER;  ptable[4].in = UNKNOWN;      ptable[4].nextstate = CLOSE;  ptable[4].fun = send_offer;

	ptable[5].currstate = ANSWER; ptable[5].in = DHCPREQUEST;  ptable[5].nextstate = ANSWER; ptable[5].fun = send_answer;
	ptable[6].currstate = ANSWER; ptable[6].in = UNKNOWN;      ptable[6].nextstate = NAK;    ptable[6].fun = send_answer;

	ptable[7].currstate = NAK;    ptable[7].in = 0;            ptable[7].nextstate = CLOSE;  ptable[7].fun = send_nak;
}

void * sm(void * arg)
{
	dserver_interface_t * interface = (dserver_interface_t *)arg;

	struct qmessage mess;
	mess.delay = TIME;
	pushmessage(mess, 2 * interface->c_idx);
	
	return NULL;
}

int check_password(DSERVER * server, char * check)
{
	if (0 == strlen(server->password)) 
	{	
		generate_salt(server->salt, sizeof(server->salt));
		generate_hash(check, strlen(check), server->salt, sizeof(server->salt), server->password, sizeof(server->password));
		return 0;
	} 
	else
	{	
		char hash[128];
		generate_hash(check, strlen(check), server->salt, sizeof(server->salt), hash, sizeof(hash));
		
		if (0 != memcmp(server->password, hash, sizeof(server->password))) return -1;
		
		generate_salt(server->salt, sizeof(server->salt));
		generate_hash(check, strlen(check), server->salt, sizeof(server->salt), server->password, sizeof(server->password));
	}
	
	return 0;
}

int get_iface_idx_by_name(char * ifname, DSERVER * server)
{
	int i;
	for ( i = 0; i < MAX_INTERFACES; i++)
		if (0 == strcmp(server->interfaces[i].name, ifname))
			return i;
	
	return -1;
}

int del_subnet_from_interface(dserver_interface_t * interface, char * args)
{
	dserver_subnet_t * subnet = search_subnet(interface, args);
	
	if (subnet == NULL) return -1;
	else 
	{
		printf("delete subnet\n");
		dserver_subnet_t * prev = subnet->prev;
		dserver_subnet_t * next = subnet->next;

		if (subnet == interface->settings.subnets)
		{
			free(interface->settings.subnets);
			interface->settings.subnets = next;
			if (next)
				interface->settings.subnets->prev = NULL;
		}
		else
		{	
			if (prev)
				prev->next = subnet->next;
			if (next)
				next->prev = subnet->prev;
			free(subnet);
		}

		return 0;
	}
	
	return -1;
}

dserver_subnet_t * add_subnet_to_interface(dserver_interface_t * interface, char * args)
{ //todo refactoring
	dserver_if_settings_t * settings = &interface->settings;
	dserver_subnet_t      * subnet   = settings->subnets;
	dserver_subnet_t      * temp     = NULL;
	
	char * address;
	char * mask;
	
	if (strlen(args) < (2 * 7))
	{
		printf("low args to add subnet:\n   args: %s\n   len: %d!\n", args, strlen(args));
		return NULL;
	}
	
	address = args;
	mask    = strchr(args, ' ');
	if (mask == NULL)
	{
		printf("low args to add subnet, please add mask\n");
		return NULL;
	}
	
	*mask = '\0';
	mask++;
	
	struct sockaddr_in a_sa, m_sa;
	if (!inet_pton(AF_INET, address, &(a_sa.sin_addr))) 
	{
		printf("can't parse address: %s\n", address);
		return NULL;
	}
	if (!inet_pton(AF_INET, mask, &(m_sa.sin_addr)))
		{
		printf("can't parse mask: %s\n", mask);
		return NULL;
	}
	
	while (subnet != NULL)
	{
		if (subnet->address == a_sa.sin_addr.s_addr && subnet->netmask == m_sa.sin_addr.s_addr)
		{
			printf("Subnet %s/%s exist!\n", address, mask);
			return NULL;
		}
		temp   = subnet;
		subnet = subnet->next;
		printf("%d go next\n", temp->address);
	}
	
	subnet = malloc(sizeof(dserver_subnet_t));
	memset(subnet, 0, sizeof(*subnet));
	subnet->address        = a_sa.sin_addr.s_addr;
	subnet->netmask        = m_sa.sin_addr.s_addr;
	subnet->free_addresses = 0;
	subnet->lease_time     = 0; 

	if (settings->subnets == NULL)
	{
		settings->subnets = subnet;
		subnet->prev      = NULL;
		subnet->next      = NULL;
	}
	else
	{
		temp->next   = subnet;
		subnet->next = NULL;
		subnet->prev = temp;
	}
		
	printf("subnet %s/%s added\n", address, mask);
	
	return subnet;
}

dserver_subnet_t * search_subnet(dserver_interface_t * interface, char * args)
{
	dserver_if_settings_t * settings = &interface->settings;
	dserver_subnet_t * subnet = settings->subnets;
	
	char * address;
	char * mask;
	
	if (strlen(args) < (2 * 7))
	{
		printf("low args to search subnet:\n   args: %s\n   len: %d!\n", args, strlen(args));
		return NULL;
	}
	
	address = args;
	mask = strchr(args, ' ');
	if (mask == NULL)
	{
		printf("low args to search subnet, please add mask\n");
		return NULL;
	}
	
	*mask = '\0';
	mask++;
	
	char * ptr = strchr(mask, ' ');
	if (ptr) 
	{
		*ptr = '\0';
		ptr++;
	}
	
	struct sockaddr_in a_sa, m_sa;
	if (!inet_pton(AF_INET, address, &(a_sa.sin_addr))) 
	{
		printf("can't parse address: %s\n", address);
		return NULL;
	}
	if (!inet_pton(AF_INET, mask, &(m_sa.sin_addr)))
		{
		printf("can't parse mask: %s\n", mask);
		return NULL;
	}
	
	while (subnet != NULL)
	{
		if (subnet->address == a_sa.sin_addr.s_addr && subnet->netmask == m_sa.sin_addr.s_addr)
		{
			printf("Subnet %s/%s found!\n", address, mask);
			if (ptr)
			{
				int i;
				int len = strlen(ptr);
				for ( i = 0; i < DCTP_ARG_MAX_LEN && i < len; i++ )
					args[i] = ptr[i];
				args[i] = '\0';
			}
			return subnet;
		}
		printf("subnet---->>"); printip(subnet->address);
		subnet = subnet->next;
	}
	
	printf("Subnet %s/%s not found!\n", address, mask);
	return NULL;
}

int add_pool_to_subnet(dserver_subnet_t * subnet, char * range)
{
	dserver_pool_t * pool = subnet->pools;
	dserver_pool_t * temp = NULL;
	ip_address_range_t a_range;
	
	if (range == NULL) return -1;
	
	if (-1 == ip_address_range_parse(range, &a_range))
	{
		printf("wrong address-range\n");
		return -1;
	}
	
	if ((a_range.start_address & subnet->netmask) != subnet->address ||
		(a_range.end_address & subnet->netmask) != subnet->address ||
		a_range.start_address == subnet->address ||
		a_range.end_address == subnet->address)
	{
		printf("address-range overlap subnet\n");
		return -1;
	}
	
	while (pool != NULL)
	{
		if (ip_address_range_is_overlap(&pool->range, &a_range))
		{
			printf("pool %s exist!\n", range);
			return -1;
		}
		printf("%d go next\n", pool->range.start_address);
		temp = pool;
		pool = pool->next;
	}
	
	pool = malloc(sizeof(dserver_pool_t));
	memset(pool, 0, sizeof(*pool));
	pool->next = NULL;	
	if (temp) temp->next = pool;
	else subnet->pools = pool;
		
	pool->range = a_range;
	subnet->free_addresses += htonl(a_range.end_address) - htonl(a_range.start_address) + 1; //  под рефакторинг
	printf("pool %s added\n", range);
	
	return 0;
}

int del_pool_from_subnet(dserver_subnet_t * subnet, char * range)
{
	dserver_pool_t * pool = subnet->pools;
	dserver_pool_t * temp = NULL;
	ip_address_range_t a_range;
	
	if (range == NULL) return -1;
	
	if (-1 == ip_address_range_parse(range, &a_range))
	{
		printf("wrong address-range\n");
		return -1;
	}
	
	while (pool != NULL)
	{
		if (ip_address_range_is_overlap(&pool->range, &a_range))
		{
			printf("delete pool %s\n", range);
			if (temp) 
				temp->next = pool->next;
			else 
				subnet->pools = pool->next;
			
			subnet->free_addresses -= htonl(a_range.end_address) - htonl(a_range.start_address) + 1; //  под рефакторинг
			free(pool);
			return 0;
		}
		temp = pool;
		pool = pool->next;
	}
	
	printf("pool %s is not exist\n", range);
	return -1;
}

int add_dns_to_subnet(dserver_subnet_t * subnet, char * address)
{
	dserver_dns_t * dns = subnet->dns_servers;
	dserver_dns_t * temp = NULL;
	
	if (address == NULL) return -1;
	
	u_int32_t ip = inet_addr(address);
	if (ip == -1) return -1;
		
	while (dns != NULL)
	{
		if (ip == dns->address) 
		{
			printf("dns-server is exist");
		}
		temp = dns;
		dns = dns->next;
	}
	
	dns = malloc(sizeof(dserver_dns_t));
	memset(dns, 0, sizeof(*dns));
	dns->next = NULL;	
	
	if (temp) 
		temp->next = dns;
	else 
		subnet->dns_servers = dns;
		
	dns->address = ip;
	printf("dns-server %s added\n", address);
	
	return 0;
}

int del_dns_from_subnet(dserver_subnet_t * subnet, char * address)
{
	dserver_dns_t * dns  = subnet->dns_servers;
	dserver_dns_t * temp = NULL;
	
	if (address == NULL) return -1;
		
	u_int32_t ip = inet_addr(address); //TODO parsing
	if (ip == -1) return -1;
		
	while (dns != NULL)
	{
		if (ip == dns->address)
		{
			printf("del dns-server %s\n", address);
			if (temp) 
				temp->next = dns->next;
			else 
				subnet->dns_servers = dns->next;
			
			free(dns);
			return 0;
		}
		
		temp = dns;
		dns = dns->next;
	}
		
	printf("dns-server %s is not exist\n", address);
	
	return -1;
}

int add_router_to_subnet(dserver_subnet_t * subnet, char * address)
{
	dserver_router_t * router = subnet->routers;
	dserver_router_t * temp   = NULL;
	
	if (address == NULL) return -1;
	
	u_int32_t ip = inet_addr(address);
	if (ip == -1) return -1;
	
	if ((ip & subnet->netmask) != subnet->address)
	{
		printf("router is not in subnet!\n");
		return -1;
	}
	
	while (router != NULL)
	{
		if (ip == router->address) 
		{
			printf("router is exist");
		}
		temp   = router;
		router = router->next;
	}
	
	router = malloc(sizeof(dserver_router_t));
	memset(router, 0, sizeof(*router));
	router->next = NULL;	
	
	if (temp) 
		temp->next = router;
	else 
		subnet->routers = router;
		
	router->address = ip;
	printf("router %s added\n", address);
	
	return 0;
}

int del_router_from_subnet(dserver_subnet_t * subnet, char * address)
{
	dserver_router_t * router = subnet->routers;
	dserver_router_t * temp = NULL;
	
	if (address == NULL) return -1;
	
	u_int32_t ip = inet_addr(address); //TODO parsing
	if (ip == -1) return -1;
	
	while (router != NULL)
	{
		if (ip == router->address)
		{
			printf("del router-server %s\n", address);
			if (temp) 
				temp->next = router->next;
			else 
				subnet->routers = router->next;
			
			free(router);
			return 0;
		}
		
		temp = router;
		router = router->next;
	}
		
	printf("router-server %s is not exist\n", address);
	
	return -1;
}

int set_domain_name_on_subnet(dserver_subnet_t * subnet, char * args)
{
	if ( args == NULL ) return -1;
	if ( strlen(args) >= sizeof(subnet->domain_name) ) return -1;
	
	strncpy(subnet->domain_name, args, sizeof(subnet->domain_name));
	printf("set domain name: %s\n", args);
	
	return 0;
}

int set_host_name_on_subnet(dserver_subnet_t * subnet, char * args)
{
	if ( args == NULL ) return -1;
	if ( strlen(args) >= sizeof(subnet->host_name) ) return -1;
	
	strncpy(subnet->host_name, args, sizeof(subnet->host_name));
	printf("set host name: %s\n", args);
	
	return 0;
}

long get_one_num(char * args)
{
	if (args == NULL || strlen(args) == 0) return -1;
		
	char * endptr;
	long ret;

	errno = 0;
	ret = strtol(args, &endptr, 10);

	if ((errno == ERANGE && (ret == LONG_MAX || ret == LONG_MIN)) || (errno != 0 && ret == 0)) 
	{
		perror("strtol");
		return -1;
	}

	if (endptr == args) 
	{
		printf("<%s> no digits were found in input str\n", __FUNCTION__);
		return -1;
	}
	
	return ret;
}

int execute_DCTP_command(DCTP_COMMAND * in, DSERVER * server)
{
	printf("<%s>\n", __FUNCTION__);
	char ifname[IFNAMELEN];
	
	DCTP_cmd_code_t code = parse_DCTP_command(in, ifname);
	if (code == UNDEF_COMMAND) return -1;
	int idx = -1;
	
	printf("%s: %s\n", in->name, in->arg);
	
	switch (code)
	{
		case SR_SET_IFACE_ENABLE:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			if ( server->interfaces[idx].enable == 1 ) return -1;
			if ( -1 == enable_interface(&server->interfaces[idx], idx) ) return -1;
			break;
			
		case SR_SET_IFACE_DISABLE:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			if ( server->interfaces[idx].enable == 0 ) return -1;
			if ( -1 == disable_interface(&server->interfaces[idx], idx) ) return -1;
			break;
		
		case SR_SET_LEASETIME:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				long ltime;
				if (strstr (in->arg, "global"))
				{
					ltime = get_one_num(in->arg + strlen("global") + 1);
					if (ltime == -1) return -1;
					server->interfaces[idx].settings.global.default_lease_time = ltime;
				}
				else
				{
					dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
					if ( NULL == sub ) return -1;
					ltime = get_one_num(in->arg);
					if (ltime == -1) return -1;
					sub->lease_time = ltime;
				}
			}
			break;
			
		case SR_SET_HOST_NAME:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if (-1 == set_host_name_on_subnet(sub, in->arg)) return -1;
			}
			break;
			
		case SR_SET_DOMAIN_NAME:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if (-1 == set_domain_name_on_subnet(sub, in->arg)) return -1;
			}
			break;
		
		case SR_ADD_SUBNET:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			if ( NULL == add_subnet_to_interface(&server->interfaces[idx], in->arg) ) return -1;
			break;
			
		case SR_DEL_SUBNET:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			if ( -1 == del_subnet_from_interface(&server->interfaces[idx], in->arg) ) return -1;
			break;
		
		case SR_ADD_POOL:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if ( -1 == add_pool_to_subnet(sub, in->arg) ) return -1;
			}
			break;
			
		case SR_DEL_POOL:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if ( -1 == del_pool_from_subnet(sub, in->arg) ) return -1;
			}
			break;
		
		case SR_ADD_DNS:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if ( -1 == add_dns_to_subnet(sub, in->arg) ) return -1;
			}
			break;
		
		case SR_DEL_DNS:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if ( -1 == del_dns_from_subnet(sub, in->arg) ) return -1;
			}
			break;
			
		case SR_ADD_ROUTER:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if ( -1 == add_router_to_subnet(sub, in->arg) ) return -1;
			}
			break;
		
		case SR_DEL_ROUTER:
			idx = get_iface_idx_by_name(ifname, server);
			if ( idx == -1 ) return -1;
			else 
			{
				dserver_subnet_t * sub = search_subnet(&server->interfaces[idx], in->arg);
				if ( NULL == sub ) return -1;
				if ( -1 == del_router_from_subnet(sub, in->arg) ) return -1;
			}
			break;
			
		case DCTP_PING:
			return 0;
			
		case DCTP_PASSWORD:
			if (-1 == check_password(server, in->arg)) return -1;
			break;
		
		case DCTP_SAVE_CONFIG:
			if (-1 == save_config(server, S_CONFIG_FILE)) return -1;
			break;
			
		default:
			printf("Unknown command!\n");
			return -1;
	}
	
	return 0;
}

void * manipulate( void * server )
{
	int sock = init_DCTP_socket(DSR_DCTP_PORT); //TODO исправить а пока пускай висит на дефолтном
	
	while(1)
	{
		DCTP_COMMAND_PACKET pack;
		memset(&pack, 0, sizeof(pack));
		struct sockaddr_in sender;
		
		receive_DCTP_command(sock, &pack, &sender); //успевает ли отработать?
		if (execute_DCTP_command(&pack.payload, (DSERVER*)server) == 0)
			send_DCTP_REPLY(sock, &pack, DCTP_SUCCESS, &sender);
		else
			send_DCTP_REPLY(sock, &pack, DCTP_FAIL, &sender);
	}
	
	release_DCTP_socket(sock);
}

int main()
{//Добавить проверку чтобы нельзя было  открыть еще один экземпляр
	pthread_t manipulate_tid;
	DSERVER server;
	memset(&server, 0, sizeof(server));

	srand(time(NULL));
	init_ptable(PTABLE_COUNT);
	initres(2 * sizeof(server.interfaces));
	initsync(2 * sizeof(server.interfaces));
	
	//LOADING CONFIGURATION
	DSERVER * temp_config = (DSERVER * )malloc(sizeof(*temp_config));
	memset(temp_config, 0, sizeof(*temp_config));
	
	if (-1 != load_config(temp_config, S_CONFIG_FILE))
	{
		memcpy(&server, temp_config, sizeof(server));
		save_config(&server, S_CONFIG_FILE);
	}
	free(temp_config);
	
	int i;
	for (i = 0; i < MAX_INTERFACES; i++)
	{	
		dserver_interface_t * interface = &server.interfaces[i];
		if ( strlen(interface->name) == 0 ) continue;
		if ( interface->enable == 0 ) continue;
		interface->cci = 5;
		if ( -1 == enable_interface(interface, i) ) return -1;
	}
	
	//END LOADING CONFIGURATION

	pthread_create(&manipulate_tid, NULL, manipulate, (void *)&server);

	pthread_join(manipulate_tid, NULL);

	uninitres(2 * sizeof(server.interfaces));
	uninitsync(2 * sizeof(server.interfaces));

	return 0;
}
