//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "DlgPasswordBox.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TPasswordDialog *PasswordDialog;
//---------------------------------------------------------------------------
__fastcall TPasswordDialog::TPasswordDialog(TComponent* Owner,  int update)
	: TForm(Owner)
{
	this->update = update;
	ButtonOK->Enabled = false;
	if (update == 0) {
		EditOldPass->Visible = true;
		EditNewPass->Visible = true;
		EditNewPass2->Visible = true;
		ButtonOK->Caption = "Change password";
		InfoText->Caption = "Enter the old and new password to change.";

	}
	else if (update == 1){
		EditOldPass->Visible = false;
		EditNewPass->Visible = true;
		EditNewPass2->Visible = true;
		ButtonOK->Caption = "Set password";
		InfoText->Caption = "After setting a password the ini file is encrypted, the file can't be unencrypted without the proper password.";
	}
	else if (update == 2){
		ButtonOK->Enabled = true;
		EditOldPass->Visible = false;
		EditNewPass->Visible = true;
		EditNewPass2->Visible = false;
		ButtonOK->Caption = "Enter password";
		InfoText->Visible = false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TPasswordDialog::EditNewPassChange(TObject *Sender)
{
	ButtonOK->Enabled = (EditNewPass->Text == EditNewPass2->Text &&  !EditNewPass->Text.IsEmpty()) || update == 2;
}
//---------------------------------------------------------------------------
void __fastcall TPasswordDialog::ButtonOKClick(TObject *Sender)
{
	newPassword = EditNewPass->Text;
	oldPassword = EditOldPass->Text;
	ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TPasswordDialog::Button2Click(TObject *Sender)
{
	ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
String TPasswordDialog::getNewPassword()
{
	return newPassword;
}
//---------------------------------------------------------------------------
String TPasswordDialog::getOldPassword()
{
	return oldPassword;
}
//---------------------------------------------------------------------------
