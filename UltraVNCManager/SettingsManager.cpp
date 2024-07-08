//---------------------------------------------------------------------------

#pragma hdrstop

#include "SettingsManager.h"
#include "Rijndael.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

void Char2Hex(const unsigned char ch, char* szHex)
{
	unsigned char byte[2];
	byte[0] = ch/16;
	byte[1] = ch%16;
	for(int i=0; i<2; i++)
	{
		if(byte[i] >= 0 && byte[i] <= 9)
			szHex[i] = '0' + byte[i];
		else
			szHex[i] = 'A' + byte[i] - 10;
	}
	szHex[2] = 0;
}

//Function to convert string of length 2 to unsigned char
void Hex2Char(const char* szHex, unsigned char& rch)
{
	rch = 0;
	for(int i=0; i<2; i++)
	{
		if(*(szHex + i) >='0' && *(szHex + i) <= '9')
			rch = (rch << 4) + (*(szHex + i) - '0');
		else if(*(szHex + i) >='A' && *(szHex + i) <= 'F')
			rch = (rch << 4) + (*(szHex + i) - 'A' + 10);
		else
			break;
	}
}

//Function to convert string of unsigned chars to string of chars
void CharStr2HexStr(const unsigned char* pucCharStr, char* pszHexStr, int iSize)
{
	int i;
	char szHex[3];
	pszHexStr[0] = 0;
	for(i=0; i<iSize; i++)
	{
		Char2Hex(pucCharStr[i], szHex);
		strcat(pszHexStr, szHex);
	}
}

//Function to convert string of chars to string of unsigned chars
void HexStr2CharStr(const char* pszHexStr, unsigned char* pucCharStr, int iSize)
{
	int i;
	unsigned char ch;
	for(i=0; i<iSize; i++)
	{
		Hex2Char(pszHexStr+2*i, ch);
		pucCharStr[i] = ch;
	}
}

SettingsManager::SettingsManager()
{
	TCHAR path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path);
	String appDataPath = path;
	String iniFilePath = appDataPath + "\\UltraVNCManager\\config.ini";
	if (!DirectoryExists(ExtractFilePath(iniFilePath))) {
		ForceDirectories(ExtractFilePath(iniFilePath));
	}
	ini = new TIniFile( iniFilePath );
	cardSetting = new CardSetting();
}

SettingsManager::~SettingsManager()
{
	delete cardSetting;
	delete ini;
}

void SettingsManager::saveTheme(String theme)
{
	ini->WriteString( "theme", "theme", theme);
}

String SettingsManager::loadTheme()
{
	return ini->ReadString( "theme", "theme", L"Windows10");
}

void SettingsManager::save(String name, String pass)
{
	if (pass.IsEmpty()) {
		ini->WriteString( "UltraViewer", "e", "UltraViewer");
		ini->WriteString( name, "host", cardSetting->host);
		ini->WriteString( name, "repeater", cardSetting->repeater);
		ini->WriteString( name, "idNumber", cardSetting->idNumber);
		ini->WriteString( name, "encryptionPlugin", cardSetting->encryptionPlugin);
		ini->WriteBool( name, "useEncryption", cardSetting->useEncryption);
		ini->WriteString( name, "user", cardSetting->user);
		ini->WriteString( name, "password", cardSetting->password);
		ini->WriteString( name, "encoder", cardSetting->encoder);
		ini->WriteBool( name, "viewOnly", cardSetting->viewOnly);
		ini->WriteInteger( name, "cursorType", cardSetting->cursorType);
		ini->WriteBool( name, "disableClipboard", cardSetting->disableClipboard);
		ini->WriteBool( name, "alternativeKeybaord", cardSetting->alternativeKeybaord);
		ini->WriteString( name, "notification", cardSetting->notification);
		ini->WriteBool( name, "useRepeater", cardSetting->useRepeater);
	}
	else {
		ini->WriteString( "UltraViewer", "e", Encrypt("UltraViewer", pass));
		ini->WriteString( name, "host", Encrypt(cardSetting->host, pass));
		ini->WriteString( name, "repeater", Encrypt(cardSetting->repeater, pass));
		ini->WriteString( name, "idNumber", Encrypt(cardSetting->idNumber, pass));
		ini->WriteString( name, "encryptionPlugin", Encrypt(cardSetting->encryptionPlugin, pass));
		ini->WriteBool( name, "useEncryption", cardSetting->useEncryption);
		ini->WriteString( name, "user", Encrypt(cardSetting->user, pass));
		ini->WriteString( name, "password", Encrypt(cardSetting->password, pass));
		ini->WriteString( name, "encoder", Encrypt(cardSetting->encoder, pass));
		ini->WriteBool( name, "viewOnly",cardSetting->viewOnly);
		ini->WriteInteger( name, "cursorType", cardSetting->cursorType);
		ini->WriteBool( name, "disableClipboard", cardSetting->disableClipboard);
		ini->WriteBool( name, "alternativeKeybaord",cardSetting->alternativeKeybaord);
		ini->WriteString( name, "notification", Encrypt(cardSetting->notification, pass));
		ini->WriteBool( name, "useRepeater", cardSetting->useRepeater);

    }
}

