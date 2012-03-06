#pragma once
#include <map>
#include "MutexGuard.h"
#include "CityLampServer.h"

using std::map;

class PCClient;

typedef struct _LampMutexSock *LPLampMutexSockStruct;
typedef struct _PER_HANDLE_DATA *LPPER_HANDLE_DATA;
typedef struct _PER_IO_DATA *LPPER_IO_DATA;

class CityLamp
{
public:
	CityLamp(void);
	virtual ~CityLamp(void);
	void TransferDataToPCClient();
	void ClearLampID2LampMutexSockMap();
	void SendOfflineCmdToPCClient(u_short lampid);
	void SendOnlineCmdToPCClient();
	void SetLampID2LampMutexSockMap(u_short id, LPLampMutexSockStruct lpLMSS);
	int SendDataToLampbyID(u_short lampid, char* buf, int data_len);
	int GetLampIDCount() { return LampID2LampMutexSockMap.size(); }
	void RemoveLampID(u_short);
	bool QueryLampID(u_short lampid);

	static Mutex LampID2LampMutexSocketMapMutex;

private:
	map<u_short, LPLampMutexSockStruct> LampID2LampMutexSockMap;
	fd_set fd;
};

