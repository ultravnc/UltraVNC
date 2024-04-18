//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "uFrmCard.h"
#include "uAddCard.h"
#include "CardList.h"
#include "uMainForm.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TfrmCard::TfrmCard(TComponent* Owner, TMainForm* mainform,String globalPassword)
	: TFrame(Owner)
{
    this->globalPassword = globalPassword;
	this->mainform = mainform;
    PreviewPanel->Visible = false;

}
//---------------------------------------------------------------------------
__fastcall TfrmCard::~TfrmCard()
{

}
//---------------------------------------------------------------------------
void TfrmCard::setCustumName(String custumName)
{
	lblCustumName->Caption = custumName;
	PreviewCustomName->Caption = custumName;
}
//---------------------------------------------------------------------------
void TfrmCard::copyCardSetting(CardSetting *setting)
{
	cardSetting.customName = setting->customName;
	cardSetting.naam = setting->naam;
	cardSetting.host = setting->host;
	cardSetting.repeater = setting->repeater;
	cardSetting.idNumber = setting->idNumber;
	cardSetting.useRepeater = setting->useRepeater;
	cardSetting.encryptionPlugin = setting->encryptionPlugin;
	cardSetting.useEncryption = setting->useEncryption;
	cardSetting.user = setting->user;
	cardSetting.password = setting->password;
	cardSetting.encoder = setting->encoder;
	cardSetting.viewOnly = setting->viewOnly;
	cardSetting.cursorType = setting->cursorType;
	cardSetting.disableClipboard = setting->disableClipboard;
	cardSetting.alternativeKeybaord = setting->alternativeKeybaord;
	cardSetting.notification = setting->notification;
	updatUI();
}
//---------------------------------------------------------------------------
void __fastcall TfrmCard::imgRemoveClick(TObject *Sender)
{
	SettingsManager sm;
	sm.eraseSection(lblCustumName->Caption);
	CardList::getInstance()->removeFromCardList(lblCustumName->Caption);
	Visible = 0;
}
//---------------------------------------------------------------------------
void __fastcall TfrmCard::imgSettingsClick(TObject *Sender)
{
	TAddCard *addCard = new TAddCard(NULL, globalPassword);

	addCard->importSettings(&cardSetting);
	int result = addCard->ShowModal();
	updatUI();
	delete addCard;
}
//---------------------------------------------------------------------------
void TfrmCard::updatUI()
{
	if (cardSetting.useRepeater)
		Host->Caption = cardSetting.repeater;
	else
		Host->Caption = cardSetting.host;

	if (cardSetting.useEncryption) {
		encryption->Visible = true;
		unencrypted->Visible = false;
	}
	else {
		encryption->Visible = false;
		unencrypted->Visible = true;
	}

	if (cardSetting.viewOnly) {
		ViewOnly->Visible = true;
		Full->Visible = false;
	}
	else {
		ViewOnly->Visible = false;
		Full->Visible = true;
    }

}

void __fastcall TfrmCard::RelativePanel2Click(TObject *Sender)
{
  mainform->addTab(lblCustumName->Caption);
}
//---------------------------------------------------------------------------
String TfrmCard::getHost()
{
	return cardSetting.host;
}
//---------------------------------------------------------------------------
void TfrmCard::setOnline(bool value)
{
	if (!preview) {
		offline->Visible = !value;
		online->Visible = value;
	}
}
//---------------------------------------------------------------------------
void TfrmCard::SetVNCParent(HWND hOldVNCWnd)
{
	::SetParent(hOldVNCWnd, PreviewPanel->Handle);
	PreviewPanel->Visible = true;
	PreviewCustomName->Visible = true;
	offline->Visible = false;
	online->Visible = false;
	ViewOnly->Visible = false;
	Full->Visible = false;
	encryption->Visible = false;
	unencrypted->Visible = false;
	imgRemove->Visible = false;
	imgSettings->Visible = false;
	preview = true;
	Host->Visible = false;
	lblCustumName->Visible = false;
	SetWindowPos(hOldVNCWnd, NULL, 0, 0, PreviewPanel->Width, PreviewPanel->Height, SWP_NOZORDER | SWP_NOACTIVATE);
}
//---------------------------------------------------------------------------
void TfrmCard::UnSetVNCParent()
{
	PreviewPanel->Visible = false;
	PreviewCustomName->Visible = false;
	imgRemove->Visible = true;
	updatUI();
	preview = false;
	Host->Visible = true;
	lblCustumName->Visible = true;
}

