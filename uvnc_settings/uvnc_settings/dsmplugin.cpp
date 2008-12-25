/*	virtual BOOL IsDSMPluginEnabled();
	virtual void EnableDSMPlugin(BOOL fEnable);
	virtual char* GetDSMPluginName();
	virtual void SetDSMPluginName(char* szDSMPlugin);
	virtual BOOL SetDSMPlugin(BOOL fForceReload);
	virtual CDSMPlugin* GetDSMPluginPointer() { return m_pDSMPlugin;};*/

#include "stdafx.h"

#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "DSMPlugin.h"
#define MAXPWLEN 8
extern char passwd[MAXPWLEN];
char * vncDecryptPasswd(char *inouttext);
//
// Utils
//
BOOL MyStrToken(LPSTR szToken, LPSTR lpString, int nTokenNum, char cSep)
{
	int i = 1;
	while (i < nTokenNum)
	{
		while ( *lpString && (*lpString != cSep) &&(*lpString != '\0'))
		{
			lpString++;
		}
		i++;
		lpString++;
	}
	while ((*lpString != cSep) && (*lpString != '\0'))
	{
		*szToken = *lpString;
		szToken++;
		lpString++;
	}
	*szToken = '\0' ;
	if (( ! *lpString ) || (! *szToken)) return NULL;
	return FALSE;
}



//
//
//
CDSMPlugin::CDSMPlugin()
{
	m_fLoaded = false;
	m_fEnabled = false;
	m_lPassLen = 0;

	m_pTransBuffer = NULL;
	m_pRestBuffer = NULL;

	sprintf(m_szPluginName, "Unknown");
	sprintf(m_szPluginVersion, "0.0.0");
	sprintf(m_szPluginDate, "12-12-2002");
	sprintf(m_szPluginAuthor, "Someone");
	sprintf(m_szPluginFileName, "Plugin.dsm"); // No path, just the filename

	m_hPDll = NULL;

	// Plugin's functions pointers init
	STARTUP			m_PStartup = NULL;
	SHUTDOWN		m_PShutdown = NULL;
	SETPARAMS		m_PSetParams = NULL;
	GETPARAMS		m_PGetParams = NULL;
	TRANSFORMBUFFER m_PTransformBuffer = NULL;
	RESTOREBUFFER	m_PRestoreBuffer = NULL;
	TRANSFORMBUFFER	m_PFreeBuffer = NULL;
	RESET			m_PReset = NULL;

}

//
//
//
CDSMPlugin::~CDSMPlugin()
{
	// TODO: Log events
	if (IsLoaded())
		UnloadPlugin();
}


//
//
//
void CDSMPlugin::SetEnabled(bool fEnable)
{
	m_fEnabled =  fEnable;
}


//
//
//
void CDSMPlugin::SetLoaded(bool fEnable)
{
	m_fLoaded =  fEnable;
}


//
//
//
char* CDSMPlugin::DescribePlugin(void)
{
	// TODO: Log events
	char* szDescription = NULL;
	if (m_PDescription)
	{
		 szDescription = (*m_PDescription)();
		 if (szDescription != NULL)
		 {
			MyStrToken(m_szPluginName, szDescription, 1, ',');
			MyStrToken(m_szPluginAuthor, szDescription, 2, ',');
			MyStrToken(m_szPluginDate, szDescription, 3, ',');
			MyStrToken(m_szPluginVersion, szDescription, 4, ',');
			MyStrToken(m_szPluginFileName, szDescription, 5, ',');
		 }
		 return szDescription;
	}

	else 
		return "No Plugin loaded";
}


//
// Init the DSMPlugin system
//
bool CDSMPlugin::InitPlugin(void)
{
	// TODO: Log events
	int nRes = (*m_PStartup)();
	if (nRes < 0) return false;
	else return true;
}

//
// Reset the DSMPlugin
//
bool CDSMPlugin::ResetPlugin(void)
{
	// TODO: Log events
	int nRes = (*m_PReset)();
	if (nRes < 0) return false;
	else return true;
}


//
// szParams is the key (or password) that is transmitted to the loaded DSMplugin
//
bool CDSMPlugin::SetPluginParams(HWND hWnd, char* szParams)
{
	// TODO: Log events
	int nRes = (*m_PSetParams)(hWnd, szParams);
	if (nRes > 0) return true; else return false;

}


//
// Return the loaded DSMplugin current param(s)
//
char* CDSMPlugin::GetPluginParams(void)
{
	// 
	return (*m_PGetParams)();

}


