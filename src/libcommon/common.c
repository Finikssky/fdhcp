#include "libcommon/common.h"

#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

int t_gets(FILE * stream, char end_simbol, char * out, int size, int start)
{
	if ( out == NULL )  return -1;
	if ( feof(stream) ) return EOF;
	if ( start < 0 )    return -1;
	
	if ( stream == stdin ) __fpurge(stream);
	int i;
	char last = ' ';
	
	for( i = start; i < (size - 1); i++ )
	{	
		char c = fgetc(stream);
		if (c == '\t') c = ' ';
		if (c == '\n' || c == EOF || (end_simbol != 0 && c == end_simbol)) 
		{
			if (i > 0 && out[ i - 1 ] == ' ') out[ i - 1] = '\0';
			if (c == EOF) return EOF;
			break;
		}
		else 
			if ( last != ' ' || c != ' ') 
				out[i] = c;
			else
				i--;
		
		last = c;
	}
	
	out[i] = '\0';
	
	if ( stream == stdin ) __fpurge(stream);
	return 0;
}

void generate_salt(char * salt, int size)
{
	int i = 0;
	for (; i < size; i++ )
	{
		salt[i] = 48 + rand()%100;
	}
	salt[i - 1] = '\0';
}

void generate_hash(char * password, int psize, char * salt, int ssize, char * hash, int hsize)
{
	int i = 0;
	for( ; i < (hsize - 1); i++)
	{
		int a = (password[i] * i + salt[i]);
		int b = (salt[i] * i + password[i]);
		
		hash[i] = 48 + (a + b) % 100;
		while (hash[i] == '\0') hash[i]++;
	}
	hash[i] = '\0';
}

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

int ip_address_range_have_address(ip_address_range_t * range, u_int32_t  * address)
{
	printf("<%s>\n", __FUNCTION__);
	if (htonl(*address) <= htonl(range->end_address) && htonl(*address) >= htonl(range->start_address)) return 1;
	return 0;
}
