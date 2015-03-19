#ifndef DHIO
#define DHIO

#include "dhcp.h"

int get_iface_ip(char * iface);
int set_config(char * buffer, char * iface);
int set_my_mac(char * iface, unsigned char * mac);
int recv_timeout(int sock, void *buf, int timeout);

int get_sip_from_pack(struct dhcp_packet *dhc); //Return ip from DHCP server IP option (54)
int get_rip_from_pack(struct dhcp_packet *dhc); //Return ip from Requested IP option (50)
char * get_host_from_pack(struct dhcp_packet *dhc); //Rturn host

void print_dhcp_options(struct dhcp_packet *dhc);
void printip(u_int32_t ip); 
void printmac(unsigned char *mac);
void add_log(char *s);
long get_lease_time();
#endif
