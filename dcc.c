#include "dhcp.h"
#include "dctp.h"
#include "dhconn.h"
#include "dhioctl.h"
#include "dleases.h"
#include <pthread.h>
#include <net/if.h>
#include <ifaddrs.h>

#include "core.h"

void * iface_loop ( void * iface );
void * manipulate ( void * client );

int get_iface_idx_by_name(char * ifname, DCLIENT * client);

int init_interfaces(DCLIENT * client)
{
	struct ifaddrs * ifa;
	struct ifaddrs * iter;
	int i;
	
	getifaddrs (&ifa);
	
	for (i = 0, iter = ifa; iter != NULL && i < MAX_INTERFACES; i++, iter = iter->ifa_next)
	{
		if ( -1 != get_iface_idx_by_name(iter->ifa_name, client) ) continue;
		if ( (iter->ifa_flags & IFF_POINTOPOINT) == IFF_POINTOPOINT ) continue;
		if ( (iter->ifa_flags & IFF_LOOPBACK) == IFF_LOOPBACK ) continue;
		
		strncpy(client->interfaces[i].name, iter->ifa_name, sizeof(client->interfaces[i].name));
		client->interfaces[i].enable = 0;
		printf("init interface: %s\n", client->interfaces[i].name);
	}
	
	freeifaddrs(ifa);
	
	return 0;
}

int enable_interface(dclient_interface_t * interface, int idx)
{
	interface->enable = 1;
	interface->listen_sock = init_packet_sock(interface->name, ETH_P_ALL); //? ETH_P_ALL
	interface->send_sock   = init_packet_sock(interface->name, ETH_P_IP); 
	
	if ( 0 != pthread_create(&interface->loop_tid, NULL, iface_loop, (void *)interface)) return -1;
	
	printf("IFACE %s ENABLED!\n", interface->name);
	
	return 0;
}

int disable_interface(dclient_interface_t * interface, int idx)
{
	interface->enable = 0;
	close(interface->listen_sock);
	close(interface->send_sock);
	
	if (0 == pthread_cancel(interface->loop_tid)) 
	{ 
		void *res;
		if (0 != pthread_join(interface->loop_tid, &res)) return -1;
		if (res == PTHREAD_CANCELED)
			printf("iface %s: thread was canceled\n", interface->name);
		else
			printf("iface %s: thread wasn't canceled (shouldn't happen!)\n", interface->name);
				//Запилить бы сборщик мусорных  ниток
	}
	
	printf("IFACE %s DISABLED!\n", interface->name);
	
	return 0;
}

int arp_proof(char * iface, char * buffer)
{
	int try_cnt = 3;
	struct dhcp_packet * dhc = (struct dhcp_packet*) (buffer + FULLHEAD_LEN);
	int sock = init_packet_sock(iface, ETH_P_ARP);
	
	while (try_cnt-- != 0)
	{
		if (-1 == sendARP(sock, iface, dhc->yiaddr.s_addr))
		{
			close(sock);
			return -1;
		}
		if (recvARP(sock, iface, dhc->yiaddr.s_addr)) 
		{ 
			close(sock);
			return 1;
		}
	}
	
	close(sock);
	return 0;
}

int printDHCP(char * buffer, char * iface)
{//Переделать
	struct dhcp_packet * dhc = (struct dhcp_packet *) (buffer + FULLHEAD_LEN);
	printf("Your offer: "); printip(dhc->yiaddr.s_addr);

	u_int32_t sip = 1;
	long time     = 1;
	
	//Получаем адрес сервера
	if (-1 == get_option(dhc, 54, &sip, sizeof(sip)))   return -1;	
	printf("SIP "); printip(sip);		
	
	if (-1 == get_option(dhc, 51, &time, sizeof(time))) return -1;	
	//Добавляем запись в лиз клиента		
	add_lease(iface, dhc->yiaddr.s_addr, sip, ntohl(time));
	
	return 0;
}

int get_iface_idx_by_name(char * ifname, DCLIENT * client)
{
	int i;
	for ( i = 0; i < MAX_INTERFACES; i++)
		if (0 == strcmp(client->interfaces[i].name, ifname))
			return i;
	
	return -1;
}

int check_password(DCLIENT * client, char * check)
{
	if (0 == strlen(client->password)) 
	{	
		generate_salt(client->salt, sizeof(client->salt));
		generate_hash(check, strlen(check), client->salt, sizeof(client->salt), client->password, sizeof(client->password));
		return 0;
	} 
	else
	{	
		char hash[128];
		generate_hash(check, strlen(check), client->salt, sizeof(client->salt), hash, sizeof(hash));
		
		if (0 != memcmp(client->password, hash, sizeof(client->password))) return -1;
		
		generate_salt(client->salt, sizeof(client->salt));
		generate_hash(check, strlen(check), client->salt, sizeof(client->salt), client->password, sizeof(client->password));
	}
	
	return 0;
}

