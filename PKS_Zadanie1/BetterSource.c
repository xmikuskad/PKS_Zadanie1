/* Pouzil som Open Source kniznicu na vypocet CRC zo stranky https://www.libcrc.org
 * 
 * Library: libcrc
 * File:    src/crckrmit.c
 * Author:  Lammert Bies
 *
 * This file is licensed under the MIT License as stated below
 *
 * Copyright (c) 1999-2016 Lammert Bies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description
 * -----------
 * The source file src/crckrmit.c contains routines which calculate the CRC
 * Kermit cyclic redundancy check value for an incomming byte string.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <Windows.h>
#include <time.h>

#pragma comment(lib,"ws2_32.lib") //winsock library

#define		CRC_POLY_KERMIT		0x8408
#define		CRC_START_KERMIT	0x0000
 //pouzivanie polynomu x16 + x12 + x5 + 1 ale inverzneho!

static unsigned short	crc_tab[256];
int stopKeepAlive;

static void init_crc_tab() {

	unsigned short i;
	unsigned short j;
	unsigned short crc;
	unsigned short c;

	for (i = 0; i < 256; i++) {

		crc = 0;
		c = i;

		for (j = 0; j < 8; j++) {

			if ((crc ^ c) & 0x0001) crc = (crc >> 1) ^ CRC_POLY_KERMIT;
			else                      crc = crc >> 1;

			c = c >> 1;
		}

		crc_tab[i] = crc;
	}

}

unsigned short crc_kermit(const unsigned char *input_str, int num_bytes) {

	unsigned short crc;
	unsigned short tmp;
	unsigned short short_c;
	unsigned short low_byte;
	unsigned short high_byte;
	const unsigned char *ptr;

	crc = CRC_START_KERMIT;
	ptr = input_str; 

		if (ptr != NULL) for (int a = 0; a < num_bytes; a++) {

			short_c = 0x00ff & (unsigned short)*ptr;
			tmp = crc ^ short_c;
			crc = (crc >> 8) ^ crc_tab[tmp & 0xff];

			ptr++;
		}

	low_byte = (crc & 0xff00) >> 8;
	high_byte = (crc & 0x00ff) << 8;
	crc = low_byte | high_byte;

	return crc;

}

int GetCommand();

DWORD WINAPI KeepAliveClient()
{
	clock_t start = clock();
	int time = 10000;

	while (1)
	{
		clock_t end = clock() - start;
		if (end > time)
		{
			stopKeepAlive = 1;
			return 0;
		}
	}

	//SEND STOP TIMER PACKET

	return 0;
}

DWORD WINAPI KeepAliveServer()
{
	clock_t start = clock();
	int time = 10000;

	while (1)
	{
		clock_t end = clock() - start;
		if (end > time)
		{
			stopKeepAlive = 1;
			return 0;
		}
	}

	//SEND STOP TIMER PACKET

	return 0;
}

char* CreateFirstPacket(unsigned int fragmentsCount, short sizeOfGFragments, char type, char *path)
{
	int size = 2;
	char *message = (char*)malloc(sizeof(char) * 256);


	*(unsigned int*)((char*)message +size) = fragmentsCount; //num of frag
	size += 4;
	//STACI SHORT NA VELKOST!
	*(short*)((char*)message + size) = sizeOfGFragments; //size
	size += 2;

	if (type == 'm')
	{
		strcpy(message + size, "msg\0"); //Za string musi ist \0 !!!
		size += 4;
	}
	else
	if(type == 'i')
	{
		strcpy(message + size, "img\0"); //Za string musi ist \0 !!!
		size += 4;
		strncpy(message + size, path,strlen(path)); 
		size += strlen(path);
		message[size++] = '\0';
	}

	//create checksum without checksum!
	*(unsigned short*)(message) = crc_kermit((const unsigned char*)message+2, 254);

	return message;
}

//WRONG PACKET SETUP !! DONT FORGET
int client(struct sockaddr_in tmp)
{ //PRIDAT CreateThread(NULL, 0, MyTimer, NULL, 0, NULL);
	char *buffer, *message = NULL, *path;
	struct sockaddr_in client;
	int socLen = sizeof(client);
	SOCKET socketVar;
	WSADATA wsaData;
	FILE *file = NULL;

#pragma region WINSOCK LOAD

	printf("Initializing client\n");

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

	client.sin_family = AF_INET;
	client.sin_port = htons(tmp.sin_port);
	client.sin_addr.S_un.S_addr = tmp.sin_addr.S_un.S_addr; //inet_addr(buffer);

#pragma endregion

	//buffer = (char*)malloc(256 * sizeof(char));

	while (1)
	{
		int fragSize = 0, windowSize = 10, sendCount = 0, fileLen = 0, fragCount = 0, fragAck = 0;
		char flag = 1, imgFlag=0;
		
		while (flag)
		{
			printf("Enter data size (1-1434): ");

			scanf("%d", &fragSize);
			getchar();
			printf("Size: %d\n", fragSize);
			if (fragSize >= 1 && fragSize <= 1434)
				flag = 0;
			else
				printf("WRONG DATA SIZE!\n");

		}


		printf("Send msg or image? m/i ");
		if (getchar() == 'm')
		{
			getchar();

			imgFlag = 0;

			//Sprava moze mat az 2000 znakov
			printf("Enter message:\n");
			message = (char*)malloc(sizeof(char) * 8000);
			fgets(message, 8000, stdin);

#pragma region SEND_FIRST_PACKET_MSG
			flag = 1;
			while (flag)
			{
				buffer = (char*)malloc(5 * sizeof(char));

				fragCount = (strlen(message) + fragSize - 1) / fragSize;
				printf("SENDING FIRST PACKET\n");
				if (sendto(socketVar, CreateFirstPacket(fragCount, fragSize+6, 'm', NULL), 256, 0, (struct sockaddr *)&client, socLen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}

				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, buffer, 5, 0, (struct sockaddr *)&client, &socLen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}

				printf("RESULT: %s\n", buffer);
				if (!strcmp(buffer, "nack"))
				{
					printf("FAIL, SEND AGAIN\n");
				}
				else
				{
					printf("GOOD, CONTINUE\n");
					flag = 0;
				}
				free(buffer);
			}
#pragma endregion


		}
		else
		{
			getchar();

			imgFlag = 1;

			char *tmpImg;
			int sizeImg = 0;

			printf("Enter image position:\n");
			path = (char*)malloc(sizeof(char) * 256);
			fgets(path, 255, stdin);
			
			/*int i =0;
			while (path[i] != '\n')
			{
				i++;
			}*/
			path[strlen(path)-1] = '\0';

			//IMAGE MANAGING
			if ((file = (fopen(path, "rb"))) == NULL)
			{
				printf("File failed to open\n%s\n", path);
				system("pause");
				return 1;
			}

			fseek(file, 0, SEEK_END);
			fileLen = ftell(file);
			fseek(file, 0, SEEK_SET);

