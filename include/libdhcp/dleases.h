#ifndef DLEASE_H 
#define DLEASE_H

#include "core.h"

void clear_lease(unsigned char * mac);

void add_lease(char * iface, u_int32_t, u_int32_t, long time);
int get_lease(char * iface, unsigned char * cip, unsigned char * sip);

int s_add_lease(dserver_interface_t * interface, u_int32_t ip, unsigned char * mac, long ltime);

int get_proof(unsigned char * mac, u_int32_t * address);
int try_give_ip(ip_address_range_t * range); // Выбираем адрес из пула и пытаемся назначить
int in_lease(int ip); //Проверяем не в аренде ли адрес
#endif
