#include "libdhcp/dhcp.h"
#include "libdhcp/dleases.h"
#include "libdhcp/dhioctl.h"
#include "core.h"

#include <stdlib.h>
#include <string.h>

//Подсчет контрольной суммы для IP и UDP заголовков

u_int32_t checksum ( buf, nbytes, sum )
unsigned char *buf;
unsigned nbytes;
u_int32_t sum;
{
	unsigned i;

	for ( i = 0; i < ( nbytes & ~1U ); i += 2 )
	{
		sum += ( u_int16_t ) ntohs( *( ( u_int16_t * ) ( buf + i ) ) );
		if ( sum > 0xFFFF )
			sum -= 0xFFFF;
	}


	if ( i < nbytes )
	{
		sum += buf [i] << 8;
		if ( sum > 0xFFFF )
			sum -= 0xFFFF;
	}

	return sum;
}
//Инверсия контрольной суммы

u_int32_t wrapsum ( sum )
u_int32_t sum;
{
	sum = ~sum & 0xFFFF;
	return htons( sum );
}

int create_server_options ( unsigned char * options, dserver_subnet_t * subnet, dserver_interface_t * interface, long * ltime )
{
	int cnt = 0;

	//Установка адреса сервера
	options[cnt++] = 54;
	options[cnt++] = 4;
	u_int32_t address = get_iface_ip( interface->name );
	memcpy( options + cnt, &address, sizeof (address ) );
	cnt += 4;

	//Установка времени аренды адреса
	options[cnt++] = 51;
	options[cnt++] = 4;
	if ( subnet->lease_time != 0 )
		*ltime = htonl( subnet->lease_time );
	else
	{
		if ( interface->settings.global.default_lease_time != 0 )
			*ltime = htonl( interface->settings.global.default_lease_time );
		else
		{
			*ltime = htonl( 60 );

		}
	}
	memcpy( options + cnt, ltime, sizeof (long ) );
	cnt += 4;

	//option 1, subnet netmask, must have
	options[cnt++] = 1;
	options[cnt++] = 4;
	memcpy( options + cnt, &subnet->netmask, sizeof (subnet->netmask ) );
	cnt += 4;

	//option 3, routers, must have 
	if ( subnet->routers->elements > 0 )
	{
		options[cnt++] = 3;
		options[cnt++] = subnet->routers->elements * sizeof(int);

		Q_FOREACH(int *, router, subnet->routers,
			memcpy( options + cnt, router, sizeof (*router) );
			cnt += sizeof (*router);
		)
	}

	// option 6, dns-servers
	if ( subnet->dns_servers->elements > 0 )
	{
		options[cnt++] = 6;
		options[cnt++] = subnet->dns_servers->elements * sizeof(u_int32_t);

		Q_FOREACH(int *, dns, subnet->dns_servers,
			memcpy( options + cnt, dns, sizeof (*dns) );
			cnt += sizeof (*dns);
		)
	}

	// option 12, host_name
	if ( strlen( subnet->host_name ) != 0 )
	{
		options[cnt++] = 12;
		options[cnt++] = strlen( subnet->host_name );
		memcpy( options + cnt, subnet->host_name, strlen( subnet->host_name ) );
		cnt += strlen( subnet->host_name );
	}

	// option 15, domain_name
	if ( strlen( subnet->domain_name ) != 0 )
	{
		options[cnt++] = 15;
		options[cnt++] = strlen( subnet->domain_name );
		memcpy( options + cnt, subnet->domain_name, strlen( subnet->domain_name ) );
		cnt += strlen( subnet->domain_name );
	}

	return cnt;
}

int create_offer ( void * iface, unsigned char * options, u_int32_t * y_addr, long * ltime )
{
	add_log( __FUNCTION__ );
	int cnt = 0;
	dserver_interface_t * interface = ( dserver_interface_t * ) iface;
	dserver_if_settings_t * settings = &interface->settings;
	dserver_subnet_t * f_subnet = NULL;

	Q_FOREACH(dserver_subnet_t *, subnet, settings->subnets,
		if ( subnet->free_addresses != 0 ) 
		{
			f_subnet = subnet;
			break;
		}
		printf( "no free addresses in subnet\n" );
	)

	if ( f_subnet == NULL )
	{
		printf( "No subnet to lease!\n" );
		return -1;
	}

	Q_FOREACH(ip_address_range_t *, entry, f_subnet->pools,
		int ip = try_give_ip( entry );
		if ( -1 != ip )
		{
			*y_addr = ip;
			printip( ip );
			break;
		}
	)

	cnt = create_server_options( options, f_subnet, interface, ltime );
	if ( -1 == cnt ) return -1;
	return (cnt + 7 );
}

