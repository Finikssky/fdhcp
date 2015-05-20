

#ifndef DCTP_H
#define DCTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>

#define DCTP_DEBUG 0

#define DCTP_COMMAND_MAX_LEN 128
#define DCTP_ARG_MAX_LEN 128
#define DCTP_REPLY_TIMEOUT 5

#define DCTP_MSG_COMM 1
#define DCTP_MSG_RPL 2
#define DCTP_MSG_CFG 3

#define DCL_DCTP_PORT 39969
#define DSR_DCTP_PORT 39970
#define DCTP_LABEL 42
#define DCTP_ERROR_DESC_SIZE 512

typedef enum {
	UNDEF_COMMAND,
	CL_DEF_SERVER_ADDR,
	CL_SET_SERVER_ADDR,
	CL_SET_IFACE_ENABLE,
	CL_SET_IFACE_DISABLE,
	SR_SET_IFACE_ENABLE,
	SR_SET_IFACE_DISABLE,
	SR_ADD_SUBNET,
	SR_DEL_SUBNET,
	SR_ADD_POOL,
	SR_DEL_POOL,
	SR_ADD_DNS,
	SR_DEL_DNS,
	SR_ADD_ROUTER,
	SR_DEL_ROUTER,
	SR_SET_LEASETIME,
	SR_SET_HOST_NAME,
	SR_SET_DOMAIN_NAME,
	DCTP_CL_SYNC,
	DCTP_SR_SYNC,
	DCTP_PING,
	DCTP_PASSWORD,
	DCTP_SAVE_CONFIG,
	DCTP_GET_CONFIG,
	DCTP_END_WORK,
} DCTP_cmd_code_t;

typedef struct{
	char text[255];
	DCTP_cmd_code_t code;
} DCTP_command_t;

DCTP_command_t DCTP_cmd_list[] = {
	//--client--
	{ "cl_set_server_addr",   CL_SET_SERVER_ADDR },
	{ "cl_def_server_addr",   CL_DEF_SERVER_ADDR },
	{ "cl_set_iface_enable",  CL_SET_IFACE_ENABLE },
	{ "cl_set_iface_disable", CL_SET_IFACE_DISABLE },
	//--server--
	{ "sr_set_iface_enable",  SR_SET_IFACE_ENABLE },
	{ "sr_set_iface_disable", SR_SET_IFACE_DISABLE },
	{ "sr_add_subnet",        SR_ADD_SUBNET },
	{ "sr_del_subnet",        SR_DEL_SUBNET }, 
	{ "sr_add_pool",          SR_ADD_POOL }, 
	{ "sr_del_pool",          SR_DEL_POOL },
	{ "sr_add_dns",           SR_ADD_DNS }, 
	{ "sr_del_dns",           SR_DEL_DNS },
	{ "sr_add_router",        SR_ADD_ROUTER }, 
	{ "sr_del_router",        SR_DEL_ROUTER },
	{ "sr_set_leasetime",     SR_SET_LEASETIME},
	{ "sr_set_host_name",     SR_SET_HOST_NAME},
	{ "sr_set_domain_name",   SR_SET_DOMAIN_NAME},
	
	//--system--
	{ "dctp_cl_sync",         DCTP_CL_SYNC },
	{ "dctp_sr_sync",         DCTP_SR_SYNC },
	{ "dctp_ping" ,           DCTP_PING },
	{ "dctp_password" ,       DCTP_PASSWORD },
	{ "dctp_save_config",     DCTP_SAVE_CONFIG }, 
	{ "dctp_stop",            DCTP_END_WORK },
	{ "",                     UNDEF_COMMAND},
};

typedef enum
{
	DCTP_SUCCESS,
	DCTP_REPEAT,
         DCTP_UPDATE_CONFIG,
	DCTP_FAIL,
} DCTP_STATUS;

typedef struct
{
	int label;
	int type;
	int csum;
	int id;
	int size;
} DCTP_PACKET;

typedef struct 
{
        DCTP_cmd_code_t code;
        char interface[32];
        char arg[DCTP_ARG_MAX_LEN];
} DCTP_COMMAND;

typedef struct
{
  DCTP_PACKET packet;
  DCTP_COMMAND payload;
} DCTP_COMMAND_PACKET;

typedef struct 
{
    DCTP_STATUS status;
    char error[DCTP_ERROR_DESC_SIZE];
} DCTP_REPLY;
typedef struct
{
    DCTP_PACKET packet;
    DCTP_REPLY payload;
} DCTP_REPLY_PACKET;

typedef enum
{
    DCTP_FILE_START,
    DCTP_FILE_MBLOCK,
    DCTP_FILE_END,
    DCTP_FILE_ONCE,
} DCTP_FILE_TYPE;

typedef struct
{
    DCTP_FILE_TYPE block_type;
    int block_size;
    char block[512];
} DCTP_FILE_BLOCK;

typedef struct
{
  DCTP_PACKET packet;
  DCTP_FILE_BLOCK payload;
} DCTP_FILE_PACKET;

int receive_DCTP_command(int sock, DCTP_COMMAND_PACKET * pack, struct sockaddr_in * sender);
int send_DCTP_COMMAND(int sock, DCTP_COMMAND command, char * ip, int port);
void send_DCTP_REPLY(int sock, DCTP_PACKET * in, DCTP_STATUS status, struct sockaddr_in * sender, char * error_string);
int receive_DCTP_reply(int sock, DCTP_REPLY_PACKET * pack);

int send_DCTP_CONFIG( int sock, const char * filename, struct sockaddr_in * sender);
int receive_DCTP_CONFIG(int sock, char * filename);


int init_DCTP_socket(int port);
int release_DCTP_socket(int sock);

DCTP_cmd_code_t parse_DCTP_command (DCTP_COMMAND * in, char * ifname );

#ifdef __cplusplus
}
#endif

#endif 


