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

#include <windows.h>
#include <windowsx.h> 

typedef struct tagDRAWMODE FAR*     LPDRAWMODE;
typedef struct tagGDIINFO  FAR*     LPGDIINFO;

#include "winddi.h"
#include "vncddihk.h"


//
// Structures
//
typedef struct tagPATCH
{
    HDDI    hddi;               // 0x00
    HWND    hwndPost;           // 0x04
    HGLOBAL hheapPost;          // 0x06
    DWORD   cCalls;             // 0x08
    DWORD   idThread;           // 0x0C
}
PATCH;                          // 0x10

//
// Globals
//
HINSTANCE   hInstance   = NULL;
char        szDdiString[1024];
char        szDdiCompare[1024];
char		szleft[5];
char		szright[5];
char		sztop[5];
char 		szbottom[5];
LPPDEVICE	myscreen=NULL; 
BOOL Failed=FALSE;
HRGN region=NULL;

int left=0;
int right=0;
int top=0;
int bottom=0;
int error_counter=0;
BOOL timeout=FALSE;
//////////////////////////
BOOL WINAPI
SetTimeOut(BOOL result)
{
	timeout=result;
	return(TRUE);
}



//
// Functions
//
int     WINAPI      LibMain(HINSTANCE, UINT, UINT, LPSTR);
DWORD   WINAPI      DdiHookProc(HDDI, LONG, DDITYPE, LPDDIPARAMS);
BOOL	WINAPI		PostThreadMessage32(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam,UINT test); 
void AddrectToRegion();
void Clear();
void reactangles();
BOOL RectToAdd(); 


extern  LPVOID NEAR PASCAL HeapAlloc(HGLOBAL, UINT);
extern  BOOL   NEAR PASCAL HeapFree(LPVOID);


////////////////////////////////////////// REGION HANDLING  
void
AddrectToRegion()
{                                   
	HRGN newregion;
	if (region == NULL)
	{
	   region=CreateRectRgn(left,top,right,bottom);
	}
	else
	{
		newregion=CreateRectRgn(left,top,right,bottom);
		CombineRgn(region, region, newregion, RGN_OR);
		DeleteObject((HGDIOBJ)(HRGN)newregion);
		newregion=NULL;	
	}
}
void Clear()
{
	if (region != NULL)
	{ 
		DeleteObject((HGDIOBJ)(HRGN)region);
		region=NULL ;
	}
	
} 

void reactangles()
{
	RECT rect;
	GetRgnBox(region, &rect);
	left=rect.left;
	right=rect.right;
	top=rect.top;
	bottom=rect.bottom;
	Clear();
	
}

BOOL RectToAdd()
{
	RECT rect;
	BOOL result; 
	if (region==NULL) return 1;
	rect.left=left;
	rect.top=top;
	rect.right=right;
	rect.bottom=bottom;
	result= RectInRegion(region,&rect);
	return (result);
}

