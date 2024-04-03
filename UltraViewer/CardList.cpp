//---------------------------------------------------------------------------

#pragma hdrstop
#include "UdpPoll.h"
#include "CardList.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

CardList* CardList::instance = nullptr;

CardList::CardList(){}
//---------------------------------------------------------------------------
CardList* CardList::getInstance() {

	return (!instance)
		? instance = new CardList()
		: instance;
}
//---------------------------------------------------------------------------
CardList::~CardList()
{
	delete instance;
}
//---------------------------------------------------------------------------
void CardList::addToCardList(String customName,TfrmCard *frmCard)
{
	cardMap[customName] = frmCard;
}
//---------------------------------------------------------------------------
void CardList::removeFromCardList(String customName)
{
	TfrmCard *frmCard = cardMap[customName];
	delete frmCard;
	cardMap.erase(customName);
}
//---------------------------------------------------------------------------
bool CardList::existInCardList(String customName)
{
	return cardMap.count(customName)>0;
}
//---------------------------------------------------------------------------
void CardList::pollAlive()
{
	 for (const auto& pair : cardMap) {
		TfrmCard* card = pair.second;
		String hosts = card->getHost();
		if (TDMUdpPoll::getInstance()->SendReceive(card->getHost())) {
			if (bstopPoll) return;
			card->setOnline(true);
		}
		else {
			if (bstopPoll) return;
			card->setOnline(false);
		}

	}
}
//---------------------------------------------------------------------------
void CardList::stopPoll()
{
	bstopPoll = true;
}

