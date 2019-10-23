#include "Server.h"

//static char end = 0;
//static char waitFlag = 0;
//static char threadDone = 0;
/*
typedef struct ThreadStruc
{
	SOCKET socketVar;
	int socLen;
	struct sockaddr_in client;
	char logFlag;
}threadStruc;*/

static int SendFragment(SOCKET socketVar, char* string, int size,struct sockaddr_in client, int socLen,char logFlag)
{
	if (sendto(socketVar, string, size, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
	{
		if(logFlag) printf("SendTo fail - %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

/*
DWORD WINAPI ContinueMsg(threadStruc *myStruc)
{
	SOCKET socketVar = myStruc->socketVar;
	int socLen = myStruc->socLen;
	struct sockaddr_in client = myStruc->client;
	char logFlag = myStruc->logFlag;
	char tmpAnswer[12];
	char maxLimit = 35;

	while (waitFlag)
	{
		if (recvfrom(socketVar, tmpAnswer, 12, 0, (struct sockaddr*)&client, &socLen) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == 10060)
			{
				if (maxLimit > 0)
				{
					//printf("Maxlimit --\n");
					maxLimit--;
					continue;
				}
				if (logFlag) printf("\nCONNECTION TIMEOUT\n");
				end = 1;
				waitFlag = 0;
				threadDone = 1;
				return 0;
			}
			else
			{
				if(logFlag)	printf("Recvfrom problem %d\n", WSAGetLastError());
				waitFlag = 0;
				threadDone = 1;
				return 0;
			}
		}

		if (*(tmpAnswer + 2) == 0) //Prislo keepalive
		{
			if (logFlag) printf("\nKEEP ALIVE arrived\n");

			maxLimit = 35;

			*(tmpAnswer + 2) = 0;
			*(unsigned int*)((char*)tmpAnswer + 3) = 0;
			strcpy(((char*)tmpAnswer + 7), "_ack\0");

			*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

			if (SendFragment(socketVar, tmpAnswer, 12, client, socLen, logFlag)) return 0;

			continue;
		}
	}

	waitFlag = 0;
	threadDone = 1;
	return 0;
}
*/


char server(struct sockaddr_in tmp, char logFlag,WSADATA wsaData)
{
	//WSADATA wsaData;
	SOCKET socketVar;
	int socLen;
	char *buffer, *tmpChar, *tmpMsg, *tmpAnswer;
	char imgFlag = 0;
	struct sockaddr_in server, client;
	HANDLE threadHandle = NULL;

	//SPECIAL VALUES:
	unsigned int fragCount, fragAck;
	short fragSize, lastFragSize, flag;
	unsigned short	checksum;

	//waitFlag = 0;
	//end = 0;

	if (logFlag) printf("LOGGING ALLOWED\n");

#pragma region load Winsock + connection

	FILE *file = NULL;

	socLen = sizeof(client);

/*	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		if(logFlag) printf("WSAStartup init problem %d\n", WSAGetLastError());
		return 1;
	}*/

	if ((socketVar = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		if(logFlag) printf("socket problem %d\n", WSAGetLastError());
		return 1;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(tmp.sin_port);

	if (bind(socketVar, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		if(logFlag) printf("bind problem %d\n", WSAGetLastError());
		return 1;
	}

	/*threadStruc *myStruc = (threadStruc*)malloc(sizeof(myStruc));
	myStruc->socketVar = socketVar;
	myStruc->socLen = socLen;
//	myStruc->client = client;
	myStruc->logFlag = logFlag;*/
#pragma endregion

	while (1)
	{

		/*if (threadHandle != NULL)
		{
			while (waitFlag || !threadDone) {};
			int timeout = 40000;
			if (setsockopt(socketVar, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR)
			{
				if (logFlag) printf("set timeout problem %d\n", WSAGetLastError());
			}
			threadHandle = NULL;
		}
		if (end)
		{
			closesocket(socketVar);
			//WSACleanup();
			return 0;
		}*/


		printf("\nStarting new listening\n");
#pragma region RECIEVE_FIRST_FRAGMENT
		//printf("RECIEVING FIRST FRAGMENT\n");
		imgFlag = 0;
		flag = 1;
		buffer = (char*)malloc(256);
		tmpAnswer = (char*)malloc(12);
		tmpChar = (char*)malloc(241);
		while (flag)
		{
			if (recvfrom(socketVar, buffer, 256, 0, (struct sockaddr*)&client, &socLen) == SOCKET_ERROR)
			{
				if (WSAGetLastError() == 10060)
				{
					if (logFlag) printf("\nCONNECTION TIMEOUT\n");
					/*if (threadHandle != NULL)
					{
						TerminateThread(threadHandle, 0);
						threadHandle = NULL;
					}*/
					closesocket(socketVar);
					//WSACleanup();

					/*if (end) 
						printf("END IS TRUE!\n");*/

					return 2;
				}

				else
				{
					if (logFlag) printf("recvfrom problem %d\n", WSAGetLastError());
					return 1;
				}
			}


			if (*(buffer + 2) == 2) //Ak prisli zabudnute data odoslat ACK
			{
				if(logFlag) printf("Fragment %d arrived. Sending ACK\n", *(unsigned int*)((char*)buffer + 3));
				*(tmpAnswer + 2) = 2;
				*(unsigned int*)((char*)tmpAnswer+3) = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 7), "_ack\0");

				*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

				if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;

				continue;
			}
			else
			{
				if (*(buffer + 2) == 0) //Prislo keepalive
				{
					if(logFlag) printf("\nKEEP ALIVE arrived\n");

					*(tmpAnswer + 2) = 0;
					*(unsigned int*)((char*)tmpAnswer + 3) = 0;
					strcpy(((char*)tmpAnswer + 7), "_ack\0");

					*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

					if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;

					continue;
				}
			}


			//printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			checksum = *(unsigned short*)(buffer);
			unsigned short checksum2 = crc_kermit((const unsigned char*)buffer + 2, 254);

			if (checksum != checksum2)
			{
				if(logFlag) printf("Checksum is different\n"); 
				if (*(buffer + 2) == 1)
				{
					*(tmpAnswer + 2) = 1;
					*(unsigned int*)((char*)tmpAnswer + 3) = 0;
					strcpy(((char*)tmpAnswer + 7), "nack\0");

					*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

					if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;

					continue;

				}
			}

			else
			{
				//printf("Checksum OK!\n");

				flag = 0;

				fragSize = *(short*)((char*)buffer + 7);
				

				fragCount = *(unsigned int*)((char*)buffer + 3);
				


				lastFragSize = *(short*)((char*)buffer + 9);
				

				int tmpLen = 0;

				sscanf(buffer + 11, "%s", tmpChar);
				
				tmpLen += strlen(tmpChar);
				tmpLen++;//lebo medzera

				if (logFlag)
				{
					printf("Velkost fragmentov je %d\n", fragSize);
					printf("Pocet fragmentov je %u\n", fragCount);
					printf("LastFragSize = %hd\n", lastFragSize);
					printf("Typ spravy je %s\n", tmpChar);
				}

				if (!strcmp(tmpChar, "img"))
				{
					imgFlag = 1;
					sscanf(buffer + 15, "%s", tmpChar);
					if(logFlag) printf("Nazov suboru je %s\n", tmpChar);
					tmpLen += strlen(tmpChar);
					tmpLen++; //lebo medzery

					if ((file = fopen(tmpChar, "wb")) == NULL)
					{
						if(logFlag) printf("fopen failed\n");
						return 1;
					}

				}
			}

			*(tmpAnswer + 2) = 1;
			*(unsigned int*)((char*)tmpAnswer + 3) = 0;
			strcpy(((char*)tmpAnswer + 7), "_ack\0");

			*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

			if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;

		}
		free(buffer);
#pragma endregion


		//LOAD VARIABLES
		tmpMsg = malloc(fragSize*fragCount);
		fragAck = 0;
		flag = 1;
		int errorFlag = 0;
		buffer = (char*)malloc(fragSize);

		//Keep listening for data
		while (flag)
		{

			if (recvfrom(socketVar, buffer, fragSize, 0, (struct sockaddr*)&client, &socLen) == SOCKET_ERROR)
			{
				if (WSAGetLastError() == 10040) continue;
				
				if (WSAGetLastError() == 10060) 
				{
					if (logFlag) printf("\nCONNECTION TIMEOUT\n");
					closesocket(socketVar);
					//WSACleanup();
					return 2;
				}
				if(logFlag) printf("recvfrom problem %d\n", WSAGetLastError());
				return 1;

			}


			if (*(unsigned short*)buffer != crc_kermit(buffer + 2, fragSize - 2))
			{
				if(logFlag) printf("Fragment %d wrong CRC - SENDING NACK\n", *(unsigned int*)((char*)buffer + 3));

				*(tmpAnswer + 2) = 2;
				*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 7), "nack\0");

				*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

				if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;

				continue;
			}
			if (*(unsigned int*)((char*)buffer + 3) > (fragAck + 1))
			{
				if(logFlag) printf("DISCARDING - fragment %d is too far ahead\n", *(unsigned int*)((char*)buffer + 3));
				continue;
			}

			if (*(unsigned int*)((char*)buffer + 3) < (fragAck + 1))
			{
				if(logFlag) printf("DISCARDING - fragment %d already recieved\n", *(unsigned int*)((char*)buffer + 3));

				*(tmpAnswer + 2) = 2;
				*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 7), "_ack\0");

				*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

				if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;

				continue;
			}

			if (logFlag) printf("Fragment %d arrived, sending ACK\n", *(unsigned int*)((char*)buffer + 3));

			*(tmpAnswer + 2) = 2;
			*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
			strcpy(((char*)tmpAnswer + 7), "_ack\0");

			*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);


			if (file != NULL)
			{
				if (fragAck + 1 == fragCount)
				{
					fwrite(buffer + 7, 1, lastFragSize, file);
				}
				else
				{
					fwrite(buffer + 7, 1, fragSize - 7, file);
				}
			}
			else
			{
				if (fragAck + 1 == fragCount)
				{
					//printf("Data: %s\n", buffer + 7);
					strncpy(tmpMsg + ((fragAck)*(fragSize - 7)), buffer + 7, lastFragSize);
				}
				else
				{
					//printf("Data: %s\n", buffer + 7);
					strncpy(tmpMsg + ((fragAck)*(fragSize - 7)), buffer + 7, fragSize - 7);
				}
			}

			fragAck++;

			/*if ((fragAck + 1) == 15 && errorFlag)
			{
				errorFlag = 0;
			}
			else
			{*///if(fragAck != 10)
				if (SendFragment(socketVar, tmpAnswer, 12, client, socLen,logFlag)) return 1;
			//}

			if (fragAck == fragCount)
			{
				char *path = (char*)malloc(255);
				//printf("All fragments arrived, I should end\n");
				if (!imgFlag)
				{
					printf("Final msg:\n%.*s\n", (fragAck - 1)*(fragSize - 7) + lastFragSize, tmpMsg);
				}
				else
				{
					_fullpath(path, tmpChar, 255);

					/*
					//strcpy(fileName, path);
					strcpy(path, __FILE__);

					int j;
					for (j = strlen(path) - 1; j > 0; j--)
					{
						if (path[j] == '\\')
						{
							break;
						}
					}

					strcpy(path + j + 1, tmpChar);*/

					printf("File path is: %s\n", path);

					if (fclose(file) == EOF)
					{
						if(logFlag) printf("File can`t be closed.\n");
						return 1;
					}
					file = NULL;					
				}
				free(tmpChar);
				free(tmpMsg);
				free(path);
				flag = 0;
			}

		}

		int timeout = 40000;
		if (setsockopt(socketVar, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR)
		{
			if(logFlag) printf("set timeout problem %d\n", WSAGetLastError());
		}

		//myStruc->client = client;

		//waitFlag = 1;
		//threadDone = 1;
	//	threadHandle = CreateThread(NULL, 0, ContinueMsg, myStruc, 0, NULL);

		printf("Do you want to continue? y/n ");
		
		if (getchar() == 'n')
		{
			getchar();
			//waitFlag = 0;
			//while (!threadDone) {};
			/*if (threadHandle != NULL)
			{
				TerminateThread(threadHandle, 0);
				threadHandle = NULL;
			}*/
			printf("CLOSING\n");
			closesocket(socketVar);
			/*if (WSACleanup() != 0)
			{
				printf("WSA problem \n");
			}*/
			/*if (end)
				return 2;
			else*/
				return 0;
		}
		else
		{
			getchar();
			//waitFlag = 0;
			//while (!threadDone) {};
			/*if (threadHandle != NULL)
			{
				TerminateThread(threadHandle, 0);
				threadHandle = NULL;
			}*/
			/*int timeout = 40000;
			if (setsockopt(socketVar, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR)
			{
				if (logFlag) printf("set timeout problem %d\n", WSAGetLastError());
			}
			if (end)
			{
				closesocket(socketVar);
				//WSACleanup();
				return 2;
			}*/
		}
		

	}


	closesocket(socketVar);
	//WSACleanup();
	return 0;

}
