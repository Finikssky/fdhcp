#include "dhcp.h"
#include "core.h"

//Подсчет контрольной суммы для IP и UDP заголовков
u_int32_t checksum (buf, nbytes, sum)
	unsigned char *buf;
	unsigned nbytes;
	u_int32_t sum;
{
	unsigned i;

	for (i = 0; i < (nbytes & ~1U); i += 2) 
	{
		sum += (u_int16_t) ntohs(*((u_int16_t *)(buf + i)));
		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}	

	
	if (i < nbytes) 
	{
		sum += buf [i] << 8;
		if (sum > 0xFFFF)
			sum -= 0xFFFF;
	}
	
	return sum;
}
//Инверсия контрольной суммы
u_int32_t wrapsum (sum)
	u_int32_t sum;
{
	sum = ~sum & 0xFFFF;
	return htons(sum);
}

int create_server_options(char * options, dserver_subnet_t * subnet, dserver_interface_t * interface)
{
	int cnt = 0;
	//option 1, subnet netmask, must have
	options[cnt++] = 1;
	options[cnt++] = 4;
	memcpy(options + cnt, &subnet->netmask, sizeof(subnet->netmask));
	cnt += 4;
	
	//option 3, routers, must have 
	options[cnt++] = 3; //TODO может быть несколько роутеров
	options[cnt++] = 4;
	memcpy(options + cnt, &subnet->routers, sizeof(subnet->routers));
	cnt += 4;
	
	// option 5, dns-servers
	dserver_dns_t * dns = subnet->dns_servers;
	if (dns != NULL)
	{
		int dns_len = 0;
		int dns_idx;
		
		options[cnt++] = 5; 
		dns_idx = cnt;
		cnt++;
		
		while (dns != NULL)
		{
			memcpy(options + cnt, &dns->address, sizeof(dns->address));
			cnt += 4;
			dns_len += 4;
			dns = dns->next;
		}
		
		options[dns_idx] = dns_len;
	}

	//Установка адреса сервера
	options[cnt++] = 54;
	options[cnt++] = 4;
	u_int32_t address = get_iface_ip(interface->name); 
	memcpy(options + cnt, &address, sizeof(address));
	cnt += 4;
	
	//Установка времени аренды адреса
	options[cnt++] = 51;
	options[cnt++] = 4;
	memcpy(options + cnt, &subnet->lease_time, 4);
	cnt += 4;
	
	return cnt;
}

int create_offer( void * iface, char * options, u_int32_t * y_addr)
{	
	add_log(__FUNCTION__);
	int cnt = 0;
	dserver_interface_t * interface = (dserver_interface_t *)iface;
	dserver_if_settings_t * settings = &interface->settings;
	dserver_subnet_t * subnet = settings->subnets;
	
	while(subnet != NULL)
	{
		if (subnet->free_addresses != 0) break;
		subnet = subnet->next;
	}
	
	if (subnet == NULL) 
	{	
		printf("No subnet to lease!\n");
		return -1;
	}
	
	dserver_pool_t * pool = subnet->pools;
	while (pool != NULL)
	{
		int ip = try_give_ip(&pool->range);
		if (-1 != ip) 
		{
			*y_addr = ip;
			printip(ip);
			break;
		}
		pool = pool->next;
	}
	
	cnt = create_server_options(options, subnet, interface);
	if ( -1 == cnt ) return -1;
	return (cnt + 7);
}