int execute_DCTP_command(DCTP_COMMAND * in, DCLIENT * client)
{
	char ifname[IFNAMELEN];
	DCTP_cmd_code_t code = parse_DCTP_command(in, ifname);
	if (code == UNDEF_COMMAND) return -1;
	int idx = -1;
	
	switch (code)
	{
		case CL_SET_SERVER_ADDR:
			idx = get_iface_idx_by_name(ifname, client);
			if ( idx == -1 ) return -1;
			strcpy(client->interfaces[idx].settings.server_address, in->arg);
			printf("set server address on interfase %s : %s\n", ifname, in->arg);
			break;
			
		case CL_SET_IFACE_ENABLE:
			idx = get_iface_idx_by_name(ifname, client);
			if ( idx == -1 ) return -1;
			if ( client->interfaces[idx].enable == 1) return -1;
			if ( -1 == enable_interface(&client->interfaces[idx], idx) ) return -1;
			break;
			
		case CL_SET_IFACE_DISABLE:
			idx = get_iface_idx_by_name(ifname, client);
			if ( idx == -1 ) return -1;
			if ( client->interfaces[idx].enable == 0) return -1;
			if ( -1 == disable_interface(&client->interfaces[idx], idx) ) return -1;
			break;
			
		case DCTP_PING:
			break;
			
		case DCTP_PASSWORD:
			if (-1 == check_password(client, in->arg)) return -1;
			break;
			
		default:
			printf("unknown command!\n");
			return -1;
	}
	
	return 0;
}

void * iface_loop (void * iface)
{
	dclient_interface_t * interface = (dclient_interface_t *)iface;
	unsigned char macs[6];
	unsigned char macd[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
	char buf[2048];
	u_int32_t myxid;
	int ret;
	u_int32_t REQ_ADDR = INADDR_BROADCAST;

start:

	while(1)
	{
		//Собираем пакет DHCPDISCOVER и отсылаем, ждем DHCPOFFER
		set_my_mac(interface->name, macs);
		memset(buf, 0, sizeof(buf));
		create_ethheader((void *)buf, macs, macd, ETH_P_IP);
		create_ipheader(buf, INADDR_ANY, INADDR_BROADCAST);
		myxid = create_packet(interface->name, buf, 1, DHCPDISCOVER, (void *)interface);
		create_udpheader(buf, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
			
		sendDHCP(interface->send_sock, interface->name, (void*)buf, 0); 
		if (recvDHCP(interface->listen_sock, interface->name, (void*)buf, DHCPOFFER, myxid ) == -1) 
		{ 
			goto start;
		}

		if (-1 == printDHCP(buf, interface->name)) goto start;
		REQ_ADDR = INADDR_BROADCAST;

request:	
		//Собираем пакет DHCPREQUEST и отсылаем, ждем ACK|NAK
		printip(REQ_ADDR);
		set_my_mac(interface->name, macs);	
		create_ethheader((void *)buf, macs, macd, ETH_P_IP);
		create_ipheader(buf, INADDR_ANY, REQ_ADDR);
		create_packet(interface->name, buf, 1, DHCPREQUEST, (void *)interface);
		create_udpheader(buf, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
	 
		sendDHCP(interface->send_sock, interface->name, (void*)buf, 0); 
		if (recvDHCP(interface->listen_sock, interface->name, (void*)buf, DHCPACK, myxid ) == -1) 
		{
			goto start;
		}
		
		REQ_ADDR = INADDR_BROADCAST;
		if (-1 == printDHCP(buf, interface->name)) goto start;	
	
		//Посылаем проверочный ARP-запрос, в случае неудачи отсылаем DHCPDECLINE
		if (arp_proof(interface->name, buf) == 0) break;
		else
		{
			set_my_mac(interface->name, macs);
			create_ethheader((void *)buf, macs, macd, ETH_P_IP);
			create_ipheader(buf, INADDR_ANY, INADDR_BROADCAST);
			create_packet(interface->name, buf, 1, DHCPDECLINE, (void *)interface);
			create_udpheader(buf, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
			sendDHCP(interface->send_sock, interface->name, (void*)buf, 0);
		}
	
	}	
	
	//Применяем полученную конфигурацию на интерфейс
	apply_interface_settings(buf, interface->name);
	
	while(1)
	{
		sleep(20);
	
		//Получаем состояние аренды
		ret = get_lease(interface->name, (unsigned char *)NULL, (unsigned char *)&REQ_ADDR);
		if (ret == T_RENEWING) 
		{ 
			printf("RENEWING! \n");
			goto request;
		}
		if (ret == T_REBINDING) 
		{ 
			printf("REBINDING! \n");
			REQ_ADDR = INADDR_BROADCAST;
			goto request;
		}
		if (ret == T_END || ret == -1) 
		{ 
			printf("END LEASE! \n");
			REQ_ADDR = INADDR_BROADCAST;
			goto start;
		}
	}

}

void * manipulate( void * client )
{
	int sock = init_DCTP_socket(DCL_DCTP_PORT);
	
	while(1)
	{
		DCTP_COMMAND_PACKET pack;
		struct sockaddr_in sender;
		
		receive_DCTP_command(sock, &pack, &sender);
		if (execute_DCTP_command(&pack.payload, (DCLIENT*)client) == 0)
			send_DCTP_REPLY(sock, &pack, DCTP_SUCCESS, &sender);
		else
			send_DCTP_REPLY(sock, &pack, DCTP_FAIL, &sender);
	}
	
	release_DCTP_socket(sock);
}

int main()
{ //Добавить проверку чтобы нельзя было  открыть еще один экземпляр
	pthread_t manipulate_tid;
	DCLIENT client;
	memset(&client, 0, sizeof(client));

	srand(time(NULL));

	init_interfaces(&client);

	pthread_create(&manipulate_tid, NULL, manipulate, (void *)&client);
	pthread_join(manipulate_tid, NULL);
	
	return 0;
}
