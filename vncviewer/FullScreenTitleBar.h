//===========================================================================
//	FullScreen Titlebar Header
//	2004 - All rights reservered
//===========================================================================
//
//	Project/Product :	FullScreenTitlebar
//  FileName		:	FullScreenTitleBar.h
//	Author(s)		:	Lars Werner
//  Homepage		:	http://lars.werner.no
//
//	Description		:	Declarations of the functions in the fullscreen window
//                  
//	Classes			:	CTitleBar
//
//	History
//	Vers.  Date      Aut.  Type     Description
//  -----  --------  ----  -------  -----------------------------------------
//	1.00   20 01 04  LW    Create   Original
//===========================================================================

//Include this .h only once
#pragma once

//All settings is stored here.
#include "FullScreenTitleBarConst.h"

class CTitleBar
{
public:
	CTitleBar();
	CTitleBar(HINSTANCE hInst, HWND ParentWindow); //Creation
	virtual ~CTitleBar(); //Destruction

	void Create(HINSTANCE hInst, HWND ParentWindow);

	void SetText(LPTSTR TextOut); //Set the header text eg: hostname, windowtitle ect...

	void DisplayWindow(BOOL Show, BOOL SetHideFlag=FALSE); //Variable like ShowWindow but it triggers the scrolling feature

	//Returns the window - This is stored in the header cause of it size :)
	HWND GetSafeHwnd()
	{
		return m_hWnd;
	};

private:
	//Init if default variables
	void Init();

	//Creates the window
	void CreateDisplay();

	//Callbacks from buttons, timers, draw, ect ect
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	//StartPaint/EndPaint routine...
	void Draw();

	//Default variabled used for creation of the window
	HINSTANCE hInstance;
	HWND Parent;
	HWND m_hWnd;

	//Variables for scrolling of the window
	BOOL SlideDown;
	BOOL AutoHide; //Is the pin pushed in or not...
	BOOL HideAfterSlide; //TRUE = Hide the dialog after slide
	int IntAutoHideCounter;

	//Routines to load pictures and free pictures
	void LoadPictures();
	void FreePictures();

	//Pictures for the menubar
	HBITMAP hClose;
	HBITMAP hMinimize;
	HBITMAP hMaximize;
	HBITMAP hPinUp;
	HBITMAP hPinDown;
	HWND Pin;

	//Text to show on titlebar and it corespondent font! :)
	LPTSTR Text;
	HFONT Font;
};