// --------------------------------------------------------------------------
//
//  LibMain()
// 
// --------------------------------------------------------------------------
int WINAPI
LibMain(HINSTANCE hInst, UINT uDataSeg, UINT uHeapSize, LPSTR lpszCmdLine)
{
	hInstance = hInst;
    myscreen=NULL;
    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  PatchDdi()
//
//  Patches the DDI for the caller
//
// --------------------------------------------------------------------------
HPATCH WINAPI
PatchDdi(HWND hwnd, HGLOBAL hHeap, UINT uType)
{
    HPATCH  hpatch;

    //
    // Allocate a structure.
    //
    hpatch = (HPATCH)LocalAlloc(LPTR, sizeof(PATCH));
    if (!hpatch)
        return(NULL);

    //
    // Save info away
    //
    hpatch->hwndPost = hwnd;
    hpatch->hheapPost = hHeap;
    hpatch->idThread = (DWORD)GetWindowTask(hwnd);

    //
    // Set the ddi hook
    //
    hpatch->hddi = SetDDIHook(NULL, hInstance, MAKELONG(hpatch, 0), uType,
        DdiHookProc);

    //
    // Installation failed, clean up.
    //
    if (!hpatch->hddi)
    {
        LocalFree((HANDLE)hpatch);
        hpatch = NULL;
    }
    return(hpatch);
}

// --------------------------------------------------------------------------
//
//  UnpatchDDI()
//
//  Unpatches the DDI for the caller
//
// --------------------------------------------------------------------------
BOOL WINAPI
UnpatchDdi(HPATCH hpatch)
{
    if (!LocalHandle((HANDLE)hpatch))
        return(FALSE);

    //
    // Unhook the DDI
    //
    UnhookDDIHook(hpatch->hddi);

    //
    // Post a message to the caller.
    //
    PostThreadMessage32(hpatch->idThread, WM_ENDDDIPATCH, 0, hpatch->cCalls, 0);

    //
    // Free structure
    //
    LocalFree((HANDLE)hpatch);

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  DdiHookProc()
//
//  This makes a string of the DDI params, and posts it to the caller.
//
// --------------------------------------------------------------------------
DWORD WINAPI
DdiHookProc(HDDI hddi, LONG lPrivateData, DDITYPE ddiCode, LPDDIPARAMS lpddi)
{
    HPATCH  hpatch = (HPATCH)LOWORD(lPrivateData);

    if (!hpatch)
        DebugBreak();

    //
    // Use data variable to avoid chewing stack space; we are protected by
    // the win16lock.
    //
    *szDdiString = 0;

        switch (ddiCode)
    {
        case DDI_BITBLT:
            #define lpbitblt    ((LPBITBLT_DDIPARAMS)lpddi)
            if (myscreen==lpbitblt->lpDevDst)
            {
            left=lpbitblt->xDst;
            right=(lpbitblt->cxSrc+lpbitblt->xDst)+1;
            top=lpbitblt->yDst;
            bottom=(lpbitblt->cySrc+lpbitblt->yDst)+1;
            if ((RectToAdd()==0 || timeout) && region)
            {
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpbitblt->xDst;
            right=(lpbitblt->cxSrc+lpbitblt->xDst)+1;
            top=lpbitblt->yDst;
            bottom=(lpbitblt->cySrc+lpbitblt->yDst)+1;
            AddrectToRegion();
            }
            else
            {
            AddrectToRegion();
            }
            }
            break;
            
        case DDI_EXTTEXTOUT:
            #define lpextto ((LPEXTTEXTOUT_DDIPARAMS)lpddi) 
            if (myscreen==lpextto->lpDev && lpextto->lprcClip)
            {
            left=lpextto->lprcClip->left;
            right=(lpextto->lprcClip->right)+1;
            top=lpextto->lprcClip->top;
            bottom=(lpextto->lprcClip->bottom)+1;
            if ((RectToAdd()==0 || timeout) && region)
            { 
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpextto->lprcClip->left;
            right=(lpextto->lprcClip->right)+1;
            top=lpextto->lprcClip->top;
            bottom=(lpextto->lprcClip->bottom)+1;
            AddrectToRegion();
            }
            else
            {
             AddrectToRegion();
            }
            }
            if (!myscreen) myscreen=lpextto->lpDev;
            break;

        case DDI_FASTBORDER:
            #define lpfastb ((LPFASTBORDER_DDIPARAMS)lpddi) 
            if (myscreen==lpfastb->lpDev && lpfastb->lprcClip) 
             {
            left=lpfastb->lprcClip->left;
            right=(lpfastb->lprcClip->right)+1;
            top=lpfastb->lprcClip->top;
            bottom=(lpfastb->lprcClip->bottom)+1;
            if ((RectToAdd()==0 || timeout) && region)
            { 
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpfastb->lprcClip->left;
            right=(lpfastb->lprcClip->right)+1;
            top=lpfastb->lprcClip->top;
            bottom=(lpfastb->lprcClip->bottom)+1;
            AddrectToRegion();
            }
            else
            {
             AddrectToRegion();
            }
            }
            break;
            
        case DDI_STRETCHBLT:
            #define lpstretch ((LPSTRETCHBLT_DDIPARAMS)lpddi) 
            if (lpstretch->lpDevDst==myscreen)
            {
            left=lpstretch->xDst;
            right=(lpstretch->cxDst+lpstretch->xDst)+1;
            top=lpstretch->yDst;
            bottom=(lpstretch->cyDst+lpstretch->yDst)+1;
            if ((RectToAdd()==0 || timeout) && region)
            { 
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpstretch->xDst;
            right=(lpstretch->cxDst+lpstretch->xDst)+1;
            top=lpstretch->yDst;
            bottom=(lpstretch->cyDst+lpstretch->yDst)+1;
            AddrectToRegion();
            }
            else
            {
             AddrectToRegion();
            }
            }
            break;

        case DDI_STRETCHDIBITS:
            #define lpsdib ((LPSTRETCHDIBITS_DDIPARAMS)lpddi) 
            if (lpsdib->lpDevDst==myscreen)
            {
            left=lpsdib->xDst;
            right=(lpsdib->cxDst+lpsdib->xDst)+1;
            top=lpsdib->yDst;
            bottom=(lpsdib->cyDst+lpsdib->yDst)+1;
            if ((RectToAdd()==0 || timeout) && region)
            { 
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpsdib->xDst;
            right=(lpsdib->cxDst+lpsdib->xDst)+1;
            top=lpsdib->yDst;
            bottom=(lpsdib->cyDst+lpsdib->yDst)+1;
            AddrectToRegion();
            }
            else
            {
             AddrectToRegion();
            }
            }
            break;

		case DDI_DIBTODEVICE:
            #define lpdibto ((LPDIBTODEVICE_DDIPARAMS)lpddi)
			if (lpdibto->lpDev==myscreen && lpdibto->lprcClip)
			{
            left=lpdibto->lprcClip->left;
            right=(lpdibto->lprcClip->right)+1;
            top=lpdibto->lprcClip->top;
            bottom=(lpdibto->lprcClip->bottom)+1;
            if ((RectToAdd()==0 || timeout) && region)
            { 
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpdibto->lprcClip->left;
            right=(lpdibto->lprcClip->right)+1;
            top=lpdibto->lprcClip->top;
            bottom=(lpdibto->lprcClip->bottom)+1;
            AddrectToRegion();
            }
            else
            {
             AddrectToRegion();
            } 
            }
            break;
       case DDI_OUTPUT:
            #define lpoutput ((LPOUTPUT_DDIPARAMS)lpddi) 
            if (lpoutput->lpDev==myscreen && lpoutput->lprcClip)
            {
            left=lpoutput->lprcClip->left;
            right=(lpoutput->lprcClip->right)+1;
            top=lpoutput->lprcClip->top;
            bottom=(lpoutput->lprcClip->bottom)+1;
            if ((RectToAdd()==0 || timeout) && region)
            { 
            reactangles();
            timeout=FALSE;
            wsprintf(szDdiString, "%04d:%04d:%04d:%04d;",left,top,right,bottom);
            left=lpoutput->lprcClip->left;
            right=(lpoutput->lprcClip->right)+1;
            top=lpoutput->lprcClip->top;
            bottom=(lpoutput->lprcClip->bottom)+1;
            AddrectToRegion();
            }
            else
            {
             AddrectToRegion();
            }
            }
            break;
    }        
	///////////////////////////////////// 
	//stoptime=clock();
	//if ((stoptime-starttime)>(CLOCKS_PER_SEC/5))
	//	{ 
		//	timeout=TRUE;
	//		starttime=clock();
	//	}

	
    if (*szDdiString && myscreen)
    {
        
        LPSTR   lpszPostClient;
        UINT    cchString;
        if (!lstrcmp(szDdiString,szDdiCompare))
        {
        	return(CallNextDDI(hddi, ddiCode, lpddi));	
        } 
        else
        {
        lstrcpy(szDdiCompare, szDdiString);
		cchString=0;		
        cchString = lstrlen(szDdiString);
        lpszPostClient = (LPSTR)HeapAlloc(hpatch->hheapPost, cchString+1);
        if (lpszPostClient)
        {
            lstrcpy(lpszPostClient, szDdiString);
            if (! PostThreadMessage32(hpatch->idThread, WM_DDICALL, ddiCode, (LPARAM)lpszPostClient,0)) 
            	{
                HeapFree(lpszPostClient);
                //error_counter++;
                //if (error_counter>100) UnpatchDdi(hpatch_backup) ;
                }
           //error_counter=0;
        }
        }
    }

    return(CallNextDDI(hddi, ddiCode, lpddi));

}
