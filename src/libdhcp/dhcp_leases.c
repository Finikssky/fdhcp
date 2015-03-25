#include "dleases.h"

//Функция очистки базы от устаревших записей
void clear_lease()
{
	FILE *fd, *newfd;
	unsigned char *iter;
	struct s_dhcp_lease lease;
	struct timeval tv;
	int exist;

	add_log("Clearing lease...");

	memset(&lease, 0, sizeof(lease));

	fd = fopen("s_dhcp.lease", "r");
	if (fd == NULL)
	{ 
		add_log("NO LEASE FILE"); 
		return;
	}

	newfd = fopen("~s_dhcp.lease", "w+");

	//Читаем записи из базы и записываем действительные во временный файл
	while(fread(&lease, sizeof(lease), 1, fd))
	{
		gettimeofday(&tv, NULL);
		if (lease.stime + lease.ltime > tv.tv_sec) 
		{
			fwrite(&lease, sizeof(lease), 1, newfd);
		}
	}

	fclose(fd);
	fclose(newfd);

	system("mv s_dhcp.lease temp");
	system("mv ~s_dhcp.lease s_dhcp.lease");
	system("mv temp ~s_dhcp.lease");

	add_log("Lease base succsecful cleared.");
}


//Проверка, арендован ли адрес
int in_lease(int ip)
{
	FILE *fd;
	unsigned char * iter;
	struct s_dhcp_lease lease,ret;
	struct timeval tv;
	int exist;

	add_log("Proof ip in lease...");

	memset(&lease, 0, sizeof(lease));

	fd = fopen("s_dhcp.lease","r");
	if(fd == NULL)  
	{ 
		add_log("NEW LEASE FILE"); 
		return 1;
	}

	exist = 0;
	while (fread(&lease, sizeof(lease), 1, fd))
	{
		memset(&ret, 0, sizeof(ret));
		printip(lease.ip);
		printip(ip);
		if(lease.ip == ip) 
		{
			ret = lease;
			exist = 1 ;
		}
	}
	//Если адреса нет в базе
	if (!exist) 
	{
		fclose(fd);
		add_log("Good IP");
		return 1;
	}

	gettimeofday(&tv, NULL);

	//Если запись с адресом недействительна
	if ((ret.stime + ret.ltime) < tv.tv_sec) 
	{
		fclose(fd);
		add_log("Good IP");
		return 1;
	}
	else 
	{
		fclose(fd);
		add_log("Bad IP, try other");
		return 0;
	}
	
	fclose(fd);
	return 0;	
}

//Пытаемся назначить адрес из заданного промежутка
int try_give_ip(ip_address_range_t * range)
{
	u_int32_t ip;
	srand(time(NULL)); //why?
	for (ip = htonl(range->start_address); ip <= htonl(range->end_address); ip++)
	{
		if (in_lease(ntohl(ip))) 
			return ntohl(ip);
	}
	
	return -1;
}

//Проверка, можем ли мы выдать запрашиваемый адрес
int get_proof(struct dhcp_packet * dhc, u_int32_t * address)
{
	FILE * fd;
	char * iter;
	struct s_dhcp_lease lease, ret;
	struct timeval tv;
	int exist;

	add_log("Search requested IP in lease...");

	memset(&lease, 0, sizeof(lease));
	iter = dhc->options + 3;

	if ( -1 == get_option(dhc, 50, (void*)address, sizeof(u_int32_t)) )
		return -1;

	fd = fopen("s_dhcp.lease", "r");
	if (fd == NULL) 
	{ 
		printf("NEW LEASE FILE\n"); 
		add_log("NEW LEASE FILE"); 
		return 1;
	}

	exist = 0;
	while (fread(&lease, sizeof(lease), 1, fd))
	{
		memset(&ret, 0, sizeof(ret));
		printip(lease.ip);
		printip(*address);
		if (lease.ip == *address) 
		{
			ret = lease;
			exist = 1;
		}
	}

	if (!exist) 
	{
		fclose(fd);
		printf("NO THIS IP IN LEASE \n");
		add_log("NO THIS IP IN LEASE ");
		return 1;
	}

	gettimeofday(&tv,NULL);

	unsigned char rhw[ETH_ALEN];
	memcpy(rhw, dhc->chaddr, ETH_ALEN);
	printf("LMAC "); printmac(ret.haddr);
	printf("INMAC "); printmac(rhw);
	
	//Запись с адресом недействительна
	if ((ret.stime + ret.ltime) < tv.tv_sec) 
	{
		printf("REPLAY LEASE \n");
		fclose(fd);
		add_log("REPLAY LEASE ");
		return 1;
	} //Запись действительна и имеет тот же мак что у клиента
	else if (0 == memcmp(ret.haddr, rhw, ETH_ALEN)) 
	{
		printf("REPLAY LEASE BY MAC \n");
		fclose(fd);
		add_log("REPLAY LEASE BY MAC");
		return 1;
	} //Запись действительна и адрес выдан другому клиенту		 
	else 
	{
		printf("LEASE FAIL \n");
		fclose(fd);
		add_log("LEASE FAIL ");
		return 0;
	}
	
	fclose(fd);
	add_log("SEARCH FAIL");

	return 0;
}

