//===========================================================================
//	FullScreen Titlebar
//	2004 - All rights reservered
//  2019 - modified for uVNc
//===========================================================================
//
//	Project/Product :	FullScreenTitlebar
//  FileName		:	FullScreenTitleBar.cpp
//	Author(s)		:	Lars Werner
//  Homepage		:	http://lars.werner.no
//
//	Description		:	Creates a titlebar window used on fullscreen apps.
//                  
//	Classes			:	CTitleBar
//
//	History
//	Vers.  Date      Aut.  Type     Description
//  -----  --------  ----  -------  -----------------------------------------
//	1.00   20 01 04  LW    Create   Original
//	1.01   03 02 80  LW    Updated  Added contextmenus and restore feature
//===========================================================================

#include "stdhdrs.h"
#include "res\\resource.h"
#include "FullScreenTitleBar.h"
#include "Log.h"
#include "common/win32_helpers.h"
extern Log vnclog;
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#include <CommCtrl.h>

//***************************************************************************************

// CTitleBar *TitleBarThis=nullptr; // Added Jef Fix

//***************************************************************************************

CTitleBar::CTitleBar()
{
	hInstance = nullptr;
	Parent = nullptr;
	this->Init();
	Pin = nullptr;
	Close = nullptr;
	Maximize = nullptr;
//	Minimize = nullptr;
	Screen = nullptr;
	Photo = nullptr;
	SwitchMonitor = nullptr;
	ScreenTip = nullptr;
	PhotoTip = nullptr;
	SwitchMonitorTip = nullptr;
	MonitorTop = 0;
}

CTitleBar::CTitleBar(HINSTANCE hInst, HWND ParentWindow, bool Fit)
{
	hInstance=hInst;
	Parent=ParentWindow;
	this->Init();
	this->Fit = Fit;
}

//***************************************************************************************

CTitleBar::~CTitleBar()
{
	DeleteObject(Font);
	if (Pin) DestroyWindow(Pin);
	if (Close) DestroyWindow(Close);
	if (Maximize) DestroyWindow(Maximize);
//	if (Minimize) DestroyWindow(Minimize);
	if (m_hWnd) DestroyWindow(m_hWnd);
	if (Screen) DestroyWindow(Screen);
	if (Photo) DestroyWindow(Photo);
	if (SwitchMonitor) DestroyWindow(SwitchMonitor);
	if (ScreenTip) DestroyWindow(ScreenTip);
	if (PhotoTip) DestroyWindow(PhotoTip);
	if (SwitchMonitorTip) DestroyWindow(SwitchMonitorTip);
}

//***************************************************************************************

void CTitleBar::Init()
{
	SlideDown=TRUE; //Slide down at startup
	AutoHide=tbPinNotPushedIn; //sf@2004 - This way, the toolbar is briefly shown then hidden
								// (and doesn't overlap the toobar)
	IntAutoHideCounter=0;
	HideAfterSlide=FALSE;

	//Create font
    HDC hdc;
    long lfHeight;
    
    hdc = GetDC(nullptr);
    lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(nullptr, hdc);

	Font=CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, "Arial");

	Text=""; //No text at startup...

	m_hWnd=nullptr;

	if(Parent!=nullptr&&hInstance!=nullptr)
	{
		// TitleBarThis=this; // Added Jef Fix
		this->CreateDisplay();
	}
	
}

//***************************************************************************************

void CTitleBar::Create(HINSTANCE hInst, HWND ParentWindow, bool Fit)
{
	hInstance=hInst;
	Parent=ParentWindow;
	this->Init();
	this->Fit = Fit;
}

//***************************************************************************************

