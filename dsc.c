#include "libdhcp/dhcp.h"
#include "libdhcp/dhconn.h"
#include "libdhcp/dhioctl.h"
#include "libdhcp/dhstate.h"
#include "libdhcp/dleases.h"

#include "libdctp/dctp.h"
#include "libtimer/timer.h"
#include "libcommon/common.h"

#include "core.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <limits.h>
#include <net/if.h>
#include <ifaddrs.h>

#define PTABLE_COUNT   9
#define S_CONFIG_FILE "dsc.conf"

void * iface_loop ( void * iface );
void * manipulate ( void * server );
void * s_recvDHCP ( void * arg );
void * s_replyDHCP ( void * arg );
void * sm ( void * arg );

dserver_subnet_t * search_subnet ( dserver_interface_t * interface, char * args );
dserver_subnet_t * add_subnet_to_interface ( dserver_interface_t * interface, char * args );
int add_range_to_subnet ( dserver_subnet_t * subnet, char * range );
int add_dns_to_subnet ( dserver_subnet_t * subnet, char * address );
int add_router_to_subnet ( dserver_subnet_t * subnet, char * address );
int set_host_name_on_subnet ( dserver_subnet_t * subnet, char * args );
int set_domain_name_on_subnet ( dserver_subnet_t * subnet, char * args );

long get_one_num ( char * args );

int get_iface_idx_by_name ( char * ifname, DSERVER * server );

int init_interface_settings ( dserver_if_settings_t * settings )
{
	//TODO
	return 0;
}

int init_interfaces ( DSERVER * server )
{
	struct ifaddrs * ifa;
	struct ifaddrs * iter;
	int i;

	getifaddrs( &ifa );

	for ( i = 0, iter = ifa; iter != NULL && i < MAX_INTERFACES; i ++, iter = iter->ifa_next )
	{
		if ( - 1 != get_iface_idx_by_name( iter->ifa_name, server ) ) continue;
		if ( ( iter->ifa_flags & IFF_POINTOPOINT ) == IFF_POINTOPOINT ) continue;
		if ( ( iter->ifa_flags & IFF_LOOPBACK ) == IFF_LOOPBACK ) continue;

		strncpy( server->interfaces[i].name, iter->ifa_name, sizeof (server->interfaces[i].name ) );
		server->interfaces[i].enable = 0;
		server->interfaces[i].cci = 5;
		init_interface_settings( &server->interfaces[i].settings );
		printf( "init interface: %s\n", server->interfaces[i].name );
	}

	freeifaddrs( ifa );

	return 0;
}

int save_config ( DSERVER * server, char * c_file )
{
	FILE * fd = fopen( c_file, "w" );
	if ( fd == NULL ) return - 1;
	int i;

	for ( i = 0; i < MAX_INTERFACES; i ++ )
	{
		dserver_interface_t * interface = & server->interfaces[i];
		if ( strlen( interface->name ) != 0 )
		{
			fprintf( fd, "interface: %s\n", interface->name );

			fprintf( fd, "    %s\n", interface->enable == 1 ? "enable" : "disable" );

			dserver_if_settings_t * settings = & interface->settings;

			dserver_subnet_t * subnet = settings->subnets;
			while ( subnet != NULL )
			{
				char subnet_mask[INET_ADDRSTRLEN];
				char subnet_addr[INET_ADDRSTRLEN];
				inet_ntop( AF_INET, &subnet->address, subnet_addr, sizeof (subnet_addr ) );
				inet_ntop( AF_INET, &subnet->netmask, subnet_mask, sizeof (subnet_mask ) );

				fprintf( fd, "    %s: %s %s\n", "subnet", subnet_addr, subnet_mask );

				if ( subnet->lease_time != 0 )
					fprintf( fd, "        %s: %ld\n", "lease_time", subnet->lease_time );

				if ( strlen( subnet->host_name ) != 0 )
					fprintf( fd, "        %s: %s\n", "host_name", subnet->host_name );

				if ( strlen( subnet->domain_name ) != 0 )
					fprintf( fd, "        %s: %s\n", "domain_name", subnet->domain_name );

				Q_FOREACH(ip_address_range_t *, range, subnet->pools,
					char start_address[INET_ADDRSTRLEN];
					char end_address[INET_ADDRSTRLEN];
					inet_ntop( AF_INET, &range->start_address, start_address, sizeof (start_address ) );
					inet_ntop( AF_INET, &range->end_address, end_address, sizeof (end_address ) );
					fprintf( fd, "        %s: %s-%s\n", "range", start_address, end_address );
				)

				Q_FOREACH(int *, dns, subnet->dns_servers,
					char dns_address[INET_ADDRSTRLEN];
					inet_ntop( AF_INET, dns, dns_address, sizeof (dns_address ) );
					fprintf( fd, "        %s: %s\n", "dns-server", dns_address );
				)

				Q_FOREACH(int *, router, subnet->routers, 
					char router_address[INET_ADDRSTRLEN];
					inet_ntop( AF_INET, router, router_address, sizeof (router_address ) );
					fprintf( fd, "        %s: %s\n", "router", router_address );
				)
				fprintf( fd, "    %s\n", "end_subnet" );

				subnet = subnet->next;
			}
			fprintf( fd, "end_interface\n" );
		}
	}

	fclose( fd );
	return 0;
}