//
// List all the plugins is the current APP directory in the given ComboBox
//
int CDSMPlugin::ListPlugins(HWND hComboBox)
{
	// TODO: Log events
	WIN32_FIND_DATA fd;
	HANDLE ff;
	int fRet = 1;
	int nFiles = 0;
	char szCurrentDir[MAX_PATH];

	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		if (p == NULL)
			return 0;
		*p = '\0';
	}
	else
		return 0;
	// MessageBox(NULL, szCurrentDir, "Current directory", MB_OK);

    if (szCurrentDir[strlen(szCurrentDir) - 1] != '\\') strcat(szCurrentDir, "\\");
	strcat(szCurrentDir, "*.dsm"); // The DSMplugin dlls must have this extension
	
	ff = FindFirstFile(szCurrentDir, &fd);
	if (ff == INVALID_HANDLE_VALUE)
	{
		// Todo: Log error here
		return 0;
	}

	while (fRet != 0)
	{
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)(fd.cFileName)); 
		nFiles++;
		fRet = FindNextFile(ff, &fd);
	}

	FindClose(ff);

	return nFiles;
}



//
// Load the given DSMplugin and map its functions
//
bool CDSMPlugin::LoadPlugin(char* szPlugin, bool fAllowMulti)
{
	// sf@2003 - Multi dll trick 
	// I really don't like doing this kind of dirty workaround but I have no time to do
	// better for now. Used only by a listening viewer.
	// Create a numbered temporary copy of the original plugin dll and load it (viewer only, for now)
	if (fAllowMulti)
	{
		bool fDllCopyCreated = false;
		int i = 1;
		char szDllCopyName[MAX_PATH];
		char szCurrentDir[MAX_PATH];
		char szCurrentDir_szPlugin[MAX_PATH];
		char szCurrentDir_szDllCopyName[MAX_PATH];
		while (!fDllCopyCreated)
		{
			strcpy(szDllCopyName, szPlugin);
			szDllCopyName[strlen(szPlugin) - 4] = '\0'; //remove the ".dsm" extension
			sprintf(szDllCopyName, "%s-tmp.d%d", szDllCopyName, i++);
			fDllCopyCreated = (FALSE != CopyFile(szPlugin, szDllCopyName, false));
			// Note: Let's be really dirty; Overwrite if it's possible only (dll not loaded). 
			// This way if for some reason (abnormal process termination) the dll wasn't previously 
			// normally deleted we overwrite/clean it with the new one at the same time.
			strcpy (szCurrentDir_szDllCopyName,szDllCopyName);
			DWORD error=GetLastError();
			if (error==2)
			{
				if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
				{
					char* p = strrchr(szCurrentDir, '\\');
					*p = '\0';
				}
				strcpy (szCurrentDir_szPlugin,szCurrentDir);
				strcat (szCurrentDir_szPlugin,"\\");
				strcat (szCurrentDir_szPlugin,szPlugin);

				strcpy (szCurrentDir_szDllCopyName,szCurrentDir);
				strcat (szCurrentDir_szDllCopyName,"\\");
				strcat (szCurrentDir_szDllCopyName,szDllCopyName);
				fDllCopyCreated = (FALSE != CopyFile(szCurrentDir_szPlugin, szCurrentDir_szDllCopyName, false));
			}
			if (i > 99) break; // Just in case...
		}
		strcpy(m_szDllName, szDllCopyName);
		m_hPDll = LoadLibrary(m_szDllName);
	}
	else // Use the original plugin dll
	{
		ZeroMemory(m_szDllName, strlen(m_szDllName));
		m_hPDll = LoadLibrary(szPlugin);
		//Try current PATH
		if (m_hPDll==NULL)
		{
			char szCurrentDir[MAX_PATH];
			char szCurrentDir_szPlugin[MAX_PATH];
			if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
				{
					char* p = strrchr(szCurrentDir, '\\');
					*p = '\0';
				}
			strcpy (szCurrentDir_szPlugin,szCurrentDir);
			strcat (szCurrentDir_szPlugin,"\\");
			strcat (szCurrentDir_szPlugin,szPlugin);
			m_hPDll = LoadLibrary(szCurrentDir_szPlugin);
		}
	}

	if (m_hPDll == NULL) return false;

	m_PDescription     = (DESCRIPTION)     GetProcAddress(m_hPDll, "Description");
	m_PStartup         = (STARTUP)         GetProcAddress(m_hPDll, "Startup");
	m_PShutdown        = (SHUTDOWN)        GetProcAddress(m_hPDll, "Shutdown");
	m_PSetParams       = (SETPARAMS)       GetProcAddress(m_hPDll, "SetParams");
	m_PGetParams       = (GETPARAMS)       GetProcAddress(m_hPDll, "GetParams");
	m_PTransformBuffer = (TRANSFORMBUFFER) GetProcAddress(m_hPDll, "TransformBuffer");
	m_PRestoreBuffer   = (RESTOREBUFFER)   GetProcAddress(m_hPDll, "RestoreBuffer");
	m_PFreeBuffer      = (FREEBUFFER)      GetProcAddress(m_hPDll, "FreeBuffer");
	m_PReset           = (RESET)           GetProcAddress(m_hPDll, "Reset");

	if (m_PStartup == NULL || m_PShutdown == NULL || m_PSetParams == NULL || m_PGetParams == NULL
		|| m_PTransformBuffer == NULL || m_PRestoreBuffer == NULL || m_PFreeBuffer == NULL)
	{
		FreeLibrary(m_hPDll); 
		if (*m_szDllName) DeleteFile(m_szDllName);
		return false;
	}

	// return ((*m_PStartup)());
	SetLoaded(true);
	return true;
}


