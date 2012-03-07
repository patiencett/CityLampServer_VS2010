// CityLampServer.cpp : main source file for CityLampServer.exe
//
#include "stdafx.h"
#include "CityLampServer.h"
#include "resource.h"
#include "aboutdlg.h"
#include "MainDlg.h"

#pragma comment(lib, "ws2_32.lib")

CAppModule _Module;

DWORD WINAPI Server();
DWORD WINAPI LampWorkerThread(LPVOID);
SOCKET CreateSocket(const int);
void closemysocket(SOCKET sock);
DWORD WINAPI PCClientThread(void* pc);
DWORD WINAPI LampThread();
void SendDataToPCClient(char *buf, int datalength);
void DelPCClientFromList(PCClient *pc);

void UpdateServerStatus();

bool beRunning = false;
SOCKET server_sock;
int LampServerPort = 5566;
struct in_addr LampServerIpAddress;
HANDLE completionPort = INVALID_HANDLE_VALUE;
bool bHideWindow = FALSE;

list<PCClient*> PCClientList;
list<SOCKET> TempSock;
CityLamp citylamp;
Mutex PCClientListMutex;	//提供对PCClientList的互斥访问

CMainDlg dlgMain;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
	
	dlgMain.ShowWindow(nCmdShow);

	StartServer();

	int nRet = theLoop.Run();
	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	LampServerIpAddress.S_un.S_addr = ::htons(INADDR_ANY);
	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);
	
	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

void StartServer()
{
	if (!beRunning)
	{
		DWORD ThreadID;
		HANDLE hLampServerThread = ::CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Server, NULL, 0, &ThreadID);
		CloseHandle(hLampServerThread);
	}
}

void StopServer()
{
	if (!beRunning || completionPort == NULL)
	{
		return;
	}
	beRunning = false;
	//清理完成端口
	PostQueuedCompletionStatus(completionPort, 0xFFFFFFFF, 0, 0);
	CloseHandle(completionPort);
	completionPort = NULL;
	citylamp.ClearLampID2LampMutexSockMap();
	closemysocket(server_sock);
	{
		MutexGuard guard(PCClientListMutex);
		list<PCClient*>::iterator lt_it = PCClientList.begin();
		while (lt_it != PCClientList.end())
		{
			closemysocket((*lt_it)->GetDataSocket());
			++lt_it;
		}
	}

	//等待pcclientthread线程全部终止
	while (PCClientList.size() > 0 || citylamp.GetLampIDCount() > 0)
	{
		Sleep(200);
	}
	dlgMain.PostMessageW(WM_UPDATELAMPLIST_MESSAGE, 2, 0);
	dlgMain.PostMessageW(WM_UPDATEPCLIST_MESSAGE, 2, 0);
}

void RestartServer()
{
	StopServer();
	StartServer();
}

