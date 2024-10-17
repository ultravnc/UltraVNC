#pragma once
#include "stdhdrs.h"

class DlgChangePassword
{
private:
	HWND DlgChangePasswordHwnd = NULL;
	char title[1024]{};
	int passwordSize;
	char passwd[1024];
public:
	DlgChangePassword();
	int ShowDlg(HWND parentWindow, char* title, int passwordSize);
	~DlgChangePassword();
	bool InitDialog(HWND hwnd);
	void onOK(HWND hwnd);
	char* getPassword();
};