int load_subnet ( FILE * fd, dserver_subnet_t * subnet )
{
	char com[128] = "";
	char args[128] = "";
	while ( EOF != t_gets( fd, ':', com, sizeof (com ), 0 ) )
	{
		if ( 0 == strcmp( com, "range" ) )
		{
			if ( EOF == t_gets( fd, 0, args, sizeof (args ), 0 ) )
				return - 1;
			if ( - 1 == add_range_to_subnet( subnet, args ) )
				printf( "parse config error: add range\n" );
		}
		else if ( 0 == strcmp( com, "dns-server" ) )
		{
			if ( EOF == t_gets( fd, 0, args, sizeof (args ), 0 ) )
				return - 1;
			if ( - 1 == add_dns_to_subnet( subnet, args ) )
				printf( "parse config error: add dns-server\n" );
		}
		else if ( 0 == strcmp( com, "router" ) )
		{
			if ( EOF == t_gets( fd, 0, args, sizeof (args ), 0 ) )
				return - 1;
			if ( - 1 == add_router_to_subnet( subnet, args ) )
				printf( "parse config error: add router\n" );
		}
		else if ( 0 == strcmp( com, "lease_time" ) )
		{
			if ( EOF == t_gets( fd, 0, args, sizeof (args ), 0 ) )
				return - 1;

			long ltime = get_one_num( args );
			if ( ltime == - 1 )
				printf( "parse config error: set lease time\n" );
			else
				subnet->lease_time = ltime;
		}
		else if ( 0 == strcmp( com, "host_name" ) )
		{
			if ( EOF == t_gets( fd, 0, args, sizeof (args ), 0 ) )
				return - 1;

			if ( - 1 == set_host_name_on_subnet( subnet, args ) )
				printf( "parse config error: set host name\n" );
		}
		else if ( 0 == strcmp( com, "domain_name" ) )
		{
			if ( EOF == t_gets( fd, 0, args, sizeof (args ), 0 ) )
				return - 1;

			if ( - 1 == set_domain_name_on_subnet( subnet, args ) )
				printf( "parse config error: set domain name\n" );
		}
		else if ( 0 == strcmp( com, "end_subnet" ) )
		{
			return 0;
		}
		memset( com, 0, sizeof (com ) );
		memset( args, 0, sizeof (args ) );
	}

	return - 1;
}

int load_interface ( FILE * fd, dserver_interface_t * interface )
{
	char com[128] = "";
	char args[128] = "";
	while ( EOF != t_gets( fd, ':', com, sizeof (com ), 0 ) )
	{
		if ( 0 == strcmp( com, "enable" ) )
		{
			interface->enable = 1;
		}
		else if ( 0 == strcmp( com, "disable" ) )
		{
			interface->enable = 0;
		}
		else if ( 0 == strcmp( com, "subnet" ) )
		{
			t_gets( fd, 0, args, sizeof (args ), 0 );
			dserver_subnet_t * subnet = add_subnet_to_interface( interface, args );
			if ( subnet != NULL )
			{
				if ( - 1 == load_subnet( fd, subnet ) )
				{
					printf( "parse config error: can't reach end of subnet\n" );
					return - 1;
				}
			}
			else
				printf( "parse config error: subnet add\n" );
		}
		else if ( 0 == strcmp( com, "end_interface" ) )
		{
			return 0;
		}
		memset( com, 0, sizeof (com ) );
		memset( args, 0, sizeof (args ) );
	}

	return - 1;
}

int load_config ( DSERVER * server, char * c_file )
{
	FILE * fd = fopen( c_file, "r" );
	if ( fd == NULL ) return - 1;
	int i;
	char com[128] = "";

	for ( i = 0; i < MAX_INTERFACES; i ++ )
	{
		dserver_interface_t * interface = & server->interfaces[i];

		while ( EOF != t_gets( fd, ':', com, sizeof (com ), 0 ) )
		{
			if ( 0 == strcmp( com, "interface" ) )
			{
				memset( interface, 0, sizeof (*interface ) );
				t_gets( fd, 0, interface->name, sizeof (interface->name ), 0 );
				if ( - 1 == load_interface( fd, interface ) )
				{
					printf( "parse config error: can't reach end of interface" );
					return - 1;
				}
				break;
			}
			memset( com, 0, sizeof (com ) );
		}
	}

	fclose( fd );
	return 0;
}

