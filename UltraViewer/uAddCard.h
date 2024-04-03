//---------------------------------------------------------------------------

#ifndef uAddCardH
#define uAddCardH
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
#include <Data.DB.hpp>
#include <SHDocVw.hpp>
#include <System.Bindings.Outputs.hpp>
#include <System.Rtti.hpp>
#include <Vcl.Bind.DBEngExt.hpp>
#include <Vcl.Bind.Editors.hpp>
#include <Vcl.Bind.Grid.hpp>
#include <Vcl.Bind.Navigator.hpp>
#include <Vcl.Buttons.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.OleCtrls.hpp>
#include <Vcl.VirtualImage.hpp>
#include <Vcl.WinXCtrls.hpp>
#include <Vcl.WinXPanels.hpp>
#include <System.SysUtils.hpp>
#include <System.NetEncoding.hpp>
#include <mshtml.h>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include "CardSetting.h"

//---------------------------------------------------------------------------
class TAddCard : public TForm
{
__published:	// IDE-managed Components
	TCardPanel *DraftCardPanel;
	TCard *DraftCard;
	TPanel *DraftHeaderPanel;
	TLabel *Label11;
	TPanel *NavPanel;
	TButton *Save;
	TButton *BackButton;
	TPanel *Panel1;
	TEdit *HostPort;
	TEdit *CustomName;
	TRadioButton *rbDirect;
	TRadioButton *rbRepeater;
	TEdit *RepeaterPort;
	TPanel *Panel2;
	TEdit *IDNumber;
	TComboBox *Encoders;
	TPanel *Panel3;
	TRadioButton *rbLocalCursor;
	TRadioButton *rbRemoteCursor;
	TRadioButton *rbHideCursor;
	TPanel *Panel4;
	TCheckBox *ViewOnly;
	TPanel *Panel5;
	TPanel *Panel6;
	TPanel *Panel7;
	TCheckBox *DisableClipboard;
	TCheckBox *UseEncryption;
	TComboBox *Encryptions;
	TCheckBox *AlternativeKeybaord;
	TImage *Image1;
	TStaticText *StaticText1;
	TEdit *User;
	TEdit *Password;
	TEdit *Notification;
	void __fastcall BackButtonClick( TObject* Sender );
	void __fastcall SaveClick( TObject* Sender );
	void __fastcall rbRepeaterClick(TObject *Sender);
	void __fastcall rbDirectClick(TObject *Sender);
	void __fastcall UseEncryptionClick(TObject *Sender);
private:	// User declarations
	CardSetting *cardSetting = NULL;
public:		// User declarations
	__fastcall TAddCard(TComponent* Owner);
	String getCustomName();
	void importSettings(CardSetting *settings);
};

#endif
