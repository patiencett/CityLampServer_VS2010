// CityLampServer.h
#pragma once

#include "windows.h"
#include "winsock2.h"
#include "PCClient.h"
#include "MutexGuard.h"
#include "CityLamp.h"

#define MAXBUFSIZE 1024

const u_short MINPORT = 1;
const u_short MAXPORT = 65535;

class PCClient;
class CityLamp;

using std::map;
using std::list;

SOCKET CreateSocket(const int port, struct in_addr ipaddress);
SOCKET CreateSocket(const int port);
void closemysocket(SOCKET sock);
void SendDataToPCClient(char *buf, int datalength);
void UpdateLampListView(bool operation, u_short lampid);
void UpdatePCListView();

void StartServer();
void StopServer();
void RestartServer();

int Recvn(SOCKET s, char *DataBuf, int DataLen);
int Sendn(SOCKET s, char *DataBuf, int DataLen);
bool CheckCheckSum(char* buf, int len);
void AddCheckSum(char *buf, int len);
wchar_t* AnsiToUnicode(const char* szStr);

extern bool beRunning;

extern list<PCClient*> PCClientList;
extern CityLamp citylamp;
extern Mutex PCClientListMutex;	//提供对PCClientList的互斥访问
extern int LampServerPort;
extern struct in_addr LampServerIpAddress;
extern bool bHideWindow;
typedef Mutex *LPMutex;

typedef enum
{
	RECV_POSTED,
	SEND_POSTED
}OPERATION_TYPE;

typedef struct _PER_HANDLE_DATA
{
	SOCKET Socket;
	SOCKADDR_STORAGE ClientAddr;
	u_short LampID;
	_PER_HANDLE_DATA(SOCKET sock, sockaddr_in *addr, u_short lampid)
	{
		memset(this, 0, sizeof(_PER_HANDLE_DATA));
		Socket = sock;
		LampID = lampid;
		memcpy(&ClientAddr, addr, sizeof(sockaddr_in));
	}
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct _PER_IO_DATA
{
	WSAOVERLAPPED Overlapped;
	WSABUF Buffer;
	char szMessage[MAXBUFSIZE];
	DWORD NumberOfBytesTransferred;
	DWORD Flags;
	OPERATION_TYPE OperationType;
	_PER_IO_DATA(OPERATION_TYPE op = RECV_POSTED)
	{
		Initialization(op);
	}
	void Initialization(OPERATION_TYPE op = RECV_POSTED)
	{
		memset(this, 0, sizeof(_PER_IO_DATA));
		Buffer.len = MAXBUFSIZE;
		Buffer.buf = szMessage;
		OperationType = op;
	}
}PER_IO_DATA, *LPPER_IO_DATA;

typedef struct _LampMutexSock
{
	LPMutex lpMutex;
	SOCKET sock;
	LPPER_HANDLE_DATA lpPerHandleData;
	LPPER_IO_DATA lpPerIOData;
	_LampMutexSock(SOCKET socket, sockaddr_in *addr, u_short lampid, OPERATION_TYPE op)
	{
		sock = socket;
		lpMutex = new Mutex();
		lpPerHandleData = new PER_HANDLE_DATA(socket, addr, lampid);
		/*
		lpPerHandleData = (LPPER_HANDLE_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_HANDLE_DATA));
		lpPerHandleData->Socket = sock;
		memcpy(&lpPerHandleData->ClientAddr, &addr, sizeof(sockaddr_in));
		lpPerHandleData->LampID = lampid;
		*/

		lpPerIOData = new PER_IO_DATA(op);
		/*
		lpPerIOData = (LPPER_IO_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_DATA));
		lpPerIOData->Buffer.len = MAXBUFSIZE;
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
		lpPerIOData->OperationType = RECV_POSTED;
		*/
	}
	~_LampMutexSock()
	{
		UpdateLampListView(false, lpPerHandleData->LampID);
		closesocket(sock);
		delete lpMutex;
		delete lpPerHandleData;
		//HeapFree(GetProcessHeap(), 0, lpPerHandleData);
		delete lpPerIOData;
		//HeapFree(GetProcessHeap(), 0, lpPerIOData);
	}
}LampMutexSockStruct, *LPLampMutexSockStruct;