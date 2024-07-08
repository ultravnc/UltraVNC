//---------------------------------------------------------------------------

#ifndef TabsheetListH
#define TabsheetListH
//---------------------------------------------------------------------------
#include "Tabsheet.h"
#include <map>
class TabsheetList
{
	private:
		std::map<String, Tabsheet*> tabMap;
		TabsheetList();
		static TabsheetList* instance;
	public:
		static TabsheetList* getInstance();
		~TabsheetList();
		void addToTabsheetList(String customName, Tabsheet *tabsheet);
		void removeFromTabsheetList(String customName);
		bool existInTabsheetList(String customName);
		void closeAll();
		bool isTabOpen();
		Tabsheet* getTabsheet(String customName);
        void updateSizes();
};
#endif
