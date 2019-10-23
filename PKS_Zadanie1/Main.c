#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //winsock library

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <time.h>
#include "Crc16.h"
#include "Client.h"
#include "Server.h"

//#pragma comment(lib,"ws2_32.lib") //winsock library

unsigned short	crc_tab[256];

void PrintStartMsg()
{
	printf("Commands are: \n");
	printf("\tset port xxx \n");
	printf("\tset ip xxx.xxx.xxx.xxx \n");
	printf("\tstart server (need to set port first)\n");
	printf("\tstart client (need to set ip and port first)\n");
	printf("\tallow logging - turns on log printing\n");
	printf("\tquit - ends program\n");

}

int GetCommand(WSADATA wsaData)
{
	char *pointerTmp;
	char myBuffer[255];
	char ipFlag = 0, portFlag = 0, logFlag = 0;
	struct sockaddr_in tmp;

	PrintStartMsg();

	while (1)
	{
		printf("\nEnter command: ");
		fgets(myBuffer, 29, stdin);
		pointerTmp = strtok(myBuffer, " \n");

		if (pointerTmp != NULL && !strcmp(pointerTmp, "set"))
		{
			pointerTmp = strtok(NULL, " \n");
			if (pointerTmp != NULL && !strcmp(pointerTmp, "port"))
			{
				pointerTmp = strtok(NULL, " \n");
				tmp.sin_port = atoi(pointerTmp);
				portFlag = 1;
				printf("Port set to %d", tmp.sin_port);
			}
			else
				if (pointerTmp != NULL && !strcmp(pointerTmp, "ip"))
				{
					pointerTmp = strtok(NULL, " \n");
					tmp.sin_addr.S_un.S_addr = inet_addr(pointerTmp);
					ipFlag = 1;
					printf("IP adress set to %s", inet_ntoa(tmp.sin_addr));
				}
				else
					printf("Wrong command");
		}
		else
		{
			if (pointerTmp != NULL && !strcmp(pointerTmp, "start"))
			{
				pointerTmp = strtok(NULL, " \n");
				if (pointerTmp != NULL && !strcmp(pointerTmp, "server"))
				{
					if (portFlag == 0)
					{
						printf("Set up port first! cmd: set port xxx");
						continue;
					}
					else
					{
						printf("Starting server\n");
						char endMsg = server(tmp,logFlag,wsaData);
						if (endMsg == 1)
						{
							printf("There was a problem with something\n");
						}
						else
							if (endMsg == 2)
							{
								printf("\n\n\n\n\n");
								printf("Connection timed out.\n");
							}
							else
							{
								printf("\n\n\n\n\n");
							}
						return 0;
					}
				}
				else
					if (pointerTmp != NULL && !strcmp(pointerTmp, "client"))
					{
						if (portFlag == 0)
						{
							printf("Set up port first! cmd: set port xxx");
							continue;
						}
						else
							if (ipFlag == 0)
							{
								printf("Set up ip first! cmd: set ip xxx.xxx.xxx.xxx");
								continue;
							}
							else

							{
								printf("Starting client\n");
								//free(myBuffer);
								char endMsg = client(tmp,logFlag,wsaData);
								if (endMsg == 1)
								{
									printf("There was a problem with something\n");
								}
								else
									if (endMsg == 2)
									{
										printf("\n\n\n\n\n");
										printf("Connection timed out.\n");
									}
									else
									{
										printf("\n\n\n\n\n");
									}
								return 0;
							}
					}
					else
						if (pointerTmp != NULL && !strcmp(pointerTmp, "tester"))
						{
							tmp.sin_port = 1000;
							portFlag = 1;
							tmp.sin_addr.S_un.S_addr = inet_addr("192.168.56.1");
							ipFlag = 1;
							logFlag = 1;
							char endMsg = client(tmp,logFlag,wsaData);
							if (endMsg == 1)
							{
								printf("There was a problem with something\n");
							}
							else
								if (endMsg == 2)
								{
									printf("\n\n\n\n\n");
									printf("Connection timed out.\n");
								}
								else
								{
									printf("\n\n\n\n\n");
								}
							//free(myBuffer);
							return 0;
						}
						else
							printf("Wrong command");
			}
			else
				if (pointerTmp != NULL && !strcmp(pointerTmp, "quit"))
				{
					//system("pause");
					//free(myBuffer);
					//WSACleanup();
					printf("VYPINAM\n");
					//exit(0);
					return 2;
					//exit(EXIT_SUCCESS);
				}
				else
					if (pointerTmp != NULL && !strcmp(pointerTmp, "allow"))
					{
							pointerTmp = strtok(NULL, " \n");
							if (pointerTmp != NULL && !strcmp(pointerTmp, "logging"))
							{
								logFlag = 1;
								printf("Log function enabled.\n");
							}
							else
								printf("Wrong command");
					}
					else
						printf("Wrong command");

		}
	}
	//free(myBuffer);

	return 0;
}

int main()
{
	init_crc_tab(); //Priprava pre CRC16 Kermit

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup init problem %d\n", WSAGetLastError());
		return 1;
	}

	while (1)
	{
		if (GetCommand(wsaData) == 2)
			break;
	}

	WSACleanup();

	return 0;
}