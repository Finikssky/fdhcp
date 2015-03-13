#ifndef DHCONN_H
#define DHCONN_H

#include "dhcp.h"

int init_packet_sock(char *ethName, u_int16_t protocol);
int sendDHCP(int sock, char * iface, void * buffer, int len);
int recvDHCP(int sock, char * iface, void * buffer, int type, u_int32_t transid);

int sendARP(char *iface, char *buffer);
int recvARP(char *iface);

void* s_recvDHCP(void*);

#endif


