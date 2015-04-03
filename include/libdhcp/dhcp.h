#ifndef DHCP_H
#define DHCP_H

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h> 
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>

#include <linux/if_ether.h>

#include <errno.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <semaphore.h>

#define T_RENEWING 5
#define T_REBINDING 6
#define T_END 7

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#define BOOTP_REQUEST 1
#define BOOTP_REPLY   2

#define FULLHEAD_LEN        sizeof(struct ethhdr) + sizeof(struct ip) + sizeof(struct udphdr)

#define DHCP_UDP_OVERHEAD	(20 + /* IP header */			\
							 8)   /* UDP header */
#define DHCP_SNAME_LEN		64
#define DHCP_FILE_LEN		128
#define DHCP_FIXED_NON_UDP	236
#define DHCP_FIXED_LEN		(DHCP_FIXED_NON_UDP + DHCP_UDP_OVERHEAD)
						/* Everything but options. */
#define DHCP_MTU_MAX		1500
#define DHCP_MTU_MIN 		576

#define DHCP_MAX_OPTION_LEN       (DHCP_MTU_MAX - DHCP_FIXED_NON_UDP - FULLHEAD_LEN)
#define DHCP_MIN_OPTION_LEN       (DHCP_MTU_MIN - DHCP_FIXED_LEN)
#define DHCP_FULL_WITHOUT_OPTIONS (DHCP_FIXED_NON_UDP + FULLHEAD_LEN)

#define DHCPDISCOVER		1
#define DHCPOFFER			2
#define DHCPREQUEST			3
#define DHCPDECLINE			4
#define DHCPACK				5
#define DHCPNAK				6
#define DHCPRELEASE			7
#define DHCPINFORM			8
#define DHCPLEASEQUERY		10
#define DHCPLEASEUNASSIGNED	11
#define DHCPLEASEUNKNOWN	12
#define DHCPLEASEACTIVE		13

int LASTRANDOM;

struct dhcp_packet 
{
	u_int8_t  op;		/* 0: Message opcode/type */
	u_int8_t  htype;	/* 1: Hardware addr type (net/if_types.h) */
	u_int8_t  hlen;		/* 2: Hardware addr length */
	u_int8_t  hops;		/* 3: Number of relay agent hops from client */
	u_int32_t xid;		/* 4: Transaction ID */
	u_int16_t secs;		/* 8: Seconds since client started looking */
	u_int16_t flags;	/* 10: Flag bits */
	struct in_addr ciaddr;	/* 12: Client IP address (if already in use) */
	struct in_addr yiaddr;	/* 16: Client IP address */
	struct in_addr siaddr;	/* 18: IP address of next server to talk to */
	struct in_addr giaddr;	/* 20: DHCP relay agent IP address */
	unsigned char chaddr [16];	/* 24: Client hardware address */
	char sname [DHCP_SNAME_LEN];	/* 40: Server name */
	char file [DHCP_FILE_LEN];	/* 104: Boot filename */
	unsigned char options [DHCP_MAX_OPTION_LEN];
				/* 212: Optional parameters (actual length dependent on MTU). */
};

struct arp_packet 
{
	unsigned short  hardware;              /* тип транспортного протокола передачи данных, для Ethernet ARPHRD_ETHE==1 -- из файла <net/if_arp.h> */
	unsigned short  arp_protocol;          /* ETH_P_IP  -- из файла <linux/if_ether.h>*/
	unsigned char   arp_hard_addr_len;      /* размер mac адреса в байтах */
	unsigned char   arp_prot_addr_len;      /* размер ip адреса в байтах */
	unsigned short  arp_operation; /* тип arp пакета: запрос ARPOP_REQUEST==1 или ответ ARPOP_REPLY==2 -- из файла <net/if_arp.h>*/
	unsigned char   arp_mac_source[ETH_ALEN];/*  mac адрес получателя пакета */
	in_addr_t       arp_ip_source;
	unsigned char   arp_mac_target[6];       /* mac адрес отправителя пакета */
	in_addr_t       arp_ip_target;
} __attribute__ ((packed)) ;


struct ethheader
{
	unsigned char dmac[ETH_ALEN];
	unsigned char smac[ETH_ALEN];
	unsigned short type;
}  __attribute__ ((packed));

struct frame
{
	struct ethheader    h_eth;
	struct ip           h_ip;
	struct udphdr       h_udp;
	struct dhcp_packet  p_dhc;
	int                 d_size;
	int                 size;
};

typedef struct frame frame_t;

struct dhcp_lease
{
	u_int32_t cip;
	u_int32_t sip;	
	long stime;
	long ltime;
};

struct s_dhcp_lease
{
	u_int32_t     ip;//Выданный адрес
	long 		  stime;		  //Время выдачи
	long 		  ltime;		  //Срок аренды
	unsigned char haddr[ETH_ALEN]; //MAC
};

u_int32_t create_packet(char * iface, frame_t * frame, int btype, int dtype, void * arg);
void create_arp(char * iface, char * buffer, int ip, unsigned char * macs, unsigned char * macd, int oper);
void create_ipheader(frame_t * frame, int srcip, int destip);
void create_udpheader(frame_t * frame, int srcport, int destport);
void create_ethheader(frame_t * frame, unsigned char * smac, unsigned char * dmac, u_int16_t proto);

int send_answer(void * info, void * iface);
int send_offer(void * info, void * iface);
int send_nak(void * info, void * iface);

#endif