bool SettingsManager::isEncryptionUsed()
{
	 return ! (ini->ReadString( "UltraViewer", "e", "UltraViewer") == "UltraViewer");
}

bool SettingsManager::ispasswordCorrect(String pass)
{
	 return (Decrypt(ini->ReadString( "UltraViewer", "e", "UltraViewer"), pass) == "UltraViewer");
}

bool SettingsManager::load(String name, String pass)
{
	if (pass.IsEmpty() && (ini->ReadString( "UltraViewer", "e", "UltraViewer") == "UltraViewer")) {
		cardSetting->customName = name;
		cardSetting->host = ini->ReadString( name, "host", "");
		cardSetting->repeater = ini->ReadString( name, "repeater", "");
		cardSetting->idNumber= ini->ReadString( name, "idNumber", "");
		cardSetting->encryptionPlugin = ini->ReadString( name, "encryptionPlugin", "");
		cardSetting->useEncryption = ini->ReadBool( name, "useEncryption", false);
		cardSetting->user = ini->ReadString( name, "user", "");
		cardSetting->password = ini->ReadString( name, "password", "");
		cardSetting->encoder = ini->ReadString( name, "encoder", "video");
		cardSetting->viewOnly = ini->ReadBool( name, "viewOnly", false);
		cardSetting->cursorType = ini->ReadInteger( name, "cursorType", local);
		cardSetting->disableClipboard = ini->ReadBool( name, "disableClipboard", false);
		cardSetting->alternativeKeybaord = ini->ReadBool( name, "alternativeKeybaord", false);
		cardSetting->notification = ini->ReadString( name, "notification", "");
		cardSetting->useRepeater = ini->ReadBool( name, "useRepeater", false);
		return true;
	}
	else if ( ispasswordCorrect(pass))
	{
		cardSetting->customName = name;
		cardSetting->host =  Decrypt(ini->ReadString( name, "host", ""), pass);
		cardSetting->repeater =  Decrypt(ini->ReadString( name, "repeater", ""), pass);
		cardSetting->idNumber=  Decrypt(ini->ReadString( name, "idNumber", ""), pass);
		cardSetting->encryptionPlugin =  Decrypt(ini->ReadString( name, "encryptionPlugin", ""), pass);
		cardSetting->useEncryption =  ini->ReadBool( name, "useEncryption", false);
		cardSetting->user =  Decrypt(ini->ReadString( name, "user", ""), pass);
		cardSetting->password =  Decrypt(ini->ReadString( name, "password", ""), pass);
		cardSetting->encoder =  Decrypt(ini->ReadString( name, "encoder", "video"), pass);
		cardSetting->viewOnly =  ini->ReadBool( name, "viewOnly", false);
		cardSetting->cursorType =  ini->ReadInteger( name, "cursorType", local);
		cardSetting->disableClipboard =  ini->ReadBool( name, "disableClipboard", false);
		cardSetting->alternativeKeybaord =  ini->ReadBool( name, "alternativeKeybaord", false);
		cardSetting->notification =  Decrypt(ini->ReadString( name, "notification", ""), pass);
		cardSetting->useRepeater =  ini->ReadBool( name, "useRepeater", false);
		return true;
	}
	return false;
}

void SettingsManager::getsections(TStrings *sections)
{
	ini->ReadSections(sections);
}

CardSetting *SettingsManager::getCardSetting()
{
	return cardSetting;
}

void SettingsManager::eraseSection(String naam)
{
	ini->EraseSection(naam);
}

void SettingsManager::AddPassword(String newpass, String oldpass)
{
	TStrings *sections = new TStringList();
	getsections(sections);
	for (int i = 0; i < sections->Count; ++i){
		load(sections->Strings[i], oldpass);
		save(sections->Strings[i], newpass);
	}
}

String  SettingsManager::Encrypt(String input, String key)
{
	AnsiString ansiInput(input);
	AnsiString ansiKey(key);
	char* mykey = ansiKey.c_str();
	char* szDataIn = ansiInput.c_str();
	char szDataOut[1024]{};
	char szHex[1024]{};
	CBlowFish oBlowFish((unsigned char*)mykey, ansiKey.Length());
	oBlowFish.Encrypt((unsigned char*)szDataIn, (unsigned char*)szDataOut, 1024);
	CharStr2HexStr((unsigned char*)szDataOut, szHex, 1024);
	return String(szHex);
}

String  SettingsManager::Decrypt(String input, String key)
{
	AnsiString ansiInput(input);
	AnsiString ansiKey(key);
	char* mykey = ansiKey.c_str();
	char* szHex = ansiInput.c_str();
	char szDataIn[1024]{};
	char szDataOut[1024]{};
	HexStr2CharStr(szHex, (unsigned char*)szDataIn, 1024);

	CBlowFish oBlowFish((unsigned char*)mykey, ansiKey.Length());
	oBlowFish.Decrypt((unsigned char*)szDataIn, (unsigned char*)szDataOut, 1024);
	return String(szDataOut);
}
