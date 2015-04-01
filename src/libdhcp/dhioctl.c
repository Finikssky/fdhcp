#include "dhioctl.h"

//Печатаем  IP
void printip(u_int32_t ip)
{
	printf("IP: %d.%d.%d.%d\n",
	 ip & 0xff,
	 (ip>>8) & 0xff,
	 (ip>>16) & 0xff, 
	 (ip>>24) & 0xff);
}

//Печатаем MAC
void printmac(unsigned char * mac)
{
	printf("MAC: %x:%x:%x:%x:%x:%x\n", 
					mac[0], mac[1],
					mac[2], mac[3],
					mac[4], mac[5]);
}

//Добавляем сообщение в лог
void add_log(char * s)
{
	char filename[32];
	sprintf(filename, "log_%d.txt", getpid());
	
	struct timeval now;
	gettimeofday(&now, NULL);
	long sec = now.tv_sec;
	
	FILE *fd = fopen(filename, "a+");
	fprintf(fd, "[ %02ld:%02ld:%02ld ] %s\n", (sec / (60 * 24)) % 24 ,(sec / 60) % 60, sec % 60 , s);
	fclose(fd);
}

int nod(int a, int b)
{
	while (a != 0 && b != 0)
	{
		if (a > b) 
			a = a % b;
		else
			b = b % a;
	}
	
	return a + b;
}

int is_char_option(int option)
{
	switch (option)
	{
		case 12:
		case 15:
			return 1;
		default:
			return 0;
	}
	return 0;
}

int get_option(struct dhcp_packet * dhc, int option, void * ret_value, int size)
{
	//printf("<%s> option: %3d ret_size: %3d\n", __FUNCTION__, option, size);
	//TODO maybe stack overborder
	if (ret_value == NULL) return -1;
	if (size <= 0) 		   return -1;
	
	int i;
	memset(ret_value, 0, size);
	
	for(i = 4; i < DHCP_MAX_OPTION_LEN; i++)
	{
		int code = dhc->options[i];
		
		if (code == 255)
		{
			if (option == 255)
			{
				memcpy(ret_value, &i, sizeof(int));
			}
			break;
		}
		
		int len  = dhc->options[i + 1];
		//printf("<%s> cnt: %d code: %d len: %d\n", __FUNCTION__, i, code, len);
		if (code == option)
		{
			if ( size >= len && (nod(size, len) > 1 || is_char_option(option) || len == 1) )
			{
				memcpy(ret_value, dhc->options + i + 2, len);
				return len;
			}
			else if ( size < len && (nod(size, len) > 1 || is_char_option(option)) )
			{
				memcpy(ret_value, dhc->options + i + 2, size);
				return size;
			}
			else
			{
				perror("incorrect option size!");
				return -1;
			}
		}
		
		i += len + 1;
	}
	
	return -1;
}

//Получение адреса интерфейса
int get_iface_ip(char * iface)
{
	struct ifreq ifr;
	int sockfd;                    
	struct sockaddr_in ip;
	int hres;
	add_log("Get interface's IP");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
       
	hres = ioctl(sockfd, SIOCGIFADDR, &ifr);
	if (hres == -1) 
		perror("get iface ip");

	memcpy(&ip, &ifr.ifr_addr, sizeof(struct sockaddr));

	close(sockfd);

	add_log("Succesfule get interface's IP");
	return ip.sin_addr.s_addr;
}

//Обертка для recvfrom с таймаутом на селекте
int recv_timeout(int sock, void * buf, int timeout)
{
	//printf("<%s> sock: %d timeout: %d\n", __FUNCTION__, sock, timeout);
	fd_set rdfs;
	struct timeval tv;
	int ret;
		
	FD_ZERO(&rdfs);
	FD_SET(sock, &rdfs);
	tv.tv_sec  = timeout;
	tv.tv_usec = 0;
	ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
		
	if (ret > 0) 
	{
		int bytes = recvfrom(sock, buf, DHCP_MTU_MAX, 0, NULL, 0);
		if (bytes == -1)
		{
			perror("<recv_timeout> recv");
		}
		return bytes;
	}
	else if (ret == -1)
	{
		perror("<recv_timeout> select");
		return -1;
	}

	return -1;
}

