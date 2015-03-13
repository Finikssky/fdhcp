#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int ip_address_range_parse(const char * range_str, ip_address_range_t * range)
{
	if (NULL == range_str) return -1;
	if (NULL == range)     return -1;

	char * saveptr;
	char * tokens = strdup((char *)range_str);
	char * start_address = strtok_r(tokens, "-", &saveptr);
	char * end_address = strtok_r(NULL, "-", &saveptr);
	
	printf("parse ip range: %s / %s %s\n", range_str, start_address, end_address);
	
	if (NULL == start_address) 
	{
		printf("<%s> no start address\n", __FUNCTION__);
		free(tokens);
		return -1;
	}

	if (1 != inet_pton(AF_INET, start_address, &range->start_address)) 
	{
		printf("<%s> inet_pton_fail start address\n", __FUNCTION__);
		free(tokens);
		return -1;
	}

	if (NULL != end_address) 
	{
		if (1 != inet_pton(AF_INET, end_address, &range->end_address)) 
		{
			printf("<%s> inet pton fail end address\n", __FUNCTION__);
			free(tokens);
			return -1;
		}
	} 
	else 
	{
		range->end_address = range->start_address;
	}

	if (htonl(range->start_address) > htonl(range->end_address)) 
	{
		printf("<%s> left > right\n", __FUNCTION__);
		free(tokens);
		return -1;
	}

	free(tokens);
	return 0;
}

int ip_address_range_is_eq(ip_address_range_t * left, ip_address_range_t * right)
{
	if (left->start_address == right->start_address && 
		left->end_address == right->end_address)
			return 1;
	return 0;
}

int ip_address_range_is_overlap(ip_address_range_t * left, ip_address_range_t * right)
{
		if (htonl(left->start_address) >= htonl(right->start_address) && 
		htonl(left->start_address) <= htonl(right->end_address))
			return 1;
			
	if (htonl(left->end_address) >= htonl(right->start_address) && 
		htonl(left->end_address) <= htonl(right->end_address))
			return 1;
	
	if (htonl(right->start_address) >= htonl(left->start_address) && 
		htonl(right->start_address) <= htonl(left->end_address))
			return 1;
	
	if (htonl(right->end_address) >= htonl(left->start_address) && 
		htonl(right->end_address) <= htonl(left->end_address))
			return 1;
			
	return 0;
}

