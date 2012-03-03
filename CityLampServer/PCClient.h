#pragma once

#include <list>
#include <map>
#include <objbase.h>
#include "CityLamp.h"
#include "CityLampServer.h"
#include "MutexGuard.h"

using std::map;
using std::list;

class CityLamp;

class PCClient
{
public:
	PCClient(void);
	PCClient(SOCKET sock, GUID guid, sockaddr_in &addr);

	SOCKET& GetDataSocket() { return PCDataSock; }

	void TransferDataToLamp();

	LPCTSTR GetHostname() { return hostname; }
	virtual ~PCClient(void);
	
	GUID GetPCID() { return pcid; }

	bool GetStatus() { return beAlive; }
	void SetStatus(bool status) { beAlive = status; }
	void Error(const TCHAR* msg);

	void SetPCAddrIn(const sockaddr_in &addr) { pc_addr_in = addr; }
	sockaddr_in& GetPCAddrIn() { return pc_addr_in; }
	
	static Mutex LampID2PCSocketMapMutex;
private:
	SOCKET PCDataSock;
	u_short DataPort;
	TCHAR hostname[256];
	map<u_short, SOCKET> LampID2LampSockMap;
	map<u_short, SOCKET> LampID2PCSocketMap;
	HANDLE hClientThread;
	bool beAlive;
	GUID pcid;		//全局唯一标识符
	fd_set fd;
	struct sockaddr_in pc_addr_in;
};