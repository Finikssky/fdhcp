#include "dhconn.h"
#include "dleases.h"

//init packet socket on interface ethName
int init_packet_sock(char *ethName, u_int16_t protocol)
{
	int sock; 
	//Открываем сырой пакетный сокет канального уровня
	sock = socket( PF_PACKET, SOCK_RAW, htons(protocol) );
	if (sock < 0)
	{
		perror("<init_packet_sock> socket");
		return -1;
	}

	struct ifreq ifMac;
	//Получем индекс сетевого интерфейса
	memset(&ifMac, 0, sizeof (ifMac));
	strncpy(ifMac.ifr_name, ethName, strlen(ethName));
	ioctl (sock, SIOCGIFINDEX, &ifMac);

	struct sockaddr_ll sAddr;
	//Привязываем сокет к сетевому интерфейсу
	memset (&sAddr, 0, sizeof (sAddr));
	sAddr.sll_family = PF_PACKET;
	sAddr.sll_ifindex = ifMac.ifr_ifindex;
	sAddr.sll_protocol = htons(protocol);
	if (0 > bind (sock, (struct sockaddr *) &sAddr, sizeof (struct sockaddr_ll)))
	{
		perror("<init_packet_sock> bind");
	}

	return sock;
}

//Функция посылки ARP-пакета
int sendARP(int sock, char * iface, u_int32_t ip)
{
	char buf[120];
	char macs[6];
	char macd[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};	

	set_my_mac(iface, macs);
	
	create_ethheader((frame_t *)buf, macs, macd, ETH_P_ARP);
	memset(macd, 0, ETH_ALEN);
	create_arp(iface, buf, ip, macs, macd, ARPOP_REQUEST);
	
	int size = sizeof(struct ethheader) + sizeof(struct arp_packet) + 18;
	if ( write(sock, buf, size) == -1 )  
	{
		perror("send ARP Request");
		return -1;
	}

	return 0;
}

//Функция ожидания ARP-пакета
int recvARP(int sock, char * iface, u_int32_t ip)
{
	int bytes;
	char buf[120];
	struct ethheader  * eth;
	struct arp_packet * arp;
	
	unsigned char iface_mac[6];
	set_my_mac(iface, iface_mac);
	
	struct timeval st, now;
	gettimeofday(&st, NULL);
	
	while(1)
	{
		gettimeofday(&now, NULL);
		if ((now.tv_sec - st.tv_sec) > 4) 
		{
			break;
		}
		
		bytes = recv_timeout(sock, buf, 2);
		if (bytes == -1) 
		{ 
			break;
		}
		if (bytes > 120) continue;
		
		printf("<%s> recv %d\n", __FUNCTION__, bytes);
		
		eth = (struct ethheader *) buf;
		arp = (struct arp_packet *) (buf + sizeof(struct ethheader));
		
		printip(ip);
		printip(arp->arp_ip_source);
		printmac(iface_mac);
		printmac(eth->dmac);
		
		if (eth->type == htons(ETH_P_ARP) && 
		    0 == memcmp(eth->dmac, iface_mac, sizeof(iface_mac)) && 
			arp->arp_operation == htons( ARPOP_REPLY ) &&
			ip == arp->arp_ip_source
			) //TODO maybe another case when searching ip requesting anyone else
		{ 
			printf("This is binded ip!\n"); 
			return 1;
		}
		
	}

	return 0;
}

//Sending dhcp packet

int sendDHCP(int sock, frame_t * frame, int size)
{
	if (size == 0) size = frame->size - 2; //2 is fucking padding
	
	char buffer[size];
	char * ptr = buffer;
	memcpy(ptr, &frame->h_eth, sizeof(struct ethhdr)); ptr += sizeof(struct ethhdr);
	memcpy(ptr, &frame->h_ip,  ntohs(frame->h_ip.ip_len)); 
	
	if ( write(sock, buffer, size) == -1)  
	{
		perror("<send DHCP> send ");
		close(sock);
		return -1;
	}

	return 0;
}

//waiting dhcp-reply packet 
int recvDHCP(int sock, char * iface, frame_t * frame, int bootp_type, int dhc_type, u_int32_t transid, int timeout)
{
	int bytes;
	int falsecnt = 0;
	struct timeval st, now;
	gettimeofday(&st, NULL);

	while(1)
	{
		if (timeout > 0)
		{
			gettimeofday(&now, NULL);
			if ((now.tv_sec - st.tv_sec) > timeout) 
			{
				printf("TIMEOUT\n");
				return -1;
			}
		}
		
		char buffer[sizeof(frame_t)];
		memset(buffer, 0, sizeof(buffer));
		memset(frame, 0, sizeof(frame_t)); //maybe not optimal
		
		if (timeout > 0 )
			frame->size = recv_timeout(sock, buffer, timeout);
		else 
			frame->size = recvfrom(sock, buffer, DHCP_MTU_MAX, 0, NULL, 0);
			
		//printf("<%s> recv_timeout returns %d\n", __FUNCTION__, frame->size);
		
		if (frame->size == -1) 
		{ 
			perror("recv");
			return -1;
		}

		if (frame->size < DHCP_FIXED_NON_UDP) continue;
		
		memcpy(&frame->h_eth, buffer, sizeof(struct ethhdr));
		memcpy(&frame->h_ip, buffer + sizeof(struct ethhdr), frame->size - sizeof(struct ethhdr));
		frame->size += 2;
		
		if (frame->p_dhc.op != bootp_type) continue;
		if (bootp_type == BOOTP_REQUEST && timeout == 0) break;
		
		if (frame->p_dhc.xid == transid)  
		{
			if (dhc_type == DHCPOFFER) break;
			if (dhc_type == DHCPACK ) 
			{	
				int sip;
				u_int32_t d_sip;
				
				get_lease(iface, NULL, (unsigned char*)&sip);
				
				if (-1 == get_option(&frame->p_dhc, 54, &d_sip, sizeof(d_sip))) continue;
				if (d_sip != sip && d_sip != get_iface_ip(iface)) continue;
 
				if (frame->p_dhc.options[6] == DHCPACK) 
				{
					printf("ACK\n"); 
					break; 
				}
				if (frame->p_dhc.options[6] == DHCPNAK) 
				{
					printf("NAK\n"); 
					return -1; 
				}
			}
		}
	}

	return 0;
}

