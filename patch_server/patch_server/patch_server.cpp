// patch_server.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#include <list>
#include <string>
using namespace std;

#pragma comment (lib,"ws2_32")


#define BUFSIZE 1024
#define SMALLBUF 100
#define SERVER_PORT 9918

unsigned int __stdcall ClntConnect(void *arg);
void SendData(SOCKET sock, char* ct, char* fileName);
void ErrorHandling(char *message);

unsigned int hWeblogThread;

extern list<string> logs;

int _tmain(int argc,TCHAR **args){
	SOCKET hServSock;
	SOCKET hClntSock;

	HANDLE hThread;
	DWORD dwThreadID;

	SOCKADDR_IN servAddr;
	SOCKADDR_IN clntAddr;
	int clntAddrSize;

	WSAData wsa;

	SetConsoleTitleA("Patch Server");

	WSAStartup(MAKEWORD(2,2), &wsa);

	hServSock=socket(PF_INET, SOCK_STREAM, 0);   
	if(hServSock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servAddr.sin_port=htons(SERVER_PORT);

	if(bind(hServSock, (SOCKADDR*) &servAddr, sizeof(servAddr))==SOCKET_ERROR)
		ErrorHandling("bind() error");

	if(listen(hServSock, 5)==SOCKET_ERROR)
		ErrorHandling("listen() error");

	printf("Patch server ready\n");
	while(1){
		clntAddrSize=sizeof(clntAddr);
		hClntSock=accept(hServSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
		if(hClntSock==INVALID_SOCKET)
			ErrorHandling("accept() error");

		printf("connected... %s\n",inet_ntoa(clntAddr.sin_addr));

		hThread = (HANDLE)_beginthreadex(NULL, 0, ClntConnect, (void*)hClntSock, 0, (unsigned *)&dwThreadID);
		if(hThread == 0) {
			printf("cannot create client thread\n");
		}
	}

	WSACleanup();

	return 0;
}

unsigned int __stdcall ClntConnect(void *arg)
{
	SOCKET hClntSock=(SOCKET)arg;
	char buf[BUFSIZE];
	char method[SMALLBUF];
	char ct[SMALLBUF];
	char fileName[SMALLBUF];

	int len;
	
	len = recv(hClntSock, buf, BUFSIZE, 0);
	buf[len] = '\0';

	if(strstr(buf, "HTTP/")==NULL){
		closesocket(hClntSock);
		return 1;
	}

	strcpy(method, strtok(buf, " /"));
	if(strcmp(method, "GET"))
		closesocket(hClntSock);

	strcpy(fileName, strtok(NULL, " /"));
	strcpy(ct, "text/plain");

	SendData(hClntSock, ct, fileName); 
	return 0;
}

void SendData(SOCKET sock, char* ct, char* fileName) 
{	
	char protocol[]="HTTP/1.1 200 OK\r\n";
	char server[]="Server:AppDeungE\r\n";
	char cntLen[]="Content-length:";
	char connectionType[]="Connection: close\r\n";
	char cntType[SMALLBUF];
	char buf[BUFSIZE];	
	int len;

	HANDLE fp;

	char path[256];
	sprintf(path,"data\\%s", fileName);

	printf("send file %s\n", fileName);

	fp = CreateFileA(	path,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

	if(fp == INVALID_HANDLE_VALUE)
		goto CloseSocket;
	

	while(1){
		char buffer[1024];
		DWORD dwRead;
		int sent;

		ReadFile(fp,buffer,1024,&dwRead,NULL);
		sent = send(sock,buffer,dwRead,0);

		if(sent == -1){
			printf("send aborted %s\n", fileName);
			goto CloseFile;
		}
		if(dwRead != 1024)
			break;
	}

	printf("send ok %s\n", fileName);

CloseFile:;
	CloseHandle(fp);
CloseSocket:;
	closesocket(sock);
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
}