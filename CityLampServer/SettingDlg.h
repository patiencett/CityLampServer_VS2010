#pragma once
#include <ws2ipdef.h>
#include <list>
#include <vector>
#include "CityLampServer.h"

using std::list;
using std::vector;

class CSettingDlg :	public CDialogImpl<CSettingDlg>, public CWinDataExchange<CSettingDlg>
{
public:
public:
	enum { IDD = IDD_SETTINGDLG };

	BEGIN_MSG_MAP(CSettingDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDSETTINGOK, OnOkCmd)
		COMMAND_ID_HANDLER(IDSETTINGCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingDlg)
		//DDX_TEXT(IDC_COMBOIPLIST, ServerIpAddress)
		DDX_COMBO_INDEX(IDC_COMBOIPLIST, IpIndex)
		DDX_INT(IDC_EDITPORT, ServerPort)
		DDX_CHECK(IDC_CHECKMINIMUM, bHideWindow)
	END_DDX_MAP()
	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOkCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void GetLocalHostIpList(vector<struct in_addr> &IpList);
protected:
	wchar_t ServerIpAddress[INET_ADDRSTRLEN];
	int IpIndex;
	u_short ServerPort;	
	vector<struct in_addr> ServerIpList;
	CComboBox ComboHandle;
};