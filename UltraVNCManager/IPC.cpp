//---------------------------------------------------------------------------

#pragma hdrstop

#include "IPC.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

void senddata(HWND hwnd, String data, HWND Handle)
{
	COPYDATASTRUCT cds;
	cds.cbData = data.Length() + 1;
	cds.lpData = data.c_str();
	cds.dwData = 0;

	SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(Handle), reinterpret_cast<LPARAM>(&cds));
}
