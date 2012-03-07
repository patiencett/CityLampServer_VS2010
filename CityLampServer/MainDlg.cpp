// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "CityLampServer.h"
#include "aboutdlg.h"
#include "SettingDlg.h"
#include "MainDlg.h"

#include <xstring>
using std::wstring;

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);
	CButton hGroupBox = GetDlgItem(IDC_GROUPBOXPC);
	hGroupBox.SetWindowTextW(L"���߼�ض�");
	hGroupBox = GetDlgItem(IDC_GROUPBOXLAMP);
	hGroupBox.SetWindowTextW(L"���߽ڵ�");
	hLampList = GetDlgItem(IDC_LISTCITYLAMP);
	hLampList.SetWindowLong(GWL_STYLE, LVS_REPORT | LVS_SINGLESEL | WS_CHILD | WS_VISIBLE);
	hLampList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);
	hLampList.AddColumn(L"�ڵ�ID", 0);
	hLampList.AddColumn(L"SOCKET", 1);
	hLampList.SetColumnWidth(0, 80);
	hLampList.SetColumnWidth(1, 60);

	hPCList = GetDlgItem(IDC_LISTPCCLIENT);
	hPCList.SetWindowLong(GWL_STYLE, LVS_REPORT | LVS_SINGLESEL | WS_CHILD | WS_VISIBLE);
	hPCList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);
	hPCList.AddColumn(L"IP��ַ", 0);
	hPCList.AddColumn(L"������", 1);
	hPCList.SetColumnWidth(0, 100);
	hPCList.SetColumnWidth(1, 60);

	//��ʼ��ϵͳ����
	pnid.cbSize = (DWORD)sizeof(NOTIFYICONDATA);//�ýṹ���С
	pnid.hWnd = m_hWnd;    //���ھ��
	pnid.uID = (UINT)hIcon;      //ͼ����
	pnid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP ; //ͼ����Ч|�Զ�����Ϣ��Ч|���ָ����ʾ������Ч
	pnid.uCallbackMessage = WM_SHOWWINDOW_MESSAGE;//�Զ������Ϣ����
	pnid.hIcon = hIconSmall;         //ͼ����
	wcscpy_s(pnid.szTip,L"���о��۵Ƽ��ϵͳ������");//���ָ������ʾ������
	return TRUE;
}

LRESULT CMainDlg::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if ((UINT)wParam == SIZE_MINIMIZED && bHideWindow)
	{
		Shell_NotifyIcon(NIM_ADD,&pnid);//�����������ͼ��
		ShowWindow(SW_HIDE);//���������� 
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	beRunning = false;
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
	//::MessageBox(NULL, L"�����˳�", L"Error", MB_OK);
	return 0;
}

LRESULT CMainDlg::OnSetting(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CSettingDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnStop(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	//CloseDialog(wID);
	/*
	TCHAR fd_setsize[10];
	_itow_s(FD_SETSIZE, fd_setsize, 10);
	::MessageBox(NULL, fd_setsize, L"Error", MB_OK);
	return 0;
	*/
	StopServer();
	
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	StartServer();
	
	return 0;
}

LRESULT CMainDlg::OnUpdatePCListView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	hPCList.DeleteAllItems();
	if (wParam <= 1)
	{
		USES_CONVERSION;
		char *ip;
		int i = 0;
		{
			std::wostringstream portstr;
			MutexGuard guard(PCClientListMutex);
			for (list<PCClient*>::iterator iter = PCClientList.begin(); iter != PCClientList.end(); ++iter)
			{
				ip = inet_ntoa((*iter)->GetPCAddrIn().sin_addr);
				hPCList.AddItem(i, 0, A2T(ip));
				hPCList.AddItem(i, 1, (*iter)->GetHostname());
				++i;
			}
		}
	}
	return 0;
}
LRESULT CMainDlg::OnUpdateLampListView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	u_short lampid = (u_short)lParam;
	TCHAR str[16];
	_itow_s(lampid, str, 10);
	if (wParam == 1)		//�����
	{
		int nIndex = hLampList.GetItemCount();
		LV_ITEM idItem = {0};
		idItem.mask = LVIF_TEXT;
		idItem.iItem = nIndex;
		idItem.iSubItem = 0;
		idItem.pszText = str;
		hLampList.InsertItem(&idItem);
	}
	else if (wParam == 0)				//ɾ����
	{
		LVFINDINFO *pFindInfo = new LVFINDINFO;  
		pFindInfo->flags = LVFI_PARTIAL|LVFI_STRING;  
		pFindInfo->psz = str;
		int nIndex = hLampList.FindItem(pFindInfo, -1);   
		hLampList.DeleteItem(nIndex);
	}
	else
	{
		hLampList.DeleteAllItems();
	}
	return 0;
}
LRESULT CMainDlg::OnServerStatus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == 0)
	{
		GetDlgItem(IDSTART).EnableWindow(true);
		GetDlgItem(IDSTOP).EnableWindow(false);
	}
	else
	{
		GetDlgItem(IDSTART).EnableWindow(false);
		GetDlgItem(IDSTOP).EnableWindow(true);
	}
	return 0;
}
LRESULT CMainDlg::OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (lParam == WM_LBUTTONDOWN && bHideWindow)
	{
		Shell_NotifyIcon(NIM_DELETE,&pnid);//ɾ������ͼ��
		ShowWindow(SW_SHOWNORMAL);//��ʾ������
		this->SetWindowPos(HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);//ʹ������������ǰ��
	}
	return 0;
}