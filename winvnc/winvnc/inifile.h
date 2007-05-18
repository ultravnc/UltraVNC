#include <windows.h>
#if (!defined(_WINVNC_INIFILE))
#define _WINVNC_INIFILE
class IniFile
{

// Fields
public:
	char myInifile[MAX_PATH];

// Methods
public:
	// Make the desktop thread & window proc friends

	IniFile();
	~IniFile();
	bool WriteString(char *key1, char *key2,char *value);
	bool WritePassword(char *value);
	bool WriteInt(char *key1, char *key2,int value);
	int ReadInt(char *key1, char *key2,int Defaultvalue);
	void ReadString(char *key1, char *key2,char *value,int valuesize);
	void ReadPassword(char *value,int valuesize);

protected:
		
};
#endif