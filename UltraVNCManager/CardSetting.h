#ifndef CardSettingH
#define CardSettingH

#include <vcl.h>

struct CardSetting{
	String customName;
	String naam;
	String host;
	String repeater;
	String idNumber;
	bool useRepeater;
	String encryptionPlugin;
	bool useEncryption;
	String user;
	String password;
	String encoder;
	bool viewOnly;
	int cursorType;
	bool disableClipboard;
	bool alternativeKeybaord;
	String notification;
};

#endif
