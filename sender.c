#include "dctp.h"
#include <string.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

char REMOTE_IP[32];
char IFACE[32];
int  DCTP_socket;

#define T_S 1
#define T_C 2

void start_menu();
void location_menu(char * type);
void comline(char * type);

int my_gets(char * out, int size)
{
	__fpurge(stdin);
	int i = 0;
	for( i = 0; i < (size - 1); i++ )
	{	
		char c = getchar();
		if (c == '\n') 
			break;
		else 
			out[i] = c;
	}
	
	out[i] = '\0';
	
	__fpurge(stdin);
	return 0;
}

void comline(char * type)
{
	char temp[255];
	
	char password[64];
	char salt[64];
	char hash[128];
	
	DCTP_COMMAND command;
	
	printf("please enter password: ");
	my_gets(password, sizeof(password)); //TODO  функция взятия пароля
	
	memset(&command, 0, sizeof(DCTP_COMMAND));
	snprintf(command.name, sizeof(command.name), "dctp_password");
	snprintf(command.arg, sizeof(command.arg), password);
	
	if (-1 == send_DCTP_COMMAND(DCTP_socket, command, REMOTE_IP, strstr(type, "server") ? DSR_DCTP_PORT : DCL_DCTP_PORT ))
	{
		printf("Incorrect password!\n");
		getchar();
		system("clear");
		location_menu(type);
	}
	
	printf("please choise interface: ");
	my_gets(IFACE, sizeof(IFACE));
	while(1)
	{
		memset(&command, 0, sizeof(DCTP_COMMAND));
		memset(temp, 0, sizeof(temp));
				
		printf("insert command or exit > ");
		my_gets(temp, sizeof(temp));
		snprintf(command.name, sizeof(command.name), "%s %s_%s", IFACE, strstr(type,"server") ? "sr" : "cl", temp);
		
		if (strcasestr(temp, "exit")) break;
		else
		{
			printf("insert arg > ");
			my_gets(command.arg, sizeof(command.arg));
		}
		
		if (-1 == send_DCTP_COMMAND(DCTP_socket, command, REMOTE_IP, strstr(type, "server") ? DSR_DCTP_PORT : DCL_DCTP_PORT ))
		{
			printf("sorry, but %s-core don't answer, press key to continue\n", type);
			getchar();
			system("clear");
			location_menu(type);
		}
	}
	location_menu(type);
}

void location_menu(char * type)
{
	char choise[255];
	
	printf("Select %s-core location:\n", type);
	printf("1. Local\n");
	printf("2. Remote\n");
	printf("3. Exit\n");
	printf("> ");
	
	scanf("%s", choise);
	if (strcasestr(choise, "local") || strstr(choise, "1"))
	{
		strcpy(REMOTE_IP, "127.0.0.1");
	} 
	else if (strcasestr(choise, "remote") || strstr(choise, "2"))
	{
		printf("insert remote %s-core ip > ", type);
		scanf("%s", REMOTE_IP);
	}
	else if (strcasestr(choise, "exit") || strstr(choise, "3"))
	{
		system("clear");
		start_menu();
	}
	else
	{
		system("clear");
		location_menu(type);
	}
		
		
	DCTP_COMMAND command;
	memset(&command, 0, sizeof(command));
	strcpy(command.name, "dctp_ping");
	if (-1 == send_DCTP_COMMAND(DCTP_socket, command, REMOTE_IP, strstr(type, "server") ? DSR_DCTP_PORT : DCL_DCTP_PORT))
	{
		printf("sorry, but %s-core don't answer\n", type);
		usleep(1000000);
		system("clear");
		location_menu(type);
	}
	else
	{
		printf("%s-core succesful connected\n", type);
		comline(type);
	}
	
}

void start_menu()
{
	char choise[255];
	printf("First, set core to setup:\n");
	printf("1. Client\n");
	printf("2. Server\n");
	printf("3. Exit\n");
	printf("> ");
	scanf("%s", choise);
	
	if (strcasestr(choise, "server") || strstr(choise, "2")) location_menu("server");
	else if (strcasestr(choise, "client") || strstr(choise, "1")) location_menu("client");
	else if (strcasestr(choise, "exit") || strstr(choise, "3")) exit(0);
	else 
	{	
		system("clear");
		start_menu();
	}
}

int main()
{
	srand(time(NULL));
	DCTP_socket = init_DCTP_socket(rand()%65534);

	printf("Hello, this is console utility for setup DHCP service by DCTP protocol.\n\n");
	
	start_menu();
	
	release_DCTP_socket(DCTP_socket);
	return 0;
}
