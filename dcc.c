#include "libdhcp/dhcp.h"
#include "libdhcp/dhconn.h"
#include "libdhcp/dhioctl.h"
#include "libdhcp/dleases.h"

#include "libdctp/dctp.h"

#include <unistd.h>	
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <net/if.h>
#include <ifaddrs.h>

#include "core.h"

#define C_CONFIG_FILE "dcc.conf"

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
		client->interfaces[i].settings.renew_interval = 10;
		if ( -1 != get_iface_idx_by_name(iter->ifa_name, client) ) continue;
		if ( (iter->ifa_flags & IFF_POINTOPOINT) == IFF_POINTOPOINT )  continue; 
		if ( (iter->ifa_flags & IFF_LOOPBACK) == IFF_LOOPBACK ) continue;
		
		strncpy(client->interfaces[i].name, iter->ifa_name, sizeof(client->interfaces[i].name));
		client->interfaces[i].enable = 0;
		printf("init interface: %d %s\n", i, client->interfaces[i].name);
	}
	
	freeifaddrs(ifa);
	
	return 0;
}

int save_config(DCLIENT * client, char * c_file)
{
	FILE * fd = fopen(c_file, "w");
	if (fd == NULL) return -1;
	int i;
	
	for (i = 0; i < MAX_INTERFACES; i++ )
	{
		dclient_interface_t * interface = &client->interfaces[i];
		if (strlen(interface->name) != 0)
		{
			fprintf(fd, "interface: %s\n", interface->name);
			
			fprintf(fd, "    %s\n", interface->enable == 1 ? "enable" : "disable");
			
			//dclient_if_settings_t * settings = &interface->settings;
			
			fprintf(fd, "end_interface\n");
		}
	}
		
	fclose(fd);
	return 0;
}

int load_interface (FILE * fd, dclient_interface_t * interface)
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
		else if (0 == strcmp(com, "end_interface"))
		{
			return 0;
		}
		memset(com, 0, sizeof(com));
		memset(args, 0, sizeof(args));
	}
	
	return -1;
}

