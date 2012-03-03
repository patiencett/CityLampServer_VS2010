#include "StdAfx.h"
#include "resource.h"
#include "SettingDlg.h"
#include <algorithm>


LRESULT CSettingDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());
	GetLocalHostIpList(ServerIpList);
	ComboHandle = GetDlgItem(IDC_COMBOIPLIST);

	IpIndex = -1;
	int i = -1;
	vector<struct in_addr>::iterator iter = ServerIpList.begin();
	while (iter != ServerIpList.end())
	{
		++i;
		if (iter->S_un.S_addr == LampServerIpAddress.S_un.S_addr)
		{
			IpIndex = i;
		}

		if (iter->S_un.S_addr == ::htons(INADDR_ANY))
		{
			ComboHandle.InsertString(i, L"全部");
		}
		else
		{
			char *paddr = inet_ntoa(*iter);
			wchar_t *wpaddr = AnsiToUnicode(paddr);
			ComboHandle.InsertString(i, wpaddr);
			delete wpaddr;
		}
		++iter;
	}
	ServerPort = LampServerPort;
	if (!DoDataExchange(FALSE))
	{
		return FALSE;
	}
	return TRUE;
}

LRESULT CSettingDlg::OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (!DoDataExchange(TRUE))
	{
		return 0;
	}
	IpIndex = ComboHandle.GetCurSel();

	if (LampServerPort != ServerPort || LampServerIpAddress.S_un.S_addr != ServerIpList[IpIndex].S_un.S_addr)
	{
		LampServerPort = ServerPort;
		LampServerIpAddress = ServerIpList[IpIndex];
		if (beRunning)
		{
			RestartServer();
		}
	}
	EndDialog(wID);
	return 0;
}
LRESULT CSettingDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}

void CSettingDlg::GetLocalHostIpList(vector<struct in_addr> &IpList)
{
	WSAData ws;
	::WSAStartup(MAKEWORD(2,2),&ws);

	//先插入默认的所有地址:0.0.0.0
	struct in_addr anyaddr;
	anyaddr.S_un.S_addr = ::htons(INADDR_ANY);
	IpList.push_back(anyaddr);

	char hostname[256];
	if (SOCKET_ERROR == gethostname(hostname, sizeof(hostname)))
	{
		return;
	}
	HOSTENT *host = gethostbyname(hostname);
	if (host != NULL)
	{
		for (int i = 0; host->h_addr_list[i] != 0; ++i)
		{
			IpList.push_back(*(struct in_addr*)(host->h_addr_list[i]));
		}
	}
}