DWORD WINAPI Server()
{
	SOCKET sock;
	DWORD ThreadID;

	SYSTEM_INFO system_info;

	struct sockaddr_in addr;
	int iSize = sizeof(sockaddr_in);

	server_sock = CreateSocket(LampServerPort, LampServerIpAddress);
	if (server_sock == INVALID_SOCKET || server_sock == SOCKET_ERROR)
	{
		MessageBox(NULL, L"端口被占用", L"Error", MB_OK);
		beRunning = FALSE;
		UpdateServerStatus();
		return false;
	}
	beRunning = true;
	UpdateServerStatus();
	
	//创建完成端口句柄
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, null, 0, 0);

	//创建工作者线程
	GetSystemInfo(&system_info);
	for (UINT i = 0; i < system_info.dwNumberOfProcessors; ++i)
	{
		HANDLE ThreadHandle;
		ThreadHandle = CreateThread(null, 0, LampWorkerThread, completionPort, 0, &ThreadID);
		CloseHandle(ThreadHandle);
	}

	HANDLE hSendLampDataToPCThread = ::CreateThread(0, 0, (LPTHREAD_START_ROUTINE)LampThread, NULL, 0, &ThreadID);
	if (hSendLampDataToPCThread != 0)
	{
		CloseHandle(hSendLampDataToPCThread);
		//MessageBox(NULL, L"SendLampDataToPCThread启动成功", L"Error", MB_OK);
	}
	char buf[40] = {0x00};

	while(server_sock != INVALID_SOCKET && beRunning)
	{
		sock = ::accept(server_sock, (sockaddr *)&addr, &iSize);
		if (sock == INVALID_SOCKET)
		{
			continue;
		}

		//设置socket为非阻塞模式
		//int iMode = 1;
		//ioctlsocket(sock, FIONBIO, (u_long FAR*) &iMode);

		fd_set fd;
		struct timeval time_out;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		time_out.tv_sec = 20;	//设置超时时间为20s
		time_out.tv_usec = 0;

		//对于每个新的lamp连接，等待20s接受其发送的消息，若没有收到则关闭连接
		int err = select(0, &fd, NULL, NULL, &time_out);
		if ((err == SOCKET_ERROR) || (err == 0))
		{
			closemysocket(sock);
			continue;
		}

		if (FD_ISSET(sock, &fd))
		{
			//接收标记，区分连接是客户端还是节点
			char label[1] = {0x00};
			if (Recvn(sock, label, sizeof(label)) < sizeof(label))
			{
				closemysocket(sock);
				continue;
			}

			if ((u_char)label[0] == 0xAA)	//客户端连接
			{
				GUID guid;
				if (S_OK != ::CoCreateGuid(&guid))	//为每个pc端生成唯一id
				{
					closemysocket(sock);
					continue;
				}

				PCClient *pc = new PCClient(sock, guid, addr);
				{
					MutexGuard guard(PCClientListMutex);
					PCClientList.push_back(pc);
				}
				UpdatePCListView();

				HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PCClientThread, (void *)pc, 0, &ThreadID);
				if (hThread != 0)
				{
					CloseHandle(hThread);//MessageBox(NULL, L"PC连接成功", L"Error", MB_OK);
				}
			}
			else if (label[0] == 0x5A)	//节点连接
			{
				//接收数据长度和节点ID
				char len_lampid[3] = {0x00};
				if (Recvn(sock, len_lampid, sizeof(len_lampid)) < sizeof(len_lampid))
				{
					closemysocket(sock);
					continue;
				}

				int len = len_lampid[0];
				char* data = new char[len + 1];
				data[0] = 0x5A;
				memcpy(data + 1, len_lampid, sizeof(len_lampid));

				if (Recvn(sock, data + 4, len - sizeof(len_lampid)) < (int)(len - sizeof(len_lampid)))
				{
					closemysocket(sock);
					continue;
				}

				SendDataToPCClient(data, len + 1);
				delete[] data;
				u_short lampid = *(u_short*)(len_lampid + 1);	//len_lampid[1]和len_lampid[2]表示id的低字节和高字节部分
				//检查此lampid是否已经存在列表中
				if (citylamp.QueryLampID(lampid))
				{
					closemysocket(sock);
					continue;
				}

				LPLampMutexSockStruct lpLMSS = new LampMutexSockStruct(sock, &addr, lampid, RECV_POSTED);
				citylamp.SetLampID2LampMutexSockMap(lampid, lpLMSS);
				UpdateLampListView(true, lampid);

				//将lampsock绑定到完成端口
				/*
				LPPER_HANDLE_DATA lpPerHandleData = (LPPER_HANDLE_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_HANDLE_DATA));
				lpPerHandleData->Socket = sock;
				memcpy(&lpPerHandleData->ClientAddr, &addr, sizeof(sockaddr_in));
				lpPerHandleData->LampID = lampid;
				CreateIoCompletionPort((HANDLE)sock, completionPort, (DWORD)lpPerHandleData, 0);
				*/
				CreateIoCompletionPort((HANDLE)sock, completionPort, (DWORD)lpLMSS->lpPerHandleData, 0);

				/*
				LPPER_IO_DATA lpPerIOData = (LPPER_IO_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_DATA));
				lpPerIOData->Buffer.len = MAXBUFSIZE;
				lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
				lpPerIOData->OperationType = RECV_POSTED;
				WSARecv(lpPerHandleData->Socket, &lpPerIOData->Buffer, 1, &lpPerIOData->NumberOfBytesTransferred, &lpPerIOData->Flags, &lpPerIOData->Overlapped, null);
				*/
				WSARecv(lpLMSS->lpPerHandleData->Socket, &lpLMSS->lpPerIOData->Buffer, 1, &lpLMSS->lpPerIOData->NumberOfBytesTransferred, &lpLMSS->lpPerIOData->Flags, &lpLMSS->lpPerIOData->Overlapped, null);
				//MessageBox(NULL, L"Lamp连接成功", L"Error", MB_OK);
			}
			else
			{
				closemysocket(sock);
				continue;
			}
		}
	}

	closemysocket(server_sock);
	WSACleanup();
	UpdateServerStatus();
	beRunning = FALSE;
	return 1;
}

