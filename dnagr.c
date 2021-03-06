#include "libdhcp/dhcp.h"
#include "libdhcp/dhioctl.h"
#include "libdhcp/dhconn.h"

#include "core.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

int REQUESTS;
int REPLYES;
int DELAY;
int R_COUNTER;

char * ename;

void * reply(void * arg)
{
	frame_t frame; 
	int rep_sock = init_packet_sock(ename, ETH_P_ALL);
	struct timeval st, now;
	gettimeofday(&st, NULL);

	while(1)
	{
		gettimeofday(&now, NULL);
		if ((now.tv_sec - st.tv_sec) > 5) break;
		
		char buffer[sizeof(frame_t)];
		memset(buffer, 0, sizeof(buffer));
		memset(&frame, 0, sizeof(frame_t)); 
		
		frame.size = recvfrom(rep_sock, buffer, DHCP_MTU_MAX, 0, NULL, 0);
		
		if (frame.size == -1) continue;
		if (frame.size < DHCP_FIXED_NON_UDP) continue;
		
		memcpy(&frame.h_eth, buffer, sizeof(struct ethhdr));
		memcpy(&frame.h_ip, buffer + sizeof(struct ethhdr), frame.size - sizeof(struct ethhdr));
		frame.size += 2;
		
		if (frame.p_dhc.op == BOOTP_REPLY)  
		{
			unsigned char type;
			if (-1 == get_option(&frame.p_dhc, 53, &type, sizeof(type))) continue;
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

	frame_t frame; memset(&frame, 0, sizeof(frame));

	if (argc > 1) 
		ename = argv[1];
	else 
		ename = "eth0";

	if(argc > 2) REQUESTS = atoi(argv[2]);
	if(argc > 3) DELAY = atoi(argv[3]);

	int sock = init_packet_sock(ename, ETH_P_IP);

	set_my_mac(ename, macs);
	

	create_packet(ename, &frame, 1, DHCPDISCOVER, NULL);
	create_ethheader(&frame, macs, macd, ETH_P_IP);
	create_ipheader(&frame, INADDR_ANY, INADDR_BROADCAST);
	create_udpheader(&frame, DHCP_CLIENT_PORT, DHCP_SERVER_PORT);
	
	gettimeofday(&start, NULL);
	
	pthread_create(&rep_t, NULL, reply, NULL);
	
	for (R_COUNTER = 0; R_COUNTER < REQUESTS; R_COUNTER++) 
	{
		usleep(DELAY * 1000);
		
		correct += 1.0 * DELAY * 1000 / 1000000;

		frame.p_dhc.xid = LASTRANDOM++;
		LASTRANDOM %= 1000000;		

		sendDHCP(sock, &frame, 0); 
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


