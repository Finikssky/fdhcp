#include "dctp.h"
#include "dhcp.h"

#include <sys/select.h>

int recv_timeout_DCTP(int sock, void * buf, int timeout, struct sockaddr * sender, size_t * size)
{
	fd_set rdfs;
	struct timeval tv;
	int ret;
		
	FD_ZERO(&rdfs);
	FD_SET(sock, &rdfs);
	tv.tv_sec  = timeout;
	tv.tv_usec = 0;
	ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
		
	if (ret) 
	{
		int bytes = recvfrom(sock, buf, 1512, 0, sender, size);
		return bytes;
	}
	else 
		return -1;
}

int create_csum(u_int8_t * buffer, size_t size)
{
	return 42;
}

int calculate_DCTP_COMMAND_LEN(DCTP_COMMAND * command)
{
	int sum = DCTP_ARG_MAX_LEN + strlen(command->name);
	
	printf("DCTP_COMMAND_SUM = %d\n", sum);
	return sum;
}

int send_DCTP_PACK(int sock, void * buffer, size_t size, struct sockaddr_in * in, int port, char * ip)
{
	struct sockaddr_in dest;
	if (in == NULL)
	{
		dest.sin_family      = AF_INET;
		dest.sin_port        = htons(port);
		dest.sin_addr.s_addr = inet_addr(ip);
	}
	else
		memcpy(&dest, in, sizeof(struct sockaddr_in));	
	
	printf("SEND OPTIONS:\n %s\n %d\n %s\n", dest.sin_family == AF_INET ? "AF_INET" : "NONE", ntohs(dest.sin_port), inet_ntoa(dest.sin_addr) );
		
	sendto(sock, buffer, size, 0, (struct sockaddr*)&dest, sizeof(dest));
	
	return 0;
}

void init_DCTP_PACK(DCTP_PACKET * pack, u_int8_t type, u_int8_t * payload, size_t payload_size)
{
	pack->label = DCTP_LABEL;
	pack->type = type;
	pack->csum = create_csum(payload, payload_size);
	pack->id = 42; //pack->csum ^ rand()%100000;
	pack->size = payload_size + sizeof(DCTP_PACKET);
}

int send_DCTP_COMMAND(int sock, DCTP_COMMAND command, char * ip, int port)
{
	DCTP_COMMAND_PACKET pack;
	DCTP_REPLY_PACKET r_pack;
	
	int fail = 0;
	
	pack.payload = command;
	while(1)
	{
		init_DCTP_PACK((DCTP_PACKET *)&pack, DCTP_MSG_COMM, (u_int8_t *)&pack.payload, sizeof(pack.payload));
		send_DCTP_PACK(sock, &pack, pack.packet.size, NULL, port, ip);
	
		if (-1 == receive_DCTP_reply(sock, &r_pack)) return -1;
		if (r_pack.payload == DCTP_SUCCESS)
		{
			return 0;
		}
		else
		{
			printf("send command error: %s fails: %d\n", "err_desc", ++fail);
			usleep(100000);
			if (fail > 3) return -1;
		}
	}
}

int init_DCTP_socket(int port)
{
	struct sockaddr_in list;
	list.sin_family      = AF_INET;
	list.sin_port        = htons(port);
	list.sin_addr.s_addr = INADDR_ANY;
	
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1 ) 
	{
		printf("<%s> SOCKET OPEN ERROR!\n", __FUNCTION__);
		perror("socket open:");
		return -1;
	}
	
	if (bind(sock, (struct sockaddr*) &list, sizeof(list)) == -1)
	{
		printf("<%s> SOCKET BIND ERROR!\n", __FUNCTION__);
		perror("socket bind:");
		return -1;
	}
	
	return sock;
}

int release_DCTP_socket(int sock)
{
	close(sock);
}

