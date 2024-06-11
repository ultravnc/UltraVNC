//---------------------------------------------------------------------------

#ifndef TopBarPanelH
#define TopBarPanelH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ToolWin.hpp>
#include <System.ImageList.hpp>
#include <Vcl.ImgList.hpp>
//---------------------------------------------------------------------------
class TfraTopPanel : public TFrame
{
__published:	// IDE-managed Components
	TToolBar *ToolBar1;
	TToolButton *Chat;
	TToolButton *FileTransfer;
	TImageList *ImageList1;
	TToolButton *ToolButton4;
	TToolButton *CtrlAltDel;
	TToolButton *Close;
	TToolButton *ToolButton1;
	TToolButton *Settings;
	TToolButton *Mon1;
	TToolButton *Mon2;
	TToolButton *Mon3;
	TToolButton *Snapshot;
	void __fastcall ChatClick(TObject *Sender);
	void __fastcall FileTransferClick(TObject *Sender);
	void __fastcall Mon1Click(TObject *Sender);
	void __fastcall Mon2Click(TObject *Sender);
	void __fastcall Mon3Click(TObject *Sender);
	void __fastcall CloseClick(TObject *Sender);
	void __fastcall CtrlAltDelClick(TObject *Sender);
	void __fastcall SettingsClick(TObject *Sender);
	void __fastcall SnapshotClick(TObject *Sender);
private:	// User declarations
	HWND VNCHandle;
	String caption;
public:		// User declarations
	__fastcall TfraTopPanel(TComponent* Owner);
	void setVNCHandle(HWND hwnd, String caption);
};
//---------------------------------------------------------------------------
extern PACKAGE TfraTopPanel *fraTopPanel;
//---------------------------------------------------------------------------
#endif
