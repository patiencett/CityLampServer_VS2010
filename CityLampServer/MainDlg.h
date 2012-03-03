// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <WinUser.h>
#define null NULL

//const int WM_UPDATEPCLIST_MESSAGE = WM_USER+1;
//const int WM_UPDATELAMPLIST_MESSAGE = WM_USER+2;

static UINT WM_UPDATEPCLIST_MESSAGE = RegisterWindowMessage(L"UPDATEPCLIST_MESSAGE");
static UINT WM_UPDATELAMPLIST_MESSAGE = RegisterWindowMessage(L"UPDATELAMPLIST_MESSAGE");
static UINT WM_SERVERSTATUS_MESSAGE = RegisterWindowMessage(L"SERVERSTATUS_MESSAGE");
static UINT WM_SHOWWINDOW_MESSAGE = RegisterWindowMessage(L"SHOWWINDOW_MESSAGE");

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler, public CWinDataExchange<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_UPDATEPCLIST_MESSAGE, OnUpdatePCListView)
		MESSAGE_HANDLER(WM_UPDATELAMPLIST_MESSAGE, OnUpdateLampListView)
		MESSAGE_HANDLER(WM_SERVERSTATUS_MESSAGE, OnServerStatus)
		MESSAGE_HANDLER(WM_SHOWWINDOW_MESSAGE, OnShowWindow)
		COMMAND_HANDLER(IDSETTING, BN_CLICKED, OnSetting)
		COMMAND_HANDLER(IDSTOP, BN_CLICKED, OnStop)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER(IDSTART, BN_CLICKED, OnStart)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetting(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStop(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStart(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);

	LRESULT OnUpdatePCListView(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnUpdateLampListView(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnServerStatus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:
	CListViewCtrl hLampList;
	CListViewCtrl hPCList;
	NOTIFYICONDATA pnid;
	HICON hIcon;
	HICON hIconSmall;
};