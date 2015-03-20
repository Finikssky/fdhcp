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
	
	create_ethheader(buf, macs, macd, ETH_P_ARP);
	memset(macd, 0, ETH_ALEN);
	create_arp(iface, buf, ip, macs, macd, ARPOP_REQUEST);
	
	int size = sizeof(struct ethheader) + sizeof(struct arp_packet) + 18;
	if ( write(sock, buf, size) == -1 )  
	{
		perror("send ARP Request");
		return -1;;
	}

	return 0;
}

//Функция ожидания ARP-пакета
int recvARP(int sock, char * iface, u_int32_t ip)
{
	int bytes;
	char buf[120];
	struct ethheader * eth;
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
			) 
		{ 
			printf("This is binded ip!\n"); 
			return 1;
		}
		
	}

	return 0;
}

//Sending dhcp packet

int sendDHCP(int sock, char *iface, void* buffer, int size)
{
	if (size == 0) size = FULLHEAD_LEN + sizeof(struct dhcp_packet);

	if ( write(sock, buffer, size) == -1)  
	{
		perror("<send DHCP> send ");
		close(sock);
		return -1;
	}

	return 0;
}

//waiting dhcp-reply packet 
int recvDHCP(int sock, char * iface, void * buffer, int type, u_int32_t transid)
{
	int bytes;
	struct dhcp_packet * dhc;
	int falsecnt = 0;
	struct timeval st, now;
	gettimeofday(&st, NULL);

	while(1)
	{
		gettimeofday(&now, NULL);
		if ((now.tv_sec - st.tv_sec) > 5) 
		{
			printf("TIMEOUT\n");
			return -1;
		}
		
		memset(buffer, 0, sizeof(buffer));
		bytes = recv_timeout(sock, buffer, 5);
		//printf("<%s> recv_timeout returns %d\n", __FUNCTION__, bytes);
		
		if (bytes == -1) 
		{ 
			printf("TIMEOUT\n");
			return -1;
		}

		if (bytes < DHCP_FIXED_NON_UDP) continue;
		//printf("<%s> recv %d bytes\n", __FUNCTION__, bytes);
		dhc = (struct dhcp_packet*) (buffer + FULLHEAD_LEN);

		int sip;
		get_lease(iface, NULL, (unsigned char*)&sip);
	 	 	
		if (dhc->xid == transid && dhc->op == 2)  
		{
			u_int32_t d_sip;
			if (type == DHCPOFFER) break;
			if (type == DHCPACK ) 
			{	
				if (-1 == get_option(dhc, 54, &d_sip, sizeof(d_sip))) continue;
				if (d_sip != sip && d_sip != get_iface_ip(iface)) continue;
 
				if (dhc->options[6] == DHCPACK) { printf("ACK\n"); break; }
				if (dhc->options[6] == DHCPNAK) { printf("NAK\n"); return -1; }
			}
		}
	}

	return 0;
}

