#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //winsock library


int client()
{
	char *buffer,*message,*path;
	struct sockaddr_in tmp, si_other;
	int socketVar,port, slen = sizeof(si_other);
	WSADATA wsa;

	printf("Enter server IP adress: ");
	buffer = malloc(20 * sizeof(char));
	scanf("%s", buffer);
	tmp.sin_addr.S_un.S_addr = inet_addr(buffer);
	printf("IP adress is: %s\n", inet_ntoa(tmp.sin_addr));

	printf("Enter port: ");
	scanf_s("%d", &port);
	getchar();
	printf("Port is %d\n", port);

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
	si_other.sin_port = htons(port);
	si_other.sin_addr.S_un.S_addr = inet_addr(buffer);

	free(buffer);
	buffer = malloc(256 * sizeof(char));

	//start communication
	while (1)
	{
		int i = 0, tmp = 0;

		printf("Send msg or image? m/i ");
		if (getchar() == 'm')
		{
			getchar();

			printf("Enter message:\n");
			message = malloc(sizeof(char) * 256);
			fgets(message, 255, stdin);

			while (message[i] != '/0' && message[i] != '\n')
			{
				i++;
			}

			//send the message
			if (sendto(socketVar, message, i, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
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
				printf("IMAGE OK FAILED\n%s",buffer);
				continue;
			}
			

			//IMAGE MANAGING
			FILE *file = NULL;
			int fileLen;

			if ((file=(fopen(path, "rb"))) == NULL)
			{
				printf("File failed to open\n%s\n",path);
				return 1;
			}

			fseek(file, 0, SEEK_END);
			fileLen = ftell(file);
			fseek(file, 0, SEEK_SET);

			tmpImg = malloc(501 * sizeof(char));
			//Sending file
			while (fileLen >0)
			{
				if (fileLen > 500)
					tmp = 500;
				else
					tmp = fileLen;

				free(tmpImg);
				tmpImg = malloc(tmp * sizeof(char));

				fread(tmpImg, 1, tmp, file);

				printf("File contains: %s", tmpImg);

				//send the message
				if (sendto(socketVar, tmpImg, tmp, 0, (struct sockaddr *)&si_other, slen) == SOCKET_ERROR)
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
				printf("%d Reply: %s\n", fileLen,buffer);
				fileLen -= 500;
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


	return 0;
}

int server()
{
	WSADATA wsa;
	SOCKET socketVar;
	int slen, recieved_len,port;
	char *buffer;
	struct sockaddr_in server, si_other;//,tmp;
	int flag = 0, deletePls = 0;
	FILE *file = NULL;

	slen = sizeof(si_other); //WTF?

	printf("Enter port: ");
	scanf_s("%d", &port);
	printf("Port is %d\n", port);

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
	server.sin_port = htons(port);

	printf("IP adress is: %s\n", inet_ntoa(server.sin_addr));

	//Bind
	if (bind(socketVar, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Binding failed, code %d\n", WSAGetLastError());
		//printf("Bind failed with error code : %d\n", WSAGetLastError());
		return 1;
	}

	printf("Server initialized\n");
	//Keep listening for data
	while (1)
	{
		printf("\nWaiting for data...\n");
		//free(buffer);
		buffer = malloc(501);

		if ((recieved_len = recvfrom(socketVar, buffer, 501, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR)
		{
			printf("Recieving data failed, code %d\n", WSAGetLastError());
			return 1;

		}

		printf("Received packet from %s:%d\n",inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n",buffer);

		if (flag)
		{
			file = fopen("C:\\Users\\Dominik\\Desktop\\ccc.png", "wb");
		}


		if (file != NULL)
		{
			//fputs(buffer, file);
			fwrite(buffer, 1, 500, file);
			printf("Appended file\n");
		}


		if (strstr(buffer, "image") == NULL)
		{
			flag = 0;
			//printf("image NOT FOUND\n");
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

		//if (sendto(socketVar, buffer, recieved_len, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
		if (sendto(socketVar, buffer, 20, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
		{
			printf("Sending back failed, code %d", WSAGetLastError());
			return 1;
		}
		
		//free(buffer);


	}


	closesocket(socketVar);
	WSACleanup();

	return 0;
	
}

int main()
{

	while (1)
	{
		char buffer[5];

		printf("\nChoose function\n");
		printf(" [1] send data\n");
		printf(" [2] accept data\n");
		printf("1/2? ");

		fgets(buffer, 5, stdin);
		//sscanf_s("%c", &choice);

		if (buffer[0] == '1')
		{
			//flag = 1;
			if (client())
			{
				printf("Sending canceled.\n");
				//flag = 0;
			}
			break;
		}
		else
			if (buffer[0] == '2')
			{
				//flag = 1;
				if (server())
				{
					printf("Accepting canceled.\n");
					//flag = 0;
				}
				break;
			}
			else
			{
				printf("Wrong choice!\n");
			}

		//while (flag) {}
	}

	system("pause");
	getchar();
	return 0;
}