int load_config(DCLIENT * client, char * c_file)
{
	FILE * fd = fopen(c_file, "r");
	if (fd == NULL) return -1;
	int i;
	char com[128] = "";
		
	for (i = 0; i < MAX_INTERFACES; i++ )
	{
		dclient_interface_t * interface = &client->interfaces[i];
		
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

int arp_proof(char * iface, u_int32_t addr)
{
	int try_cnt = 3;
	int sock = init_packet_sock(iface, ETH_P_ARP);
	
	while (try_cnt-- != 0)
	{
		if (-1 == sendARP(sock, iface, addr))
		{
			close(sock);
			return -1;
		}
		if (recvARP(sock, iface, addr)) 
		{ 
			close(sock);
			return 1;
		}
	}
	
	close(sock);
	return 0;
}

int printDHCP(frame_t * frame, char * iface)
{//Переделать
	printf("Your offer: "); printip(frame->p_dhc.yiaddr.s_addr);

	u_int32_t sip = 1;
	long time     = 1;
	
	//Получаем адрес сервера
	if (-1 == get_option(&frame->p_dhc, 54, &sip, sizeof(sip)))   return -1;	
	printf("SIP "); printip(sip);		
	
	if (-1 == get_option(&frame->p_dhc, 51, &time, sizeof(time))) return -1;	
	//Добавляем запись в лиз клиента		
	add_lease(iface, frame->p_dhc.yiaddr.s_addr, sip, ntohl(time));
	
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
	int idx = -1;
	
	switch (in->code)
	{
		case CL_SET_SERVER_ADDR:
			idx = get_iface_idx_by_name(in->interface, client);
			if ( idx == -1 ) return -1;
			strcpy(client->interfaces[idx].settings.server_address, in->arg);
			//printf("set server address on interfase %s : %s\n", ifname, in->arg);
			break;
			
		case CL_SET_IFACE_ENABLE:
			idx = get_iface_idx_by_name(in->interface, client);
			if ( idx == -1 ) return -1;
			if ( client->interfaces[idx].enable == 1) return -1;
			if ( -1 == enable_interface(&client->interfaces[idx], idx) ) return -1;
			break;
			
		case CL_SET_IFACE_DISABLE:
			idx = get_iface_idx_by_name(in->interface, client);
			if ( idx == -1 ) return -1;
			if ( client->interfaces[idx].enable == 0) return -1;
			if ( -1 == disable_interface(&client->interfaces[idx], idx) ) return -1;
			break;
			
		case DCTP_PING:
			break;
			
		case DCTP_PASSWORD:
			if (-1 == check_password(client, in->arg)) return -1;
			break;
			
		case DCTP_SAVE_CONFIG:
			if (-1 == save_config(client, C_CONFIG_FILE)) return -1;
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
	frame_t frame; memset(&frame, 0, sizeof(frame));
	u_int32_t myxid = 0;
	int ret;
	u_int32_t REQ_ADDR = INADDR_BROADCAST;

	goto renewal;
	
start:

	while(1)
	{
		//Собираем пакет DHCPDISCOVER и отсылаем, ждем DHCPOFFER
		set_my_mac(interface->name, macs);
		memset(&frame, 0, sizeof(frame));
		myxid = create_packet(interface->name, &frame, BOOTP_REQUEST, DHCPDISCOVER, (void *)interface);
		create_ethheader(&frame, macs, macd, ETH_P_IP);
		create_ipheader(&frame, INADDR_ANY, INADDR_BROADCAST);
		create_udpheader(&frame, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
		
		sendDHCP(interface->send_sock, &frame, 0); 
		if (recvDHCP(interface->listen_sock, interface->name, &frame, BOOTP_REPLY, DHCPOFFER, myxid, 5 ) == -1) 
		{ 
			goto start;
		}

		if (-1 == printDHCP(&frame, interface->name)) goto start;
		REQ_ADDR = INADDR_BROADCAST;

request:	
		//Собираем пакет DHCPREQUEST и отсылаем, ждем ACK|NAK
		printip(REQ_ADDR);
		set_my_mac(interface->name, macs);	
		memset(&frame, 0, sizeof(frame));
		int req_xid = create_packet(interface->name, &frame, BOOTP_REQUEST, DHCPREQUEST, (void *)interface);
		if (myxid == 0) myxid = req_xid;
		
		frame.p_dhc.xid = myxid; //important
		create_ethheader(&frame, macs, macd, ETH_P_IP);
		create_ipheader(&frame, INADDR_ANY, REQ_ADDR);
		create_udpheader(&frame, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
	 
		sendDHCP(interface->send_sock, &frame, 0); 
		if (recvDHCP(interface->listen_sock, interface->name, &frame, BOOTP_REPLY, DHCPACK, myxid, 5 ) == -1) 
		{
			goto start;
		}
		
		REQ_ADDR = INADDR_BROADCAST;
		if (-1 == printDHCP(&frame, interface->name)) goto start;	
	
		//Посылаем проверочный ARP-запрос, в случае неудачи отсылаем DHCPDECLINE
		if (arp_proof(interface->name, frame.p_dhc.yiaddr.s_addr) == 0) break;
		else
		{
			set_my_mac(interface->name, macs);
			memset(&frame, 0, sizeof(frame));
			create_packet(interface->name, &frame, BOOTP_REQUEST, DHCPDECLINE, (void *)interface);
			frame.p_dhc.xid = myxid; //important
			create_ethheader(&frame, macs, macd, ETH_P_IP);
			create_ipheader(&frame, INADDR_ANY, INADDR_BROADCAST);
			create_udpheader(&frame, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
			sendDHCP(interface->send_sock, &frame, 0);
		}
	
	}	
	
	//Применяем полученную конфигурацию на интерфейс
	apply_interface_settings(&frame, interface->name);

renewal:		
	while(1)
	{
		if (myxid != 0) usleep(interface->settings.renew_interval * 1000000);
	
		//Получаем состояние аренды
		ret = get_lease(interface->name, (unsigned char *)NULL, (unsigned char *)&REQ_ADDR);
		if (ret == T_RENEWING) 
		{ 
			printf("RENEWING! \n");
			goto request;
		}
		else if (ret == T_REBINDING) 
		{ 
			printf("REBINDING! \n");
			REQ_ADDR = INADDR_BROADCAST;
			goto request;
		}
		else if (ret == T_END || ret == -1) 
		{ 
			printf("END LEASE! \n");
			REQ_ADDR = INADDR_BROADCAST;
			goto start;
		}
		else if (myxid == 0)
		{
			goto request;
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
                char error_string[DCTP_ERROR_DESC_SIZE] = "";
		if (execute_DCTP_command(&pack.payload, (DCLIENT*)client) == 0)
                        send_DCTP_REPLY(sock, &pack.packet, DCTP_SUCCESS, &sender, error_string);
		else
                        send_DCTP_REPLY(sock, &pack.packet, DCTP_FAIL, &sender, error_string);
	}
	
	release_DCTP_socket(sock);
}

int main()
{ //Добавить проверку чтобы нельзя было  открыть еще один экземпляр
	pthread_t manipulate_tid;
	DCLIENT client;
	memset(&client, 0, sizeof(client));

	srand(time(NULL));
	//LOADING CONFIGURATION
	DCLIENT * temp_config = (DCLIENT * )malloc(sizeof(*temp_config));
	memset(temp_config, 0, sizeof(*temp_config));
	
	if (-1 != load_config(temp_config, C_CONFIG_FILE))
	{
		init_interfaces(temp_config);
		memcpy(&client, temp_config, sizeof(client));
	}
	else
		init_interfaces(&client);
		
	free(temp_config);
	save_config(&client, C_CONFIG_FILE);
	
	int i;
	for (i = 0; i < MAX_INTERFACES; i++)
	{	
		dclient_interface_t * interface = &client.interfaces[i];
		if ( strlen(interface->name) == 0 ) continue;
		if ( interface->enable == 0 ) continue;
		if ( -1 == enable_interface(interface, i) ) return -1;
	}
	
	//END LOADING CONFIGURATION

	pthread_create(&manipulate_tid, NULL, manipulate, (void *)&client);
	pthread_join(manipulate_tid, NULL);
	
	return 0;
}