void CTitleBar::CreateDisplay()
{
	//Consts are used to select margins
	//GetParent size and size after that!
	RECT lpRect;	
	::GetWindowRect(::GetDesktopWindow(), &lpRect);

	// Create the window
	WNDCLASS wndclass;

	wndclass.style			= CS_DBLCLKS;
	wndclass.lpfnWndProc	= CTitleBar::WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= sizeof (CTitleBar *); // Added Jef Fix
	wndclass.hInstance		= hInstance;
	wndclass.hIcon=nullptr;
	wndclass.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName	= (const TCHAR *) nullptr;
	wndclass.lpszClassName	= _T("FSTITLEBAR");

	RegisterClass(&wndclass);

	//Create window without any titlbar
	DWORD winstyle = WS_POPUP | WS_SYSMENU ;

	int CenterX=(lpRect.right-lpRect.left)/2-tbWidth/2;
	int HeightPlacement=-tbHeigth+1;

	if(tbScrollWindow==FALSE)
		HeightPlacement=0;

	m_hWnd = CreateWindow(_T("FSTITLEBAR"),
			      /*_T("Titlebar")*/nullptr,
			      winstyle,
			      CenterX,
			      HeightPlacement,
			      tbWidth,       // x-size
			      tbHeigth,       // y-size
			      Parent,                // Parent handle
			      nullptr,                // Menu handle
			      hInstance,
			      nullptr);

    helper::SafeSetWindowUserData(m_hWnd, (LONG_PTR)this);
	//Set region to window so it is non rectangular
	HRGN Range;
	POINT Points[4];
	Points[0].x=0;
	Points[0].y=0;
	Points[1].x=tbTriangularPoint;
	Points[1].y=tbHeigth;
	Points[2].x=tbWidth-tbTriangularPoint;
	Points[2].y=tbHeigth;
	Points[3].x=tbWidth;
	Points[3].y=0;
	Range=::CreatePolygonRgn(Points, 4, ALTERNATE );

	::SetWindowRgn(m_hWnd, Range, TRUE); // Added Jef Fix

	//Close button
	Close=CreateWindow("STATIC",
				"Close",
				WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_OWNERDRAW,
                tbWidth-tbRightSpace-tbcxPicture, tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_CLOSE,
                hInstance,
				nullptr);

	//Maximize button
	Maximize=CreateWindow("STATIC",
				"Maximize",
				WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_OWNERDRAW,
                tbWidth-tbRightSpace-(tbcxPicture*2)-(tbButtonSpace*1), tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_MAXIMIZE,
                hInstance,
				nullptr);
	
	//Minimize button
/*	Minimize=CreateWindow("STATIC",
				"Minimize",
				WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_OWNERDRAW,
                tbWidth-tbRightSpace-(tbcxPicture*3)-(tbButtonSpace*2), tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_MINIMIZE,
                hInstance,
				nullptr);*/

	Screen=CreateWindow("STATIC",
				"Screen",
				WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_OWNERDRAW,
                tbLeftSpace + (tbcxPicture*1) +(tbButtonSpace*1), tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_SCREEN,
                hInstance,
				nullptr);
	CreateToolTipForRect(Screen, ScreenTip, "Scaling: Fit to screen, 1:1");

	Photo=CreateWindow("BUTTON",
				"Photo",
				WS_CHILD | WS_VISIBLE | BS_NOTIFY | BS_OWNERDRAW,
                tbLeftSpace + (tbcxPicture*2) +(tbButtonSpace*2), tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_PHOTO,
                hInstance,
				nullptr);
	CreateToolTipForRect(Photo, PhotoTip, "Screenshot, dubble click for settings");

	SwitchMonitor=CreateWindow("STATIC",
				"SwitchMonitor",
				WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_OWNERDRAW,
                tbLeftSpace + (tbcxPicture*3) +(tbButtonSpace*3), tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_SWITCHMONITOR,
                hInstance,
				nullptr);
	CreateToolTipForRect(SwitchMonitor, SwitchMonitorTip, "Swicth monitor");

	//Pin button
	Pin=CreateWindow("STATIC",
				"Pin",
				WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_OWNERDRAW,
                tbLeftSpace, tbTopSpace, tbcxPicture, tbcyPicture, m_hWnd,
				(HMENU)tbIDC_PIN,
                hInstance,
				nullptr);


	//Set the creation of the window
    helper::SafeSetWindowUserData(m_hWnd, (LONG_PTR)this);

	//Load pictures
	this->LoadPictures();

	//Show window and start animation
	if(tbHideAtStartup==FALSE)
		ShowWindow(m_hWnd, SW_SHOW);
	if(tbScrollWindow==TRUE)
		SetTimer(m_hWnd, tbScrollTimerID, tbScrollDelay, nullptr);
	if(AutoHide==TRUE)
		SetTimer(m_hWnd, tbAutoScrollTimer, tbAutoScrollDelay, nullptr);
}

