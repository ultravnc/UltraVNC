#pragma once
#include "stdhdrs.h"
#include "vncsetauth.h"

#define DIALOG_MAKEINTRESOURCE MAKEINTRESOURCE
class vncServer;
class RulesListView;

class PropertiesDialog
{
private:
	HWND hTabControl, hTabAuthentication, hTabIncoming, hTabInput, hTabMisc, hTabNotifications,
		hTabReverse, hTabRules, hTabCapture, hTabLog, hTabAdministration;
	BOOL		m_dlgvisible;
	BOOL bConnectSock = true;
	int ListPlugins(HWND hComboBox);
	RulesListView *rulesListView;
	vncServer* m_server = NULL;
	vncSetAuth	m_vncauth;	
	void onTabsOK(HWND hwnd);
	void onTabsAPPLY(HWND hwnd);
	void InitPortSettings(HWND hwnd);

	
	void ShowImpersonateDialog();
	HWND PropertiesDialogHwnd = NULL;
	bool showAdminPanel = false;	

public:
	PropertiesDialog();
	~PropertiesDialog();

	BOOL Init(vncServer* server);
	int ShowDialog();
	void UpdateServer();
	int HandleNotify(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
	bool InitDialog(HWND hwnd);
	void onOK(HWND hwnd);
	void onApply(HWND hwnd);
	void onCancel(HWND hwnd);
	bool DlgInitDialog(HWND hwnd);
	bool onCommand(int command, HWND hwnd, int subcommand);

	static void Secure_Plugin_elevated(char* szPlugin);
	static void Secure_Save_Plugin_Config(char* szPlugin);
	static void Secure_Plugin(char* szPlugin);
	static void LogToEdit(const std::string& message);
	static HWND hEditLog;
};