int create_ack ( void * iface, unsigned char * options, u_int32_t * y_addr, long * ltime )
{
	add_log( __FUNCTION__ );
	int cnt = 0;
	dserver_interface_t * interface = ( dserver_interface_t * ) iface;
	dserver_if_settings_t * settings = &interface->settings;
	dserver_subnet_t * f_subnet = NULL;

	int found = 0;
	Q_FOREACH(dserver_subnet_t *, subnet, settings->subnets, 
		Q_FOREACH(ip_address_range_t *, entry, subnet->pools, 
			if ( ip_address_range_have_address( entry, y_addr ) )
			{
				found = 1;
				break;
			}
		)
		if ( found ) 
		{	
			f_subnet = subnet;
			break;
		}
	)

	if ( !found )
	{
		printf( "Not found requested address ->>" );
		printip( *y_addr );
		return -1;
	}

	cnt = create_server_options( options, f_subnet, interface, ltime );
	if ( -1 == cnt ) return -1;
	return (cnt + 7 );
}

// Create DHCP packet

u_int32_t create_packet ( char * iface, frame_t * frame, int btype, int dtype, void * arg )
{
	long ltime = 0;
	add_log( "Creating DHCP packet..." );

	frame->p_dhc.op = btype;
	frame->p_dhc.htype = 1;
	frame->p_dhc.hlen = 6;
	frame->p_dhc.hops = 0;
	if ( frame->p_dhc.xid == 0 )
	{
		frame->p_dhc.xid = rand( );
	}
	frame->p_dhc.secs = 0;
	frame->p_dhc.flags = 0x0000;

	frame->p_dhc.ciaddr.s_addr = 0;
	if ( dtype != DHCPACK ) frame->p_dhc.yiaddr.s_addr = 0;
	if ( dtype != DHCPACK ) frame->p_dhc.siaddr.s_addr = 0;
	frame->p_dhc.giaddr.s_addr = 0;

	if ( btype == BOOTP_REQUEST ) set_my_mac( iface, frame->p_dhc.chaddr );

	memset( frame->p_dhc.options, 0, sizeof (frame->p_dhc.options ) );
	//Установка магических чисел					
	frame->p_dhc.options[0] = 99;
	frame->p_dhc.options[1] = 130;
	frame->p_dhc.options[2] = 83;
	frame->p_dhc.options[3] = 99;

	//Установка типа сообщения (опция 53) 	
	frame->p_dhc.options[4] = 53;
	frame->p_dhc.options[5] = 1;
	frame->p_dhc.options[6] = dtype;

	int cnt = 7;

	if ( dtype == DHCPOFFER )
	{
		cnt = create_offer( arg, frame->p_dhc.options + cnt, &frame->p_dhc.yiaddr.s_addr, &ltime );
		if ( cnt == -1 )
		{
			add_log( "create offer fail" );
			return -1;
		}
	}

	if ( dtype == DHCPACK )
	{
		//Установка возвращаемого адреса		
		cnt = create_ack( arg, frame->p_dhc.options + cnt, &frame->p_dhc.yiaddr.s_addr, &ltime );
		if ( cnt == -1 )
		{
			add_log( "create ack fail" );
			return -1;
		}
	}

	if ( dtype == DHCPNAK )
	{
		dserver_interface_t * interface = ( dserver_interface_t * ) arg;
		//Установка адреса сервера
		frame->p_dhc.options[cnt++] = 54;
		frame->p_dhc.options[cnt++] = 4;
		u_int32_t address = get_iface_ip( interface->name );
		memcpy( frame->p_dhc.options + cnt, &address, sizeof (address ) );
		cnt += 4;
	}

	if ( dtype == DHCPREQUEST )
	{
		dclient_interface_t * interface = ( dclient_interface_t * ) arg;
		//Установка адреса сервера
		frame->p_dhc.options[cnt++] = 54;
		frame->p_dhc.options[cnt++] = 4;
		if ( get_lease( interface->name, NULL, frame->p_dhc.options + cnt ) == -1 ) return -1;
		cnt += 4;

		//Установка запрашиваемого адреса
		frame->p_dhc.options[cnt++] = 50;
		frame->p_dhc.options[cnt++] = 4;
		if ( get_lease( interface->name, frame->p_dhc.options + cnt, NULL ) == -1 ) return -1;
		cnt += 4;

		if ( get_lease( interface->name, NULL, ( unsigned char * ) &frame->p_dhc.siaddr.s_addr ) == -1 ) return -1;
	}

	if ( dtype == DHCPDISCOVER )
	{
		//TODO
	}
	
	if ( dtype == DHCPINFORM)
	{
		frame->p_dhc.ciaddr.s_addr = get_iface_ip(iface);
	}

	frame->p_dhc.options[cnt++] = 255; //options end

	frame->d_size = DHCP_FIXED_NON_UDP + ( cnt * sizeof (unsigned char ) ); //dhcp pack len
	frame->size += frame->d_size;

	add_log( "Succesful created DHCP packet" );

	if ( dtype == DHCPACK || dtype == DHCPOFFER ) return ntohl( ltime );
	return frame->p_dhc.xid; //Возврат идентификатора сессии
}