#pragma region SEND_FIRST_PACKET_IMG
			
			flag = 1;
			while (flag)
			{
				buffer = (char*)malloc(5);
				printf("SENDING FIRST PACKET\n");
				fragCount = (fileLen + fragSize - 1) / fragSize;
				if (sendto(socketVar, CreateFirstPacket(fragCount, fragSize+6, 'i', path), 256, 0, (struct sockaddr *)&client, socLen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}

				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, buffer, 5, 0, (struct sockaddr *)&client, &socLen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}

				printf("RESULT: %s\n", buffer);
				if (!strcmp(buffer, "nack"))
				{
					printf("FAIL, SEND AGAIN\n");
				}
				else
				{
					printf("GOOD, CONTINUE\n");
					flag = 0;
				}
				free(buffer);
			}
#pragma endregion

			/*
			int tmpCount = 0;
			tmpImg = (char*)malloc(fragSize + 6);
			buffer = (char*)malloc(9);
			//Sending file
			while (fileLen > 0)
			{
				free(tmpImg);
				tmpImg = (char*)malloc((fragSize+6) * sizeof(char));

				printf("READ : %zd\n",fread(tmpImg, 1, fragSize, file));

				//printf("File contains: %s", tmpImg);

				//send the message
				if (sendto(socketVar, tmpImg, fragSize+6, 0, (struct sockaddr *)&client, socLen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}


				printf("\nRecieving...\n");
				//try to receive some data, this is a blocking call
				if (recvfrom(socketVar, buffer, 9, 0, (struct sockaddr *)&client, &socLen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}
				printf("%d Reply: %s\n", fileLen, buffer);
				printf("Number od packets %d\n", ++tmpCount);
				fileLen -= fragSize;
			}
			*/
		}

		while (fragCount > fragAck)
		{
			//printf("Count %d > Ack %d\n", fragCount, fragAck);
			if (sendCount - fragAck < windowSize-2 && sendCount < fragCount)
			{
				buffer = (char*)malloc(fragSize + 6);
				sendCount++;

				*(unsigned int*)((char*)buffer + 2) = sendCount;

				if (imgFlag)
				{
					fseek(file, 0, SEEK_SET);
					fseek(file, ((sendCount - 1) * fragSize), SEEK_CUR);
					int readChars = fread(buffer + 6, 1, fragSize, file);
					if(readChars < fragSize)
					{
						printf("Loaded less chars - %d\n",readChars);
						*(buffer + 6 + readChars) = '\0';
					}
				}
				else
				{
					strncpy(buffer + 6, message + ((sendCount - 1) * fragSize), fragSize);
				}

				*(unsigned short*)buffer = crc_kermit(buffer + 2, fragSize - 2 + 6);

				//UROBENIE CHYBY
				/*if (testTmp && sendCount == 8)
				{
					*(buffer + 6) = '$';
					*(buffer + fragSize - 1) = '#';
					testTmp = 0;
				}*/

				printf("Sending %d fragment\n", sendCount);
				//printf("Info %d %s\n\n", *(unsigned int*)((char*)buffer + 2), buffer + 6);

				if (sendto(socketVar, buffer, fragSize + 6, 0, (struct sockaddr *)&client, socLen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}
				free(buffer);
			}
			else
			{
				buffer = (char*)malloc(9);
				//DOROBIT TIMEOUT
				if (recvfrom(socketVar, buffer, 9, 0, (struct sockaddr *)&client, &socLen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}

				printf("Recieved %d %s\n", *(unsigned int*)buffer, buffer + 4);

				if (*(unsigned int*)buffer == (fragAck+1) && !strcmp(buffer + 4, "ack\0"))
				{
					fragAck++;
				}
				else
				{
					sendCount = fragAck - 1;
				}
				free(buffer);
			}
		}
		printf("Do you want to continue? y/n ");
		if (getchar() == 'n')
		{
			getchar();
			break;
		}
		getchar();
	}

	closesocket(socketVar);
	WSACleanup();

	return 0;
}

