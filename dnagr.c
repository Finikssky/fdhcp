#include "dhcp.h"
#include "core.h"
#include "dhioctl.h"
#include "dhconn.h"
#include <pthread.h>
#include <sys/time.h>

int REQUESTS;
int REPLYES;
int DELAY;
int R_COUNTER;

char * ename;

void * reply(void * arg)
{
	char buf[DHCP_MTU_MAX];
	int rep_sock = init_packet_sock(ename, ETH_P_ALL);
	struct timeval st, now;
	gettimeofday(&st, NULL);
	
	while (1) 
	{	
		gettimeofday(&now, NULL);
		
		if ( (now.tv_sec - st.tv_sec) > 5 ) break;
		
		recvfrom(rep_sock, buf, DHCP_MTU_MAX, 0, NULL, 0);
		struct dhcp_packet * dhc = (struct dhcp_packet *) (buf + FULLHEAD_LEN);
		
		if (dhc->op == 2)  
		{
			unsigned char type;
			if (-1 == get_option(dhc, 53, &type, sizeof(type))) continue;
			if (type != DHCPOFFER) continue;
			
			REPLYES++;
			gettimeofday(&st, NULL);
		}
	}
	
	close(rep_sock);
	return NULL;
}

int main(int argc, char* argv[])
{
	unsigned char macs[6];
	unsigned char macd[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
	FILE * f;
	struct timeval start, end;
	pthread_t rep_t;
	
	REQUESTS  = 10000;
	DELAY     = 0;
	LASTRANDOM    = 1;
	REPLYES       = 0;
	double correct = 0;

	char buf[DHCP_MTU_MAX];

	if (argc > 1) 
		ename = argv[1];
	else 
		ename = "eth0";

	if(argc > 2) REQUESTS = atoi(argv[2]);
	if(argc > 3) DELAY = atoi(argv[3]);

	int sock = init_packet_sock(ename, ETH_P_IP);

	struct dhcp_packet *dhc;

	set_my_mac(ename, macs);
	memset(buf, 0, sizeof(buf));

	int opt_size = 0;
	create_packet(ename, buf, 1, DHCPDISCOVER, &opt_size, NULL);
	create_ethheader(buf, macs, macd, ETH_P_IP);
	create_ipheader(buf, opt_size, INADDR_ANY, INADDR_BROADCAST);
	create_udpheader(buf, opt_size, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
	
	gettimeofday(&start, NULL);
	
	pthread_create(&rep_t, NULL, reply, NULL);
	
	for (R_COUNTER = 0; R_COUNTER < REQUESTS; R_COUNTER++) 
	{
		usleep(DELAY * 1000);
		
		correct += 1.0 * DELAY * 1000 / 1000000;
		
		dhc = ( struct dhcp_packet * )(buf + FULLHEAD_LEN);
		//printf("XID: %d\n",myxid);
		dhc->xid = LASTRANDOM++;
		LASTRANDOM %= 1000000;		

		//sendDHCP(ename,(void*)buf,0); 
		if ( write( sock, buf, DHCP_FULL_WITHOUT_OPTIONS + opt_size) == -1 )  
		{
			perror("Error: send ");
			close(sock);
			exit(1);
		}

	 }	
	 
	pthread_join(rep_t, NULL);

	gettimeofday(&end, NULL);

	close(sock);
	
	f = fopen("test_results.test", "a+");
	
	long fulltime_sec = end.tv_sec - start.tv_sec - 5 - correct;
	fprintf(f, "REQ: %d REP: %d DEL: %d FULLTIME: %ld \n", REQUESTS, REPLYES, DELAY, fulltime_sec);
	fclose(f);
	
	return 0;
}