//
// Unload the current DSMPlugin from memory
//
bool CDSMPlugin::UnloadPlugin(void)
{
	// TODO: Log events
	// Force the DSMplugin to free the buffers it allocated
	if (m_pTransBuffer != NULL) (*m_PFreeBuffer)(m_pTransBuffer);
	if (m_pRestBuffer != NULL) (*m_PFreeBuffer)(m_pRestBuffer);
	
	m_pTransBuffer = NULL;
	m_pRestBuffer = NULL;

	SetLoaded(false);

	if ((*m_PShutdown)())
	{
		bool fFreed = false;
		fFreed = (FALSE != FreeLibrary(m_hPDll));
		if (*m_szDllName) DeleteFile(m_szDllName);
		return fFreed;
	}
	else
		return false;
	
}


//
// Tell the plugin to do its transformation on the source data buffer
// Return: pointer on the new transformed buffer (allocated by the plugin)
// nTransformedDataLen is the number of bytes contained in the transformed buffer
//
BYTE* CDSMPlugin::TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen)
{
	m_pTransBuffer = (*m_PTransformBuffer)(pDataBuffer, nDataLen, pnTransformedDataLen);
	return m_pTransBuffer;
}


// - If pRestoredDataBuffer = NULL, the plugin check its local buffer and return the pointer
// - Otherwise, restore data contained in its rest. buffer and put the result in pRestoredDataBuffer
//   pnRestoredDataLen is the number bytes put in pRestoredDataBuffers
BYTE* CDSMPlugin::RestoreBufferStep1(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
	//m_RestMutex.lock();
	m_pRestBuffer = (*m_PRestoreBuffer)(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	return m_pRestBuffer;
}

BYTE* CDSMPlugin::RestoreBufferStep2(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
	m_pRestBuffer = (*m_PRestoreBuffer)(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	//m_RestMutex.unlock();
	return NULL;
}

void CDSMPlugin::RestoreBufferUnlock()
{
	//m_RestMutex.unlock();
}

BOOL m_fDSMPluginEnabled;
char m_szDSMPlugin[128];
extern CDSMPlugin *m_pDSMPlugin;

// sf@2002 - v1.1.x - DSM Plugin
//
BOOL IsDSMPluginEnabled()
{
	return m_fDSMPluginEnabled;
}

void EnableDSMPlugin(BOOL fEnable)
{
	m_fDSMPluginEnabled = fEnable;
}

char* GetDSMPluginName()
{
	return m_szDSMPlugin;
}


void SetDSMPluginName(char* szDSMPlugin)
{
	strcpy(m_szDSMPlugin, szDSMPlugin);
	return;
}

//
// Load if necessary and initialize the current DSMPlugin
// This can only be done if NO client is connected (for the moment)
//
BOOL SetDSMPlugin(BOOL bForceReload)
{
	if (!IsDSMPluginEnabled()) return FALSE;

	// If the plugin is loaded, unload it first
	// sf@2003 - it has been possibly pre-configured by
	// clicking on the plugin config button
	if (m_pDSMPlugin->IsLoaded())
	{

		// sf@2003 - We check if the loaded plugin is the same than
		// the currently selected one or not
		m_pDSMPlugin->DescribePlugin();
		if (_stricmp(m_pDSMPlugin->GetPluginFileName(), GetDSMPluginName()) || bForceReload)
		{
			m_pDSMPlugin->SetEnabled(false);
			m_pDSMPlugin->UnloadPlugin();
		}
	}

	if (!m_pDSMPlugin->IsLoaded())
	{
		if (!m_pDSMPlugin->LoadPlugin(GetDSMPluginName(), false))
		{
			return FALSE;
		}
	}


	// Now that it is loaded, init it
	if (m_pDSMPlugin->InitPlugin())
	{
		char szParams[MAXPWLEN + 64];
		// Does the plugin need the VNC password to do its job ?
		if (!_stricmp(m_pDSMPlugin->GetPluginParams(), "VNCPasswordNeeded"))
			strcpy(szParams, vncDecryptPasswd((char *)passwd));
		else
			strcpy(szParams, "NoPassword");

		// The second parameter tells the plugin the kind of program is using it
		// (in WinVNC : "server-app" or "server-svc"
		strcat(szParams, ",");
		strcat(szParams, "server-app");

		if (m_pDSMPlugin->SetPluginParams(NULL, szParams/*vncDecryptPasswd((char *)password)*/))
		{
			m_pDSMPlugin->SetEnabled(true); // The plugin is ready to be used
			return TRUE;
		}
		else
		{
			m_pDSMPlugin->SetEnabled(false);
		}
	}
	else
	{
		m_pDSMPlugin->SetEnabled(false);
	}

	return TRUE;
}