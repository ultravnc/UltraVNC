#include "stdafx.h"
#include <fstream>
#include <shellapi.h>
using namespace std;

extern char myname[256];
extern HWND hTab0dialog,hTab1dialog,hTab2dialog,hTab3dialog,hTab4dialog,hTab5dialog,hTab6dialog,hTab7dialog;

void Save_settings_sfx(char *myfile);

void create_config_txt(char *cmd)
{
	char szFileName[MAX_PATH];
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
		{
		char* p = strrchr(szFileName, '\\');
		if (p == NULL) return;
			*p = '\0';
		strcat (szFileName,"\\sfx\\config.txt");
		}

	ofstream outFile(szFileName);
	outFile << ";!@Install@!UTF-8!\n";
	outFile << "Title=\"UltraVNC\"\n";
	outFile << "RunProgram=\"winvnc.exe ";
	outFile << cmd;
	outFile << "\"\n";
	outFile << ";!@InstallEnd@!\n";

	outFile.close();
}

void create_bat(char *cmd)
{
	char szFileName[MAX_PATH];
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
		{
		char* p = strrchr(szFileName, '\\');
		if (p == NULL) return;
			*p = '\0';
		strcat (szFileName,"\\sfx\\");
		strcat (szFileName,myname);
		strcat (szFileName,".bat");
		}

	ofstream outFile(szFileName);
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
		{
		char* p = strrchr(szFileName, '\\');
		if (p == NULL) return;
			*p = '\0';
		strcat (szFileName,"\\vncviewer.exe ");
		}
	outFile << szFileName;
	outFile << cmd;
	outFile << "\n";
	outFile.close();
}

void
create_uvnc_ini()
{
	char szFileName[MAX_PATH];
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
		{
		char* p = strrchr(szFileName, '\\');
		if (p == NULL) return;
			*p = '\0';
		strcat (szFileName,"\\sfx\\UltraVNC.ini");
		}
	Save_settings_sfx(szFileName);
}

void Make_Folder(char *szFileName)
{
	CreateDirectory(szFileName,NULL);
}

void createsfx3(char *cmd,char *cmd2)
{
	char szFileName[MAX_PATH];
	char szcmd[MAX_PATH];
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
		{
		char* p = strrchr(szFileName, '\\');
		if (p == NULL) return;
			*p = '\0';
		strcat (szFileName,"\\sfx\\");
		}
	Make_Folder(szFileName);
	create_config_txt(cmd);
	create_bat(cmd2);

	SendMessage(hTab0dialog, WM_COMMAND, IDOK, 0);
	SendMessage(hTab1dialog, WM_COMMAND, IDOK, 0);
	SendMessage(hTab2dialog, WM_COMMAND, IDOK, 0);
	SendMessage(hTab3dialog, WM_COMMAND, IDOK, 0);
	SendMessage(hTab4dialog, WM_COMMAND, IDOK, 0);
	SendMessage(hTab5dialog, WM_COMMAND, IDOK, 0);
	SendMessage(hTab6dialog, WM_COMMAND, IDOK, 0);	
	SendMessage(hTab7dialog, WM_COMMAND, IDOK, 0);	

	create_uvnc_ini();
	strcpy(szcmd,szFileName);
	strcat(szcmd,"7z.exe");
	ShellExecute(NULL,"open",szcmd,"a -mx=9 -t7z 7zip.7z winvnc.exe schook.dll MSRC4Plugin.dsm UltraVNC.ini",szFileName,SW_SHOW);

	char szcmd1[MAX_PATH];
	char szcmd2[MAX_PATH];
	char szcmd3[MAX_PATH];
	char szcmd4[MAX_PATH];
	strcpy(szcmd1,szFileName);
	strcpy(szcmd2,szFileName);
	strcpy(szcmd3,szFileName);
	strcpy(szcmd4,szFileName);

	strcat(szcmd1,"7zip.sfx");
	strcat(szcmd2,"config.txt");
	strcat(szcmd3,"7zip.7z");
	strcat(szcmd4,myname);
	strcat(szcmd4,".exe");
	Sleep(2000);
	ifstream myFile1 (szcmd1, ios::in | ios::binary);
	ifstream myFile2 (szcmd2, ios::in | ios::binary);
	ifstream myFile3 (szcmd3, ios::in | ios::binary);
	ofstream myFile4 (szcmd4, ios::out | ios::binary);

	myFile1.seekg(0,ifstream::end);
	int size=myFile1.tellg();
	myFile1.seekg(0);
	char *buffer = new char [size];
	myFile1.read (buffer,size);
	myFile4.write (buffer,size);
	delete[] buffer;
	myFile1.close();

	myFile2.seekg(0,ifstream::end);
	size=0;
	size=myFile2.tellg();
	myFile2.seekg(0);
	buffer = new char [size];
	myFile2.read (buffer,size);
	myFile4.write (buffer,size);
	delete[] buffer;
	myFile2.close();

	myFile3.seekg(0,ifstream::end);
	size=0;
	size=myFile3.tellg();
	myFile3.seekg(0);
	buffer = new char [size];
	myFile3.read (buffer,size);
	myFile4.write (buffer,size);
	delete[] buffer;
	myFile3.close();
	myFile4.close();

}