//Функция добавления записи в базу клиента
void add_lease(char * iface, u_int32_t cip, u_int32_t sip, long time)
{
	FILE *fd;
	struct dhcp_lease lease;
	struct timeval tv;
	
	char lease_file[255];
	snprintf(lease_file, sizeof(lease_file), "%s_dhcp.lease", iface);
	
	gettimeofday(&tv,NULL);

	fd = fopen(lease_file, "w");

	printf("ADD LEASE: TIME = %ld LTIME= %d\n", tv.tv_sec, ntohl(time));
	printf("OFFERED IP: "); printip(cip); 
	printf("SERVER IP: ");  printip(sip);	

	lease.cip = cip;
	lease.sip = sip;
	lease.stime = tv.tv_sec;
	lease.ltime = ntohl(time);

	fwrite(&lease, sizeof(lease), 1, fd);

	fclose(fd);
}

//Функция получения записи из базы клиента
int get_lease(char * iface, unsigned char * cip, unsigned char * sip)
{
	FILE *fd;
	struct dhcp_lease lease;
	struct timeval tv;
	long interval;
	
	char lease_file[255];
	snprintf(lease_file, sizeof(lease_file), "%s_dhcp.lease", iface);

	fd = fopen(lease_file, "r");
	if (fd == NULL) 
	{
		printf("can't open lease_file %s\n", lease_file);
		perror("");
		return -1;
	}

	fread(&lease, sizeof(lease), 1, fd);

	if (cip != NULL) memcpy(cip, &lease.cip, sizeof(lease.cip));
	if (sip != NULL) memcpy(sip, &lease.sip, sizeof(lease.sip));
	
	gettimeofday(&tv, NULL);
	
	interval = tv.tv_sec - lease.stime;
	if (interval >= (lease.ltime / 2) && interval < (lease.ltime * 7 / 8)) return T_RENEWING; 
	if (interval >= (lease.ltime * 7 / 8) && (interval < lease.ltime)) return T_REBINDING;
	if (interval >= lease.ltime) return T_END;
	
	fclose(fd);
	return 0;
}

//Функция добавления записи в базу сервера
int s_add_lease(u_int32_t ip, long time, unsigned char * mac, char * host)
{
	FILE *fd;
	struct s_dhcp_lease lease;
	struct timeval tv;

	gettimeofday(&tv,NULL);

	add_log("Start server adding lease..");
	fd = fopen("s_dhcp.lease", "a+");
	if (fd == NULL) 
	{
		perror("<s_add_lease> cannot open file");
	}
	
	lease.ip = ip;
	lease.stime = tv.tv_sec;
	lease.ltime = time;
	
	printf("ADDING MAC: ");	
	printmac(mac);
	if (mac  != NULL) memcpy(lease.haddr, mac, ETH_ALEN);
	if (host != NULL) memcpy(lease.hostname, host, 9); //why 9?
	
	fwrite(&lease, sizeof(lease), 1, fd);
	fclose(fd);

	add_log("Lease added succesful");

	return 0;
}


