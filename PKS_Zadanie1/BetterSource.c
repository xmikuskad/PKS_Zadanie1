#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //winsock library

int GetCommand(char clientFlag, char serverFlag, char ipFlag, char portFlag);

//WRONG PACKET SETUP !! DONT FORGET
int client(struct sockaddr_in tmp)
{
	char *buffer, *message, *path;
	struct sockaddr_in si_other;
	int socketVar, slen = sizeof(si_other);
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
		int i = 0, tmp = 0;

		printf("Send msg or image? m/i ");
		if (getchar() == 'm')
		{
			getchar();

			//printf("Enter message:\n");
			message = malloc(sizeof(char) * 256);
			*(int*)message = 9999; //num of frag
			*(int*)((char*)message + 4) = 35; //size
			//*(message + 8) = "msg";
			//*(message + 11) = "test.png";
			//*(int*)((char*)message + 8) = 14;
			//*(short*)((char*)message + 8) = 14;
			strcpy(message + 8, "msg ");
			strcpy(message + 12, "test.png ");
			*(int*)((char*)message + 21) = 1110011;
				/*fgets(message, 255, stdin);
				printf("MSg later: %d\n", *(int*)message);
				while (message[i] != '/0' && message[i] != '\n')
				{
					i++;
				}*/
			printf("SENDING FIRT DATA");

			//send the message
			if (sendto(socketVar, message, 40, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
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

				printf("READ : %d\n",fread(tmpImg, 1, tmp, file));

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
	char *buffer;
	struct sockaddr_in server, si_other;
	int flag = 0, deletePls = 0;
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
	//server.sin_addr.s_addr = inet_addr("147.175.180.71");
	server.sin_port = htons(tmp.sin_port);

	printf("IP adress is: %s\n", inet_ntoa(server.sin_addr));

	//Bind
	if (bind(socketVar, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Binding failed, code %d\n", WSAGetLastError());
		//printf("Bind failed with error code : %d\n", WSAGetLastError());
		return 1;
	}

	//TESTING
	buffer = malloc(1500);
	if ((recieved_len = recvfrom(socketVar, buffer, 1500, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR)
	{
		printf("Recieving data failed, code %d\n", WSAGetLastError());
		return 1;

	}

	printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
	printf("Pocet fragmentov je %d\n", *(int*)buffer);
	printf("Velkost fragmentov je %d\n", *(int*)((char*)buffer + 4));

	char *tmpPointer = strtok(buffer+8, " ");
	int tmpLen = 0;

	printf("Typ spravy je %s ", tmpPointer);
	tmpLen += strlen(tmpPointer);
	tmpPointer = strtok(NULL, " ");

	printf("Nazov je %s ", tmpPointer);
	tmpLen += strlen(tmpPointer);
	tmpLen += 2; //lebo medzery
	printf("Checksum je %d\n", *(int*)((char*)buffer + 8+tmpLen));

	free(buffer);
	//KONIEC TESTU

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
			buffer = "Packets are OK";
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
			//printf("Set found\n");
			pointerTmp = strtok(NULL, " \n");
			if (!strcmp(pointerTmp, "port"))
			{
				pointerTmp = strtok(NULL, " \n");
				tmp.sin_port = atoi(pointerTmp);
				portFlag = 1;
				printf("Port: %d\n", tmp.sin_port);
			}
			else
				if (!strcmp(pointerTmp, "ip"))
				{
					pointerTmp = strtok(NULL, " \n");
					tmp.sin_addr.S_un.S_addr = inet_addr(pointerTmp);
					ipFlag = 1;
					printf("IP adress is: %s\n", inet_ntoa(tmp.sin_addr));
				}
				else
					printf("Wrong command\n");
		}
		else
		{
			//strcmp vracia 0 ak sa to podari
			if (!strcmp(pointerTmp, "start"))
			{
				//printf("Start found\n");
				pointerTmp = strtok(NULL, " \n");
				if (!strcmp(pointerTmp, "server"))
				{
					if (portFlag == 0)
					{
						printf("Set up port first! cmd: set port xxx\n");
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
						if (portFlag == 0 || ipFlag == 0)
						{
							printf("Set up port and ip first!\n");
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
						printf("Wrong command\n");
			}
			else
				if (!strcmp(pointerTmp, "end"))
				{
					printf("End called.\n");
					return 0;
				}
				else
					printf("Wrong command\n");

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
	//mkdir("images");
	PrintStartMsg();
	GetCommand(0,0,0,0);

	system("pause");
	getchar();
	return 0;
}