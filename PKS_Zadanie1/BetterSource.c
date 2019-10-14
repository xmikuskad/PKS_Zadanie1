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
#include <math.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //winsock library

#define		CRC_POLY_KERMIT		0x8408
#define		CRC_START_KERMIT	0x0000
 //pouzivanie polynomu x16 + x12 + x5 + 1 ale inverzneho!

static unsigned short	crc_tab[256];

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
	int a;

	crc = CRC_START_KERMIT;
	ptr = input_str; 

		if (ptr != NULL) for (a = 0; a < num_bytes; a++) {

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

int GetCommand(char clientFlag, char serverFlag, char ipFlag, char portFlag);

char* CreateFirstPacket(unsigned int fragmentsCount, short sizeOfGFragments, char type, char *path)
{
	int size = 2;
	char *message = malloc(sizeof(char) * 256);


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
		strcpy(message + size, path); 
		size += strlen(path);
		message[size++] = '\0';
	}

	//create checksum without checksum!
	*(unsigned short*)(message) = crc_kermit(message+2, 254);

	//free(message);
	return message;
}

//WRONG PACKET SETUP !! DONT FORGET
int client(struct sockaddr_in tmp)
{
	char *buffer, *message, *path;
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	SOCKET socketVar;
	WSADATA wsa;

	printf("Initializing client\n");

	//Set up WSA
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
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

	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(tmp.sin_port);
	si_other.sin_addr.S_un.S_addr = tmp.sin_addr.S_un.S_addr; //inet_addr(buffer);

	buffer = malloc(256 * sizeof(char));

	while (1)
	{
		int i = 0, tmp = 0, fragSize = 0, flag = 1, windowSize = 20, sendCount = 0;
		
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

			//Sprava moze mat az 2000 znakov
			printf("Enter message:\n");
			message = malloc(sizeof(char) * 2800);
			fgets(message, 2800, stdin);
			while (message[i] != '/0' && message[i] != '\n')
			{
				i++;
			}

#pragma region SEND_FIRST_PACKET_MSG
			flag = 1;
			while (flag)
			{
				printf("SENDING FIRST PACKET\n");
				if (sendto(socketVar, CreateFirstPacket((i + fragSize - 1) / fragSize, fragSize, 'm', NULL), 256, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}

				if (recvfrom(socketVar, buffer, 5, 0, (struct sockaddr *)&si_other, &slen) == SOCKET_ERROR)
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

			char *tmpImg;
			int sizeImg = 0;

			printf("Enter image position:\n");
			path = malloc(sizeof(char) * 256);
			fgets(path, 255, stdin);

			while (path[i] != '\n')
			{
				i++;
			}
			path[i] = '\0';

#pragma region SEND_FIRST_PACKET_IMG
			flag = 1;
			while (flag)
			{
				printf("SENDING FIRST PACKET\n");
				if (sendto(socketVar, CreateFirstPacket((i + fragSize - 1) / fragSize, fragSize, 'i', path), 256, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}

				if (recvfrom(socketVar, buffer, 5, 0, (struct sockaddr *)&si_other, &slen) == SOCKET_ERROR)
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

			message = malloc(20);
			message = "image";


			//send the message
			if (sendto(socketVar, message, 20, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
			{
				printf("Sending message failed, code %d\n", WSAGetLastError());
				return 1;
			}


			printf("Recieving...\n");
			//try to receive some data, this is a blocking call
			if (recvfrom(socketVar, buffer, 255, 0, (struct sockaddr *)&si_other, &slen) == SOCKET_ERROR)
			{
				printf("recvfrom() failed, code %d: \n", WSAGetLastError());
				return 1;
			}

			printf("Buffer %s\n", buffer);

			if (strstr(buffer, "IMAGE OK") == NULL)
			{
				printf("IMAGE OK FAILED\n%s", buffer);
				continue;
			}


			//IMAGE MANAGING
			FILE *file = NULL;
			int fileLen;

			if ((file = (fopen(path, "rb"))) == NULL)
			{
				printf("File failed to open\n%s\n", path);
				return 1;
			}

			fseek(file, 0, SEEK_END);
			fileLen = ftell(file);
			fseek(file, 0, SEEK_SET);

			int tmpCount = 0;
			tmpImg = malloc(1500 * sizeof(char));
			//Sending file
			while (fileLen > 0)
			{
				if (fileLen > 1500)
					tmp = 1500;
				else
					tmp = fileLen;
					
				//tmp = 1500;
				free(tmpImg);
				tmpImg = malloc(tmp * sizeof(char));

				printf("READ : %zd\n",fread(tmpImg, 1, tmp, file));

				printf("File contains: %s", tmpImg);

				//send the message
				if (sendto(socketVar, tmpImg, tmp, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
				{
					printf("Sending message failed, code %d\n", WSAGetLastError());
					return 1;
				}


				printf("\nRecieving...\n");
				//try to receive some data, this is a blocking call
				if (recvfrom(socketVar, buffer, tmp, 0, (struct sockaddr *)&si_other, &slen) == SOCKET_ERROR)
				{
					printf("recvfrom() failed, code %d: \n", WSAGetLastError());
					return 1;
				}
				printf("%d Reply: %s\n", fileLen, buffer);
				printf("Number od packets %d\n", ++tmpCount);
				fileLen -= tmp;
			}
		}


		printf("Do you want to continue? y/n ");
		if (getchar() == 'n')
		{
			break;
		}
		getchar();
	}

	closesocket(socketVar);
	WSACleanup();

	GetCommand(0, 0, 0, 0);

	return 0;
}

int server(struct sockaddr_in tmp)
{
	WSADATA wsa;
	SOCKET socketVar;
	int slen, recieved_len;
	char *buffer, *tmpChar,*ackString, *nackString;
	char imgFlag = 0;
	struct sockaddr_in server, si_other;
	int flag = 0, deletePls = 0;
	//SPECIAL VALUES:
	unsigned int fragCount;
	short fragSize;
	unsigned short	checksum;
	
	//Load variables
	ackString = malloc(4);
	strcpy(ackString, "ack\0");
	nackString = malloc(5);
	strcpy(nackString, "nack\0");

#pragma region load Winsock + connection

	FILE *file = NULL;

	slen = sizeof(si_other); //WTF?

	printf("Initializing server\n");

	//Set up WSA
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
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
		buffer = malloc(256);
		if ((recieved_len = recvfrom(socketVar, buffer, 256, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR)
		{
			printf("Recieving data failed, code %d\n", WSAGetLastError());
			return 1;

		}

		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		checksum = *(unsigned short*)(buffer);
		unsigned short checksum2 = crc_kermit(buffer + 2, 254);

		if (checksum != checksum2)
		{
			printf("Checksum is different :(\n"); //TO DO!
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
			tmpChar = malloc(242);

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
			}

		}


		if (flag)
		{
			if (sendto(socketVar, nackString, 5, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
			{
				printf("Sending back failed, code %d", WSAGetLastError());
				return 1;
			}
		}
		else
		{
			if (sendto(socketVar, ackString, 4, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
			{
				printf("Sending back failed, code %d", WSAGetLastError());
				return 1;
			}
		}

		//free(tmpChar);//UVOLNIT AK SKONCIM S IMG!
		free(buffer);
	}
#pragma endregion
	

	int tmpCount = -1;
	printf("Server initialized\n");
	//Keep listening for data
	while (1)
	{
		printf("\nWaiting for data...\n");
		//free(buffer);
		buffer = malloc(1500);

		if ((recieved_len = recvfrom(socketVar, buffer, 1500, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR)
		{
			printf("Recieving data failed, code %d\n", WSAGetLastError());
			return 1;

		}

		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buffer);
		printf("Pocet prijatych packetov %d\n", ++tmpCount);
		if (flag)
		{
			//char *finalPath = malloc(200);
			//strcat(finalPath, __FILE__);
			//strcat(finalPath, "\ccc.png");

			//file = fopen("C:\\Users\\Dominik\\Desktop\\ccc.png", "wb");
			//mkdir("images");
			if((file = fopen("ccc.png", "wb")) == NULL)
				printf("fopen failed\n");
			//file = fopen(*finalPath, "wb");
		}


		if (file != NULL)
		{
			fwrite(buffer, 1, 1500, file);
			printf("Appended file\n");
		}


		if (strstr(buffer, "image") == NULL)
		{
			flag = 0;
			free(buffer);
			buffer = malloc(30 * sizeof(char));
			*(int*)buffer = tmpCount;
			//buffer = "IMAGE OK";
			strcpy(buffer + 4, "PACKETS OK");
		}
		else
		{
			flag = 1;
			free(buffer);
			buffer = malloc(30 * sizeof(char));
			buffer = "IMAGE OK";
		}

		if (sendto(socketVar, buffer, 20, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
		{
			printf("Sending back failed, code %d", WSAGetLastError());
			return 1;
		}

	}


	closesocket(socketVar);
	WSACleanup();

	return 0;

}

int GetCommand(char clientFlag, char serverFlag, char ipFlag, char portFlag)
{
	char *pointerTmp;
	char *buffer = malloc(sizeof(char) * 255);
	struct sockaddr_in tmp;

	//NA ZISTENIE ADRESY
	//printf("__FILE__: %s\n", __FILE__);

	while (1)
	{
		printf("\nEnter command: ");
		fgets(buffer, 29, stdin);
		pointerTmp = strtok(buffer, " ");

		if (!strcmp(pointerTmp, "set"))
		{
			pointerTmp = strtok(NULL, " \n");
			if (!strcmp(pointerTmp, "port"))
			{
				pointerTmp = strtok(NULL, " \n");
				tmp.sin_port = atoi(pointerTmp);
				portFlag = 1;
				printf("Port set to %d", tmp.sin_port);
			}
			else
				if (!strcmp(pointerTmp, "ip"))
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
			if (!strcmp(pointerTmp, "start"))
			{
				pointerTmp = strtok(NULL, " \n");
				if (!strcmp(pointerTmp, "server"))
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
						server(tmp);
						return 0;
					}
				}
				else
					if (!strcmp(pointerTmp, "client"))
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
								client(tmp);
								return 0;
							}
					}
					else
						if (!strcmp(pointerTmp, "tester"))
						{
							tmp.sin_port = 1000;
							portFlag = 1;
							tmp.sin_addr.S_un.S_addr = inet_addr("192.168.56.1");
							ipFlag = 1;
							client(tmp);
						}
						else
						printf("Wrong command");
			}
			else
				if (!strcmp(pointerTmp, "quit"))
				{
					printf("quit called.\n");
					return 0;
				}
				else
					printf("Wrong command");

		}
	}

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

int main()
{
	init_crc_tab(); //Priprava pre CRC16 Kermit
	
	printf("%d\n", 5 / 2);
	printf("CEIL %d\n", (5 + 2 - 1) / 2);

	PrintStartMsg();
	GetCommand(0,0,0,0);

	system("pause");
	getchar();
	return 0;
}