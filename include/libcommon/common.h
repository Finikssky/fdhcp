#ifndef DCOMMON_H
#define DCOMMON_H

#include <sys/types.h>

typedef struct
{
	u_int32_t start_address;
	u_int32_t end_address;
} ip_address_range_t;

int ip_address_range_parse(const char * range_str, ip_address_range_t * range);
int ip_address_range_is_eq(ip_address_range_t * left, ip_address_range_t * right);
int ip_address_range_is_overlap(ip_address_range_t * left, ip_address_range_t * right);
int ip_address_range_have_address(ip_address_range_t * range, u_int32_t  * address);

void generate_hash(char * password, int psize, char * salt, int ssize, char * hash, int hsize);
void generate_salt(char * salt, int size);

#endif