int create_ack( void * iface, char * options, u_int32_t * y_addr)
{	
	add_log(__FUNCTION__);
	int cnt = 0;
	dserver_interface_t * interface = (dserver_interface_t *)iface;
	dserver_if_settings_t * settings = &interface->settings;
	dserver_subnet_t * subnet = settings->subnets;
	
	int found = 0;
	while(subnet != NULL)
	{
		dserver_pool_t * pool = subnet->pools;
		while (pool != NULL)
		{
			if (ip_address_range_have_address(&pool->range, y_addr)) 
			{
				found = 1;
				break;
			}
			pool = pool->next;
		}
		if (found) break;
		subnet = subnet->next;
	}
	
	if (!found)
	{
		printf("Not found requested address!\n");
		return -1;
	}
	
	if (subnet == NULL) 
	{	
		printf("No subnet to lease!\n");
		return -1;
	}
		
	cnt = create_server_options(options, subnet, interface);
	if ( -1 == cnt ) return -1;
	return (cnt + 7);
}

// Create DHCP packet
u_int32_t create_packet(char * iface, char * buffer, int btype, int dtype, void * arg)
{
	struct dhcp_packet * cldhcp = (struct dhcp_packet *)(buffer + FULLHEAD_LEN);
	add_log("Creating DHCP packet...");

	cldhcp->op    = btype;
	cldhcp->htype = 1;
	cldhcp->hlen  = 6;
	cldhcp->hops  = 0;
	if (cldhcp->xid == 0) 
	{ 
		 cldhcp->xid = rand();
		 #ifdef XIDSTEP
		 cldhcp->xid = htonl(LASTRANDOM++);		
		 #endif
		 LASTRANDOM=LASTRANDOM%1000000;
	 }
    cldhcp->secs  = 0;
    cldhcp->flags = 0x0000;
	
    if (dtype != DHCPACK) cldhcp->ciaddr.s_addr = 0;	
    cldhcp->yiaddr.s_addr = 0;
    cldhcp->siaddr.s_addr = 0;
    cldhcp->giaddr.s_addr = 0;	
	
	if (cldhcp->chaddr[0] == 0 &&
		cldhcp->chaddr[1] == 0 &&
		cldhcp->chaddr[2] == 0 &&
		cldhcp->chaddr[3] == 0) set_my_mac(iface, cldhcp->chaddr);   

    //Установка магических чисел					
	cldhcp->options[0] = 99; 
	cldhcp->options[1] = 130;
	cldhcp->options[2] = 83; 
	cldhcp->options[3] = 99;
   
    //Установка типа сообщения (опция 53) 	
    cldhcp->options[4] = 53; 
	cldhcp->options[5] = 1;
    cldhcp->options[6] = dtype; 

	int cnt = 7;
			
	if	(dtype == DHCPOFFER)
	{
		cnt = create_offer(arg, cldhcp->options + cnt, &cldhcp->yiaddr.s_addr);
		if ( cnt == -1 ) 
		{	
			add_log("create offer fail");
			return -1;
		}
	}
   
    if (dtype == DHCPACK)
	{
		//Установка возвращаемого адреса
		cldhcp->yiaddr.s_addr = get_rip_from_pack(cldhcp);
		
		cnt = create_ack(arg, cldhcp->options + cnt, &cldhcp->yiaddr.s_addr);
		if ( cnt == -1 ) 
		{	
			add_log("create ack fail");
			return -1;
		}
	}	
	//TODO переделать NAK
    if (dtype == DHCPNAK)
	{
	//Установка адреса сервера
		cldhcp->options[cnt++]=54;
		cldhcp->options[cnt++]=4;
		get_my_ip(cldhcp->options+cnt);
		cnt+=4;
    }	

    if (dtype==DHCPREQUEST){
	//Установка адреса сервера
		cldhcp->options[cnt++]=54;
		cldhcp->options[cnt++]=4;
		if (get_lease(NULL, cldhcp->options + cnt) == -1) return -1;
		cnt+=4;
	
	 //Установка запрашиваемого адреса
   	 cldhcp->options[cnt++]=50;
   	 cldhcp->options[cnt++]=4;
	 if (get_lease(cldhcp->options+cnt,NULL)==-1) return -1;
	 cnt+=4;
	
	if (get_lease(NULL,&cldhcp->siaddr.s_addr)==-1) return -1;
    }


	cldhcp->options[cnt++] = 255; //Конец опций
	memset(cldhcp->options + cnt, 0, sizeof(cldhcp->options) - cnt); //Очистка оставшегося поля пакета

	add_log("Succesful created DHCP packet");
	return cldhcp->xid; //Возврат идентификатора сессии
}

