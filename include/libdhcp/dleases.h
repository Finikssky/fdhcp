#ifndef DLEASE_H 
#define DLEASE_H
#include "dhcp.h"
#include "common.h"

void clear_lease();

void add_lease(u_int32_t, u_int32_t, long time);
int get_lease(unsigned char * ip, unsigned char * sip);

int s_add_lease(u_int32_t ip, long time, unsigned char * mac, char * host);

int get_proof(struct dhcp_packet *dhc);
int try_give_ip(ip_address_range_t * range); // Выбираем адрес из пула и пытаемся назначить
int in_lease(int ip); //Проверяем не в аренде ли адрес
#endif
