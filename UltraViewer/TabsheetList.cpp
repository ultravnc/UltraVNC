//---------------------------------------------------------------------------

#pragma hdrstop

#include "TabsheetList.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

TabsheetList* TabsheetList::instance = nullptr;

TabsheetList::TabsheetList(){}
TabsheetList* TabsheetList::getInstance() {

	return (!instance)
		? instance = new TabsheetList()
		: instance;
}

TabsheetList::~TabsheetList()
{
	delete instance;
}
//---------------------------------------------------------------------------
void TabsheetList::addToTabsheetList(String customName, Tabsheet *tabsheet){
   tabMap[customName] = tabsheet;
}
//---------------------------------------------------------------------------
void TabsheetList::removeFromTabsheetList(String customName){
	if (tabMap[customName])
		tabMap[customName]->tab->TabVisible = false;
	tabMap.erase(customName);
}
//---------------------------------------------------------------------------
bool TabsheetList::existInTabsheetList(String customName){
	return tabMap.count(customName)>0;
}
//---------------------------------------------------------------------------
void TabsheetList::closeAll()
{
	for (const auto& pair : tabMap) {
		Tabsheet* tabsheet = pair.second;
		SendMessage(tabsheet->hVNCWnd, 0x0010, 0, 0);
		::SetParent(tabsheet->hVNCWnd, NULL);
	}
}
//---------------------------------------------------------------------------
bool TabsheetList::isTabOpen()
{
	for (const auto& pair : tabMap) {
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
Tabsheet* TabsheetList::getTabsheet(String customName)
{
	return tabMap[customName];
}
//---------------------------------------------------------------------------
void TabsheetList::updateSizes()
{
	for (const auto& pair : tabMap) {
		Tabsheet* tabsheet = pair.second;
		tabsheet->updateSize();
	}
}
//---------------------------------------------------------------------------


