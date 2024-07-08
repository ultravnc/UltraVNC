//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Tabsheet.h"
#include "TabsheetList.h"
#include "TopBarPanel.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

 Tabsheet::Tabsheet(TComponent* owner, String classname, String caption, TPageControl *pageControl, bool left)
 {
	this->caption = caption;
	tab.reset(new TTabSheet(pageControl));
	tab->PageControl = pageControl;
	tab->Caption = caption + "   ";

	panel.reset(new MyPanel(tab.get()));
	panel->Parent = tab.get(); // Set parent to the new tab
	panel->Align = alClient; // Make panel fill the tab
	panel->BevelOuter = bvNone;
	panel->OnResize = PanelResize;

	toppanel.reset(new TPanel(tab.get()));
	fraTopPanel.reset(new TfraTopPanel(toppanel.get()));
	toppanel->Parent = tab.get(); // Set parent to the new tab

	toppanel->BevelOuter = bvNone;
	toppanel->Height = fraTopPanel->Height;
	if (left) {
		toppanel->Align = alLeft;
		toppanel->Width = 120;
		fraTopPanel->ToolBar1->AlignWithMargins = false;
	}
	else {
		toppanel->Align = alTop;
		fraTopPanel->ToolBar1->AlignWithMargins = true;
	}

	fraTopPanel->Parent = toppanel.get();
	fraTopPanel->Align = alClient;


    panel->ParentBackground = false;
	panel->Color = clRed;

	hVNCWnd = NULL;
	int counter = 0;
	while (hVNCWnd == NULL && counter < 60) {
		hVNCWnd = FindWindowW(classname.c_str(), NULL);
		Sleep(100); counter++;
	}
    ShowWindow(hVNCWnd, SW_HIDE);

	if (hVNCWnd == NULL) {
		tab->TabVisible = false;
		TabsheetList::getInstance()->removeFromTabsheetList(caption);
		return;
	}


	::SetParent(hVNCWnd, panel->Handle);
	SetWindowPos(hVNCWnd, nullptr, 0, 0, 0, 0, SWP_NOSIZE);
	RedrawWindow(hVNCWnd, nullptr, nullptr, RDW_INVALIDATE);
	SetWindowLong(hVNCWnd, GWL_STYLE, GetWindowLong(hVNCWnd, GWL_STYLE) & ~WS_CAPTION);
	RECT rect;
	::GetClientRect(panel->Handle, &rect);
	SetWindowPos(hVNCWnd, NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
	ShowWindow(hVNCWnd, SW_SHOW);
	pageControl->ActivePage = tab.get();
	sendDataToVNC(hVNCWnd, 0, tab->Handle, aspectration);
	fraTopPanel->setVNCHandle(hVNCWnd, caption);
	sendDataToVNC(hVNCWnd, 0, tab->Handle, nbrMonitord);
	switch(panel->nbrMonitors) {
		case 1:
		default:
			fraTopPanel->Mon1->Visible = true;
			fraTopPanel->Mon2->Visible = false;
			fraTopPanel->Mon3->Visible = false;
			break;
		case 2:
			fraTopPanel->Mon1->Visible = true;
			fraTopPanel->Mon2->Visible = true;
			fraTopPanel->Mon3->Visible = false;
			break;
		case 3:
			fraTopPanel->Mon1->Visible = true;
			fraTopPanel->Mon2->Visible = true;
			fraTopPanel->Mon3->Visible = true;
			break;
	}
	PanelResize(NULL);
 }
//---------------------------------------------------------------------------
 Tabsheet::~Tabsheet()
 {

 }
 //---------------------------------------------------------------------------
 void __fastcall Tabsheet::PanelResize(TObject *Sender)
{

	int maxHeight =panel->Height;
	int maxWidth = panel->Width;


	// Calculate new dimensions while maintaining aspect ratio
    int newWidth, newHeight;
	if (panel->aspectRatio > (double)maxWidth / maxHeight) {
		newWidth = maxWidth;
		newHeight = maxWidth / panel->aspectRatio;
    } else {
		newWidth = maxHeight * panel->aspectRatio;
		newHeight = maxHeight;
	}

    // Center the image in the form
	int left = (maxWidth - newWidth) / 2;
	int top = (maxHeight - newHeight) / 2;

	//SetWindowPos(hVNCWnd, NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
	SetWindowPos(hVNCWnd, NULL, left, top, newWidth, newHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	oldWidth = newWidth;
	oldHeight = newHeight;
}
//---------------------------------------------------------------------------
void Tabsheet::sendDataToVNC(HWND hwnd, int data, HWND Handle, int what)
{
	HWND handle = panel->Handle;
	COPYDATASTRUCT cds;
	cds.cbData = sizeof(HWND);
	cds.lpData = &handle;
	cds.dwData = what;
	SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(panel->Handle), reinterpret_cast<LPARAM>(&cds));
}
//---------------------------------------------------------------------------
void Tabsheet:: updateSize()
{
	RECT rect;
	GetWindowRect(hVNCWnd, &rect);
	if ((rect.bottom -rect.top) != oldHeight  ||  (rect.right-rect.left) != oldWidth)
		PanelResize(NULL);
}
//---------------------------------------------------------------------------
void __fastcall Tabsheet::Close()
{
	SendMessage(hVNCWnd, 0x0010, 0, 0);
	TabsheetList::getInstance()->removeFromTabsheetList(caption);
	::SetParent(hVNCWnd, NULL);
}
//---------------------------------------------------------------------------
void __fastcall Tabsheet::RemoveToolbar()
{
	toppanel->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall Tabsheet::TopToolbar()
{
	toppanel->Visible = false;
	toppanel->Align = alTop;
	fraTopPanel->ToolBar1->AlignWithMargins = true;
	toppanel->Height = 64;
	toppanel->Visible = true;
}
//---------------------------------------------------------------------------
void __fastcall Tabsheet::LeftToolbar()
{
	toppanel->Visible = false;
	toppanel->Align = alLeft;
	toppanel->Width = 120;
	fraTopPanel->ToolBar1->AlignWithMargins = false;
	toppanel->Visible = true;
}
//---------------------------------------------------------------------------
void  Tabsheet::SetVNCParent()
{
	::SetParent(hVNCWnd, panel->Handle);
	PanelResize(NULL);
}

