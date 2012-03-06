#include "StdAfx.h"
#include "PCClient.h"

Mutex PCClient::LampID2PCSocketMapMutex;

PCClient::PCClient(void)
{
}

PCClient::PCClient(SOCKET sock, GUID guid, sockaddr_in &addr) : beAlive(true)
{
	PCDataSock = sock;
	pcid = guid;
	
	hostent* phost = gethostbyaddr((const char*)&(addr.sin_addr), sizeof(addr.sin_addr), AF_INET);
	if (phost == NULL)
		wsprintfW(hostname, L"%hs", "Unknown");
	else
		wsprintfW(hostname, L"%hs", phost->h_name);
	pc_addr_in = addr;
}

void PCClient::Error(const TCHAR* msg)
{
	closemysocket(PCDataSock);
	//MessageBox(NULL, msg, L"Error", MB_OK);
	beAlive = false;
}

void PCClient::TransferDataToLamp()
{
	char* buf = new char[MAXBUFSIZE];
	int data_head_len = 2, data_body_len, data_len;
	u_short lampid;
	while(beAlive && beRunning)
	{
		if (Recvn(PCDataSock, buf, data_head_len) < data_head_len)
		{
			Error(L"pcdatasock error");
			return;
		}
		data_body_len = buf[1] - 1;
		data_len = data_body_len + 2;

		if (Recvn(PCDataSock, buf + 2, data_body_len) < data_body_len)
		{
			Error(L"pcdatasock error");
			return;
		}
		if (CheckCheckSum(buf, data_len))
		{
			lampid = *(u_short*)(buf + 2);
			citylamp.SendDataToLampbyID(lampid, buf, data_len);
		}
	}
	delete[] buf;
}

//废除
/*
void PCClient::TransferDataToLamp(char *buf)
{
	int mapSize = 0;
	{
		MutexGuard guard(LampID2PCSocketMapMutex);
		mapSize = LampID2PCSocketMap.size();
	}
	if (mapSize == 0)
	{
		Sleep(100);
		return;
	}

	struct timeval time_out;
	time_out.tv_sec = 2;	//设置超时时间为2s，需要循环更新fd集合以加入新建的连接
	time_out.tv_usec = 0;
	
	FD_ZERO(&fd);
	{
		MutexGuard guard(LampID2PCSocketMapMutex);
		map<u_short,SOCKET>::iterator map_it = LampID2PCSocketMap.begin();
		while (map_it != LampID2PCSocketMap.end())
		{
			FD_SET(map_it->second, &fd);
			++map_it;
		}
	}
	int err = select(0, &fd, NULL, NULL, &time_out);
	if ((err == SOCKET_ERROR) || (err == 0))
	{
		return;
	}
	{
		MutexGuard guard(LampID2PCSocketMapMutex);
		map<u_short,SOCKET>::iterator map_it = LampID2PCSocketMap.begin();
		while (map_it != LampID2PCSocketMap.end())
		{
			if (FD_ISSET(map_it->second, &fd))		//有数据读或者连接关闭
			{
				u_short lampid = map_it->first;
				int datalength = recv(map_it->second, buf, MAXBUFSIZE, 0);
				if (datalength <= 0)	//连接关闭则删除此lampid sock pair
				{
					map_it = LampID2PCSocketMap.erase(map_it);
					UpdatePCListView();
					continue;
				}
				//正常接收到数据，转发至相应的pcsock
				else
				{
					if (TransferDataToCityLamp(map_it->first, buf, datalength))	//转发成功
					{
						++map_it;
					}
					else		//转发失败
					{
						closemysocket(map_it->second);
						map_it = LampID2PCSocketMap.erase(map_it);	
						UpdatePCListView();
					}
				}
			}
			else ++map_it;
		}
	}
}
void PCClient::RemovePCSocketByLampID(u_short id)
{
	{
		MutexGuard guard(LampID2PCSocketMapMutex);

		if (LampID2PCSocketMap.count(id))
		{
			if (LampID2PCSocketMap[id] != 0 && LampID2PCSocketMap[id] != INVALID_SOCKET)
			{
				closesocket(LampID2PCSocketMap[id]);
			}
			LampID2PCSocketMap.erase(id);
		}
	}
}
*/

PCClient::~PCClient(void)
{
	/*
	{
		MutexGuard guard(LampID2PCSocketMapMutex);
		map<u_short,SOCKET>::iterator map_it = LampID2PCSocketMap.begin();
		while (map_it != LampID2PCSocketMap.end())
		{
			closemysocket(map_it->second);
			++map_it;
		}
		LampID2PCSocketMap.clear();
	}
	*/
}