int send_offer ( void * info, void * arg )
{

	if ( info == NULL ) return - 1;
	if ( arg == NULL ) return - 1;

	unsigned char macs[6];
	frame_t frame;
	memset( &frame, 0, sizeof (frame ) );
	request_t * request = ( request_t * ) info;
	dserver_interface_t * interface = ( dserver_interface_t * ) arg;

	add_log( "Sending DHCPOFFER.." );

	set_my_mac( interface->name, macs );
	frame.p_dhc.xid = request->xid;
	memcpy( frame.p_dhc.chaddr, request->mac, sizeof (frame.p_dhc.chaddr ) );

	if ( - 1 == create_packet( interface->name, &frame, 2, DHCPOFFER, interface ) )
	{
		perror( "create dhc packet:" );
		return - 1;
	}

	create_ethheader( &frame, macs, request->mac, ETH_P_IP );
	create_ipheader( &frame, get_iface_ip( interface->name ), frame.p_dhc.yiaddr.s_addr );
	create_udpheader( &frame, DHCP_SERVER_PORT, DHCP_CLIENT_PORT );

	qmessage_t message;
	message.size = frame.size;
	message.packet = malloc( message.size );
	memcpy( message.packet, &frame, message.size );
	push_queue( interface->qtransport, 1, &message, sizeof (message ) );

	add_log( "DHCPOFFER sended!" );
	return 0;
}

