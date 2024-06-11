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
    String Encrypt(String input, String key);
	String Decrypt(String input, String key);

public:		// User declarations
	SettingsManager();
	~SettingsManager();
	void save(String naam, String pass);
	bool load(String naam, String pass);
	void getsections(TStrings *sections);
	void saveTheme(String theme);
	String loadTheme();
	CardSetting *getCardSetting();
	void eraseSection(String naam);
	void AddPassword(String newpass, String oldpass);
	bool isEncryptionUsed();
    bool ispasswordCorrect(String pass);

};
#endif
