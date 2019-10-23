#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //winsock library

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <time.h>
#include "Crc16.h"

char server(struct sockaddr_in tmp, char logFlag,WSADATA wsaData);