//---------------------------------------------------------------------------

#ifndef CardListH
#define CardListH
//---------------------------------------------------------------------------
#include "uFrmCard.h"
#include <map>

class CardList
{
	private:
		std::map<String, TfrmCard*> cardMap;
		CardList();
		static CardList* instance;
		bool bstopPoll = false;
	public:
		static CardList* getInstance();
		~CardList();
		void addToCardList(String customName, TfrmCard *frmCard);
		void removeFromCardList(String customName);
		bool existInCardList(String customName);
		void pollAlive();
		void stopPoll();
		TfrmCard * getCard(String customName);
};
#endif
