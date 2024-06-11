//---------------------------------------------------------------------------

#ifndef DlgPasswordBoxH
#define DlgPasswordBoxH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TPasswordDialog : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TButton *Button2;
	TButton *ButtonOK;
	TEdit *EditOldPass;
	TEdit *EditNewPass;
	TStaticText *InfoText;
	TEdit *EditNewPass2;
	void __fastcall EditNewPassChange(TObject *Sender);
	void __fastcall ButtonOKClick(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
private:	// User declarations
	String newPassword;
	String oldPassword;
    int update;
public:		// User declarations
	__fastcall TPasswordDialog(TComponent* Owner, int update);
	String getNewPassword();
	String getOldPassword();

};

//---------------------------------------------------------------------------
extern PACKAGE TPasswordDialog *PasswordDialog;
//---------------------------------------------------------------------------
#endif
