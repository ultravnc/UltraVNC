//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "TopBarPanel.h"
#include "TabsheetList.h"

#define ID_TEXTCHAT                     50112
#define ID_FILETRANSFER                 50100
#define ID_CONN_CTLALTDEL               110
#define IDC_OPTIONBUTTON                1007
#define ID_SNAPSHOT                          50110

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfraTopPanel *fraTopPanel;
//---------------------------------------------------------------------------
__fastcall TfraTopPanel::TfraTopPanel(TComponent* Owner)
	: TFrame(Owner)
{
}
void TfraTopPanel::setVNCHandle(HWND hwnd, String caption)
{
	VNCHandle = hwnd;
	this->caption = caption;
}
//---------------------------------------------------------------------------
void __fastcall TfraTopPanel::ChatClick(TObject *Sender)
{
	SendMessage(VNCHandle, WM_SYSCOMMAND,(WPARAM)ID_TEXTCHAT,(LPARAM)0);
}
//---------------------------------------------------------------------------

void __fastcall TfraTopPanel::FileTransferClick(TObject *Sender)
{
	SendMessage(VNCHandle,WM_SYSCOMMAND,(WPARAM)ID_FILETRANSFER,(LPARAM)0);
}
//---------------------------------------------------------------------------
void __fastcall TfraTopPanel::Mon1Click(TObject *Sender)
{
	int mon = 0;
	COPYDATASTRUCT cds;
	cds.cbData = sizeof(HWND);
	cds.lpData = &mon;
	cds.dwData = 2;
	SendMessage(VNCHandle, WM_COPYDATA, reinterpret_cast<WPARAM>(VNCHandle), reinterpret_cast<LPARAM>(&cds));
}
//---------------------------------------------------------------------------

void __fastcall TfraTopPanel::Mon2Click(TObject *Sender)
{
	int mon = 1;
	COPYDATASTRUCT cds;
	cds.cbData = sizeof(HWND);
	cds.lpData = &mon;
	cds.dwData = 2;
	SendMessage(VNCHandle, WM_COPYDATA, reinterpret_cast<WPARAM>(VNCHandle), reinterpret_cast<LPARAM>(&cds));
}
//---------------------------------------------------------------------------

void __fastcall TfraTopPanel::Mon3Click(TObject *Sender)
{
	int mon = 3;
	COPYDATASTRUCT cds;
	cds.cbData = sizeof(HWND);
	cds.lpData = &mon;
	cds.dwData = 2;
	SendMessage(VNCHandle, WM_COPYDATA, reinterpret_cast<WPARAM>(VNCHandle), reinterpret_cast<LPARAM>(&cds));
}
//---------------------------------------------------------------------------
void __fastcall TfraTopPanel::CloseClick(TObject *Sender)
{
	SendMessage(VNCHandle, 0x0010, 0, 0);
	TabsheetList::getInstance()->removeFromTabsheetList(caption);
	::SetParent(VNCHandle, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TfraTopPanel::CtrlAltDelClick(TObject *Sender)
{
	SendMessage(VNCHandle,WM_SYSCOMMAND,(WPARAM)ID_CONN_CTLALTDEL,(LPARAM)0);
}
//---------------------------------------------------------------------------
void __fastcall TfraTopPanel::SettingsClick(TObject *Sender)
{
	SendMessage(VNCHandle,WM_SYSCOMMAND,(WPARAM)IDC_OPTIONBUTTON,(LPARAM)0);
}
//---------------------------------------------------------------------------
void __fastcall TfraTopPanel::SnapshotClick(TObject *Sender)
{
	SendMessage(VNCHandle,WM_SYSCOMMAND,(WPARAM)ID_SNAPSHOT,(LPARAM)0);
}
//---------------------------------------------------------------------------

