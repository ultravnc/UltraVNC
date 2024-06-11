//---------------------------------------------------------------------------

#ifndef uFrmCardH
#define uFrmCardH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.VirtualImage.hpp>
#include <Vcl.WinXCtrls.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include "SettingsManager.h"
#include <Vcl.Graphics.hpp>
//---------------------------------------------------------------------------
class TMainForm;
class TfrmCard : public TFrame
{
__published:	// IDE-managed Components
	TRelativePanel *RelativePanel2;
	TLabel *lblCustumName;
	TImage *imgSettings;
	TImage *offline;
	TImage *imgRemove;
	TImage *encryption;
	TImage *Full;
	TLabel *Host;
	TImage *online;
	TPanel *PreviewPanel;
	TLabel *PreviewCustomName;
	TImage *unencrypted;
	TImage *ViewOnly;
	void __fastcall imgRemoveClick(TObject *Sender);
	void __fastcall imgSettingsClick(TObject *Sender);
	void __fastcall RelativePanel2Click(TObject *Sender);
private:	// User declarations
	CardSetting cardSetting;
	void updatUI();
	TMainForm * mainform;
	String globalPassword;
    bool preview = false;
public:		// User declarations
	__fastcall TfrmCard(TComponent* Owner, TMainForm* Owner2, String globalPassword);
	__fastcall ~TfrmCard();
	void setCustumName(String custumName);
	void copyCardSetting(CardSetting *cardSetting);
	String getHost();
	void setOnline(bool value);
	void SetVNCParent(HWND hOldVNCWnd);
	void UnSetVNCParent();
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
