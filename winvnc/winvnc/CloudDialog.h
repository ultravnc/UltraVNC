#pragma once
#include "stdhdrs.h"
#include "inifile.h"
class vncServer;

class CloudDialog
{
private:
	vncServer* m_server = NULL;
	void SaveToIniFile();
	bool SC = false;
public:
	CloudDialog();
	~CloudDialog();
	void LoadFromIniFile();
	void Init(vncServer* server);
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Show(bool show, bool SC = false);
	bool m_dlgvisible;
};

