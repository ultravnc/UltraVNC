//---------------------------------------------------------------------------

#ifndef uMainFormH
#define uMainFormH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Data.Bind.Components.hpp>
#include <Data.Bind.Controls.hpp>
#include <Data.Bind.DBScope.hpp>
#include <Data.Bind.EngExt.hpp>
#include <Data.Bind.Grid.hpp>
#include <System.Bindings.Outputs.hpp>
#include <System.ImageList.hpp>
#include <System.Rtti.hpp>
#include <Vcl.BaseImageCollection.hpp>
#include <Vcl.Bind.DBEngExt.hpp>
#include <Vcl.Bind.Editors.hpp>
#include <Vcl.Bind.Grid.hpp>
#include <Vcl.Bind.Navigator.hpp>
#include <Vcl.Buttons.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ImageCollection.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include <Vcl.ImgList.hpp>
#include <Vcl.VirtualImage.hpp>
#include <Vcl.VirtualImageList.hpp>
#include <Vcl.WinXCalendars.hpp>
#include <Vcl.WinXCtrls.hpp>
#include <Vcl.TitleBarCtrls.hpp>
#include <Vcl.Menus.hpp>

#include <vector>
#include "uFrmCard.h"
#include "SizeChangedThread.h"
#include "PollThread.h"

//---------------------------------------------------------------------------
class TMainForm : public TForm
{
__published:	// IDE-managed Components
	TSplitView *SplitView;
	TPanel *pnlToolbar;
	TLabel *lblTitle;
	TPanel *NavPanel;
	TImage *Image5;
	TPageControl *PageControl;
	TTabSheet *DashboardTab;
	TPanel *Panel5;
	TLabel *Label4;
	TFlowPanel *FlowPanel1;
	TRelativePanel *AddSession;
	TTitleBarPanel *TitleBarPanel1;
	TComboBox *VCLStylesCB;
	TScrollBox *ScrollBox1;
	TImage *Image3;
	TImage *Image4;
	TImage *HamburgerMenu;
	TLabel *Label1;
	TImageList *ImageList1;
	TPopupMenu *PopupMenu1;
	TMenuItem *close1;
	TMenuItem *close2;
	TMenuItem *addtoolbar1;
	TMenuItem *op1;
	TMenuItem *Right1;
	TButton *btnPasword;
	void __fastcall VCLStylesCBChange(TObject *Sender);
	void __fastcall MenuVirtualImageClick(TObject *Sender);
	void __fastcall DashboardButtonClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall AddSessionClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall VCLStylesCBMouseEnter(TObject *Sender);
	void __fastcall VCLStylesCBMouseLeave(TObject *Sender);
	void __fastcall DashboardTabMouseEnter(TObject *Sender);
	void __fastcall PageControlDrawTab(TCustomTabControl *Control, int TabIndex, const TRect &Rect,
          bool Active);
	void __fastcall PageControlMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y);
	void __fastcall close1Click(TObject *Sender);
	void __fastcall RemoveToolbarClick(TObject *Sender);
	void __fastcall Top1Click(TObject *Sender);
	void __fastcall Left1Click(TObject *Sender);
	void __fastcall btnPaswordClick(TObject *Sender);
	void __fastcall PageControlChange(TObject *Sender);
	void __fastcall PageControlChanging(TObject *Sender, bool &AllowChange);
private:	// User declarations
	 TStrings *sections;
	 SizeChangedThread *thread;
	 PollThread *pollThread;
     int selectedTab;
	 String globalPassword;
	 HWND hOldVNCWnd = NULL;
	 bool fromsession = false;
	 TfrmCard *oldVNCCard = NULL;

public:		// User declarations
	__fastcall TMainForm(TComponent* Owner);
     __fastcall ~TMainForm();
	void addTab(String Caption);
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