//Получение мак-адреса сетевого интерфейса
int set_my_mac(char * iface, unsigned char * mac)
{
	struct ifreq ifr;
	int sockfd;                     /* socket fd we use to manipulate stuff with */
	int hres;
	add_log("Get interface's MAC");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&ifr, 0, sizeof(ifr));
	memcpy(ifr.ifr_name, iface, IFNAMSIZ);
	hres = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
	if (hres == -1) 
	{
		perror("set my mac");
	}

	memcpy(mac, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	close(sockfd);

	add_log("Succesful get interface's MAC");

	return 0;
}

//Установка параметров сетевого интерфейса
int apply_interface_settings(char * buffer, char * iface)
{
	struct ifreq ifr;
	struct sockaddr_in sai;
	int sockfd;                     
	int selector;
	unsigned char mask;
	char * p;
	int hres = -1;

	struct dhcp_packet * dhc = (struct dhcp_packet * ) (buffer + FULLHEAD_LEN);

	add_log("Set interface configuration..");
 
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	strncpy(ifr.ifr_name, iface, IFNAMSIZ);

	memset(&sai, 0, sizeof(struct sockaddr));
	sai.sin_family = AF_INET;
	sai.sin_port   = 0;
	sai.sin_addr   = dhc->yiaddr;

	p = (char *) &sai;
	memcpy( &ifr.ifr_addr, p, sizeof(struct sockaddr));
	
	//Устанавливаем адрес пока не установится
	int failcnt = 0;
	while(hres-- <= -1 )
	{
		hres = ioctl(sockfd, SIOCSIFADDR, &ifr);
		if (hres == -1) perror("ioctl set ip");
		if (failcnt++ > 5) 
		{
			perror("cant'set ip");
			close(sockfd);
			return -1;
		}
	}

	ioctl(sockfd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING | IFF_BROADCAST;

 	hres = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
	if (hres == -1)  perror("ioctl set flags");

	close(sockfd);
	
	printf("IP SETUP!\n");
	
	//set routers
	char routers[12];
	hres = get_option(dhc, 3, (void *)routers, sizeof(routers));
	if (hres > 0)
	{
		printf("SETUP ROUTERS: \n");
		int router_cnt;
		int router_max = hres / 4;
		for ( router_cnt = 0; router_cnt < router_max; router_cnt++ )
		{
			u_int32_t ip;
			memcpy(&ip, &routers[ 4 * router_cnt ], sizeof(ip)); 
			printf("    SET ROUTER %d ---> ", router_cnt + 1); printip(ip);
			//TODO
		}
	}
	
	//set dns
	char dns[12];
	hres = get_option(dhc, 6, (void *)dns, sizeof(dns));
	if (hres > 0)
	{
		printf("SETUP DNS:\n");
		
		FILE *fd = fopen("/etc/resolv.conf", "a+");
		if (fd == NULL) 
		{
			perror("cant open dns file");
			return -1;
		}
		
		int dns_cnt;
		int dns_max = hres / 4;
		for ( dns_cnt = 0; dns_cnt < dns_max; dns_cnt++ )
		{
			u_int32_t ip;
			memcpy(&ip, &dns[ 4 * dns_cnt ], sizeof(ip)); 
			printf("    SET DNS-SERVER %d ---> ", dns_cnt + 1); printip(ip);
			fprintf(fd, "nameserver %d.%d.%d.%d\n", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
		}
		fclose(fd);
	}
	
	//set host_name
	char hostname[32];
	hres = get_option(dhc, 12, (void *)hostname, sizeof(hostname));
	if (hres > 0)
	{
		printf("SETUP HOSTNAME: %s\n", hostname);
		//TODO
	}
	
	//set domain_name
	char dname[32];
	hres = get_option(dhc, 15, (void *)dname, sizeof(dname));
	if (hres > 0)
	{
		printf("SETUP DOMAIN NAME: %s\n", dname);
		//TODO
	}
	
	add_log("Successful set interface configuration");
	return 0;
}


