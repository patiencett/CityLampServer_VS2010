#include "StdAfx.h"
#include "CityLamp.h"

Mutex CityLamp::LampID2LampMutexSocketMapMutex;

CityLamp::CityLamp(void)
{
	
}

int CityLamp::SendDataToLampbyID(u_short lampid, char* buf, int data_len)
{
	int data_sent = 0;
	//����map���Ƿ������lampid��Ӧ��sock��mutex
	map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.find(lampid);
	if (map_it != LampID2LampMutexSockMap.end())	//�����ҵ���Mutex��sock���л���д��
	{
		SOCKET sock = LampID2LampMutexSockMap[lampid]->sock;
		Mutex *pmutex = LampID2LampMutexSockMap[lampid]->lpMutex;
		{
			MutexGuard guard(*pmutex);
			data_sent = Sendn(sock, buf, data_len);
		}
		if (data_sent < data_len)
		{
			delete map_it->second;
			{
				MutexGuard guard(LampID2LampMutexSocketMapMutex);
				LampID2LampMutexSockMap.erase(lampid);						
			}
		}
	}
	else		//��������lampid��Ӧ��sock��mutex
	{
		return -1;
	}
	return data_sent;
}

void CityLamp::RemoveLampID(u_short lampid)
{
	{
		MutexGuard guard(LampID2LampMutexSocketMapMutex);
		map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.find(lampid);
		if (map_it != LampID2LampMutexSockMap.end())	//�����ҵ���Mutex��sock���л���д��
		{
			delete map_it->second;
			LampID2LampMutexSockMap.erase(lampid);
		}
	}
}

bool CityLamp::QueryLampID(u_short lampid)
{
	{
		MutexGuard guard(LampID2LampMutexSocketMapMutex);
		map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.find(lampid);
		if (map_it != LampID2LampMutexSockMap.end())	//�����ҵ���Mutex��sock���л���д��
			return TRUE;
		else return FALSE;
	}
}

void CityLamp::SetLampID2LampMutexSockMap(u_short id, LPLampMutexSockStruct lpLMSS)
{
	{
		MutexGuard guard(LampID2LampMutexSocketMapMutex);
		LampID2LampMutexSockMap[id] = lpLMSS;
	}
}

/*
void CityLamp::TransferDataToPCClient()
{
	char *buf = new char[MAXBUFSIZE];
	while (beRunning)
	{
		int mapSize = 0;
		bool beNeedToUpdateLampListView = false;
		mapSize = LampID2LampMutexSockMap.size();
		if (mapSize == 0)
		{
			Sleep(100);
			continue;
		}

		struct timeval time_out;
		time_out.tv_sec = 2;	//���ó�ʱʱ��Ϊ2s����Ҫѭ������fd�����Լ����½�������
		time_out.tv_usec = 0;

		FD_ZERO(&fd);
		{
			MutexGuard guard(LampID2LampMutexSocketMapMutex);
			map<u_short, LampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.begin();
			while (map_it != LampID2LampMutexSockMap.end())
			{
				FD_SET(map_it->second.sock, &fd);
				++map_it;
			}
		}

		int err = select(0, &fd, NULL, NULL, &time_out);
		if ((err == SOCKET_ERROR) || (err == 0))
		{
			continue;
		}
		{
			MutexGuard guard(LampID2LampMutexSocketMapMutex);
			map<u_short, LampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.begin();
			while (map_it != LampID2LampMutexSockMap.end() && beRunning)
			{
				if (FD_ISSET(map_it->second.sock, &fd))		//�����ݶ��������ӹر�
				{
					u_short lampid = map_it->first;
					Mutex *pmutex = map_it->second.mutex;
					int datalength = recv(map_it->second.sock, buf, MAXBUFSIZE, 0);
					if (datalength <= 0)	//���ӹر���ɾ����lampid sock pair
					{
						//closemysocket(map_it->second);
						delete pmutex;
						map_it = LampID2LampMutexSockMap.erase(map_it);
						beNeedToUpdateLampListView = true;
						UpdateLampListView(false, lampid);
						SendOfflineCmdToPCClient(lampid);
						//FD_CLR(map_it->second.sock, &fd);
						continue;
					}
					//�������յ����ݣ�ת������Ӧ��pcsock
					else
					{
						SendDataToPCClient(buf, datalength);
					}
				}
				++map_it;
			}
		}
	}

	ClearLampID2LampMutexSockMap();
	delete[] buf;
}
*/
//��pc����lampid��������
void CityLamp::SendOfflineCmdToPCClient(u_short lampid)
{
	char buf[7];
	buf[0] = 0x5A;
	buf[1] = 0x06;
	char* pid = (char*)&lampid;
	buf[2] = pid[0];
	buf[3] = pid[1];
	buf[4] = 0xFF;
	buf[5] = 0x00;
	buf[6] = 0x00;
	AddCheckSum(buf, sizeof(buf));
	SendDataToPCClient(buf, sizeof(buf));
}

void CityLamp::SendOnlineCmdToPCClient()
{
	char buf[7];
	MutexGuard guard(LampID2LampMutexSocketMapMutex);
	map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.begin();
	while (map_it != LampID2LampMutexSockMap.end())
	{
		u_short lampid = map_it->first;
		buf[0] = 0x5A;
		buf[1] = 0x06;
		char* pid = (char*)&lampid;
		buf[2] = pid[0];
		buf[3] = pid[1];
		buf[4] = 0xFE;
		buf[5] = 0x00;
		buf[6] = 0x00;
		AddCheckSum(buf, sizeof(buf));
		SendDataToPCClient(buf, sizeof(buf));
		++map_it;
	}
}
void CityLamp::ClearLampID2LampMutexSockMap(void)
{
	MutexGuard guard(LampID2LampMutexSocketMapMutex);
	map<u_short, LPLampMutexSockStruct>::iterator map_it = LampID2LampMutexSockMap.begin();
	while (map_it != LampID2LampMutexSockMap.end())
	{
		delete map_it->second;
		map_it = LampID2LampMutexSockMap.erase(map_it);
	}
	/*
	while (map_it != LampID2LampMutexSockMap.end())
	{
	//SendOfflineCmdToPCClient(map_it->first);
	//closemysocket(map_it->second.sock);
	shutdown(map_it->second.sock, SD_BOTH);
	//UpdateLampListView(false, map_it->first);
	++map_it;
	}
	*/
	//LampID2LampMutexSockMap.clear();
}

CityLamp::~CityLamp(void)
{
	//ClearLampID2LampMutexSockMap();
}