int server(struct sockaddr_in tmp)
{
	WSADATA wsaData;
	SOCKET socketVar;
	int socLen, recieved_len;
	char *buffer, *tmpChar, *tmpMsg;// , *ackString, *nackString;
	char imgFlag = 0;
	struct sockaddr_in server, client;
	int flag = 0, deletePls = 0;
	//SPECIAL VALUES:
	unsigned int fragCount, fragAck;
	short fragSize;
	unsigned short	checksum;
	

#pragma region load Winsock + connection

	FILE *file = NULL;

	socLen = sizeof(client); //WTF?

	printf("Initializing server\n");

	//Set up WSA
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSA initialization failed, code %d\n", WSAGetLastError());
		return 1;
	}

	//Set up socket
	//if ((socketVar = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	if ((socketVar = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("Socket failed, code %d\n", WSAGetLastError());
		return 1;
	}

	//Prepare sockaddr_in
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(tmp.sin_port);

	printf("IP adress is: %s\n", inet_ntoa(server.sin_addr));

	//Bind
	if (bind(socketVar, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Binding failed, code %d\n", WSAGetLastError());
		//printf("Bind failed with error code : %d\n", WSAGetLastError());
		return 1;
	}
#pragma endregion


#pragma region RECIEVE_FIRST_FRAGMENT
	imgFlag = 0;
	flag = 1;
	while (flag)
	{
		//TESTING - CATCH FIRST FRAGMENT
		buffer = (char*)malloc(256);
		//DOROBIT TIMEOUT
		if ((recieved_len = recvfrom(socketVar, buffer, 256, 0, (struct sockaddr*)&client, &socLen)) == SOCKET_ERROR)
		{
			printf("Recieving data failed, code %d\n", WSAGetLastError());
			return 1;

		}

		printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		checksum = *(unsigned short*)(buffer);
		unsigned short checksum2 = crc_kermit((const unsigned char*)buffer + 2, 254);

		if (checksum != checksum2)
		{
			printf("Checksum is different :(\n"); //TO DO!
			if (sendto(socketVar, "nack\0", 5, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
			{
				printf("Sending back failed, code %d", WSAGetLastError());
				return 1;
			}
		}
		else
		{
			printf("Checksum OK!\n");

			flag = 0;

			printf("Pocet fragmentov je %u\n", *(unsigned int*)((char*)buffer + 2));
			fragCount = *(unsigned int*)((char*)buffer + 2);
			printf("Velkost fragmentov je %d\n", *(short*)((char*)buffer + 6));
			fragSize = *(short*)((char*)buffer + 6);

			int tmpLen = 0;
			tmpChar = (char*)malloc(242);

			sscanf(buffer + 8, "%s", tmpChar);
			printf("Typ spravy je %s\n", tmpChar);
			tmpLen += strlen(tmpChar);
			tmpLen++;//lebo medzera
			//free(tmpChar);
			if (!strcmp(tmpChar, "img"))
			{
				imgFlag = 1;
				sscanf(buffer + 12, "%s", tmpChar);
				printf("Nazov je %s\n", tmpChar);
				tmpLen += strlen(tmpChar);
				tmpLen++; //lebo medzery

				//ONLY NAME - NEFUNGUJE
				int num = 0;
				for (int j = strlen(tmpChar)-1; j > 0; j--)
				{
					if (tmpChar[j] != '\\')
					{
						num++;
					}
					else
						break;
				}

				char*tmpTmp = (char*)malloc(num+1);
				strcpy(tmpTmp,buffer - num + strlen(tmpChar)+12);
				printf("NAME: %s\n", tmpTmp);

				if ((file = fopen(tmpTmp, "wb")) == NULL)
				{
					printf("fopen failed\n");
					return 1;
				}
				//KONIEC
			}
			else
			{
				//tmpMsg = (char*)malloc((fragSize - 6)*fragCount);
			}

		}


		if (flag)
		{
			if (sendto(socketVar, "nack\0", 5, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
			{
				printf("Sending back failed, code %d", WSAGetLastError());
				return 1;
			}
		}
		else
		{
			if (sendto(socketVar, "ack\0", 5, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
			{
				printf("Sending back failed, code %d", WSAGetLastError());
				return 1;
			}
		}

		//free(tmpChar);//UVOLNIT AK SKONCIM S IMG!
		free(buffer);
	}
#pragma endregion
	
	tmpMsg = (char*)malloc((fragSize - 6)*fragCount);
	int tmpCount = 0;
	fragAck = 0;
	flag = 1;
	printf("Server initialized\n");
	//Keep listening for data
	while (flag)
	{
		printf("\nWaiting for data...\n");
		buffer = (char*)malloc(fragSize);

		if ((recieved_len = recvfrom(socketVar, buffer, fragSize, 0, (struct sockaddr*)&client, &socLen)) == SOCKET_ERROR)
		{
			printf("Recieving data failed, code %d\n", WSAGetLastError());
			return 1;

		}


		if (*(unsigned short*)buffer != crc_kermit(buffer + 2, fragSize - 2))
		{
			char *tmpAnswer = (char*)malloc(9);
			printf("Wrong CRC\n");
			*(unsigned int*)tmpAnswer = *(unsigned int*)((char*)buffer + 2);
			strcpy(((char*)tmpAnswer + 4), "nack\0");

			if (sendto(socketVar, tmpAnswer, 9, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
			{
				printf("Sending back failed, code %d", WSAGetLastError());
				return 1;
			}
			free(tmpAnswer);
			free(buffer);
			continue;
		}
		if (*(unsigned int*)((char*)buffer + 2) > (fragAck + 1))
		{
			printf("DISCARDING - fragment %d is too far ahead\n", *(unsigned int*)((char*)buffer + 2));
			free(buffer);
			continue;
		}

		if (*(unsigned int*)((char*)buffer + 2) < (fragAck+1))
		{
			printf("DISCARDING - fragment %d already recieved\n", *(unsigned int*)((char*)buffer + 2));
			free(buffer);
			continue;
		}
		
		//printf("Received packet from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		char *tmpAnswer = (char*)malloc(9);
		*(unsigned int*)tmpAnswer = *(unsigned int*)((char*)buffer + 2);
		strcpy(((char*)tmpAnswer + 4), "ack\0");

		if (file != NULL)
		{
			if (fragAck + 1 == fragCount)
			{
				fwrite(buffer + 6, 1, strlen(buffer+6), file);
				printf("Appended file %.*s\n", strlen(buffer + 6), buffer + 6);
				//printf("Want to write %d but should write %d %.*s\n", fragSize - 6, strlen(buffer + 6), strlen(buffer + 6), buffer + 6);
			}
			else
			{
				fwrite(buffer + 6, 1, fragSize - 6, file);
				printf("Appended file %.*s\n", fragSize - 6, buffer + 6);
			}
		}
		else
		{
			if (fragAck + 1 == fragCount)
			{
				printf("Data: %s\n", buffer + 6);
				strncpy(tmpMsg + ((fragAck)*(fragSize - 6)), buffer + 6, strlen(buffer+6)-1);
			}
			else
			{
				printf("Data: %s\n", buffer + 6);
				strncpy(tmpMsg + ((fragAck)*(fragSize - 6)), buffer + 6, fragSize - 6);
			}
		}

		if (fragAck + 1 == fragCount)
		{
			printf("Want to write %d but should write %d %.*s\n", fragSize - 6, strlen(buffer + 6), strlen(buffer + 6),buffer+6);
		}

		tmpCount++;
		printf("Pocet prijatych packetov %d\n", tmpCount);


		if (sendto(socketVar, tmpAnswer, 9, 0, (struct sockaddr*)&client, socLen) == SOCKET_ERROR)
		{
			printf("Sending back failed, code %d", WSAGetLastError());
			return 1;
		}
		free(tmpAnswer);
		free(buffer);

		fragAck++;

		printf("SENDING ACK\n");

		if (fragAck == fragCount)
		{
			printf("All fragments arrived, I should end\n");
			if (file == NULL)
			{
				printf("Final msg: %s\n", tmpMsg);
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
		}

	}


	closesocket(socketVar);
	WSACleanup();
	return 0;

}

void PrintStartMsg()
{
	printf("Commands are: \n");
	printf("\tset port xxx \n");
	printf("\tset ip xxx.xxx.xxx.xxx \n");
	printf("\tstart server (need to set port first)\n");
	printf("\tstart client (need to set ip and port first)\n");
	printf("\tend - ends program\n");

}

int GetCommand()
{
	char *pointerTmp;
	char *buffer = (char*)malloc(sizeof(char) * 255);
	struct sockaddr_in tmp;
	char ipFlag = 0, portFlag = 0;

	//NA ZISTENIE ADRESY
	//printf("__FILE__: %s\n", __FILE__);

	system("cls"); 

	PrintStartMsg();

	while (1)
	{
		printf("\nEnter command: ");
		fgets(buffer, 29, stdin);
		pointerTmp = strtok(buffer, " \n");

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
						free(buffer);
						if (server(tmp))
						{
							printf("There was a problem with something");
						}
						return 0;
						portFlag = 0;
						ipFlag = 0;
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
								free(buffer);
								if (client(tmp))
								{
									printf("There was a problem with something");
								}
								return 0;
								portFlag = 0;
								ipFlag = 0;
							}
					}
					else
						if (pointerTmp != NULL && !strcmp(pointerTmp, "tester"))
						{
							tmp.sin_port = 1000;
							portFlag = 1;
							tmp.sin_addr.S_un.S_addr = inet_addr("192.168.56.1");
							ipFlag = 1;
							client(tmp);
							return 0;
						}
						else
						printf("Wrong command");
			}
			else
				if (pointerTmp != NULL && !strcmp(pointerTmp, "quit"))
				{
					system("pause");
					exit(0);
				}
				else
					printf("Wrong command");

		}
	}

	return 0;
}

int main()
{
	init_crc_tab(); //Priprava pre CRC16 Kermit

	while (1)
	{
		GetCommand();
	}

	system("pause");

	return 0;
}