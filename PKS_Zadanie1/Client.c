#include "Client.h"

int keepAliveSignal = 0;
int end = 0;

typedef struct ThreadStruc
{
	SOCKET socketVar;
	int socLen;
	struct sockaddr_in server;
}threadStruc;

static int SendPacket(SOCKET socketVar, char* string, int size, struct sockaddr_in server, int socLen);

DWORD WINAPI KeepAliveClient(threadStruc *myStruc)
{
	clock_t start;
	int time = 25000;
	char *tmp = (char*)malloc(9);
	int flag = 1;
	SOCKET socketVar = myStruc->socketVar;
	int socLen = myStruc->socLen;
	struct sockaddr_in server = myStruc->server;

	while (keepAliveSignal)
	{
		*(tmp + 2) = 0;
		*(unsigned int*)((char*)tmp + 3) = 0;
		start = clock();
		flag = 1;
		while (keepAliveSignal)
		{
			clock_t end = clock() - start;
			if (end > time)
			{
				break;
			}
		}

		if (!keepAliveSignal)
		{
			return 0;
		}

		while (flag)
		{
			printf("Sending keepalive signal\n");

			if (SendPacket(socketVar, tmp, 7, server, socLen)) return 1;


			if (recvfrom(socketVar, tmp, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != 10060)
				{
					//printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					end = 1;
					keepAliveSignal = 0;
					printf("\nKeep alive failed\n");
					return 0;
				}
			}

			if (crc_kermit(tmp + 2, 10) != *(unsigned int*)tmp || *(tmp + 2) != 0 || *(unsigned int*)((char*)tmp+3) !=0 )
			{
				printf("Wrong CRC KeepAlive\n");
				continue;
			}

			if (!strcmp(tmp + 7, "_ack\0"))
			{
				printf("keep alive ack recieved\n");
				flag = 0;
			}
			else
			{
				printf("NACK KeepAlive\n");
				continue;
			}
		}

	}

	return 0;
}

char* CreateFirstPacket(unsigned int arraySize, short sizeOfFragments, char type, char *fileName)
{
	int size = 2;
	char *message = (char*)malloc(sizeof(char) * 256);

	*(message + size) = 1;
	size++;

	*(unsigned int*)((char*)message + size) = (arraySize + sizeOfFragments - 7 - 1) / (sizeOfFragments - 7); //num of frag
	size += 4;
	//STACI SHORT NA VELKOST!

	*(short*)((char*)message + size) = sizeOfFragments; //size
	size += 2;

	*(short*)((char*)message + size) = arraySize % (sizeOfFragments-7); //size
	if (*(short*)((char*)message + size) == 0)
		*(short*)((char*)message + size) = sizeOfFragments-7;
	size += 2;

	if (type == 'm')
	{
		strcpy(message + size, "msg\0"); //Za string musi ist \0 !!!
		size += 4;
	}
	else
		if (type == 'i')
		{
			strcpy(message + size, "img\0"); //Za string musi ist \0 !!!
			size += 4;
			strncpy(message + size, fileName, strlen(fileName));
			size += strlen(fileName);
			message[size] = '\0';
		}

	//create checksum without checksum!
	*(unsigned short*)(message) = crc_kermit((const unsigned char*)message + 2, 254);

	return message;
}

