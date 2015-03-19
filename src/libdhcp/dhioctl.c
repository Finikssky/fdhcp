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
void printmac(unsigned char *mac)
{
printf("MAC: %x:%x:%x:%x:%x:%x\n", mac[0], mac[1],
				   mac[2], mac[3],
				   mac[4], mac[5]);
}

//Добавляем сообщение в лог
void add_log(char * s)
{
	char filename[32];
	sprintf(filename, "log_%d.txt", getpid());
	FILE *fd = fopen(filename, "a+");
	fprintf(fd, "%s\n", s);
	fclose(fd);
}

//Получение адреса сервера из DHCP пакета
int get_sip_from_pack(struct dhcp_packet *dhc)
{
	int i;
	int ret = 0;
	add_log("Get SIP OPTION from DHCP packet");

//print_dhcp_options(dhc);

	for( i = 4; dhc->options[i] != 255 && i < DHCP_MAX_OPTION_LEN; i++)
	{
		if (dhc->options[i] == 54) 
		{
			memcpy(&ret, dhc->options + i + 2, sizeof(int));
			add_log("Succesful get SIP");
			return ret; 
		}
	}

	add_log("Error get SIP");
	return ret;
}

//Печать всех опций DHCP пакета
void print_dhcp_options(struct dhcp_packet *dhc)
{
	int i;
	printf("DHCP OPTIONS\n");
	
	for(i = 0; i < DHCP_MIN_OPTION_LEN; i++)
	{
		printf("%d ",dhc->options[i]);
		if(i%8 == 0 && i != 0) printf("\n"); 
	}

}

//Получение запрашиваемого адреса из DHCP пакета
int get_rip_from_pack(struct dhcp_packet *dhc)
{
	int ret = 0;
	int i;
	add_log("Get RIP OPTION from DHCP packet");

	//print_dhcp_options(dhc);
	for (i = 4; dhc->options[i] != 255 && i < DHCP_MAX_OPTION_LEN; i++)
	{
		if (dhc->options[i]==50) 
		{ 
			ret = 1;  
			break; 
		}
	}        

	if (ret != 0) memcpy(&ret,dhc->options + i + 2, 4);
	if (ret == 0 && dhc->ciaddr.s_addr != 0) ret = dhc->ciaddr.s_addr;	

	printf("RETURN RIP: "); printip(ret);
	add_log("Succesful get RIP OPTION from DHCP packet");

	return ret;
}

long get_lease_time()
{ //TODO убрать
	return 60;
}

//Получение имени хоста из DHCP пакета
char* get_host_from_pack(struct dhcp_packet *dhc){
char *iter;
char *ret=NULL;
add_log("Get HOST OPTION from DHCP packet");
    
	iter=dhc->options+4;
        while(*iter != 255){
                if(*iter == 12){
			 ret=malloc(*(iter+1)); //TODO СЖЕЧЬ!
			 printf("hi\n");
			 memcpy(&ret,iter+2,*(iter+1));
			 return ret;
		 }
                iter++;
        }

add_log("Succesful get HOST OPTION from DHCP packet");
return NULL;
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

	memset(&ifr,0,sizeof(ifr));
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
	printf("<%s> sock: %d timeout: %d\n", __FUNCTION__, sock, timeout);
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
		int bytes = recvfrom(sock, buf, 1518, 0, NULL, 0);
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
int set_config(char * buffer, char * iface)
{

	struct ifreq ifr;
	struct sockaddr_in sai;
	int sockfd;                     
	int selector;
	unsigned char mask;
	char *p;
	int hres = -1;

	struct dhcp_packet *dhc = (struct dhcp_packet*) (buffer + FULLHEAD_LEN);

	add_log("Set interface configuration..");
 
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	strncpy(ifr.ifr_name, iface, IFNAMSIZ);

	memset(&sai, 0, sizeof(struct sockaddr));
	sai.sin_family = AF_INET;
	sai.sin_port = 0;
	sai.sin_addr = dhc->yiaddr;

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
			close(sockfd);
			return -1;
		}
	}

	ioctl(sockfd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING | IFF_BROADCAST;

 	hres = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
	if (hres == -1)  perror("ioctl set flags");

	close(sockfd);
	
	//Устанавливаем маршруты
	int cnt;
	for (cnt = 4; dhc->options[cnt] != 255; cnt++)
	{
		if (dhc->options[cnt] == 3 && (dhc->options[cnt + 1] % 4) == 0)
		{
			printf("Setup routes..");
			
			int router_cnt;
			int router_max = dhc->options[cnt + 1] / 4;
			for ( router_cnt = 0; router_cnt < router_max; router_cnt++ )
			{
				u_int32_t ip;
				memcpy(&ip, &dhc->options[cnt + 2 + 4 * router_cnt], sizeof(ip)); 
			}
		}
	}
	
	//Устанавливаем DNS
	for (cnt = 4; dhc->options[cnt] != 255; cnt++)
	{
		if (dhc->options[cnt] == 5 && (dhc->options[cnt + 1] % 4) == 0)
		{
			printf("Setup dns..");
			FILE *fd = fopen("/etc/resolv.conf", "a+");
			if (fd == NULL) 
			{
				perror("cant open dns file");
				return -1;
			}
			int dns_cnt;
			int dns_max = dhc->options[cnt + 1] / 4;
			for ( dns_cnt = 0; dns_cnt < dns_max; dns_cnt++ )
			{
				u_int32_t ip;
				memcpy(&ip, &dhc->options[cnt + 2 + 4 * dns_cnt], sizeof(ip));
				fprintf(fd, "nameserver %d.%d.%d.%d\n", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
			}
			fclose(fd);
			break;
		}
	}
	
	
	add_log("Successful set interface configuration");
	return 0;
}


