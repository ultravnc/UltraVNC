//---------------------------------------------------------------------------

#ifndef SettingsManagerH
#define SettingsManagerH
#include "IniFiles.hpp"
#include "CardSetting.h"
//---------------------------------------------------------------------------

#define remote 1
#define local 2
#define none 3

class SettingsManager
{

private:	// User declarations
	TIniFile *ini;
	CardSetting *cardSetting;

public:		// User declarations
	SettingsManager();
	~SettingsManager();
	void save(String naam);
	void load(String naam);
	void getsections(TStrings *sections);
	void saveTheme(String theme);
	String loadTheme();
	CardSetting *getCardSetting();
	void eraseSection(String naam);
};
#endif