static int SendPacket(SOCKET socketVar, char* string, int size, struct sockaddr_in server, int socLen)
{
	if (sendto(socketVar, string, size, 0, (struct sockaddr*)&server, socLen) == SOCKET_ERROR)
	{
		printf("SendTo fail - %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int client(struct sockaddr_in tmp)
{ //PRIDAT CreateThread(NULL, 0, MyTimer, NULL, 0, NULL);
	char *buffer, *message = NULL, *fileName,*smallBuffer,*path;
	struct sockaddr_in server;
	int socLen = sizeof(server);
	SOCKET socketVar;
	WSADATA wsaData;
	FILE *file = NULL;

#pragma region WINSOCK LOAD

	//Set up WSA
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSA initialization failed, code %d\n", WSAGetLastError());
		return 1;
	}

	//Set up socket
	if ((socketVar = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("Socket failed, code %d\n", WSAGetLastError());
		return 1;
	}

	int timeout = 50;
	if (setsockopt(socketVar, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR)
	{
		printf("Timeout setting failed, code %d\n", WSAGetLastError());
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(tmp.sin_port);
	server.sin_addr.S_un.S_addr = tmp.sin_addr.S_un.S_addr; //inet_addr(buffer);

#pragma endregion

	//Setting up for timeout later
	threadStruc *myStruc = (threadStruc*)malloc(sizeof(myStruc));
	myStruc->server = server;
	myStruc->socketVar = socketVar;
	myStruc->socLen = socLen;

	while (1)
	{
		int fragSize = 0, windowSize = 20, sendCount = 0, fileLen = 0, fragCount = 0, fragAck = 0;
		char flag = 1, imgFlag = 0;
		char *tester = (char*)malloc(2000);
		if (end) return 2;

		while (flag)
		{
			printf("Enter data size (1-1473): ");

			if (scanf("%d", &fragSize) == EOF)
			{
				printf("WRONG DATA SIZE!\n");
				continue;
			}

			if (end) return 2;
			getchar();
			if (end) return 2;

			if (fragSize >= 1 && fragSize <= 1473)
				flag = 0;
			else
			{
				printf("WRONG DATA SIZE!\n");
				continue;
			}

			printf("Size: %d\n", fragSize);
		}

		char command;
		do {
			if (end) return 2;
			printf("\nSend msg or file? m/f ");
			command = getchar();
			printf("%c\n", command);
			getchar();
		} while (command != 'm' && command != 'f');

		if (command == 'm')
		{
			imgFlag = 0;

			//Sprava moze mat az 2000 znakov
			printf("Enter message:\n");
			message = (char*)malloc(sizeof(char) * 10000);
			fgets(message, 10000, stdin);

			while (1)
			{
				fgets(tester, 2000, stdin);
				if (tester[0] == '\n')
					break;
				strcat(message, tester);
			}
			//strcat(message, tester);

			printf("MSG: %s", message);

			if (end) return 2;

#pragma region SEND_FIRST_PACKET_MSG
			flag = 1;
			smallBuffer = (char*)malloc(12 * sizeof(char));
			while (flag)
			{
				keepAliveSignal = 0;
				//buffer = (char*)malloc(12 * sizeof(char));

				fragCount = (strlen(message) + fragSize - 1) / fragSize;
				printf("SENDING FIRST PACKET\n");
				if (SendPacket(socketVar, CreateFirstPacket(strlen(message) - 1, fragSize + 7, 'm', NULL), 256, server, socLen)) return 1;

				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, smallBuffer, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}

				if (*(unsigned short*)smallBuffer != crc_kermit(smallBuffer + 2, 10) || *(smallBuffer + 2) != 1)
				{
					printf("Wrong CRC or type\n");
					continue;
				}

				printf("RESULT: %s\n", smallBuffer+7);
				if (!strcmp(smallBuffer+7, "nack"))
				{
					printf("FAIL, SEND AGAIN\n");
				}
				else
				{
					printf("GOOD, CONTINUE\n");
					flag = 0;
				}
			}
			free(smallBuffer);
#pragma endregion


		}
		else
		{
			imgFlag = 1;

			printf("Enter file position:\n");
			fileName = (char*)malloc(sizeof(char) * 256);
			path = (char*)malloc(sizeof(char) * 256);
			fgets(path, 255, stdin);

			if (end) return 2;

			//char nameFlag = 0;

		

			path[strlen(path) - 1] = '\0';
			//IMAGE MANAGING
			if ((file = (fopen(path, "rb"))) == NULL)
			{
				printf("File failed to open\n%s\n", path);
				system("pause");

				return 1;
			}

			if (strstr(path, "\\")==NULL)
			{
				strcpy(fileName, path);
				strcpy(path, __FILE__);

				int j;
				for (j = strlen(path) - 1; j > 0; j--)
				{
					if (path[j] == '\\')
					{
						break;
					}
				}

				strcpy(path + j + 1, fileName);
			}
			else
			{
				//int num = 0;
				int j;
				for (j = strlen(path) - 1; j > 0; j--)
				{
					if (path[j] == '\\')
					{
						break;
					}
				}

				strncpy(fileName, path + j + 1, strlen(path) - j);
			}
			//printf("filename: %s\n", fileName);
			printf("File path: %s\n", path);


			free(path);
			fseek(file, 0, SEEK_END);
			fileLen = ftell(file);
			fseek(file, 0, SEEK_SET);

#pragma region SEND_FIRST_PACKET_IMG

			flag = 1;
			smallBuffer = (char*)malloc(12);
			while (flag)
			{
				keepAliveSignal = 0;
				
				printf("SENDING FIRST PACKET\n");
				fragCount = (fileLen + fragSize - 1) / fragSize;

				if (SendPacket(socketVar, CreateFirstPacket(fileLen, fragSize + 7, 'i', fileName), 256, server, socLen)) return 1;


				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, smallBuffer, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}

				if (*(unsigned short*)smallBuffer != crc_kermit(smallBuffer + 2, 10) || *(smallBuffer + 2) != 1)
				{
					continue;
				}

				printf("RESULT: %s\n", smallBuffer+7);
				if (!strcmp(smallBuffer+7, "nack"))
				{
					printf("FAIL, SEND AGAIN\n");
				}
				else
				{
					printf("GOOD, CONTINUE\n");
					flag = 0;
				}
			}
			free(smallBuffer);
			free(fileName);
#pragma endregion

		}

		char *tmpBuffer = (char*)malloc(12);
		buffer = (char*)malloc(fragSize + 7);
		char testTmp = 1;
		while (fragCount > fragAck)
		{
			//printf("Count %d > Ack %d\n", fragCount, fragAck);
			if (sendCount - fragAck < windowSize && sendCount < fragCount)
			{
				sendCount++;

				*(unsigned int*)((char*)buffer + 3) = sendCount;
				*(buffer + 2) = 2;


				if (imgFlag)
				{
					fseek(file, 0, SEEK_SET);
					fseek(file, ((sendCount - 1) * fragSize), SEEK_CUR);
					fread(buffer + 7, 1, fragSize, file);
				}
				else
				{
					strncpy(buffer + 7, message + ((sendCount - 1) * fragSize), fragSize);
				}

				*(buffer + 2) = 2;
				*(unsigned short*)buffer = crc_kermit(buffer + 2, fragSize - 2 + 7);

				//UROBENIE CHYBY
				/*
				if (testTmp && sendCount == 8)
				{
					*(buffer + 8) = '$';
					*(buffer + fragSize - 2) = '#';
					testTmp = 0;
				}
				*/
				//printf("Sending %d fragment\n", sendCount);
				//printf("Info %d %s\n\n", *(unsigned int*)((char*)buffer + 2), buffer + 6);

				if (SendPacket(socketVar, buffer, fragSize + 7, server, socLen)) return 1;

			}
			else
			{

				flag = 1;
				while (flag)
				{
					if (recvfrom(socketVar, tmpBuffer, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
					{
						if (WSAGetLastError() != 10060)
						{
							printf("recvfrom() failed, code %d: \n", WSAGetLastError());
							system("pause");
							return 1;
						}
						else
						{
							sendCount = fragAck;
							flag = 0;
							printf("\nTIMEOUT!\n");
							continue;
						}
					}

					if (*(unsigned short*)tmpBuffer != crc_kermit(tmpBuffer + 2, 10))
					{
						printf("Wrong CRC\n");
						continue;
					}

					if (*(tmpBuffer + 2) != 2)
					{
						printf("Wrong type\n");
						continue;
					}

					//printf("Recieved %d %s\n", *(unsigned int*)((char*)tmpBuffer + 3), tmpBuffer + 7);
					//printf("ACK: %d SEND %d\n", fragAck, sendCount);

					if (*(unsigned int*)((char*)tmpBuffer+3) == (fragAck + 1) && !strcmp(tmpBuffer + 7, "_ack\0"))
					{
						fragAck++;
						flag = 0;
					}
					else
						if (*(unsigned int*)((char*)tmpBuffer + 3) > (fragAck + 1) && !strcmp(tmpBuffer + 7, "_ack\0"))
						{
							continue;
						}
						else
						{
							sendCount = fragAck;
							flag = 0;
						}
				}
			}
		}

		keepAliveSignal = 1;
		CreateThread(NULL, 0, KeepAliveClient, myStruc, 0, NULL);

		printf("Do you want to continue? y/n ");
		if (getchar() == 'n')
		{
			getchar();
			keepAliveSignal = 0;
			break;
		}
		else
			getchar();

	}

	closesocket(socketVar);
	WSACleanup();

	return 0;
}