DWORD WINAPI LampWorkerThread(LPVOID cPort)
{
	HANDLE CompletionPort = (HANDLE)cPort;
	DWORD dwBytesTransferred;
	LPOVERLAPPED lpOverlapped = null;
	LPPER_HANDLE_DATA lpPerHandleData = null;
	LPPER_IO_DATA lpPerIOData = null;

	while (true)
	{
		bool bOk = GetQueuedCompletionStatus(CompletionPort, &dwBytesTransferred, (PULONG_PTR)&lpPerHandleData, (LPOVERLAPPED *)&lpPerIOData, INFINITE);

		//根据PostQueuedCompletionStatus发送的参数判断是否退出工作者线程
		if (lpPerHandleData == 0 || lpPerIOData == 0)
		{
			break;
		}

		if (!bOk || dwBytesTransferred == 0)
		{
			citylamp.RemoveLampID(lpPerHandleData->LampID);
			//closesocket(lpPerHandleData->Socket);
			//HeapFree(GetProcessHeap(), 0, lpPerHandleData);
			//HeapFree(GetProcessHeap(), 0, lpPerIOData);
			continue;
		}

		else if (lpPerIOData->OperationType == RECV_POSTED)
		{
			SendDataToPCClient(lpPerIOData->szMessage, dwBytesTransferred);
			/*
			memset(lpPerIOData, 0, sizeof(PER_IO_DATA));
			lpPerIOData->Buffer.len = MAXBUFSIZE;
			lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
			lpPerIOData->OperationType = RECV_POSTED;
			*/
			lpPerIOData->Initialization(RECV_POSTED);
			WSARecv(lpPerHandleData->Socket, &lpPerIOData->Buffer, 1, &lpPerIOData->NumberOfBytesTransferred, &lpPerIOData->Flags, &lpPerIOData->Overlapped, null);
		}
	}
	return 0;
}

DWORD WINAPI LampThread()
{
	//citylamp.TransferDataToPCClient();
	return 1;
}

DWORD WINAPI PCClientThread(void* pc)
{
	//首先将当前在线的节点发送给客户端
	citylamp.SendOnlineCmdToPCClient();

	PCClient* pcclient = (PCClient*)pc;

	pcclient->TransferDataToLamp();

	DelPCClientFromList(pcclient);
	delete pcclient;
	pcclient = NULL;

	return 1;
}
void SendDataToPCClient(char *buf, int datalength)
{
	{
		MutexGuard guard(PCClientListMutex);
		list<PCClient*>::iterator pciter = PCClientList.begin();
		while (pciter != PCClientList.end())
		{
			SOCKET &sock = (*pciter)->GetDataSocket();
			if (sock != 0 && sock != INVALID_SOCKET)
			{
				if (Sendn(sock, buf, datalength) < datalength)
				{
					closemysocket(sock);
					pciter = PCClientList.erase(pciter);
					continue;
				}
			}
			++pciter;
		}
	}
}