//***************************************************************************************

LRESULT CALLBACK CTitleBar::WndProc(HWND hwnd, UINT iMsg, 
					   WPARAM wParam, LPARAM lParam)
{
	// Added Jef Fix
    CTitleBar *TitleBarThis=nullptr;
    TitleBarThis = helper::SafeGetWindowUserData<CTitleBar>(hwnd);

	switch (iMsg)
	{

	case WM_CREATE:
		return 0;

	case WM_PAINT:
			TitleBarThis->Draw();
			return 0;

	case WM_CLOSE:
		{
			HWND Window=TitleBarThis->GetSafeHwnd();
			TitleBarThis->FreePictures();
			DestroyWindow(Window);
//			vnclog.Print(0,_T(" Q6 \n"));
//			PostQuitMessage(0);
			return 0;
		}

	case WM_DESTROY:
			TitleBarThis->FreePictures();
//			vnclog.Print(0,_T(" Q7 \n"));
//			PostQuitMessage(0);
			return 0;

	case WM_DRAWITEM:
		{
			HDC hdcMem; 
			LPDRAWITEMSTRUCT lpdis; 

            lpdis = (LPDRAWITEMSTRUCT) lParam; 
            hdcMem = CreateCompatibleDC(lpdis->hDC); 
			HGDIOBJ hbrOld=nullptr;
 
			if(lpdis->CtlID==tbIDC_CLOSE)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hClose); 
			if(lpdis->CtlID==tbIDC_MAXIMIZE)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hMaximize); 
			if(lpdis->CtlID==tbIDC_MINIMIZE)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hMinimize); 
			
			if(lpdis->CtlID==tbIDC_PIN)
			{
				if(TitleBarThis->AutoHide==TRUE)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hPinUp); 
				else
					hbrOld=SelectObject(hdcMem, TitleBarThis->hPinDown); 
			}

			if(lpdis->CtlID==tbIDC_SCREEN)
			{
				if(TitleBarThis->Fit==TRUE)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hFitScreen); 
				else
					hbrOld=SelectObject(hdcMem, TitleBarThis->hNoScaleScreen); 
			}
			if(lpdis->CtlID==tbIDC_PHOTO)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hPhoto); 
			if(lpdis->CtlID==tbIDC_SWITCHMONITOR)
					hbrOld=SelectObject(hdcMem, TitleBarThis->hSwitchMonitor); 

			BitBlt(lpdis->hDC,
					lpdis->rcItem.left,
					lpdis->rcItem.top,
	                lpdis->rcItem.right - lpdis->rcItem.left, 
					lpdis->rcItem.bottom - lpdis->rcItem.top, 
	                hdcMem,
					0,
					0,
					SRCCOPY);
			if (hbrOld) SelectObject(hdcMem,hbrOld);
            DeleteDC(hdcMem); 
            return TRUE; 
		}

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			//Handle the Pin for holding the window
			if(LOWORD(wParam) == tbIDC_PIN)
			{
				if(TitleBarThis->AutoHide==TRUE)
				{
					TitleBarThis->AutoHide=FALSE;
					TitleBarThis->DisplayWindow(TRUE);
				}
				else
				{
					TitleBarThis->AutoHide=TRUE;
					TitleBarThis->DisplayWindow(FALSE);
				}

				//Redraw window to show the new gfx...
				::RedrawWindow(TitleBarThis->Pin, nullptr, nullptr, RDW_INVALIDATE);
			}
			if(LOWORD(wParam) == tbIDC_SCREEN)
			{
					if (TitleBarThis->Fit == TRUE)
						TitleBarThis->Fit = FALSE;
					else 
						TitleBarThis->Fit = TRUE;
					::RedrawWindow(TitleBarThis->Screen, nullptr, nullptr, RDW_INVALIDATE);
			}

			//If default = true we'll send usally showwindow and close messages
			if(tbDefault==TRUE)
			{
				if(LOWORD(wParam) == tbIDC_CLOSE)
					::SendMessage(TitleBarThis->Parent, WM_CLOSE, 0, 0);
				if(LOWORD(wParam) == tbIDC_MAXIMIZE)
				{
					//if(::IsZoomed(TitleBarThis->Parent)==TRUE)
						ShowWindow(TitleBarThis->Parent, SW_RESTORE);
					/*else
						ShowWindow(TitleBarThis->Parent, SW_MAXIMIZE);*/
				}
				if(LOWORD(wParam) == tbIDC_MINIMIZE)
					ShowWindow(TitleBarThis->Parent, SW_MINIMIZE);
			}
			else //default = false - send custom message on buttons
			{
				if(LOWORD(wParam) == tbIDC_CLOSE)
					::SendMessage(TitleBarThis->Parent, tbWM_CLOSE, 0, 0);
				if(LOWORD(wParam) == tbIDC_MAXIMIZE)
					::SendMessage(TitleBarThis->Parent, tbWM_MAXIMIZE, 0, 0);
				if(LOWORD(wParam) == tbIDC_MINIMIZE)
					::SendMessage(TitleBarThis->Parent, tbWM_MINIMIZE, 0, 0);

				if(LOWORD(wParam) == tbIDC_SCREEN) {
					if (TitleBarThis->Fit == TRUE)
						::SendMessage(TitleBarThis->Parent, tbWM_NOSCALE, 0, 0);
					else
						::SendMessage(TitleBarThis->Parent, tbWM_FITSCREEN, 0, 0);
				}
				if(LOWORD(wParam) == tbIDC_PHOTO)
					if (TitleBarThis->Fit == TRUE)
						::SendMessage(TitleBarThis->Parent, tbWM_PHOTO, 0, 0);
					else
						MessageBox(TitleBarThis->Parent, _T("Function only supported in 1:1 mode"), _T("uVNC snapshot"), MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
				if(LOWORD(wParam) == tbIDC_SWITCHMONITOR)
					::SendMessage(TitleBarThis->Parent, tbWM_SWITCHMONITOR, 0, 0);
			}
        }

		//Menu part starts here
		{
			UINT IDNum=LOWORD(wParam);
		
			if(IDNum>=tbWMCOMMANDIDStart&&IDNum<tbWMCOMMANDIDEnd) //The ID is in range for a menuclick
			{
				UINT Num=IDNum-tbWMCOMMANDIDStart;

				//When the close,minimize, maximize is not present just send! :)
				if(tbLastIsStandard==FALSE)
					::SendMessage(TitleBarThis->Parent, WM_USER+tbWMUSERID+Num, 0, 0);
				else //Handle close, minimize and maximize
				{
					HMENU Menu=LoadMenu(TitleBarThis->hInstance,MAKEINTRESOURCE (tbMENUID));
					HMENU SubMenu=GetSubMenu(Menu,0);;

					UINT Total=0;

					//Get the real number of entries (exluding seperators)
					for(int i=0;i<GetMenuItemCount(SubMenu);i++)
					{
						int res=::GetMenuString(SubMenu, i, nullptr, 0, MF_BYPOSITION);
						if(res!=0)
							Total++;
					}

					if(Num==Total-1) //Close button
						::SendMessage(TitleBarThis->m_hWnd,WM_COMMAND,MAKEWPARAM(tbIDC_CLOSE,BN_CLICKED),0);
					else if(Num==Total-2) //Minimize button
						::SendMessage(TitleBarThis->m_hWnd,WM_COMMAND,MAKEWPARAM(tbIDC_MINIMIZE,BN_CLICKED), 0);
					else if(Num==Total-3) //Maximize button
						::SendMessage(TitleBarThis->m_hWnd,WM_COMMAND,MAKEWPARAM(tbIDC_MAXIMIZE,BN_CLICKED), 0);
					else
						::SendMessage(TitleBarThis->Parent, WM_USER+tbWMUSERID+Num, 0, 0);

					DestroyMenu (SubMenu);
					DestroyMenu (Menu);
				}
			}
		}

        break;
	
	case WM_MOUSEMOVE:
			if(TitleBarThis->HideAfterSlide==FALSE)
			{
				TitleBarThis->SlideDown=TRUE;
				::SetTimer(TitleBarThis->m_hWnd, tbScrollTimerID, 20, nullptr);
			}
		break;

	case WM_LBUTTONDBLCLK:
			//If the default entries on the context menu is activated then doubleclick is restore :)
			if(tbLastIsStandard==TRUE)
				::SendMessage(TitleBarThis->m_hWnd,WM_COMMAND,MAKEWPARAM(tbIDC_MAXIMIZE,BN_CLICKED), 0);
		break;

	case WM_RBUTTONDOWN:
		{
			HMENU Menu=LoadMenu(TitleBarThis->hInstance,MAKEINTRESOURCE (tbMENUID));
			HMENU SubMenu=GetSubMenu(Menu,0);;

			POINT  lpPoint;
			::GetCursorPos(&lpPoint);

			int Pos=0;

			//Set ID values to each item
			for(int i=0;i<GetMenuItemCount(SubMenu);i++)
			{
				TCHAR Text[MAX_PATH];
				ZeroMemory(Text,sizeof(Text));
				int res=::GetMenuString(SubMenu, i, Text, MAX_PATH, MF_BYPOSITION);
				
				if(res!=0)
				{
					::ModifyMenu(SubMenu,i,MF_BYPOSITION, tbWMCOMMANDIDStart+Pos,Text);
					Pos++;
				}
			}

			//Loop through each item from pos to set the default value on restore
			if(tbLastIsStandard==TRUE)
			{
				int RealPos=0;
				for(int i=0;i<GetMenuItemCount(SubMenu);i++)
				{
					TCHAR Text[MAX_PATH];
					ZeroMemory(Text,sizeof(Text));
					int res=::GetMenuString(SubMenu, i, Text, MAX_PATH, MF_BYPOSITION);
					
					if(res!=0)
					{
						RealPos++;

						if(RealPos==Pos-2)
						::SetMenuDefaultItem(SubMenu, i, TRUE);
					}
				}
			}

			TrackPopupMenu(SubMenu,TPM_LEFTALIGN, lpPoint.x, lpPoint.y, 0, TitleBarThis->m_hWnd, nullptr);

			SetForegroundWindow (TitleBarThis->m_hWnd);
			DestroyMenu (SubMenu);
			DestroyMenu (Menu);

			break;
		}

	case WM_TIMER:
		{
			UINT TimerID=(UINT)wParam;
			
			if(TimerID==tbScrollTimerID)
			{
				RECT lpRect;
				::GetWindowRect(TitleBarThis->m_hWnd, &lpRect);

				if( ((lpRect.top== TitleBarThis->MonitorTop)&&(TitleBarThis->SlideDown==TRUE))
					||
					((lpRect.top== TitleBarThis->MonitorTop -tbHeigth+1)&&(TitleBarThis->SlideDown==FALSE)))
				{
					KillTimer(TitleBarThis->m_hWnd, tbScrollTimerID);

					if(TitleBarThis->HideAfterSlide==TRUE)
					{
						TitleBarThis->HideAfterSlide=FALSE;
						ShowWindow(TitleBarThis->GetSafeHwnd(), SW_HIDE);
					}
					return 0;
				}

				if(TitleBarThis->SlideDown==TRUE)
				{
					lpRect.top++; lpRect.bottom++;
				}
				else
				{
					lpRect.top--; lpRect.bottom--;
				}

				::MoveWindow(TitleBarThis->m_hWnd, lpRect.left, lpRect.top, lpRect.right-lpRect.left, lpRect.bottom-lpRect.top, TRUE);
			}

			//Check mouse cordinates and hide if the mouse haven't been in the window for a few seconds
			if(TimerID==tbAutoScrollTimer)
			{
				RECT lpRect;
				POINT pt;
				::GetWindowRect(TitleBarThis->m_hWnd, &lpRect);
				::GetCursorPos(&pt);

				if(PtInRect(&lpRect, pt)==FALSE) 
				{
					TitleBarThis->IntAutoHideCounter++;

					if(TitleBarThis->IntAutoHideCounter==tbAutoScrollTime)
					{
						TitleBarThis->SlideDown=FALSE;
						::SetTimer(TitleBarThis->m_hWnd, tbScrollTimerID, tbScrollDelay, nullptr);
					}
				}
				else
				{
					TitleBarThis->IntAutoHideCounter=0;
				}
			}

			break;
		}
	}//Case - end
	
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

