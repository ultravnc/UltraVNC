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
__fastcall TfrmCard::TfrmCard(TComponent* Owner, TMainForm* mainform)
	: TFrame(Owner)
{
	this->mainform = mainform;
}
//---------------------------------------------------------------------------
__fastcall TfrmCard::~TfrmCard()
{

}
//---------------------------------------------------------------------------
void TfrmCard::setCustumName(String custumName)
{
	lblCustumName->Caption = custumName;
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
	TAddCard *addCard = new TAddCard(NULL);

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

	encryption->Visible = cardSetting.useEncryption;

	Viewonly->Visible = !cardSetting.viewOnly;
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
	offline->Visible = !value;
	online->Visible = value;
}

