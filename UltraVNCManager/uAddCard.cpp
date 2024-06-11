//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "uAddCard.h"
#include "uMainForm.h"
#include "SettingsManager.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TAddCard::TAddCard(TComponent* Owner, String globalPassword)
	: TForm(Owner)
{
	this->globalPassword = globalPassword;
	Encryptions->Enabled = UseEncryption->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TAddCard::BackButtonClick( TObject* Sender )
{
	ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TAddCard::SaveClick( TObject* Sender )
{
	if (CustomName->Text.IsEmpty()) {
		ModalResult = mrCancel;
	}
	else {
		SettingsManager sm;
		CardSetting *setting = sm.getCardSetting();
		setting->host = HostPort->Text;
		setting->repeater = RepeaterPort->Text;
		setting->idNumber = IDNumber->Text;
		setting->useRepeater = rbRepeater->Checked;
		setting->encryptionPlugin = Encryptions->Text;
		setting->useEncryption = UseEncryption->Checked;
		setting->user = User->Text;
		setting->password = Password->Text;
		setting->encoder = Encoders->Text;
		setting->viewOnly = ViewOnly->Checked;
		if (rbLocalCursor->Checked) setting->cursorType = 1;
		else if (rbRemoteCursor->Checked) setting->cursorType = 2;
		else if (rbHideCursor->Checked) setting->cursorType = 3;
		setting->disableClipboard =  DisableClipboard->Checked;
		setting->alternativeKeybaord =  AlternativeKeybaord->Checked;
		setting->notification =  Notification->Text;
		setting->customName = CustomName->Text.TrimRight();
		sm.save(CustomName->Text, globalPassword);

		if (cardSetting != NULL) {
			cardSetting->customName = setting->customName.TrimRight();
			cardSetting->naam = setting->naam;
			cardSetting->host = setting->host;
			cardSetting->repeater = setting->repeater;
			cardSetting->idNumber = setting->idNumber;
			cardSetting->useRepeater = setting->useRepeater;
			cardSetting->encryptionPlugin = setting->encryptionPlugin;
			cardSetting->useEncryption = setting->useEncryption;
			cardSetting->user = setting->user;
			cardSetting->password = setting->password;
			cardSetting->encoder = setting->encoder;
			cardSetting->viewOnly = setting->viewOnly;
			cardSetting->cursorType = setting->cursorType;
			cardSetting->disableClipboard = setting->disableClipboard;
			cardSetting->alternativeKeybaord = setting->alternativeKeybaord;
			cardSetting->notification = setting->notification;
		}
		ModalResult = mrOk;
	}
}
//---------------------------------------------------------------------------
void __fastcall TAddCard::rbRepeaterClick(TObject *Sender)
{
	IDNumber->Top = HostPort->Top - 1;
	RepeaterPort->Top = HostPort->Top;
	HostPort->Visible = false;
	RepeaterPort->Visible = true;
	IDNumber->Visible = true;

}
//---------------------------------------------------------------------------
void __fastcall TAddCard::rbDirectClick(TObject *Sender)
{
	HostPort->Visible = true;
	RepeaterPort->Visible = false;
	IDNumber->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TAddCard::UseEncryptionClick(TObject *Sender)
{
	Encryptions->Enabled = UseEncryption->Checked;
}
//---------------------------------------------------------------------------
String TAddCard::getCustomName()
{
	return CustomName->Text;
}
//---------------------------------------------------------------------------
void TAddCard::importSettings(CardSetting *settings)
{
	this->cardSetting = settings;
	CustomName->Text = cardSetting->customName.TrimRight();
	HostPort->Text = cardSetting->host;
	RepeaterPort->Text =	cardSetting->repeater;
	IDNumber->Text =	cardSetting->idNumber;
	rbRepeater->Checked	= cardSetting->useRepeater;
	Encryptions->Text =	cardSetting->encryptionPlugin;
	UseEncryption->Checked = cardSetting->useEncryption;
	User->Text = cardSetting->user;
	Password->Text = cardSetting->password;
	Encoders->Text = cardSetting->encoder;
	ViewOnly->Checked =	cardSetting->viewOnly;

	if (cardSetting->cursorType == 1)
		rbLocalCursor->Checked = true;
	else if (cardSetting->cursorType == 2)
		rbRemoteCursor->Checked = true;
	else if (cardSetting->cursorType == 3)
		rbHideCursor->Checked = true;

	DisableClipboard->Checked = cardSetting->disableClipboard;
	AlternativeKeybaord->Checked = cardSetting->alternativeKeybaord;
	Notification->Text = cardSetting->notification ;
	CustomName->Enabled = false;
}
//---------------------------------------------------------------------------