//***************************************************************************************

void CTitleBar::LoadPictures()
{
	hClose=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_CLOSE));
	hMaximize=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_MAXIMIZE));
	hMinimize=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_MINIMIZE));
	hPinUp=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PINUP));
	hPinDown=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PINDOWN));
	hFitScreen=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FITSCREEN));
	hNoScaleScreen=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_NOSCALE));
	hPhoto=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PHOTO));
	hSwitchMonitor=LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_SWITCHMONITOR));
}


void CTitleBar::FreePictures()
{
	DeleteObject(hClose);
	DeleteObject(hMaximize);
	DeleteObject(hMinimize);
	DeleteObject(hPinUp);
	DeleteObject(hPinDown);
	DeleteObject(hFitScreen);
	DeleteObject(hNoScaleScreen);
	DeleteObject(hPhoto);
	DeleteObject(hSwitchMonitor);
}

//***************************************************************************************

void CTitleBar::Draw()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hWnd, &ps);

	int r1 = GetRValue(tbStartColor);
	int g1 = GetGValue(tbStartColor);
	int b1 = GetBValue(tbStartColor);
	int r2 = GetRValue(tbEndColor);
	int g2 = GetGValue(tbEndColor);
	int b2 = GetBValue(tbEndColor);

	//2 different styles of gradient is available... :)
	if(tbGradientWay==TRUE)
	{
		for ( int x = 0; x<tbWidth; x++)
		{ 
			RECT Rect;
			Rect.left=x;
			Rect.top=0;
			Rect.right=x+1;
			Rect.bottom=tbHeigth;
			HBRUSH Brush=CreateSolidBrush(RGB(r1 * (tbWidth-x)/tbWidth + r2 * x/tbWidth, 
				g1 * (tbWidth-x)/tbWidth + g2 * x/tbWidth, b1 * (tbWidth-x)/tbWidth + b2 * x/tbWidth));

			::FillRect(hdc, &Rect, Brush);
			DeleteObject(Brush);
		}
	}
	else
	{
		for ( int y = 0; y<tbHeigth; y++)
		{ 
			RECT Rect;
			Rect.left=0;
			Rect.top=y;
			Rect.right=tbWidth;
			Rect.bottom=y+1;
			
			HBRUSH Brush=CreateSolidBrush(RGB(r1 * (tbHeigth-y)/tbHeigth + r2 * y/tbHeigth, 
				g1 * (tbHeigth-y)/tbHeigth + g2 * y/tbHeigth, b1 * (tbHeigth-y)/tbHeigth + b2 * y/tbHeigth));

			::FillRect(hdc, &Rect, Brush);
			DeleteObject(Brush);
		}
	}

	//Draw border around window
	HPEN Border=::CreatePen(PS_SOLID, tbBorderWidth, tbBorderPenColor);
	HGDIOBJ hbmOld=SelectObject(hdc, Border);

	//Draw border around window
	::MoveToEx(hdc, 0,0, nullptr);
	::LineTo(hdc, tbTriangularPoint, tbHeigth);
	::LineTo(hdc, tbWidth-tbTriangularPoint, tbHeigth);
	::LineTo(hdc, tbWidth, 0);
	::LineTo(hdc, 0,0);

	//Draw extra shadow at bottom
	SelectObject(hdc,hbmOld);
	DeleteObject(Border);
	Border=::CreatePen(PS_SOLID, tbBorderWidth, tbBorderPenShadow);
	hbmOld=SelectObject(hdc, Border);
	::MoveToEx(hdc, tbTriangularPoint+1,tbHeigth-1, nullptr);
	::LineTo(hdc, tbWidth-tbTriangularPoint-1, tbHeigth-1);
	SelectObject(hdc,hbmOld);
	DeleteObject(Border);

	//Create rect for drawin the text
	RECT lpRect;
	lpRect.left=tbLeftSpace+tbcxPicture+tbButtonSpace;
	lpRect.top=tbBorderWidth;
	lpRect.right=tbWidth-tbRightSpace-(tbcxPicture*3)-(tbButtonSpace*3);
	lpRect.bottom=tbHeigth-tbBorderWidth;
	
	//Draw text
	::SelectObject(hdc, Font);
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, tbTextColor);
	::DrawText(hdc, Text,-1,&lpRect, DT_CENTER|DT_SINGLELINE|DT_VCENTER);

	EndPaint(m_hWnd, &ps);
}