//Create IP header
void create_ipheader(char * buffer, int srcip, int destip ) 
{
	struct ip *ip = (struct ip*) (buffer + sizeof(struct ethheader));

	add_log("Creating IP header...");

    memset(ip, 0, sizeof(struct ip));
    ip->ip_hl  = 5;
    ip->ip_v   = 4;
    ip->ip_tos = IPTOS_LOWDELAY; 
    ip->ip_len = htons(sizeof(struct ip) + sizeof(struct udpheader)+sizeof(struct dhcp_packet));
    ip->ip_id  = 0;
    ip->ip_off = 0;
    ip->ip_ttl = 128; // hops
    ip->ip_p   = IPPROTO_UDP; // UDP

    // Source IP address, can use spoofed address here!!!
	ip->ip_src.s_addr = srcip;
    // The destination IP address
	ip->ip_dst.s_addr = destip;

    // Calculate the checksum for integrity
	ip->ip_sum = wrapsum(checksum((unsigned short *)ip, sizeof(struct ip),0));

	add_log("Succesful created IP header");	
};

//Create UDP header

void create_udpheader(char * buffer, int srcport, int destport)
{
	struct udphdr * udp = (struct udphdr *) (buffer + sizeof(struct ethheader) + sizeof(struct ip));
	struct ip * ip      = (struct ip*) (buffer + sizeof(struct ethheader));
	struct dhcp_packet *dhc = (struct dhcp_packet *)(buffer + FULLHEAD_LEN);

	add_log("Creating UDP header...");

	memset(udp,0,sizeof(udp));

	udp->source = htons(srcport);
	udp->dest   = htons(destport);
	udp->len    = htons(sizeof(struct udphdr) + sizeof(struct dhcp_packet));
	memset(&udp->check, 0, sizeof(udp->check));

	udp->check = wrapsum( checksum ((unsigned char*) udp, sizeof(struct udphdr),
						  checksum ((unsigned char*) dhc, sizeof(struct dhcp_packet),
						  checksum ((unsigned char*) &ip->ip_src, 8,
						  IPPROTO_UDP + (u_int32_t)ntohs(udp->len))))
				);

	add_log("Succesful created UDP header");
}

//Create ETH header
void create_ethheader(void * buffer, unsigned char * macs, unsigned char * macd, u_int16_t proto)
{
	struct ethheader * eth = (struct ethheader*)buffer;
	add_log("Creating ETHERNET header...");

	memcpy(eth->dmac, macd, ETH_ALEN);         //Установка адреса назначения
	memcpy(eth->smac, macs, ETH_ALEN);         //Установка адреса отправителя
	eth->type = htons(proto);                  //Установка типа протокола

	add_log("Succesful created ETHERNET header");
}

void create_arp(char * iface, char *buffer, int ip, char *macs, char *macd, int oper)
{
	struct arp_packet * arp = (struct arp_packet*)(buffer + sizeof(struct ethheader));
	add_log("Creating ARP header..");

	arp->hardware = htons( ARPHRD_ETHER );
	arp->arp_protocol = htons( ETH_P_IP );
	arp->arp_hard_addr_len = ETH_ALEN;
	arp->arp_prot_addr_len = 4; /* размер ip адреса в байтах*/
	arp->arp_operation = htons( ARPOP_REQUEST );
	memcpy( arp->arp_mac_source, macs, ETH_ALEN );
	memcpy( arp->arp_mac_target, macd, ETH_ALEN ); 
	arp->arp_ip_target = ip;
	arp->arp_ip_source = get_iface_ip(iface);

	add_log("Succesful created ARP header");
}


