// stop_servicehelper.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <Shellapi.h>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	char WORKDIR[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return 0;
		*p = '\0';
		}
	strcat(WORKDIR,"\\");
	strcat(WORKDIR,"stop_service.exe");

	SHELLEXECUTEINFO shExecInfo;

     shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
      shExecInfo.fMask = NULL;
      shExecInfo.hwnd = GetForegroundWindow();
      shExecInfo.lpVerb = "runas";
      shExecInfo.lpFile = WORKDIR;
      shExecInfo.lpParameters = lpCmdLine;
      shExecInfo.lpDirectory = NULL;
      shExecInfo.nShow = SW_SHOWNORMAL;
      shExecInfo.hInstApp = NULL;
	  ShellExecuteEx(&shExecInfo);
	return 0;
}



