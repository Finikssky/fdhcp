#ifndef CORE_H
#define CORE_H

#include "common.h"

#define MAX_INTERFACES 32
#define IFNAMELEN      32

//SERVER

struct dserver_pool_s
{
	struct dserver_pool_s * next;
	ip_address_range_t range;
};

typedef struct dserver_pool_s dserver_pool_t;

struct dserver_dns_s
{
	struct dserver_dns_s * next;
	u_int32_t address;
};

typedef struct dserver_dns_s dserver_dns_t;

struct dserver_router_s
{
	struct dserver_router_s * next;
	u_int32_t address;
};

typedef struct dserver_router_s dserver_router_t;

typedef struct {
	long default_lease_time;
} dserver_settings_global_t;

struct dserver_subnet_s
{
	struct dserver_subnet_s * next;
	struct dserver_subnet_s * prev;
	
	u_int32_t netmask;
	u_int32_t address;
	
	long lease_time;
	
	dserver_router_t  * routers;
	dserver_pool_t    * pools;
	dserver_dns_t     * dns_servers;
	
	int free_addresses;
};

typedef struct dserver_subnet_s dserver_subnet_t;

typedef struct 
{
	dserver_settings_global_t global;
	dserver_subnet_t * subnets;
} dserver_if_settings_t;

typedef struct {
	char                  name[IFNAMELEN];
	int                   enable;
	int                   listen_sock; //нужно ли их 2
	int                   send_sock;
	int                   cci;
	int 				  c_idx;
	pthread_t             listen;
	pthread_t             sender;
	pthread_t             fsm;
	pthread_t             fsm_timer;
	dserver_if_settings_t settings;
} dserver_interface_t;

typedef struct {
	dserver_interface_t interfaces[MAX_INTERFACES];
	char password[128];
	char salt[64];
} DSERVER;

//CLIENT
typedef struct {
	int  dhcp6_enable;
	char server_address[INET_ADDRSTRLEN];
	int  server_port;
	int  client_port;
} dclient_if_settings_t;

typedef struct {
	char                  name[IFNAMELEN];
	int                   enable;
	int                   listen_sock; //нужно ли их 2
	int                   send_sock;
	pthread_t 	          loop_tid;
	dclient_if_settings_t settings;
} dclient_interface_t;

typedef struct {
	dclient_interface_t interfaces[MAX_INTERFACES];
	char password[128];
	char salt[64];
} DCLIENT;


#endif

