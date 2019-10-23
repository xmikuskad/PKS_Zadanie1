#include "Client.h"

static char keepAliveSignal = 0;
static char end = 0;
static char threadDone = 0;

typedef struct ThreadStruc
{
	SOCKET socketVar;
	int socLen;
	struct sockaddr_in server;
	char logFlag;
}threadStruc;

static int SendFragment(SOCKET socketVar, char* string, short size, struct sockaddr_in server, int socLen,char logFlag);
	
DWORD WINAPI KeepAliveClient(threadStruc *myStruc)
{
	clock_t start;
	int time = 30000;
	char tmp[12];
	int flag = 1;
	SOCKET socketVar = myStruc->socketVar;
	int socLen = myStruc->socLen;
	struct sockaddr_in server = myStruc->server;
	char logFlag = myStruc->logFlag;
	char timeoutLimit = 10;

	threadDone = 0;

	while (keepAliveSignal)
	{
		*(tmp + 2) = 0;
		*(unsigned int*)((char*)tmp + 3) = 0;
		start = clock();
		flag = 1;
		while (keepAliveSignal)
		{
			clock_t endTime = clock() - start;
			if (endTime > time)
			{
				break;
			}
		}

		if (!keepAliveSignal)
		{
			threadDone = 1;
			return 0;
		}

		timeoutLimit = 10;
		while (flag && keepAliveSignal)
		{
			if(logFlag) printf("\nSending keepalive signal\n");

			if (SendFragment(socketVar, tmp, 7, server, socLen,logFlag)) return 1;


			if (recvfrom(socketVar, tmp, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
			{
				if (WSAGetLastError() == 10060)
				{
					if (timeoutLimit > 0) 
					{
						timeoutLimit--;
						continue;
					}
					else
					{
						end = 1;
						keepAliveSignal = 0;
						threadDone = 1;
						printf("\nKeep alive failed %d\nPress enter to continue...\n", WSAGetLastError());
						return 0;
					}
				}
				

			}

			if (crc_kermit(tmp + 2, 10) != *(unsigned short*)tmp || *(tmp + 2) != 0 || *(unsigned int*)((char*)tmp+3) !=0 )
			{
				if(logFlag) printf("Wrong CRC or %c = 0?\n",*(tmp+2));
				continue;
			}

			if (!strcmp(tmp + 7, "_ack\0"))
			{
				if(logFlag) printf("keep alive ack recieved\n");
				flag = 0;
			}
			else
			{
				if (logFlag) printf("NACK KeepAlive %s\n", tmp + 7);
				continue;
			}
		}

	}
	threadDone = 1;
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

static int SendFragment(SOCKET socketVar, char* string, short size, struct sockaddr_in server, int socLen, char logFlag)
{
	if (sendto(socketVar, string, size, 0, (struct sockaddr*)&server, socLen) == SOCKET_ERROR)
	{
		if(logFlag) printf("sendto problem %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}


char client(struct sockaddr_in tmp,char logFlag,WSADATA wsaData)
{ //PRIDAT CreateThread(NULL, 0, MyTimer, NULL, 0, NULL);
	//char *message, *fileName,*smallBuffer,*path,*dataBuffer;
	struct sockaddr_in server;
	int socLen = sizeof(server);
	SOCKET socketVar;
	//WSADATA wsaData;
	FILE *file = NULL;
	HANDLE threadHandle;

	keepAliveSignal = 0;
	end = 0;
	//threadDone = 0;

	/*char *tmpBuffer = (char*)malloc(12);
	fileName = (char*)malloc(sizeof(char) * 240);
	path = (char*)malloc(sizeof(char) * 240);
	message = (char*)malloc(8000);
	char *tester = (char*)malloc(2000);
	smallBuffer = (char*)malloc(12);*/
	char dataBuffer[1500];
	char message[8000], fileName[240], path[240], tmpBuffer[12], tester[2000], smallBuffer[12];

	if (logFlag) printf("LOGGING ALLOWED\n");

#pragma region WINSOCK LOAD

	//Set up WSA
	/*if (!CheckWinsock())
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			if (logFlag) printf("WSAStartup init problem %d\n", WSAGetLastError());
			return 1;
		}
	}
	else
	{
		printf("Winsock was loaded\n");
	}*/

	//Set up socket
	if ((socketVar = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("Socket problem %d\n", WSAGetLastError());
		return 1;
	}

	int timeout = 50;
	if (setsockopt(socketVar, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int)) == SOCKET_ERROR)
	{
		printf("Timeout setting problem %d\n", WSAGetLastError());
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(tmp.sin_port);
	server.sin_addr.S_un.S_addr = tmp.sin_addr.S_un.S_addr;

#pragma endregion

	//Setting up for timeout later
	threadStruc *myStruc = (threadStruc*)malloc(sizeof(myStruc));
	myStruc->server = server;
	myStruc->socketVar = socketVar;
	myStruc->socLen = socLen;
	myStruc->logFlag = logFlag;

	while (1)
	{
		//int fragSize = 0, windowSize = 20, sendCount = 0, fileLen = 0, fragCount = 0, fragAck = 0;
		char flag = 1, imgFlag = 0, timeoutLimit = 10;
		unsigned int sendCount = 0, fragAck = 0, fileLen = 0, fragCount = 0, failNumber = 0;
		short fragSize = 0;
		unsigned char windowSize = 20;

		if (end)
		{
			if (closesocket(socketVar) != 0)
				printf("CLOSE SOCKET FAIL!");
		/*	if (WSACleanup() != 0)
				printf("WSA CLEANUP FAILED\n");
			else
				printf("I closed it\n");*/
			return 2;
		}

		while (1)
		{
			printf("Enter data size (1-1473): ");

			if (scanf("%hd", &fragSize) == EOF)
			{
				printf("WRONG DATA SIZE!\n");
				continue;
			}

			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				return 2;
			}
			getchar();
			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				return 2;
			}

			if (fragSize >= 1 && fragSize <= 1473)
			{
				if(logFlag) printf("Size: %d\n", fragSize);
				break;
			}
			else
			{
				printf("WRONG DATA SIZE!\n");
				continue;
			}

			
		}

		char command;
		do {
			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				/*if (WSACleanup() != 0)
					printf("WSA CLEANUP FAILED\n");
				else
					printf("I closed it\n");*/
				return 2;
			}
			printf("\nSend msg or file? m/f ");
			command = getchar();
			//printf("%c\n", command);
			getchar();
		} while (command != 'm' && command != 'f');

		if (command == 'm')
		{
			imgFlag = 0;

			//Sprava moze mat az 2000 znakov
			printf("Enter message:\n");
			fgets(message, 10000, stdin);

			while (1)
			{
				fgets(tester, 2000, stdin);
				if (tester[0] == '\n')
					break;
				strcat(message, tester);
			}

			//if(logFlag) printf("MSG: \n%s", message);

			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				/*if (WSACleanup() != 0)
					printf("WSA CLEANUP FAILED\n");*/
				else
					printf("I closed it\n");
				return 2;
			}
			keepAliveSignal = 0;

			fragCount = (strlen(message) - 1 + fragSize - 1) / fragSize;

			while (1)
			{
				printf("\nDo you want to send wrong packet?\nType 1-%d to send or 0 to do nothing: ", fragCount);

				if (scanf("%d", &failNumber) == EOF)
				{
					printf("WRONG INPUT!\n");
					continue;
				}

				getchar();

				if (failNumber >= 0 && failNumber <= fragCount)
				{
					if (logFlag) printf("Corrupted fragment number: %d\n", failNumber);
					break;
				}
				else
				{
					printf("WRONG INPUT!\n");
					continue;
				}


			}


#pragma region SEND_FIRST_PACKET_MSG
			//flag = 1;
			timeoutLimit = 10;
			while (1)
			{
				printf("Sending first info\n");
				if (SendFragment(socketVar, CreateFirstPacket(strlen(message)-1, fragSize + 7, 'm', NULL), 256, server, socLen,logFlag)) return 1;

				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, smallBuffer, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
				{
					if (WSAGetLastError() == 10060)
					{
						if(logFlag) printf("First frag timeout\n");
						timeoutLimit--;
						if (timeoutLimit <= 0)
						{
							if(logFlag) printf("Timeout limit reached\n");
							if (closesocket(socketVar) != 0)
								printf("CLOSE SOCKET FAIL!");
							/*if (WSACleanup() != 0)
								printf("WSA CLEANUP FAILED\n");
							else
								printf("I closed it\n");*/
							return 2;
						}
						continue;
					}
					else
					if(logFlag) printf("recvfrom problem %d: \n", WSAGetLastError());
					return 1;
				}

				if (*(unsigned short*)smallBuffer != crc_kermit(smallBuffer + 2, 10) || *(smallBuffer + 2) != 1)
				{
					if(logFlag) printf("Wrong CRC or type\n");
					continue;
				}

				if(logFlag) printf("Answer: %s\n", smallBuffer+7);
				if (strcmp(smallBuffer+7, "nack"))
				{
					break; //ACK
				}
			}
#pragma endregion


		}
		else
		{
			imgFlag = 1;

			printf("Enter file position:\n");
			fgets(path, 255, stdin);

			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				/*if (WSACleanup() != 0)
					printf("WSA CLEANUP FAILED\n");
				else
					printf("I closed it\n");*/
				return 2;
			}

			//char nameFlag = 0;

		

			path[strlen(path) - 1] = '\0';
			//IMAGE MANAGING
			if ((file = (fopen(path, "rb"))) == NULL)
			{
				if(logFlag) printf("File failed to open\n%s\n", path);
				//system("pause");

				return 1;
			}

			if (strstr(path, "\\")==NULL)
			{
				strcpy(fileName, path);
				/*strcpy(path, __FILE__);

				int j;
				for (j = strlen(path) - 1; j > 0; j--)
				{
					if (path[j] == '\\')
					{
						break;
					}
				}

				strcpy(path + j + 1, fileName);*/

				_fullpath(path, fileName, 240);

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


			//free(path);
			fseek(file, 0, SEEK_END);
			fileLen = ftell(file);
			fseek(file, 0, SEEK_SET);

			fragCount = (fileLen + fragSize - 1) / fragSize;

			while (1)
			{
				printf("\nDo you want to send wrong packet?\nType 1-%d to send or 0 to do nothing: ", fragCount);

				if (scanf("%d", &failNumber) == EOF)
				{
					printf("WRONG INPUT!\n");
					continue;
				}

				getchar();

				if (failNumber >= 0 && failNumber <= fragCount)
				{
					if (logFlag) printf("Corrupted fragment number: %d\n", failNumber);
					break;
				}
				else
				{
					printf("WRONG INPUT!\n");
					continue;
				}


			}

#pragma region SEND_FIRST_PACKET_IMG
			keepAliveSignal = 0;
			//flag = 1;
			timeoutLimit = 10;
			while (1)
			{
				
				printf("Sending first info\n");

				if (SendFragment(socketVar, CreateFirstPacket(fileLen, fragSize + 7, 'i', fileName), 256, server, socLen,logFlag)) return 1;


				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, smallBuffer, 12, 0, (struct sockaddr *)&server, &socLen) == SOCKET_ERROR)
				{
					if (WSAGetLastError() == 10060)
					{
						if (logFlag) printf("First frag timeout\n");
						timeoutLimit--;
						if (timeoutLimit <= 0)
						{
							if (logFlag) printf("Timeout limit reached\n");
							if (closesocket(socketVar) != 0)
								printf("CLOSE SOCKET FAIL!");
							/*if (WSACleanup() != 0)
								printf("WSA CLEANUP FAILED\n");
							else
								printf("I closed it\n");*/
							return 2;
						}
						continue;
					}

					else
					if(logFlag) printf("recvfrom problem %d\n", WSAGetLastError());
					return 1;
				}

				if (*(unsigned short*)smallBuffer != crc_kermit(smallBuffer + 2, 10) || *(smallBuffer + 2) != 1)
				{
					continue;
				}

				if(logFlag) printf("Answer: %s\n", smallBuffer+7);
				if (strcmp(smallBuffer+7, "nack"))
				{
					break;
				}

			}
#pragma endregion

		}

		/*
		while (1)
		{
			printf("Do you want to send wrong packet?\nType 1-%d to send or 0 to do nothing",fragCount);

			if (scanf("%d", &failNumber) == EOF)
			{
				printf("WRONG INPUT!\n");
				continue;
			}

			getchar();

			if (failNumber >= 1 && failNumber <= fragCount)
			{
				if (logFlag) printf("Corrupted fragment number: %d\n", failNumber);
				break;
			}
			else
			{
				printf("WRONG INPUT!\n");
				continue;
			}


		}
		*/

		//char *dataBuffer = (char*)malloc(fragSize + 7);
		char testTmp = 1;
		timeoutLimit = 10;
		while (fragCount > fragAck)
		{
			if (sendCount - fragAck < windowSize && sendCount < fragCount)
			{
				sendCount++;

				*(unsigned int*)((char*)dataBuffer + 3) = sendCount;
				*(dataBuffer + 2) = 2;


				if (imgFlag)
				{
					fseek(file, 0, SEEK_SET);
					fseek(file, ((sendCount - 1) * fragSize), SEEK_CUR);
					fread(dataBuffer + 7, 1, fragSize, file);
				}
				else
				{
					strncpy(dataBuffer + 7, message + ((sendCount - 1) * fragSize), fragSize);
				}

				*(dataBuffer + 2) = 2;
				*(unsigned short*)dataBuffer = crc_kermit(dataBuffer + 2, fragSize - 2 + 7);

				if (*(unsigned int*)((char*)dataBuffer + 3) == failNumber)
				{
					*(dataBuffer + fragSize -1) = '~';
					*(dataBuffer + fragSize / 2) = '_';
					testTmp = 0;
					failNumber = 0;
				}

				if(logFlag) printf("Sending %d fragment\n", sendCount);
				//printf("Info %d %s\n\n", *(unsigned int*)((char*)dataBuffer + 2), dataBuffer + 6);

				if (SendFragment(socketVar, dataBuffer, fragSize + 7, server, socLen,logFlag)) return 1;

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
							if(logFlag) printf("recvfrom problem %d\n", WSAGetLastError());
							//system("pause");
							return 1;
						}
						else
						{
							sendCount = fragAck;
							flag = 0;
							if(logFlag) printf("\nTIMEOUT!\n");
							timeoutLimit--;
							if (timeoutLimit <= 0)
							{
								if(logFlag) printf("timeout limit reached\n");
								if (closesocket(socketVar) != 0)
									printf("CLOSE SOCKET FAIL!");
								/*if (WSACleanup() != 0)
									printf("WSA CLEANUP FAILED\n");
								else
									printf("I closed it\n");*/
								return 2;
							}
							continue;
						}
					}

					if(logFlag)	printf("Recieved %d %s\n",*(unsigned int*)((char*)tmpBuffer + 3), tmpBuffer + 7);

					if (*(unsigned short*)tmpBuffer != crc_kermit(tmpBuffer + 2, 10))
					{
						if(logFlag)	printf("Wrong CRC\n");
						continue;
					}

					if (*(tmpBuffer + 2) != 2)
					{
						if(logFlag)	printf("Wrong type\n");
						continue;
					}

					//printf("ACK: %d SEND %d\n", fragAck, sendCount);

					if (*(unsigned int*)((char*)tmpBuffer+3) == (fragAck + 1) && !strcmp(tmpBuffer + 7, "_ack\0"))
					{
						fragAck++;
						flag = 0;
						timeoutLimit = 10;
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
		threadHandle = CreateThread(NULL, 0, KeepAliveClient, myStruc, 0, NULL);
		//free(dataBuffer);

		printf("Do you want to continue? y/n ");
		if (getchar() == 'n')
		{
			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				/*if (WSACleanup() != 0)
					printf("WSA CLEANUP FAILED\n");
				else
					printf("I closed it\n");*/
				return 2;
			}
			keepAliveSignal = 0;
			getchar();
			break;
		}
		else
		{
			if (end)
			{
				if (closesocket(socketVar) != 0)
					printf("CLOSE SOCKET FAIL!");
				/*if (WSACleanup() != 0)
					printf("WSA CLEANUP FAILED\n");
				else
					printf("I closed it\n");*/
				return 2;
			}
			getchar();
		}

	}


	/*free(message);
	free(smallBuffer);
	free(tester);
	free(fileName);
	free(path);*/

	while (!threadDone) {}; //pockanie na thread

	printf("KONCIM\n");
	/*if (threadHandle != NULL)
	{
		TerminateThread(threadHandle, 0);
		threadHandle = NULL;
	}*/

	if (closesocket(socketVar)!=0)
		printf("CLOSE SOCKET FAIL!");

	/*if (WSACleanup() != 0)
		printf("WSA CLEANUP FAILED\n");
	else
		printf("I closed it\n");*/


	return 0;
}