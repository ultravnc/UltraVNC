/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////

#define WM_APP	0x8000
#include "windows.h"            /* required for all Windows applications */
#include "16bithlp.h"            /* specific to this program          */

typedef struct tagDRAWMODE FAR*     LPDRAWMODE;
typedef struct tagGDIINFO FAR*      LPGDIINFO;
#define WM_COPYDATA             0x004A 
// same for COPYDATA struct
typedef struct tagCOPYDATASTRUCT {
    DWORD dwData;
    DWORD cbData;
    LPVOID lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT;


#include "winddi.h"
#include "vncddihk.h"

HANDLE hInst;               /* current instance */
BYTE        rgDdiHooked[DDI_MAX];
HPATCH      hpatch;
HWND        hWndmain,hWndRemote;
COPYDATASTRUCT cpd;
char mydata[1024]; 
char mydata2[1024];
int counter=0;
BOOL timeset=FALSE;
int  idTimer;

int     NEAR PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
long    CALLBACK    MainWndProc(HWND, UINT, WPARAM, LPARAM);  
          



int NEAR PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;                /* current instance         */
HANDLE hPrevInstance;            /* previous instance        */
LPSTR lpCmdLine;                 /* command line             */
int nCmdShow;                    /* show-window type (open/icon) */
{
    MSG msg;
    hpatch=NULL;
    hWndmain=NULL;
    hWndRemote=NULL;
    cpd.dwData = 112233; 
    //SetMessageQueue(96);
    hWndRemote=FindWindow("WinVNC desktop sink", "WinVNC");
    if (!hWndRemote) return(FALSE);  

    if (!hPrevInstance)          /* Other instances of app running? */
    if (!InitApplication(hInstance)) /* Initialize shared things */
        return (FALSE);      /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, SW_HIDE))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,    /* message structure              */
        NULL,          /* handle of window receiving the message */
        NULL,          /* lowest message to examine          */
        NULL))         /* highest message to examine         */ 
        {
    		if (!msg.hwnd)
            		msg.hwnd = hWndmain;

		        if (!IsDialogMessage(hWndmain, &msg))
		        {
            		TranslateMessage(&msg);
            		DispatchMessage(&msg);
        		}
        }
    KillTimer(hWndmain, 1); 
    if (hpatch)
    		{
        		UnpatchDdi(hpatch);
        		hpatch = NULL;
        		while (PeekMessage(&msg, NULL, WM_DDICALL, WM_DDICALL, PM_REMOVE))
        		{
            		msg.hwnd = hWndmain;
            		DispatchMessage(&msg);
        		}
            }
    return (msg.wParam);       /* Returns the value from PostQuitMessage */
}




BOOL InitApplication(hInstance)
HANDLE hInstance;                  /* current instance       */
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = NULL;                    /* Class style(s).                    */
    wc.lpfnWndProc = MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;           /* Application that owns the class.   */
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "GenericMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "GenericWClass"; /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}




BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;          /* Current instance identifier.       */
    int             nCmdShow;           /* Param for first ShowWindow() call. */
{

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hWndmain = CreateWindow(
        "GenericWClass",                /* See RegisterClass() call.          */
        "Generic Sample Application",   /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );

    /* If window could not be created, return "failure" */

    if (!hWndmain)
        return (FALSE);
    else
    { 
    int iddi;                     /* message              */       
    DDITYPE ddiType;
    for (iddi = 0; iddi < DDI_MAX; iddi++)
        rgDdiHooked[iddi] = TRUE; 
	ddiType=DDIHOOK_RECORDER;      
    hpatch = PatchDdi(hWndmain, hInstance, ddiType);
    }
    counter=0;

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hWndmain, nCmdShow);  /* Show the window                        */
    //UpdateWindow(hWndmain);          /* Sends WM_PAINT message                 */
    return (TRUE);               /* Returns the value from PostQuitMessage */

}



long CALLBACK 
MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {        
        case WM_CREATE:
        	idTimer=SetTimer(hWnd, 1, 50, NULL);
        	break;

        case WM_DESTROY:          /* message: window being destroyed */
		case WM_QUIT:
		case WM_CLOSE:
            if (hpatch)
    		{
        		MSG msg;
        		UnpatchDdi(hpatch);
        		hpatch = NULL;
		
		        //
			        // Log the pending Ddis
        		//
        		while (PeekMessage(&msg, NULL, WM_DDICALL, WM_DDICALL, PM_REMOVE))
        		{
            		msg.hwnd = hWnd;
            		DispatchMessage(&msg);
        		}
            		
            }  
            KillTimer(hWnd, idTimer);
			PostQuitMessage(0);
			break;
            
        case WM_DDICALL:
            
       		if (counter==0) lstrcpyn(mydata, (LPSTR)lParam, lstrlen((LPSTR)lParam)+1);
       		else 
       		{
       		 lstrcpyn(mydata2, (LPSTR)lParam, lstrlen((LPSTR)lParam)+1);
       		 lstrcat(mydata, mydata2);
       		} 
       		counter++;
       		if (counter>45 || timeset)
       		{
       			hWndRemote=FindWindow("WinVNC desktop sink", "WinVNC");
    			if (!hWndRemote) {PostQuitMessage(0);break;}
				timeset=FALSE;
				cpd.cbData = lstrlen(mydata)+1;
				cpd.lpData = (LPSTR)mydata;
				SendMessage(hWndRemote,
				WM_COPYDATA,
				(WPARAM) hWndmain,
				(LPARAM)&cpd);
				counter=0;
			}
            
            LocalFree(LOWORD(lParam));
            break;
            
        case WM_TIMER:
        	{
        	  BOOL test=SetTimeOut(TRUE);
        	  timeset=TRUE;
        	}
        	break;

        default:                  /* Passes it on if unproccessed    */
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
}