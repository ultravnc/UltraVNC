//---------------------------------------------------------------------------

#pragma hdrstop

#include "SettingsManager.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

SettingsManager::SettingsManager()
{
	ini = new TIniFile( ChangeFileExt( Application->ExeName, ".INI" ) );
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

void SettingsManager::save(String name)
{
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

void SettingsManager::load(String name)
{
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
