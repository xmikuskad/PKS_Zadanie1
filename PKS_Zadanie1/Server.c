#include "Server.h"

static int SendPacket(SOCKET socketVar, char* string, int size,struct sockaddr_in client, int socLen)
{
	if (sendto(socketVar, string, size, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
	{
		printf("SendTo fail - %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int server(struct sockaddr_in tmp)
{
	WSADATA wsaData;
	SOCKET socketVar;
	int socLen, flag,lastFragSize;
	char *buffer, *tmpChar, *tmpMsg, *tmpAnswer;
	char imgFlag = 0;
	struct sockaddr_in server, client;

	//SPECIAL VALUES:
	unsigned int fragCount, fragAck;
	short fragSize;
	unsigned short	checksum;

	//if (logFlag) printf("TURNED ON!\n");

#pragma region load Winsock + connection

	FILE *file = NULL;

	socLen = sizeof(client);

	printf("Initializing server\n");

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSA initialization failed, code %d\n", WSAGetLastError());
		return 1;
	}

	if ((socketVar = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("Socket failed, code %d\n", WSAGetLastError());
		return 1;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(tmp.sin_port);

	printf("IP adress is: %s\n", inet_ntoa(server.sin_addr));

	if (bind(socketVar, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Binding failed, code %d\n", WSAGetLastError());
		return 1;
	}
#pragma endregion

	while (1)
	{
#pragma region RECIEVE_FIRST_FRAGMENT
		printf("\nRECIEVING FIRST FRAGMENT\n");
		imgFlag = 0;
		flag = 1;
		buffer = (char*)malloc(256);
		tmpAnswer = (char*)malloc(12);
		while (flag)
		{
			if (recvfrom(socketVar, buffer, 256, 0, (struct sockaddr*)&client, &socLen) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != 10060)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					system("pause");
					return 1;
				}
				else
				{
					printf("\nCONNECTION TIMEOUT\n");
					system("pause");
					return 2;
				}

			}

			if (*(buffer + 2) == 2) //Ak prisli zabudnute data odoslat ACK
			{
				printf("Fragment %d arrived. Sending ACK\n", *(unsigned int*)((char*)buffer + 3));
				*(tmpAnswer + 2) = 2;
				*(unsigned int*)((char*)tmpAnswer+3) = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 7), "_ack\0");

				*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

				if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;

				continue;
			}
			else
			{
				if (*(buffer + 2) == 0) //Prislo keepalive
				{
					printf("KEEP ALIVE arrived\n");
					*(tmpAnswer + 2) = 0;
					*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
					strcpy(((char*)tmpAnswer + 7), "_ack\0");

					*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

					if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;

					continue;
				}
			}

			//printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			checksum = *(unsigned short*)(buffer);
			unsigned short checksum2 = crc_kermit((const unsigned char*)buffer + 2, 254);

			if (checksum != checksum2)
			{
				printf("Checksum is different :(\n"); 
				if (*(buffer + 2) == 1)
				{
					*(tmpAnswer + 2) = 1;
					*(unsigned int*)((char*)tmpAnswer + 3) = 0;
					strcpy(((char*)tmpAnswer + 7), "nack\0");

					*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

					if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;

					continue;

				}
			}

			else
			{
				printf("Checksum OK!\n");

				flag = 0;

				fragSize = *(short*)((char*)buffer + 7);
				printf("Velkost fragmentov je %d\n", fragSize);

				fragCount = *(unsigned int*)((char*)buffer + 3);
				printf("Pocet fragmentov je %u\n", fragCount);


				lastFragSize = *(short*)((char*)buffer + 9);
				printf("LastFragSize = %d\n", lastFragSize);

				int tmpLen = 0;
				tmpChar = (char*)malloc(241);

				sscanf(buffer + 11, "%s", tmpChar);
				printf("Typ spravy je %s\n", tmpChar);
				tmpLen += strlen(tmpChar);
				tmpLen++;//lebo medzera

				if (!strcmp(tmpChar, "img"))
				{
					imgFlag = 1;
					sscanf(buffer + 15, "%s", tmpChar);
					printf("Nazov je %s\n", tmpChar);
					tmpLen += strlen(tmpChar);
					tmpLen++; //lebo medzery

					if ((file = fopen(tmpChar, "wb")) == NULL)
					{
						printf("fopen failed\n");
						return 1;
					}

				}
				free(tmpChar);
			}

			*(tmpAnswer + 2) = 1;
			*(unsigned int*)((char*)tmpAnswer + 3) = 0;
			strcpy(((char*)tmpAnswer + 7), "_ack\0");

			*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

			if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;

		}
		free(buffer);
#pragma endregion


		//LOAD VARIABLES
		tmpMsg = malloc(fragSize*fragCount);
		fragAck = 0;
		flag = 1;
		int errorFlag = 0;
		buffer = (char*)malloc(fragSize);
		//tmpAnswer = (char*)malloc(9);

		//Keep listening for data
		while (flag)
		{
			//printf("\nWaiting for data...\n");
			//buffer = (char*)malloc(fragSize);

			if (recvfrom(socketVar, buffer, fragSize, 0, (struct sockaddr*)&client, &socLen) == SOCKET_ERROR)
			{
				printf("Recieving data failed, code %d\n", WSAGetLastError());
				return 1;

			}


			if (*(unsigned short*)buffer != crc_kermit(buffer + 2, fragSize - 2))
			{
				printf("Wrong CRC\n");
				/*
				*(unsigned short*)tmpAnswer = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 4), "nack\0");

				if (SendPacket(socketVar, tmpAnswer, 9, client, socLen)) return 1;*/


				*(tmpAnswer + 2) = 1;
				*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 7), "nack\0");

				*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

				if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;

				//free(tmpAnswer);
				//free(buffer);
				continue;
			}
			if (*(unsigned int*)((char*)buffer + 3) > (fragAck + 1))
			{
				printf("DISCARDING - fragment %d is too far ahead\n", *(unsigned int*)((char*)buffer + 3));
				//free(buffer);
				continue;
			}

			if (*(unsigned int*)((char*)buffer + 3) < (fragAck + 1))
			{
				printf("DISCARDING - fragment %d already recieved\n", *(unsigned int*)((char*)buffer + 3));
				//tmpAnswer = (char*)malloc(9);
				/*
				*(unsigned int*)tmpAnswer = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 4), "ack\0");

				if (SendPacket(socketVar, tmpAnswer, 9, client, socLen)) return 1;*/



				*(tmpAnswer + 2) = 2;
				*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
				strcpy(((char*)tmpAnswer + 7), "_ack\0");

				*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);

				if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;

				//free(tmpAnswer);
				//free(buffer);
				continue;
			}

			//printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			/*
			*(unsigned int*)tmpAnswer = *(unsigned int*)((char*)buffer + 3);
			strcpy(((char*)tmpAnswer + 4), "ack\0");*/

			*(tmpAnswer + 2) = 2;
			*(unsigned int*)((char*)tmpAnswer + 3) = *(unsigned int*)((char*)buffer + 3);
			strcpy(((char*)tmpAnswer + 7), "_ack\0");

			*(unsigned short*)tmpAnswer = crc_kermit(tmpAnswer + 2, 10);


			if (file != NULL)
			{
				if (fragAck + 1 == fragCount)
				{
					fwrite(buffer + 7, 1, lastFragSize, file);
					//printf("Appended file %.*s\n", strlen(buffer + 7), buffer + 7);
					//printf("Want to write %d but should write %d %.*s\n", fragSize - 7, strlen(buffer + 7), strlen(buffer + 7), buffer + 7);
				}
				else
				{
					fwrite(buffer + 7, 1, fragSize - 7, file);
					//printf("Appended file %.*s\n", fragSize - 7, buffer + 7);
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

			if ((fragAck + 1) == 15 && errorFlag)
			{
				errorFlag = 0;
			}
			else
			{
				if (SendPacket(socketVar, tmpAnswer, 12, client, socLen)) return 1;
			}

			fragAck++;

			//printf("SENDING ACK\n");

			if (fragAck == fragCount)
			{
				//printf("All fragments arrived, I should end\n");
				//free(buffer);
				//free(tmpAnswer);
				if (file == NULL)
				{
					printf("Final msg:\n%.*s\n", (fragAck - 1)*(fragSize - 7) + lastFragSize, tmpMsg);
					//free(tmpMsg);
				}
				else
				{
					if (fclose(file) == EOF)
					{
						printf("File can`t be closed.\n");
						return 1;
					}
					file = NULL;
				}
				flag = 0;
			}

		}

		int timeout = 30000;
		if (setsockopt(socketVar, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR)
		{
			printf("Timeout setting failed, code %d\n", WSAGetLastError());
		}

		/*printf("Do you want to continue? y/n ");
		if (getchar() == 'n')
		{
			getchar();
			break;
		}
		else
			getchar();*/

	}


	closesocket(socketVar);
	WSACleanup();
	return 0;

}
