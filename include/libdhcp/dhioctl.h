#ifndef DHIO
#define DHIO

#include "dhcp.h"

int get_iface_ip(char * iface);
int get_option(struct dhcp_packet * dhc, int option, void * ret_value, int size);
int apply_interface_settings(char * buffer, char * iface);
int set_my_mac(char * iface, unsigned char * mac);
int recv_timeout(int sock, void *buf, int timeout);

char * get_host_from_pack(struct dhcp_packet *dhc); //Rturn host

void print_dhcp_options(struct dhcp_packet *dhc);
void printip(u_int32_t ip); 
void printmac(unsigned char *mac);
void add_log(char *s);
#endif
