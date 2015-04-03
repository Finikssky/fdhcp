#ifndef DHCONN_H
#define DHCONN_H

int init_packet_sock(char *ethName, u_int16_t protocol);
int sendDHCP(int sock, frame_t * frame, int size);
int recvDHCP(int sock, char * iface, frame_t * frame, int bootp_type, int dhc_type, u_int32_t transid, int timeout);

int sendARP(int sock, char * iface, u_int32_t ip);
int recvARP(int sock, char * iface, u_int32_t ip);

void * s_recvDHCP(void *);

#endif