//***************************************************************************************

void CTitleBar::SetText(LPTSTR TextOut)
{
	Text=TextOut;
}

//***************************************************************************************

void CTitleBar::DisplayWindow(BOOL Show, BOOL SetHideFlag)
{
	IntAutoHideCounter=0;

	if(Show==TRUE)
	{
		if(tbScrollWindow==TRUE)
		{
			if(SetHideFlag==TRUE)
			{
				HideAfterSlide=FALSE;
				SlideDown=TRUE;
			}
			ShowWindow(m_hWnd, SW_SHOW);
			SetTimer(m_hWnd, tbScrollTimerID, tbScrollDelay, nullptr);
		}
		else
			ShowWindow(m_hWnd, SW_SHOW);

		if(AutoHide==TRUE)
			SetTimer(m_hWnd, tbAutoScrollTimer, tbAutoScrollDelay, nullptr);
		else
			KillTimer(m_hWnd, tbAutoScrollTimer);
	}
	else
	{
		if(tbScrollWindow==TRUE)
		{
			if(SetHideFlag==TRUE)
			{
				HideAfterSlide=TRUE;
				SlideDown=FALSE;
			}
			SetTimer(m_hWnd, tbScrollTimerID, tbScrollDelay, nullptr);
		}
		else
			ShowWindow(m_hWnd, SW_HIDE);

		if(AutoHide==TRUE)
			SetTimer(m_hWnd, tbAutoScrollTimer, tbAutoScrollDelay, nullptr);
		else
			KillTimer(m_hWnd, tbAutoScrollTimer);
	}
}