int receive_DCTP_PACKET(int sock, void * buffer, struct sockaddr_in * sender, int timeout) 
{
	struct timeval st, now;
	gettimeofday(&st, NULL);
	
	while(1)
	{
		printf("waiting...\n");
		int bytes;
		size_t size = sizeof(struct sockaddr);
		
		if (timeout == 0)
		{	
			bytes = recvfrom(sock, buffer, 1512, 0, (struct sockaddr *)sender, &size);
			if (bytes == -1) 
			{
				perror("receive ");
				return -1;
			}
		}
		else	
		{
			bytes = recv_timeout_DCTP(sock, buffer, timeout, (struct sockaddr *)sender, &size);
			gettimeofday(&now, NULL);
			if (bytes == -1 || (now.tv_sec - st.tv_sec) > timeout) 
			{
				perror("timeout");
				return -1;
			}
		}
		
		printf("<%s> recv bytes: %d\n", __FUNCTION__, bytes);
		//printf("RECV OPTIONS:\n %s\n %d\n %s\n", sender->sin_family == AF_INET ? "AF_INET" : "NONE", ntohs(sender->sin_port), inet_ntoa(sender->sin_addr) );
		
		DCTP_PACKET * temp = (DCTP_PACKET *)buffer;
		
		if (temp->label == DCTP_LABEL) 
		{
			//printf("haha! das dctp packet!\n"); 
			return bytes; 
		}
	}
}

int receive_DCTP_command(int sock, DCTP_COMMAND_PACKET * pack, struct sockaddr_in * sender) 
{
	
	while(1)
	{	
		int bytes = receive_DCTP_PACKET(sock, pack, sender, 0);
		if (pack->packet.type == DCTP_MSG_COMM)
		{  
			if (pack->packet.csum != create_csum((u_int8_t*)&pack->payload, sizeof(pack->payload)))
			{
				printf("Error in control sum!\n");
				send_DCTP_REPLY(sock, pack, DCTP_REPEAT, sender);
				continue;
			}
				
			//printf("and yes! das dctp command!\n");
			return 0;
		}
		else
		{
			printf("<%s> wrong packet type", __FUNCTION__);
		}
	}

}

int receive_DCTP_reply(int sock, DCTP_REPLY_PACKET * pack)
{
	int err_counter = 0;
	struct sockaddr_in sender;

	while(1)
	{	
		int bytes = receive_DCTP_PACKET(sock, pack, &sender, DCTP_REPLY_TIMEOUT);
		if (bytes == -1) err_counter++;
		if (err_counter == 3)
		{
			printf("sorry but network is down\n");
			return -1;
		}
		
		if (pack->packet.type == DCTP_MSG_RPL)
		{  				
			printf("and yes! das dctp reply!\n");
			return 0;
		}
	}

}

void send_DCTP_REPLY(int sock, DCTP_COMMAND_PACKET * in, DCTP_STATUS status, struct sockaddr_in * sender)
{
	DCTP_REPLY_PACKET pack;
	memset(&pack, 0, sizeof(pack));
	
	pack.payload = status;
	init_DCTP_PACK((DCTP_PACKET *)&pack, DCTP_MSG_RPL, (u_int8_t *)&pack.payload, sizeof(pack.payload));
	pack.packet.id = in->packet.id;
	
	printf("send reply, status %d\n", status);
	send_DCTP_PACK(sock, &pack, pack.packet.size, sender, 0, NULL);
}

DCTP_cmd_code_t parse_DCTP_command (DCTP_COMMAND * in, char * ifname )
{	
	char option [255];
	int i;
	
	if (strstr(in->name, "dctp_") == in->name)
	{
		strcpy(option, in->name);
		strcpy(ifname, "");
	}
	else 
	{
		sscanf(in->name, "%s %s", ifname, option);
	}
	printf("<%s> iface: %s option: %s\n", __FUNCTION__, ifname, option);
	
	for ( i = 0; strlen(DCTP_cmd_list[i].text) != 0; i++ )
		if (!strcmp(option, DCTP_cmd_list[i].text))
			return DCTP_cmd_list[i].code;
		
	return UNDEF_COMMAND;
}