int send_nak ( void * info, void * arg )
{
	if ( info == NULL ) return - 1;
	if ( arg == NULL ) return - 1;

	frame_t frame;
	memset( &frame, 0, sizeof (frame ) );
	unsigned char macs[6];
	unsigned char macb[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	dserver_interface_t * interface = ( dserver_interface_t * ) arg;
	request_t * request = ( request_t * ) info;

	add_log( "Sending DHCPNAK.." );

	set_my_mac( interface->name, macs );
	frame.p_dhc.xid = request->xid;

	create_packet( interface->name, &frame, 2, DHCPNAK, ( void * ) interface );
	create_ethheader( &frame, macs, macb, ETH_P_IP );
	create_ipheader( &frame, get_iface_ip( interface->name ), INADDR_BROADCAST );
	create_udpheader( &frame, DHCP_SERVER_PORT, DHCP_CLIENT_PORT );

	qmessage_t message;
	message.size = frame.size;
	message.packet = malloc( message.size );
	memcpy( message.packet, &frame, message.size );
	push_queue( interface->qtransport, 1, &message, sizeof (message ) );

	add_log( "DHCPNAK sended!" );
	return 0;
}

int send_answer ( void * info, void * arg )
{
	if ( info == NULL ) return - 1;
	if ( arg == NULL ) return - 1;

	long ltime = 0;
	unsigned char macs[6];
	unsigned char macb[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	frame_t frame;
	memset( &frame, 0, sizeof (frame ) );
	dserver_interface_t * interface = ( dserver_interface_t * ) arg;
	request_t * request = ( request_t * ) info;

	add_log( "Sending DHCPACK/DHCPNAK.." );

	set_my_mac( interface->name, macs );
	frame.p_dhc.xid = request->xid;
	frame.p_dhc.yiaddr.s_addr = request->req_address;
	memcpy( frame.p_dhc.chaddr, request->mac, sizeof (frame.p_dhc.chaddr ) );

	//Проверяем доступен ли адрес
	int result = get_proof( request->mac, &request->req_address );
	if ( result > 0 )
	{
		//В случае положительного ответа отправляем АСК
		ltime = create_packet( interface->name, &frame, 2, DHCPACK, ( void * ) interface );
		if ( ltime == - 1 ) return - 1;
		create_ethheader( &frame, macs, request->mac, ETH_P_IP );
		create_ipheader( &frame, get_iface_ip( interface->name ), frame.p_dhc.yiaddr.s_addr );
		s_add_lease( interface, frame.p_dhc.yiaddr.s_addr, request->mac, ltime ); //TODO refactoring lease_time
	}
	else if ( result == 0 )
	{
		//В случае отрицательного ответа отправляем NAK
		create_packet( interface->name, &frame, 2, DHCPNAK, ( void * ) interface );
		create_ethheader( &frame, macs, macb, ETH_P_IP );
		create_ipheader( &frame, get_iface_ip( interface->name ), INADDR_BROADCAST );
	}
	else if ( result == - 1 ) return - 1;

	create_udpheader( &frame, DHCP_SERVER_PORT, DHCP_CLIENT_PORT );

	qmessage_t message;
	message.size = frame.size;
	message.packet = malloc( message.size );
	memcpy( message.packet, &frame, message.size );
	push_queue( interface->qtransport, 1, &message, sizeof (message ) );

	add_log( "DHCPACK/DHCPNAK sended!" );
	return ltime;
}

void * s_recvDHCP ( void * arg )
{
	dserver_interface_t * interface = ( dserver_interface_t * ) arg;
	frame_t frame;

	add_log( "Start server receiving thread!" );

	while ( 1 )
	{
		memset( &frame, 0, sizeof (frame ) );

		recvDHCP( interface->listen_sock, NULL, &frame, BOOTP_REQUEST, 0, 0, 0 );
		if ( frame.size == - 1 ) continue;

		printf( "DHCP REQUEST:\n" );
		printf( "   FRAME SIZE: %d\n", frame.size );
		printf( "   DTYPE: %s\n", stringize_dtype( frame.p_dhc.options[6] ) );
		printf( "   SRC   " );
		printmac( frame.h_eth.smac );
		printf( "   DST   " );
		printmac( frame.h_eth.dmac );
		fflush( stdout );

		qmessage_t message;
		message.code = STEP;
		message.size = frame.size;
		message.packet = malloc( frame.size );
		memcpy( message.packet, &frame, message.size );

		push_queue( interface->qtransport, 0, &message, sizeof (message ) ); //Помещаем в очередь
	}
}

void * s_replyDHCP ( void * arg )
{
	dserver_interface_t * interface = ( dserver_interface_t * ) arg;
	qmessage_t reply;

	add_log( "Start server replying thread" );

	while ( 1 )
	{
		add_log( "SEND_SERVER_REPLY" );
		pop_queue( interface->qtransport, 1, &reply, sizeof (reply ) );
		sendDHCP( interface->send_sock, ( frame_t * ) reply.packet, reply.size - 2 ); //padding 2
		free( reply.packet );
	}
}

void * s_fsmDHCP ( void * arg )
{
	add_log( "Start fsm" );

	dserver_interface_t * interface = ( dserver_interface_t * ) arg;
	struct timer t = { interface->cci, sm, ( void * ) interface };
	int i = 0;

	int result = pthread_create( &interface->fsm_timer, NULL, timer, ( void * ) &t ); //Создание потока таймера
	if ( result != 0 )
	{
		perror( "Ошибка создания потока таймера" );
		pthread_exit( NULL );
	}

	while ( 1 )
	{
		qmessage_t request;
		pop_queue( interface->qtransport, 0, &request, sizeof (request ) );

		if ( request.code == STEP )
		{
			frame_t * t = ( frame_t * ) request.packet;
			//Делаем шаг автомата
			change_state( t, interface->qsessions, ( void * ) interface );
		}
		if ( request.code == TIME )
		{
			//printf("\n\ntime to change, scount= %d\n", interface->qsessions->elements);
			clear_context( interface->qsessions, ( void * ) interface ); //Очищаем базу сессий
			//printf("cleared sessions, new scount = %d\n", interface->qsessions->elements);
			if ( ++ i % ( 60 / interface->cci ) == 0 )
			{
				clear_lease( );
				i = 0;
			}
		}
		if ( request.code == DISABLE )
		{
			printf( "disabling fsm...\n" );
			pthread_exit( PTHREAD_CANCELED );
		}
	}

}

int enable_interface ( dserver_interface_t * interface, int idx )
{
	interface->enable = 1;
	interface->c_idx = idx;
	interface->listen_sock = init_packet_sock( interface->name, ETH_P_ALL ); //? ETH_P_ALL
	interface->send_sock = init_packet_sock( interface->name, ETH_P_IP );
	interface->qtransport = init_queues( 2, Q_TRANSPORT_MODE );
	interface->qsessions = init_queues( 1, Q_STANDART_MODE );

	if ( interface->listen_sock == - 1 || interface->send_sock == - 1 ) return - 1;

	int result;

	result = pthread_create( &interface->listen, NULL, s_recvDHCP, ( void * ) interface ); //Создание потока приема
	if ( result != 0 )
	{
		perror( "Ошибка создания потока приема" );
		pthread_exit( NULL );
	}

	result = pthread_create( &interface->fsm, NULL, s_fsmDHCP, ( void * ) interface ); //Создание потока контекстов
	if ( result != 0 )
	{
		perror( "Ошибка создания потока FSM" );
		pthread_exit( NULL );
	}

	result = pthread_create( &interface->sender, NULL, s_replyDHCP, ( void * ) interface ); //Создание потока ответа
	if ( result != 0 )
	{
		perror( "Ошибка создания потока ответа" );
		pthread_exit( NULL );
	}

	printf( "IFACE %s ENABLED!\n", interface->name );

	return 0;
}

int disable_interface ( dserver_interface_t * interface, int idx )
{
	if ( 0 == pthread_cancel( interface->listen ) )
	{
		void *res;
		if ( 0 != pthread_join( interface->listen, &res ) ) return - 1;
		if ( res == PTHREAD_CANCELED )
			printf( "iface %s: listen thread was canceled\n", interface->name );
		else
			printf( "iface %s: listen thread wasn't canceled (shouldn't happen!)\n", interface->name );
		//Запилить бы сборщик мусорных  ниток
	}

	if ( 0 == pthread_cancel( interface->sender ) )
	{
		void *res;
		if ( 0 != pthread_join( interface->sender, &res ) ) return - 1;
		if ( res == PTHREAD_CANCELED )
			printf( "iface %s: sender thread was canceled\n", interface->name );
		else
			printf( "iface %s: sender thread wasn't canceled (shouldn't happen!)\n", interface->name );
		//Запилить бы сборщик мусорных  ниток
	}

	if ( 0 == pthread_cancel( interface->fsm_timer ) )
	{
		void *res;
		if ( 0 != pthread_join( interface->fsm_timer, &res ) ) return - 1;
		if ( res == PTHREAD_CANCELED )
			printf( "iface %s: timer thread was canceled\n", interface->name );
		else
			printf( "iface %s: timer thread wasn't canceled (shouldn't happen!)\n", interface->name );
		//Запилить бы сборщик мусорных  ниток
	}

	struct qmessage mess;
	mess.code = DISABLE;
	push_queue( interface->qtransport, 0, &mess, sizeof (mess ) );

	void * res;
	if ( 0 != pthread_join( interface->fsm, &res ) ) return - 1;
	if ( res == PTHREAD_CANCELED )
		printf( "iface %s: fsm thread was canceled\n", interface->name );
	else
		printf( "iface %s: fsm thread wasn't canceled (shouldn't happen!)\n", interface->name );
	//Запилить бы сборщик мусорных  ниток


	interface->enable = 0;
	close( interface->listen_sock );
	close( interface->send_sock );
	uninit_queues( interface->qtransport, 2 );
	uninit_queues( interface->qsessions, 1 );

	printf( "IFACE %s DISABLED!\n", interface->name );
	return 0;
}

void init_ptable ( int size ) //TODO release ptable
{
	ptable_count = size;
	ptable = realloc( ptable, size * sizeof (struct pass ) );

	ptable[0].currstate = START;
	ptable[0].in = DHCPDISCOVER;
	ptable[0].nextstate = OFFER;
	ptable[0].fun = NULL;
	ptable[1].currstate = START;
	ptable[1].in = DHCPREQUEST;
	ptable[1].nextstate = ANSWER;
	ptable[1].fun = NULL;
	ptable[2].currstate = START;
	ptable[2].in = UNKNOWN;
	ptable[2].nextstate = CLOSE;
	ptable[2].fun = NULL;

	ptable[3].currstate = OFFER;
	ptable[3].in = DHCPREQUEST;
	ptable[3].nextstate = ANSWER;
	ptable[3].fun = send_offer;
	ptable[4].currstate = OFFER;
	ptable[4].in = UNKNOWN;
	ptable[4].nextstate = CLOSE;
	ptable[4].fun = send_offer;

	ptable[5].currstate = ANSWER;
	ptable[5].in = DHCPREQUEST;
	ptable[5].nextstate = ANSWER;
	ptable[5].fun = send_answer;
	ptable[6].currstate = ANSWER;
	ptable[6].in = DHCPDECLINE;
	ptable[6].nextstate = NAK;
	ptable[6].fun = send_answer;
	ptable[7].currstate = ANSWER;
	ptable[7].in = UNKNOWN;
	ptable[7].nextstate = NAK;
	ptable[7].fun = send_answer;

	ptable[8].currstate = NAK;
	ptable[8].in = 0;
	ptable[8].nextstate = CLOSE;
	ptable[8].fun = send_nak;
}

void * sm ( void * arg )
{
	dserver_interface_t * interface = ( dserver_interface_t * ) arg;

	qmessage_t mess;
	mess.code = TIME;
	push_queue( interface->qtransport, 0, &mess, sizeof (mess ) );

	return NULL;
}

int check_password ( DSERVER * server, char * check )
{
	if ( 0 == strlen( server->password ) )
	{
		generate_salt( server->salt, sizeof (server->salt ) );
		generate_hash( check, strlen( check ), server->salt, sizeof (server->salt ), server->password, sizeof (server->password ) );
		return 0;
	}
	else
	{
		char hash[128];
		generate_hash( check, strlen( check ), server->salt, sizeof (server->salt ), hash, sizeof (hash ) );

		if ( 0 != memcmp( server->password, hash, sizeof (server->password ) ) ) return - 1;

		generate_salt( server->salt, sizeof (server->salt ) );
		generate_hash( check, strlen( check ), server->salt, sizeof (server->salt ), server->password, sizeof (server->password ) );
	}

	return 0;
}

int get_iface_idx_by_name ( char * ifname, DSERVER * server )
{
	int i;
	for ( i = 0; i < MAX_INTERFACES; i ++ )
		if ( 0 == strcmp( server->interfaces[i].name, ifname ) )
			return i;

	return - 1;
}

int del_subnet_from_interface ( dserver_interface_t * interface, char * args )
{
	dserver_subnet_t * subnet = search_subnet( interface, args );

	if ( subnet == NULL ) return - 1;
	else
	{ //TODO fucking utechka uninit queues
		printf( "delete subnet\n" );
		dserver_subnet_t * prev = subnet->prev;
		dserver_subnet_t * next = subnet->next;

		if ( subnet == interface->settings.subnets )
		{
			free( interface->settings.subnets );
			interface->settings.subnets = next;
			if ( next )
				interface->settings.subnets->prev = NULL;
		}
		else
		{
			if ( prev )
				prev->next = subnet->next;
			if ( next )
				next->prev = subnet->prev;
			free( subnet );
		}

		return 0;
	}

	return - 1;
}

dserver_subnet_t * add_subnet_to_interface ( dserver_interface_t * interface, char * args )
{ //todo refactoring
	dserver_if_settings_t * settings = & interface->settings;
	dserver_subnet_t * subnet = settings->subnets;
	dserver_subnet_t * temp = NULL;

	char * address;
	char * mask;

	if ( strlen( args ) < ( 2 * 7 ) )
	{
		printf( "low args to add subnet:\n   args: %s\n   len: %d!\n", args, strlen( args ) );
		return NULL;
	}

	address = args;
	mask = strchr( args, ' ' );
	if ( mask == NULL )
	{
		printf( "low args to add subnet, please add mask\n" );
		return NULL;
	}

	*mask = '\0';
	mask ++;

	struct sockaddr_in a_sa, m_sa;
	if ( ! inet_pton( AF_INET, address, &( a_sa.sin_addr ) ) )
	{
		printf( "can't parse address: %s\n", address );
		return NULL;
	}
	if ( ! inet_pton( AF_INET, mask, &( m_sa.sin_addr ) ) )
	{
		printf( "can't parse mask: %s\n", mask );
		return NULL;
	}

	while ( subnet != NULL )
	{
		if ( subnet->address == a_sa.sin_addr.s_addr && subnet->netmask == m_sa.sin_addr.s_addr )
		{
			printf( "Subnet %s/%s exist!\n", address, mask );
			return NULL;
		}
		temp = subnet;
		subnet = subnet->next;
		printf( "%d go next\n", temp->address );
	}

	subnet = malloc( sizeof (dserver_subnet_t ) );
	memset( subnet, 0, sizeof (*subnet ) );
	subnet->address = a_sa.sin_addr.s_addr;
	subnet->netmask = m_sa.sin_addr.s_addr;
	subnet->free_addresses = 0;
	subnet->lease_time = 0;
	subnet->pools = init_queues( 1, Q_STANDART_MODE );
	subnet->dns_servers = init_queues(1, Q_STANDART_MODE);
	subnet->routers = init_queues(1, Q_STANDART_MODE);

	if ( settings->subnets == NULL )
	{
		settings->subnets = subnet;
		subnet->prev = NULL;
		subnet->next = NULL;
	}
	else
	{
		temp->next = subnet;
		subnet->next = NULL;
		subnet->prev = temp;
	}

	printf( "subnet %s/%s added\n", address, mask );

	return subnet;
}

dserver_subnet_t * search_subnet ( dserver_interface_t * interface, char * args )
{
	dserver_if_settings_t * settings = & interface->settings;
	dserver_subnet_t * subnet = settings->subnets;

	char * address;
	char * mask;

	if ( strlen( args ) < ( 2 * 7 ) )
	{
		printf( "low args to search subnet:\n   args: %s\n   len: %d!\n", args, strlen( args ) );
		return NULL;
	}

	address = args;
	mask = strchr( args, ' ' );
	if ( mask == NULL )
	{
		printf( "low args to search subnet, please add mask\n" );
		return NULL;
	}

	*mask = '\0';
	mask ++;

	char * ptr = strchr( mask, ' ' );
	if ( ptr )
	{
		*ptr = '\0';
		ptr ++;
	}

	struct sockaddr_in a_sa, m_sa;
	if ( ! inet_pton( AF_INET, address, &( a_sa.sin_addr ) ) )
	{
		printf( "can't parse address: %s\n", address );
		return NULL;
	}
	if ( ! inet_pton( AF_INET, mask, &( m_sa.sin_addr ) ) )
	{
		printf( "can't parse mask: %s\n", mask );
		return NULL;
	}

	while ( subnet != NULL )
	{
		if ( subnet->address == a_sa.sin_addr.s_addr && subnet->netmask == m_sa.sin_addr.s_addr )
		{
			printf( "Subnet %s/%s found!\n", address, mask );
			if ( ptr )
			{
				int i;
				int len = strlen( ptr );
				for ( i = 0; i < DCTP_ARG_MAX_LEN && i < len; i ++ )
					args[i] = ptr[i];
				args[i] = '\0';
			}
			return subnet;
		}
		printf( "subnet---->>" );
		printip( subnet->address );
		subnet = subnet->next;
	}

	printf( "Subnet %s/%s not found!\n", address, mask );
	return NULL;
}

int add_range_to_subnet ( dserver_subnet_t * subnet, char * range )
{
	ip_address_range_t a_range;

	if ( range == NULL ) return - 1;

	if ( - 1 == ip_address_range_parse( range, &a_range ) )
	{
		printf( "wrong address-range\n" );
		return - 1;
	}

	if ( ( a_range.start_address & subnet->netmask ) != subnet->address ||
	 ( a_range.end_address & subnet->netmask ) != subnet->address ||
	 a_range.start_address == subnet->address ||
	 a_range.end_address == subnet->address )
	{
		printf( "address-range overlap subnet\n" );
		return - 1;
	}

	Q_FOREACH(ip_address_range_t *, entry, subnet->pools,
		if ( ip_address_range_is_overlap( entry, &a_range ) )
		{
			printf( "range %s exist!\n", range );
			return - 1;
		}
	)

	push_queue( subnet->pools, 0, &a_range, sizeof (a_range ) );

	subnet->free_addresses += htonl( a_range.end_address ) - htonl( a_range.start_address ) + 1; //  под рефакторинг
	printf( "pool %s added\n", range );

	return 0;
}

int del_range_from_subnet ( dserver_subnet_t * subnet, char * range )
{
	ip_address_range_t a_range;

	if ( range == NULL ) return - 1;

	if ( - 1 == ip_address_range_parse( range, &a_range ) )
	{
		printf( "wrong address-range\n" );
		return - 1;
	}

	qelement_t * iter;
	for ( iter = subnet->pools->head; iter != NULL; iter = iter->next )
	{
		if ( ip_address_range_is_overlap( ( ip_address_range_t * ) iter->data, &a_range ) )
		{
			printf( "delete pool %s\n", range );
			subnet->free_addresses -= htonl( a_range.end_address ) - htonl( a_range.start_address ) + 1;
			delete_ptr( subnet->pools, iter );
			return 0;
		}
	}

	printf( "pool %s is not exist\n", range );
	return - 1;
}

int add_dns_to_subnet ( dserver_subnet_t * subnet, char * address )
{
	if ( address == NULL ) return - 1;

	u_int32_t ip = inet_addr( address );
	if ( ip == - 1 ) return - 1;

	Q_FOREACH(int *, entry, subnet->dns_servers,
		if ( ip == *entry )
		{
			printf( "dns-server is exist\n" );
			return -1;
		}
	)

	push_queue(subnet->dns_servers, 0, &ip, sizeof(ip));
	printf( "dns-server %s added\n", address );

	return 0;
}

int del_dns_from_subnet ( dserver_subnet_t * subnet, char * address )
{
	if ( address == NULL ) return - 1;

	u_int32_t ip = inet_addr( address ); //TODO parsing
	if ( ip == - 1 ) return - 1;

	Q_FOREACH(int *, entry, subnet->dns_servers, 
		if ( *entry == ip )
		{
			printf( "delete dns-server %s\n", address );
			delete_ptr( subnet->dns_servers, Q_ITER );
			return 0;
		}
	)

	printf( "dns-server %s is not exist\n", address );

	return - 1;
}

int add_router_to_subnet ( dserver_subnet_t * subnet, char * address )
{
	if ( address == NULL ) return - 1;

	u_int32_t ip = inet_addr( address );
	if ( ip == - 1 ) return - 1;

	if ( ( ip & subnet->netmask ) != subnet->address )
	{
		printf( "router is not in subnet!\n" );
		return - 1;
	}

	Q_FOREACH(int *, entry, subnet->routers,
		if ( ip == *entry )
		{
			printf( "router is exist\n" );
			return -1;
		}
	)

	push_queue(subnet->routers, 0, &ip, sizeof(ip));
	printf( "router %s added\n", address );

	return 0;
}

int del_router_from_subnet ( dserver_subnet_t * subnet, char * address )
{
	if ( address == NULL ) return - 1;

	u_int32_t ip = inet_addr( address ); //TODO parsing
	if ( ip == - 1 ) return - 1;

	Q_FOREACH(int *, entry, subnet->routers,
		if ( ip == *entry )
		{
			printf( "del router-server %s\n", address );
			delete_ptr(subnet->routers, Q_ITER);
			return 0;
		}
	)

	printf( "router-server %s is not exist\n", address );

	return - 1;
}

int set_domain_name_on_subnet ( dserver_subnet_t * subnet, char * args )
{
	if ( args == NULL ) return - 1;
	if ( strlen( args ) >= sizeof (subnet->domain_name ) ) return - 1;

	strncpy( subnet->domain_name, args, sizeof (subnet->domain_name ) );
	printf( "set domain name: %s\n", args );

	return 0;
}

int set_host_name_on_subnet ( dserver_subnet_t * subnet, char * args )
{
	if ( args == NULL ) return - 1;
	if ( strlen( args ) >= sizeof (subnet->host_name ) ) return - 1;

	strncpy( subnet->host_name, args, sizeof (subnet->host_name ) );
	printf( "set host name: %s\n", args );

	return 0;
}

long get_one_num ( char * args )
{
	if ( args == NULL || strlen( args ) == 0 ) return - 1;

	char * endptr;
	long ret;

	errno = 0;
	ret = strtol( args, &endptr, 10 );

	if ( ( errno == ERANGE && ( ret == LONG_MAX || ret == LONG_MIN ) ) || ( errno != 0 && ret == 0 ) )
	{
		perror( "strtol" );
		return - 1;
	}

	if ( endptr == args )
	{
		printf( "<%s> no digits were found in input str\n", __FUNCTION__ );
		return - 1;
	}

	return ret;
}

int execute_DCTP_command ( DCTP_COMMAND * in, DSERVER * server )
{
	printf( "<%s>\n", __FUNCTION__ );
	char ifname[IFNAMELEN];

	DCTP_cmd_code_t code = parse_DCTP_command( in, ifname );
	if ( code == UNDEF_COMMAND ) return - 1;
	int idx = - 1;

	printf( "%s: %s\n", in->name, in->arg );

	switch ( code )
	{
		case SR_SET_IFACE_ENABLE:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			if ( server->interfaces[idx].enable == 1 ) return - 1;
			if ( - 1 == enable_interface( &server->interfaces[idx], idx ) ) return - 1;
			break;

		case SR_SET_IFACE_DISABLE:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			if ( server->interfaces[idx].enable == 0 ) return - 1;
			if ( - 1 == disable_interface( &server->interfaces[idx], idx ) ) return - 1;
			break;

		case SR_SET_LEASETIME:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				long ltime;
				if ( strstr( in->arg, "global" ) )
				{
					ltime = get_one_num( in->arg + strlen( "global" ) + 1 );
					if ( ltime == - 1 ) return - 1;
					server->interfaces[idx].settings.global.default_lease_time = ltime;
				}
				else
				{
					dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
					if ( NULL == sub ) return - 1;
					ltime = get_one_num( in->arg );
					if ( ltime == - 1 ) return - 1;
					sub->lease_time = ltime;
				}
			}
			break;

		case SR_SET_HOST_NAME:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == set_host_name_on_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_SET_DOMAIN_NAME:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == set_domain_name_on_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_ADD_SUBNET:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			if ( NULL == add_subnet_to_interface( &server->interfaces[idx], in->arg ) ) return - 1;
			break;

		case SR_DEL_SUBNET:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			if ( - 1 == del_subnet_from_interface( &server->interfaces[idx], in->arg ) ) return - 1;
			break;

		case SR_ADD_POOL:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == add_range_to_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_DEL_POOL:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == del_range_from_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_ADD_DNS:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == add_dns_to_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_DEL_DNS:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == del_dns_from_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_ADD_ROUTER:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == add_router_to_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case SR_DEL_ROUTER:
			idx = get_iface_idx_by_name( ifname, server );
			if ( idx == - 1 ) return - 1;
			else
			{
				dserver_subnet_t * sub = search_subnet( &server->interfaces[idx], in->arg );
				if ( NULL == sub ) return - 1;
				if ( - 1 == del_router_from_subnet( sub, in->arg ) ) return - 1;
			}
			break;

		case DCTP_PING:
			return 0;

		case DCTP_PASSWORD:
			if ( - 1 == check_password( server, in->arg ) ) return - 1;
			break;

		case DCTP_SAVE_CONFIG:
			if ( - 1 == save_config( server, S_CONFIG_FILE ) ) return - 1;
			break;

		case DCTP_END_WORK:
			return 1;

		default:
			printf( "Unknown command!\n" );
			return - 1;
	}

	return 0;
}

void * manipulate ( void * server )
{
	DSERVER * caller = ( DSERVER * ) server;
	int sock = init_DCTP_socket( DSR_DCTP_PORT ); //TODO исправить а пока пускай висит на дефолтном

	while ( 1 )
	{
		DCTP_COMMAND_PACKET pack;
		memset( &pack, 0, sizeof (pack ) );
		struct sockaddr_in sender;

		receive_DCTP_command( sock, &pack, &sender ); //успевает ли отработать?
		int exec_status = execute_DCTP_command( &pack.payload, caller );
		if ( exec_status >= 0 )
			send_DCTP_REPLY( sock, &pack, DCTP_SUCCESS, &sender );
		else
			send_DCTP_REPLY( sock, &pack, DCTP_FAIL, &sender );

		if ( exec_status == 1 )
		{
			int i;
			save_config( server, S_CONFIG_FILE );
			for ( i = 0; i < MAX_INTERFACES; i ++ )
			{
				dserver_interface_t * interface = & caller->interfaces[i];
				if ( strlen( interface->name ) == 0 ) continue;
				if ( interface->enable == 1 ) disable_interface( &caller->interfaces[i], i );

				//free interfaces
			}
			pthread_exit( NULL );
		}
	}

	release_DCTP_socket( sock );
}

int main ( )
{//Добавить проверку чтобы нельзя было  открыть еще один экземпляр
	pthread_t manipulate_tid;
	DSERVER server;
	memset( &server, 0, sizeof (server ) );

	srand( time( NULL ) );
	init_ptable( PTABLE_COUNT );

	//LOADING CONFIGURATION
	DSERVER * temp_config = ( DSERVER * ) malloc( sizeof (*temp_config ) );
	memset( temp_config, 0, sizeof (*temp_config ) );

	if ( - 1 != load_config( temp_config, S_CONFIG_FILE ) )
	{
		init_interfaces( temp_config );
		memcpy( &server, temp_config, sizeof (server ) );
	}
	else
		init_interfaces( &server );

	free( temp_config );
	save_config( &server, S_CONFIG_FILE );

	int i;
	for ( i = 0; i < MAX_INTERFACES; i ++ )
	{
		dserver_interface_t * interface = & server.interfaces[i];
		if ( strlen( interface->name ) == 0 ) continue;
		if ( interface->enable == 0 ) continue;
		interface->cci = 5;
		if ( - 1 == enable_interface( interface, i ) ) return - 1;
	}

	//END LOADING CONFIGURATION

	pthread_create( &manipulate_tid, NULL, manipulate, ( void * ) &server );

	pthread_join( manipulate_tid, NULL );

	return 0;
}