//***************************************************************************************
// 7 May 2008 jdp
void CTitleBar::MoveToMonitor(HMONITOR hMonitor)
{
    int dx;
    int dy;

    HMONITOR hOrigMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
    // don't do anything if we're on the same monitor
    if (hOrigMonitor == hMonitor)
        return;

    // get our window rect 
    RECT wndRect;
    ::GetWindowRect(m_hWnd, &wndRect);

    MONITORINFO mi;
    mi.cbSize = sizeof (MONITORINFO);
    GetMonitorInfo(hOrigMonitor, &mi);
    // ... and calculate the offsets of our origin from the desktop's origin
    dx = wndRect.left - mi.rcMonitor.left;
    dy = wndRect.top - mi.rcMonitor.top;


    // now calculate our new origin relative to the new monitor.
    GetMonitorInfo(hMonitor, &mi);
    int x = mi.rcMonitor.left + (  mi.rcMonitor.right-mi.rcMonitor.left)/2-tbWidth/2;
    int y = mi.rcMonitor.top -tbHeigth;
	MonitorTop = mi.rcMonitor.top;
    // finally move the window.


    ::SetWindowPos(m_hWnd, 0, x, y, 0,0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);
}

//***************************************************************************************
void CTitleBar::CreateToolTipForRect(HWND hwndParent, HWND hwndTip, char * text)
{
    hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 
                                 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                 hwndParent, NULL, hInstance, NULL);
    SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    TOOLINFO ti = { 0 };
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_SUBCLASS;
    ti.hwnd     = hwndParent;
    ti.hinst    = hInstance;
    ti.lpszText = text;    
    GetClientRect (hwndParent, &ti.rect);
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti); 
} 
//***************************************************************************************
