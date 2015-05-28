#include "libdctp/dctp.h"
#include "libdhcp/dhcp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

const char * stringize_DCTP_COMMAND_CODE(DCTP_cmd_code_t in)
{
    switch (in)
    {
        case DCTP_PING:
            return "ping";
        case DCTP_UPDATE_CONFIG:
            return "update config";
        case DCTP_PASSWORD:
            return "authorization";
        case SR_SET_IFACE_ENABLE:
        case CL_SET_IFACE_ENABLE:
            return "iface enable";
        case SR_SET_IFACE_DISABLE:
        case CL_SET_IFACE_DISABLE:
            return "iface disable";
        case SR_ADD_SUBNET:
            return "add subnet";
        case SR_DEL_SUBNET:
            return "del subnet";
        default: break;
    }

    return "";
}
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
        int i;
        int sum;
        for (i = 0; i < size; i++ )
        {
            sum += buffer[i];
            if (sum & 1) sum = sum >> 1;
        }
	return 42;
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
	return close(sock);
}

void init_DCTP_PACK(DCTP_PACKET * pack, u_int8_t type, u_int8_t * payload, size_t payload_size)
{
        static int id = 0;

        pack->label = DCTP_LABEL;
        pack->type = type;
        pack->csum = create_csum(payload, payload_size);
        pack->id = id++;
        pack->size = payload_size + sizeof(DCTP_PACKET);

        if (id > 1000000) id = 0;
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

int send_DCTP_COMMAND(int sock, DCTP_COMMAND command, char * ip, int port, char * error_ret)
{
        DCTP_COMMAND_PACKET pack;
        DCTP_REPLY_PACKET r_pack;
        if (NULL != error_ret) memset(error_ret, 0, DCTP_ERROR_DESC_SIZE);

        pack.payload = command;
        int repeats = 0;
        while (1)
        {
                init_DCTP_PACK((DCTP_PACKET *)&pack, DCTP_MSG_COMM, (u_int8_t *)&pack.payload, sizeof(pack.payload) - sizeof(pack.payload.arg) + strlen(pack.payload.arg) * sizeof(char));
                send_DCTP_PACK(sock, &pack, pack.packet.size, NULL, port, ip);

                int bytes = 0;
                while (1)
                {
                    bytes = receive_DCTP_reply(sock, &r_pack);
                    if (-1 == bytes)
                    {
                        snprintf(error_ret, DCTP_ERROR_DESC_SIZE, "%s", "network is inaccessible");
                        return -1;
                    }
                    if (r_pack.packet.id == pack.packet.id) break;
                }

                if (r_pack.payload.status == DCTP_SUCCESS)
                {
                        return 0;
                }

                if (r_pack.payload.status == DCTP_REPEAT)
                {
                        repeats++;
                        if (repeats > 3)
                        {
                            if (error_ret != NULL) snprintf(error_ret, DCTP_ERROR_DESC_SIZE, "%s", "Incorrect control sum of command");
                            return -1;
                        }
                        continue;
                }

                if (r_pack.payload.status == DCTP_FAIL)
                {
                        int r_esize = bytes - sizeof(DCTP_PACKET) - sizeof(r_pack.payload.status);
                        if (error_ret != NULL) memcpy(error_ret, r_pack.payload.error, r_esize);
                        return -1;
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
                        if (pack->packet.csum != create_csum((u_int8_t*)&pack->payload, bytes - sizeof(DCTP_PACKET)))
			{
                                send_DCTP_REPLY(sock, &pack->packet, DCTP_REPEAT, sender, "Error in control sum!");
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

void send_DCTP_REPLY(int sock, DCTP_PACKET * in, DCTP_STATUS status, struct sockaddr_in * sender, char * error_string)
{
        DCTP_REPLY_PACKET pack;
        memset(&pack, 0, sizeof(pack));

        pack.payload.status = status;
        if( error_string != NULL)
        {
            printf("put error in reply: %s", error_string);
            strncpy(pack.payload.error, error_string, sizeof(pack.payload.error));
        }

        init_DCTP_PACK((DCTP_PACKET *)&pack, DCTP_MSG_RPL, (u_int8_t *)&pack.payload, sizeof(pack.payload.status) + strlen(pack.payload.error));
        pack.packet.id = in->id;

        printf("send reply, status %d\n", status);
        send_DCTP_PACK(sock, &pack, pack.packet.size, sender, 0, NULL);
}

int receive_DCTP_reply(int sock, DCTP_REPLY_PACKET * pack)
{
	struct sockaddr_in sender;

	while(1)
	{	
                memset(pack, 0, sizeof(DCTP_REPLY_PACKET));
		int bytes = receive_DCTP_PACKET(sock, pack, &sender, DCTP_REPLY_TIMEOUT);
                if (bytes == -1) return -1;
		
		if (pack->packet.type == DCTP_MSG_RPL)
		{  				
                        printf("and yes! das dctp reply! ERRORS: %s\n", pack->payload.error);
                        return bytes;
		}
	}

}

int read_stream(char * buffer, int maxsize, FILE * stream)
{
   int i = 0;
   for (i = 0; i < maxsize && !feof(stream); i++)
   {
       char c = fgetc(stream);
       if (c == EOF)
           return i;
       else
           buffer[i] = c;
   }

   return i;
}

int write_stream(char * buffer, int maxsize, FILE * stream)
{
   int i = 0;
   for (i = 0; i < maxsize; i++)
   {
        fputc(buffer[i],stream);
   }

   return i;
}

int send_DCTP_CONFIG( int sock, const char * filename, struct sockaddr_in * sender)
{
        FILE * f = fopen(filename, "r");
	
	int first = 1;
	while (1)
	{
		DCTP_FILE_PACKET packet;
		memset(packet.payload.block, 0, sizeof(packet.payload.block));
                packet.payload.block_size = read_stream(packet.payload.block, sizeof(packet.payload.block), f);
		int file_ended = feof(f);         

		if (first) 
		{	
			if (file_ended) packet.payload.block_type = DCTP_FILE_ONCE;
			else packet.payload.block_type = DCTP_FILE_START;
			first = 0;
		}
		else
		{
			if (!file_ended)
			{
				packet.payload.block_type = DCTP_FILE_MBLOCK;
			}
			else
			{
				packet.payload.block_type = DCTP_FILE_END;
			}
		}
		
                init_DCTP_PACK((DCTP_PACKET *)&packet, DCTP_MSG_CFG, (u_int8_t *)&packet.payload, sizeof(packet.payload) - sizeof(packet.payload.block) + packet.payload.block_size);
		send_DCTP_PACK(sock, &packet, packet.packet.size, sender, 0, NULL);
		
		DCTP_REPLY_PACKET r_pack;
		if (-1 == receive_DCTP_reply(sock, &r_pack))
		{
			return -1;
		}
		else
		{
                        if (r_pack.payload.status == DCTP_SUCCESS) continue;
                        else return -1;
		}	
		
		if (file_ended) return 0;
	}
	
	return 0;
}

int receive_DCTP_CONFIG(int sock, char * filename)
{
        FILE * f = fopen(filename, "w");
	if (!f) return -1;
	
	DCTP_FILE_PACKET pack;
	struct sockaddr_in sender;
	
	while(1)
	{	
                int bytes = receive_DCTP_PACKET(sock, &pack, &sender, DCTP_REPLY_TIMEOUT);
		if (bytes == -1)
		{
			printf("connection lost");
			fclose(f);
			unlink(filename);
			return -1;
		}
		if (pack.packet.type == DCTP_MSG_CFG)
		{  
			if (pack.packet.csum != create_csum((u_int8_t*)&pack.payload, sizeof(pack.payload)))
			{
                                send_DCTP_REPLY(sock, &pack.packet, DCTP_REPEAT, &sender, "Error in control sum!");
				continue;
			}
                        write_stream(pack.payload.block, pack.payload.block_size, f);
			
                        send_DCTP_REPLY(sock, &pack.packet, DCTP_SUCCESS, &sender, NULL);
			if (pack.payload.block_type == DCTP_FILE_END)
			{
				fclose(f);
				return 0;
			}	
		}
		else
		{
			printf("<%s> wrong packet type", __FUNCTION__);
		}
	}
	
	return 0;
}