void DelPCClientFromList(PCClient *pc)
{
	{
		MutexGuard guard(PCClientListMutex);
		for (list<PCClient*>::iterator iter = PCClientList.begin(); iter != PCClientList.end(); ++iter)
		{
			if ((*iter)->GetPCID() == pc->GetPCID())
			{
				PCClientList.erase(iter);
				break;
			}
		}	
	}
	UpdatePCListView();
}

void UpdateLampListView(bool operation, u_short lampid)
{
	if (operation)	//增加lamp
	{
		dlgMain.PostMessageW(WM_UPDATELAMPLIST_MESSAGE, 1, (LPARAM)lampid);
	} 
	else			//删除lamp
	{
		citylamp.SendOfflineCmdToPCClient(lampid);
		dlgMain.PostMessageW(WM_UPDATELAMPLIST_MESSAGE, 0, (LPARAM)lampid);
		//PostMessageW((HWND)dlgMain, WM_UPDATELAMPLIST_MESSAGE, 0, (LPARAM)lampid);
	}
}

void UpdatePCListView()
{
	dlgMain.PostMessageW(WM_UPDATEPCLIST_MESSAGE, 0, 0);
}

void UpdateServerStatus()
{
	if (beRunning)
	{
		dlgMain.PostMessageW(WM_SERVERSTATUS_MESSAGE, 1, 0);
	} 
	else
	{
		dlgMain.PostMessageW(WM_SERVERSTATUS_MESSAGE, 0, 0);
	}
}

bool CheckCheckSum(char *buf, int len)
{
	int checksum = 0;
	for (int i = 1; i < len - 1; ++i)
	{
		checksum += buf[i];
	}
	return (buf[len - 1] == (char)checksum);
}

void AddCheckSum(char *buf, int len)
{
	int checksum = 0;
	for (int i = 1; i < len - 1; ++i)
	{
		checksum += buf[i];
	}
	buf[len - 1] = (char)checksum;
}

int Sendn(SOCKET s, char *DataBuf, int DataLen)//将DataBuf中的DataLen个字节发到s去
{
	int nBytesLeft = DataLen;
	int nBytesSent = 0;
	int ret;

	while(nBytesLeft > 0)
	{
		ret = send(s, DataBuf + nBytesSent, nBytesLeft, 0);
		if(ret <= 0)
			return -1;
		nBytesSent += ret;
		nBytesLeft -= ret;
	}
	return nBytesSent;
}

int Recvn(SOCKET s, char *DataBuf, int DataLen)
{
	int nBytesLeft = DataLen;
	int nBytesRecv = 0;
	int ret;

	while(nBytesLeft > 0)
	{
		ret = recv(s, DataBuf + nBytesRecv, nBytesLeft, 0);
		if(ret <= 0)
			return -1;
		nBytesRecv += ret;
		nBytesLeft -= ret;
	}
	return nBytesRecv;
}
void closemysocket(SOCKET sock)
{
	if (sock != INVALID_SOCKET && sock != 0)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

SOCKET CreateSocket(const int port)
{
	WSADATA	ws;
	SOCKET ssock;
	int ret;

	sockaddr_in server_addr;

	::WSAStartup(MAKEWORD(2,2),&ws);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(port);
	server_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);

	ssock = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	ret = ::bind(ssock,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret != 0) return 0;

	ret = ::listen(ssock, SOMAXCONN);
	if(ret != 0) return 0;
	return ssock;
}

SOCKET CreateSocket(const int port, struct in_addr ipaddress)
{
	WSADATA	ws;
	SOCKET ssock;
	int ret;

	sockaddr_in server_addr;

	::WSAStartup(MAKEWORD(2,2),&ws);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(port);
	server_addr.sin_addr = ipaddress;

	ssock = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (ssock == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}
	ret = ::bind(ssock,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret != 0) return ret;

	ret = ::listen(ssock, SOMAXCONN);
	if(ret != 0) return ret;
	return ssock;
}

wchar_t* AnsiToUnicode( const char* szStr )
{
	int nLen = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szStr, -1, NULL, 0 );
	if (nLen == 0)
	{
		return NULL;
	}
	wchar_t* pResult = new wchar_t[nLen];
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szStr, -1, pResult, nLen );
	return pResult;
} 