//Create IP header

void create_ipheader ( frame_t * frame, int srcip, int destip )
{
	add_log( "Creating IP header..." );

	frame->h_ip.ip_hl = 5;
	frame->h_ip.ip_v = 4;
	frame->h_ip.ip_tos = IPTOS_LOWDELAY;
	frame->h_ip.ip_len = htons( sizeof (struct ip ) + sizeof (struct udphdr ) +frame->d_size );
	frame->h_ip.ip_id = 0;
	frame->h_ip.ip_off = 0;
	frame->h_ip.ip_ttl = 128; // hops
	frame->h_ip.ip_p = IPPROTO_UDP; // UDP

	// Source IP address, can use spoofed address here!!!
	frame->h_ip.ip_src.s_addr = srcip;
	// The destination IP address
	frame->h_ip.ip_dst.s_addr = destip;

	// Calculate the checksum for integrity
	frame->h_ip.ip_sum = wrapsum( checksum( ( unsigned short * ) &frame->h_ip, sizeof (struct ip ), 0 ) );

	frame->size += sizeof (struct ip );

	add_log( "Succesful created IP header" );
};

//Create UDP header

void create_udpheader ( frame_t * frame, int srcport, int destport )
{
	add_log( "Creating UDP header..." );

	frame->h_udp.source = htons( srcport );
	frame->h_udp.dest = htons( destport );
	frame->h_udp.len = htons( sizeof (struct udphdr ) +frame->d_size );
	memset( &frame->h_udp.check, 0, sizeof (frame->h_udp.check ) );

	frame->h_udp.check = wrapsum( checksum( ( unsigned char* ) &frame->h_udp, sizeof (struct udphdr ),
		checksum( ( unsigned char* ) &frame->p_dhc, frame->d_size,
		checksum( ( unsigned char* ) &frame->h_ip.ip_src, 8,
		IPPROTO_UDP + ( u_int32_t ) ntohs( frame->h_udp.len ) ) ) )
		);

	frame->size += sizeof (struct udphdr );
	add_log( "Succesful created UDP header" );
}

//Create ETH header

void create_ethheader ( frame_t * frame, unsigned char * macs, unsigned char * macd, u_int16_t proto )
{
	add_log( "Creating ETHERNET header..." );

	memcpy( frame->h_eth.dmac, macd, ETH_ALEN ); //Установка адреса назначения
	memcpy( frame->h_eth.smac, macs, ETH_ALEN ); //Установка адреса отправителя
	frame->h_eth.type = htons( proto ); //Установка типа протокола

	frame->size += sizeof (struct ethhdr ) + 2; //2 is fucking padding, because eth struct is packed
	add_log( "Succesful created ETHERNET header" );
}

void create_arp ( char * iface, char * buffer, int ip, unsigned char * macs, unsigned char * macd, int oper )
{

	struct arp_packet * arp = ( struct arp_packet * ) ( buffer + sizeof (struct ethheader ) );
	add_log( "Creating ARP header.." );

	arp->hardware = htons( ARPHRD_ETHER );
	arp->arp_protocol = htons( ETH_P_IP );
	arp->arp_hard_addr_len = ETH_ALEN;
	arp->arp_prot_addr_len = 4; /* размер ip адреса в байтах*/
	arp->arp_operation = htons( ARPOP_REQUEST );
	memcpy( arp->arp_mac_source, macs, ETH_ALEN );
	memcpy( arp->arp_mac_target, macd, ETH_ALEN );
	arp->arp_ip_target = ip;
	arp->arp_ip_source = get_iface_ip( iface );

	int offset = sizeof (struct ethheader ) + sizeof (struct arp_packet );
	memset( ( buffer + offset ), 0, 60 - offset );

	add_log( "Succesful created ARP header" );
}


