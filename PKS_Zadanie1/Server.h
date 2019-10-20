#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //winsock library

#include <Windows.h>
#include <time.h>
#include "Crc16.h"

int server(struct sockaddr_in tmp);