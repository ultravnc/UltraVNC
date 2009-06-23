//  Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//
//  Created by Sean E. Covel based on UltraVNC's excellent TestPlugin project.
//
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://home.comcast.net/~msrc4plugin
// or
// mail: msrc4plugin@comcast.net
//
//
//
/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2005 Ultr@VNC All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/
//
////////////////////////////////////////////////////////////////////////////
// IF YOU USE THIS GPL SOURCE CODE TO MAKE YOUR DSM PLUGIN, PLEASE ADD YOUR
// COPYRIGHT TO THIS FILE AND SHORTLY DESCRIBE/EXPLAIN THE MODIFICATIONS
// YOU'VE MADE. THANKS.
//
// IF YOU DON'T USE THIS CODE AS A BASE FOR YOUR PLUGIN, THE HEADER ABOVE AND
// ULTR@VNC COPYRIGHT SHOULDN'T BE FOUND IN YOUR PLUGIN SOURCE CODE.
//

//
// Mar 22 2009 Update
// The #ifdef _WITH_LOG test and fix for Vista and Win7 versions was done 
// by pgm of Advantig Corporation
//

//#include "stdafx.h"
#define _WIN32_WINNT 0x0410     //must be defined for the crypto api
#define _WIN32_WINDOWS 0x0410
#define WINVER 0x0400

#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <tchar.h>

#include <wincrypt.h>       //windows crypto api

#ifdef _WITH_REGISTRY // PGM registry not used in 1.2.4
	#include "REGISTRY.h"
#endif  

#include "MSRC4Plugin.h"
#include "crypto.h"
#include "utils.h"
#include "version.h"

#ifdef _WITH_LOG // PGM ifdef'ed all logging out as any logging can cause crash trying to access log file in program files folder on Vista and Win7 so log not used in 1.4.0.  Might move log to temp or public folder later if needed. Do same with registry access #ifdef _WITH_REGISTRY
	static const TCHAR * INDEXVAL_LOG = _T("DebugLog");
#endif 
static const TCHAR * INDEXVAL_KEYFILE = _T("KeyFile");

HINSTANCE hInstance = 0;
PLUGINSTRUCT* pPlugin = NULL;  // struct (or class instance) that handles all Plugin params.

char * sDefaultGenKey = "new_rc4.key";
char* sVariable = "msrc4pluginkey";

#ifdef _WITH_LOG  
	#define  LOG_FILE  "msrc4.log"
#endif  

//Salt flags
bool bSaltE = true;	//still need Salt/IV for Decryption
bool bSaltD = true; //still need Salt/IV for Encryption
bool bTransHeader = true;  //consider Salt/IV length in the calculation

int HeaderLen = SALT_SIZE + IV_SIZE; //length of additional header

// Plugin Description
// Please use the following format (with ',' (comma) as separator)
// Name,Version,Date,Author,FileName
// For the version, we recommend the following format: x.y.z
// The other fields (Name, Date, Author, FileName) are format free (don't use ',' in them, of course)

#ifdef _WITH_REGISTRY
#define PLUGIN_FILE "MSRC4Plugin.dsm"
#define PLUGIN_DESCRIPTION  "MS RC4 Plugin,Sean E. Covel,Mar 12 2005," BUILD_NUMBER ",MSRC4Plugin.dsm"
#else
#define PLUGIN_FILE "MSRC4Plugin140.dsm"
#define PLUGIN_DESCRIPTION  "MS RC4 Encryption Plug-in, Sean E. Covel, Updated Mar 22 2009," BUILD_NUMBER ",MSRC4Plugin140.dsm"
#endif

char getParams[BUFFER_SIZE];

// ----------------------------------------------------------------------
//
// A VNC DSM Plugin MUST export (extern "C" (__cdecl)) all the following
// functions (same names, same signatures)
//
// For return values, the rule is:
//    < 0, 0, and NULL pointer mean Error
//    > 0 and pointer != NULL mean Ok
//
// ----------------------------------------------------------------------
//

// Returns the ID string of the Plugin
//
PLUGIN_API char* Description(void)
{
    //PrintLog((DEST,"Plugin Description. %s",PLUGIN_DESCRIPTION));
    return PLUGIN_DESCRIPTION;
}


//
// Initialize the plugin and all its internals
// Return -1 if error
//

PLUGIN_API int Startup(void)
{
	char output[OUTPUT_BUFFER_SIZE];
    HANDLE      hKeyFile = 0;

#ifdef _WITH_REGISTRY
	char defaultKeyFile[BufSize];
	char defaultGenFile[BufSize];
	char sProgramFiles[KEYFILENAME_SIZE];
	char sLogit[BufSize];

		//get the local version of 'Program Files'
	GetEnvVar(PROGRAMFILES, sProgramFiles, BufSize);

		//add 'UltraVNC' to it.
	if (_snprintf(defaultKeyFile, sizeof(defaultKeyFile),"%s%s%s",sProgramFiles,sDefault,sDefaultKeyName) < 0)
		PrintLog((DEST,"_snprintf failed - defaultKeyFile too small"));
	if (_snprintf(defaultGenFile, sizeof(defaultGenFile),"%s%s%s",sProgramFiles,sDefault,sDefaultGenKey) < 0)
		PrintLog((DEST,"_snprintf failed - defaultGenFile too small"));

    char keyFile[KEYFILENAME_SIZE];

	//load some default values into the registry if the keys don't exist.
    REGISTRY *m_pREGISTRY;

    //HKLM Key
	m_pREGISTRY = new REGISTRY(HKEY_LOCAL_MACHINE, MSRC4_KEY_NAME_SERVER, true);
    m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, defaultKeyFile);	//key file
    m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);							//debuglog
    m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYGEN, defaultGenFile);	//GenKey
    delete m_pREGISTRY;
    
	//HKCU/Server
    m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_SERVER, true);
    m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, defaultKeyFile);	//key file
    m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);							//debuglog
    m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYGEN, defaultGenFile);	//GenKey
    delete m_pREGISTRY;
	
	//HKCU/Viewer
    m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_VIEWER, true);
    m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, defaultKeyFile);	//key file
    m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);							//debuglog
    m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYGEN, defaultGenFile);	//GenKey
    delete m_pREGISTRY;

#ifdef _WITH_LOG  
	LOGIT = SetLogging(LOG_FILE, sLogit);
#else
	//set Logging based on an environment variable
	LOGIT = SetLogging(LOG_FILE);
#endif

    PrintLog((DEST,"StartUp"));
	//get some important info into the logs
#ifdef _WITH_REGISTRY
    PrintLog((DEST,"REGISTRY %s",PLUGIN_DESCRIPTION));
#else
    PrintLog((DEST,"NOREG %s",PLUGIN_DESCRIPTION));
#endif

#endif  

    // Init everything
    memset(szExternalKey, 0, sizeof(szExternalKey));
    // Create threads if any

	if (FindKey(PLUGIN_FILE, sDefaultKeyName, sVariable, &hKeyFile, output))
		strcpy(getParams,"NothingNeeded");
	else
		strcpy(getParams,"VNCPasswordNeeded");

    return 1;
}


//
// Stop and Clean up the plugin 
// Return -1 if error
// 
PLUGIN_API int Shutdown(void)
{
    // Terminate Threads if any
    // Cleanup everything
	// Clean up: release handles, close files.
	
    //destroy the keys
    if (hKey) {
		CryptDestroyKey(hKey);
		CryptDestroyKey(hKey2);
    }
	
    if (hExchangeKey) {
		CryptDestroyKey(hExchangeKey);
		CryptDestroyKey(hExchangeKey2);
    }
	
	//We’re finished using the CSP handle, so we must release it.
	if (hProvider) {
		CryptReleaseContext(hProvider, 0);
		CryptReleaseContext(hProvider2, 0);
	}

#ifdef _WITH_LOG  
	PrintLog((DEST,"Shutting Down.\r\n"));
#endif 

	return 1;
}


//
// Stop and Clean up the plugin 
// Return -1 if error
// 
PLUGIN_API int Reset(void)
{
int rc = 0;
	char output[OUTPUT_BUFFER_SIZE];
    HANDLE      hKeyFile = 0;

	//reset the salt flag so the password gets re-salted for the new session
	bSaltD = true;
	bSaltE = true;

	rc = ResetCrypto(hKey);
	rc = ResetCrypto(hKey2);

#ifdef _WITH_LOG  
	PrintLog((DEST,"Reset Plugin."));
#endif 
	if (FindKey(PLUGIN_FILE, sDefaultKeyName, sVariable, &hKeyFile, output))
		strcpy(getParams,"NothingNeeded");
	else
		strcpy(getParams,"VNCPasswordNeeded");

	CloseHandle(hKeyFile);


return 1;
}


//
// Set the plugin params (Key or password )
// If several params are needed, they can be transmitted separated with ',' (comma)
// then translated if necessary. They also can be taken from the internal Plugin config
// 
// WARNING: The plugin is responsible for implementing necessary GUI or File/Registry reading
// to acquire additionnal parameters and to ensure their persistence if necessary.
// Same thing for events/errors logging.
// 
// This function can be called 2 times, both from vncviewer and WinVNC:
// 
// 1.If the user clicks on the Plugin's "config" button in vncviewer and WinVNC dialog boxes
//   In this case this function is called with hVNC != 0 (CASE 1)
//
//   -> szParams is a string formatted as follow: "Part1,Part2"
//   Part1 = "NoPassword"
//   Part2 = type of application that has loaded the plugin
//     "viewer"     : for vncviewer
//     "server-svc" : for WinVNC run as a service
//     "server-app" : for WINVNC run as an application
//
//   -> The Plugin Config dialog box is displayed if any.
// 
// 2.When then plugin is Inited from VNC viewer or Server, right after Startup() call (CASE 2);
//   In this case, this function is called with hVNC = 0 and
//   szParams is a string formatted as follows: "part1,Part2"
//   Part1 = The VNC password, if required by the GetParams() function return value
//   Part2 = type of application that has loaded the plugin
//      "viewer"     : for vncviewer
//      "server-svc" : for WinVNC run as a service
//      "server-app" : for WINVNC run as an application
//   (this info can be used for application/environnement dependent
//    operations (config saving...))
//   
// 
PLUGIN_API int SetParams(HWND hVNC, char* szParams)
{
#ifdef _WITH_REGISTRY
    REGISTRY *m_pREGISTRY;
	int rc = 0;
#else
//	char sEnvVar[BufSize];
	DWORD rc = 0;
#endif

	char sLogit[BufSize];
	char output[OUTPUT_BUFFER_SIZE];
    HANDLE      hKeyFile = 0;
	//BYTE *		pbBuff;

	//BYTE		*pbBuffer = new BYTE[OUT_BUFFER_SIZE];
    //char        keyFile[KEYFILENAME_SIZE];
    PLUGINSTRUCT strPlugin;
	long iWinVer=0;
	long iCryptVer=0;
//	char *stop;
	char		sProgramFiles[KEYFILENAME_SIZE];
	char  CSPName[70];

	strPlugin.szHKLMServer[0] = '\0';
	strPlugin.szHKCUViewer[0] = '\0';
	strPlugin.szHKCUServer[0] = '\0';
	strPlugin.szHKCUGenKey[0] = '\0';

#ifdef _WITH_LOG  
	//setup logging
#ifdef _WITH_REGISTRY
    m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_VIEWER, false);
    m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);
    delete m_pREGISTRY;
	LOGIT = SetLogging(LOG_FILE, sLogit);
#else
	LOGIT = SetLogging(LOG_FILE);
#endif

//LOGIT = strtol( sLogit, &stop, 10 );

PrintLog((DEST,"Set Params"));

	//get some important info into the logs
#ifdef _WITH_REGISTRY
    PrintLog((DEST,"REGISTRY %s",PLUGIN_DESCRIPTION));
#else
    PrintLog((DEST,"NOREG %s",PLUGIN_DESCRIPTION));
#endif
#endif  

	//figure out what OS, provider, CSP, yada yada, we are running on currently
	InitVars(CSPName, &iWinVer, &iCryptVer, &maxKeyLen);

#ifdef _WITH_LOG  
	PrintLog((DEST,"Crypto Version = %d",iCryptVer));
	PrintLog((DEST,"OS is '%s'",WindowsName[iWinVer]));
	PrintLog((DEST,"Using provider '%s'",CSPName));
	PrintLog((DEST,"Max Key Length %d",(DWORD)(HIWORD(maxKeyLen))));
#endif  

    // Get the environnement (szLoaderType) value that is always sent from 
    // VNC viewer or server
    MyStrToken(szLoaderType, szParams, 2, ',');

	GetEnvVar(PROGRAMFILES, sProgramFiles, BufSize);
#ifdef _WITH_LOG  
	if (strlen(sProgramFiles)== 0)
		PrintLog((DEST,"ProgramFiles not found"));
#endif     

// ***CASE 1 - CONFIG***
    // If hVNC != 0, display for instance the Plugin Config Dialog box 
    if (hVNC)
    {
#ifdef _WITH_LOG  
        PrintLog((DEST,"SetParams. - Config."));
#endif 
        
#ifdef _WITH_REGISTRY

		char defaultKeyFile[BufSize];
		char defaultGenFile[BufSize];

			//add 'UltraVNC' to it.
		if (_snprintf(defaultKeyFile, sizeof(defaultKeyFile),"%s%s%s",sProgramFiles,sDefault,sDefaultKeyName) < 0)
			PrintLog((DEST,"_snprintf failed - defaultKeyFile too small"));
		if (_snprintf(defaultGenFile, sizeof(defaultGenFile),"%s%s%s",sProgramFiles,sDefault,sDefaultGenKey) < 0)
			PrintLog((DEST,"_snprintf failed - defaultGenFile too small"));

		//load some default values into the registry if the keys don't exist.
		REGISTRY *m_pREGISTRY;

		//HKLM Key
		m_pREGISTRY = new REGISTRY(HKEY_LOCAL_MACHINE, MSRC4_KEY_NAME_SERVER, true);
		m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, defaultKeyFile);	//key file
        strcpy(strPlugin.szHKLMServer,keyFile);
		m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);							//debuglog
		m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYGEN, defaultGenFile);	//GenKey
		delete m_pREGISTRY;
    
		//HKCU/Server
		m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_SERVER, true);
		m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, defaultKeyFile);	//key file
        strcpy(strPlugin.szHKCUServer,keyFile);
		m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);							//debuglog
		m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYGEN, defaultGenFile);	//GenKey
		delete m_pREGISTRY;
		
		//HKCU/Viewer
		m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_VIEWER, true);
		m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, defaultKeyFile);	//key file
        strcpy(strPlugin.szHKCUViewer,keyFile);
		m_pREGISTRY->ReadItem(sLogit, 2,INDEXVAL_LOG, sLogItDef);							//debuglog
		m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYGEN, defaultGenFile);	//GenKey
        strcpy(strPlugin.szHKCUGenKey,keyFile);
		delete m_pREGISTRY;

#else
        strcpy(strPlugin.szHKLMServer,sDefaultKeyName);
        strcpy(strPlugin.szHKCUViewer,sDefaultKeyName);
        strcpy(strPlugin.szHKCUServer,sDefaultKeyName);
        strcpy(strPlugin.szHKCUGenKey,sDefaultGenKey);
		strcpy(sLogit, sLogItDef);
#endif

        pPlugin = &strPlugin;       
        
        // Display the Plugin Config dialog box
        DoDialog();
    } 
    else 
    {
// ***CASE 2: - INITIALIZE PLUGIN***

#ifdef _WITH_LOG  
        PrintLog((DEST,"SetParams - StartUp."));
#endif 
        
//#ifdef _WITH_REGISTRY
        // Use szParams to setup the Plugin.
        // (it corresponds to the VNC password as we require it in the GetParams() function below)
        MyStrToken(szExternalKey, szParams, 1, ',');
        
        // Use szParams to setup the Plugin.
        MyStrToken(szExternalKey, szParams, 1, ',');
        
        // The second parameter is the type of application that has loaded the plugin
        // "viewer"     : for vncviewer
        // "server-svc" : for WinVNC run as a service
        // "server-app" : for WINVNC run as an application
        // this info can be used for application/environnement dependent operations (config saving...)
        MyStrToken(szLoaderType, szParams, 2, ',');
//#endif
        
        // Odditional params may be added later if necessary
        if (strlen(szLoaderType) > 0) 
		{
#ifdef _WITH_REGISTRY
            //szLoaderType determines key hive to open...
            if (strcmp(szLoaderType, "server-svc") == 0) {
                m_pREGISTRY = new REGISTRY(HKEY_LOCAL_MACHINE, MSRC4_KEY_NAME_SERVER, false);
                PrintLog((DEST,"***** SERVER -> SERVICE *****"));
            }else
            if (strcmp(szLoaderType, "server-app") == 0) {
                m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_SERVER, false);
                PrintLog((DEST,"***** SERVER -> Application *****"));
            }else
            if (strcmp(szLoaderType, "viewer") == 0) {
                m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_VIEWER, false);
                PrintLog((DEST,"***** VIEWER Application *****"));
            }
            else {
                PrintLog((DEST,"***** Bad LoaderType: %s *****", szLoaderType));
            }
#else
#ifdef _WITH_LOG  
            if (strcmp(szLoaderType, "server-svc") == 0) {
                PrintLog((DEST,"***** SERVER -> SERVICE *****")); }
            else {
				if (strcmp(szLoaderType, "server-app") == 0)  {
					PrintLog((DEST,"***** SERVER -> Application *****"));}
				else {
					if (strcmp(szLoaderType, "viewer") == 0) {
						PrintLog((DEST,"***** VIEWER Application *****")); }
					else {
						PrintLog((DEST,"***** Bad LoaderType: %s *****", szLoaderType)); }
				}
			}
#endif  
#endif
        }
        
#ifdef _WITH_REGISTRY
		PrintLog((DEST,"Trying to find the key file"));
		PrintLog((DEST,"Looking at the registry"));

        m_pREGISTRY->ReadItem(keyFile, KEYFILENAME_SIZE,INDEXVAL_KEYFILE, NULL);
        delete m_pREGISTRY;

		PrintLog((DEST,"Looking for %s", keyFile));
		//open the key file
        hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
#ifdef _WITH_LOG  
		PrintLog((DEST,"Trying to find the key file"));
#endif 
		//default to "didn't work"
		hKeyFile = INVALID_HANDLE_VALUE;

		FindKey(PLUGIN_FILE, sDefaultKeyName, sVariable, &hKeyFile, output);

#endif
        
        
		rc = PrepContext(iWinVer, &hProvider);
		rc = PrepContext(iWinVer, &hProvider2);

		if (hKeyFile == INVALID_HANDLE_VALUE) {
			//PrintLog((DEST,"Key Not Found at %s",keyFile));

#ifdef _WITH_LOG  
			PrintLog((DEST,"No key Found, using password."));
#endif 

			rc = CreateDerivedCryptKey(hProvider, &hKey, szExternalKey);
			rc = CreateDerivedCryptKey(hProvider, &hKey2, szExternalKey);

		}
        else
		{
			//PrintLog((DEST,"Found key at %s",keyFile));

			ImportCryptKey(hProvider, &hKey, hKeyFile);

			rc = SetFilePointer(hKeyFile, 0, NULL, FILE_BEGIN);

			ImportCryptKey(hProvider2, &hKey2, hKeyFile);

			CloseHandle(hKeyFile);
#ifdef _WITH_LOG  
            PrintLog((DEST,"Key File Read."));		
#endif 
		}

		CleanupCryptoKey(hExchangeKey);
        CleanupCryptoKey(hExchangeKey2);
        hExchangeKey = 0;
        hExchangeKey2 = 0;

		//reset the salt flag so the password gets re-salted for the new session
		bSaltD = true;
		bSaltE = true;
        

    }
    return 1;
}

//
// Return the current plugin params
// As the plugin is basically a blackbox, VNC doesn't need to know 
// the Plugin parameters. Should not often be used...
//
PLUGIN_API char* GetParams(void)
{
#ifdef _WITH_LOG  
    PrintLog((DEST,"GetParams. %s",getParams ));
#endif 
	if (strlen(szExternalKey) > 0)
		return szExternalKey; // Return the already stored externalkey params
	else
		return getParams;
}


// 
// TransformBuffer function
//
// Transform the data given in pDataBuffer then return the pointer on the allocated 
// buffer containing the resulting data.
// The length of the resulting data is given by pnTransformedDataLen
//	
PLUGIN_API BYTE* TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen)
{
    DWORD       dwByteCount;
	unsigned char *	salt = 0;
	unsigned char *	iv = 0;
	int headerLen = 0;

	bTransHeader = bSaltE;

    BYTE* pTransBuffer = CheckLocalTransBufferSize(GiveTransDataLen(nDataLen));
    if (pTransBuffer == NULL)
    {
        *pnTransformedDataLen = -1;
        return NULL;
    }

	if (bSaltE)
	{
		//If the SALT/IV for the session hasn't been setup yet, set it up.
#ifdef _WITH_LOG  
        PrintLog((DEST,"Salting E key"));
#endif  

		__try
		{
		//create the session key using the salt
		salt = new unsigned char[SALT_SIZE+1];
		iv = new unsigned char[IV_SIZE+1];
		memset(salt,0,SALT_SIZE);
		memset(iv,0,IV_SIZE);

		//generate some random data for the SALT
		CryptGenRandom(hProvider, SALT_SIZE, salt);
		CryptGenRandom(hProvider, IV_SIZE, iv);

//		memset(salt,1,SALT_SIZE);
//		memset(iv,1,IV_SIZE);

		//SALT the key
		//if(!CryptSetKeyParam(hKey, KP_SALT, salt, 0)) {
		//	PrintLog((DEST,"Error %x during CryptSetKeyParam!\n", GetLastError()));
		//	//return NULL;
		//}

#ifdef _WITH_LOG  
		if(!CryptSetKeyParam(hKey2, KP_IV, iv, 0)) {
			PrintLog((DEST,"Error %x during CryptSetKeyParam!\n", GetLastError()));
			//return NULL;
		}
#endif

		//copy the salt into the beginning of the buffer
		memcpy(pTransBuffer, salt, SALT_SIZE);
		memcpy(pTransBuffer+SALT_SIZE, iv, IV_SIZE);

		//PrintLog((DEST,"Password: %16.16s",szExternalKey));
#ifdef _WITH_LOG  
		PrintLog((DEST,"SALTE:%*.*s",SALT_SIZE,SALT_SIZE,salt));
#endif 
		}
		__finally{
		delete[] salt;
		delete[] iv;
		}

		headerLen = HeaderLen;	//add header length
	}
	
    memcpy(pTransBuffer+headerLen, pDataBuffer, nDataLen);
	
    dwByteCount = nDataLen;
	
    //Call CryptEncrypt with the key and the buffer, it transforms the buffer inplace
    //Since I'm using a STREAM encryption, the size does not change.
    if (! CryptEncrypt(hKey, 0, false, 0, pTransBuffer+headerLen, &dwByteCount, nDataLen)) {
#ifdef _WITH_LOG  
		PrintLog((DEST,"CryptEncrypt failed."));
#endif  
		return NULL;
	}
	
	//It SHOULD not change...
    if (dwByteCount != (DWORD)nDataLen)  {
#ifdef _WITH_LOG  
        PrintLog((DEST,"TransformBuffer Assertion failed."));
#endif 
        return NULL;
    }
	
    // return the transformed data length
    *pnTransformedDataLen = GiveTransDataLen(nDataLen);

	//don't need additional salt, turn it off.
	bSaltE = bTransHeader = false;
	
    return pTransBuffer; // Actually, pTransBuffer = pLocalTransBuffer
}


//
// RestoreBuffer function
//
// This function has a 2 mandatory behaviors:
//
// 1. If pRestoredDataBuffer is NULL, the function must return the pointer to current
//    LocalRestBuffer that is going to receive the Transformed data to restore
//    from VNC viewer/server's socket.
//    This buffer must be of the size of transformed data, calculated from nDataLen
//    and this size must be given back in pnRestoredDataLen.
//
// 2. If pRestoredDataBuffer != NULL, it is the destination buffer that is going to receive
//    the restored data. So the function must restore the data that is currently in the
//    local pLocalRestBuffer (nDataLen long) and put the result in pRestoredDataBuffer.
//    The length of the resulting data is given back in pnTransformedDataLen
//
// Explanation: Actually, when VNC viewer/server wants to restore some data, it does the following:
// - Calls RestoreBuffer with NULL to get the buffer (and its length) to store incoming transformed data
// - Reads incoming transformed data from socket directly into the buffer given (and of given length)
// - Calls RestoreBuffer again to actually restore data into the given destination buffer.
// This way the copies of data buffers are reduced to the minimum.
// 
PLUGIN_API BYTE* RestoreBuffer(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
    DWORD         dwByteCount;
	unsigned char *	salt = 0;
	unsigned char *	iv = 0;
	int headerLen = 0;
	
	
    // If given buffer is NULL, allocate necessary space here and return the pointer.
    // Additinaly, calculate the resulting length based on nDataLen and return it at the same time.
    if (pRestoredDataBuffer == NULL)
    {
		bTransHeader = bSaltD;
		// Give the size of the transformed data buffer, based on the original data length
        *pnRestoredDataLen = GiveTransDataLen(nDataLen);
		
        // Ensure the pLocalRestBuffer that receive transformed data is big enough
        BYTE* pBuffer = CheckLocalRestBufferSize(*pnRestoredDataLen);
        return pBuffer; // Actually pBuffer = pLocalRestBuffer
    }
	

    if (bSaltD)
	{
		__try
		{
		//If we haven't setup the key with the Salt, then get the salt off the buffer and setup the key
		salt = new unsigned char[SALT_SIZE+1];
		iv = new unsigned char[IV_SIZE+1];
#ifdef _WITH_LOG  
        PrintLog((DEST,"Salting D key"));
#endif  

		//Get the salt off the beginning of the buffer
		memcpy(salt,pLocalRestBuffer,SALT_SIZE);
		//Get the IV off the buffer
		memcpy(iv,pLocalRestBuffer+SALT_SIZE,IV_SIZE);

		//PrintLog((DEST,"Password: %16.16s",szExternalKey));
#ifdef _WITH_LOG  
		PrintLog((DEST,"SALTD:%*.*s",SALT_SIZE,SALT_SIZE,salt));
#endif 

//		memset(salt,1,SALT_SIZE);
//		memset(iv,1,IV_SIZE);

			//SALT the key
		//if(!CryptSetKeyParam(hKey2, KP_SALT, salt, 0)) {
		//	PrintLog((DEST,"Error %x during CryptSetKeyParam!\n", GetLastError()));
		//	//return NULL;
		//}

					//set the IV
		if(!CryptSetKeyParam(hKey2, KP_IV, iv, 0)) {
#ifdef _WITH_LOG  
			PrintLog((DEST,"Error %x during CryptSetKeyParam!\n", GetLastError()));
#endif 
			return NULL;
		}
		}
		__finally
		{
		delete[] salt;
		delete[] iv;
		}
		headerLen = HeaderLen;	//add header length
	}

    // If we reach this point, pLocalTransBuffer must contain the transformed data to restore
    // Do the actual data restoration/unpadding etc...
    // Copy data into the destination buffer, without modifiying it.
	
    memcpy(pRestoredDataBuffer, pLocalRestBuffer+headerLen, nDataLen-headerLen);
	
    dwByteCount = nDataLen-headerLen;
	
    //Call CryptDecrypt with the key and the buffer.  The buffer will be transformed in place
    if (! CryptDecrypt(hKey2, 0, false, 0, pRestoredDataBuffer, &dwByteCount)) {
#ifdef _WITH_LOG  
		PrintLog((DEST,"CryptDecrypt failed."));
#endif 
		return NULL;
	}

	//The size should not change
    if (dwByteCount != (DWORD)(nDataLen-headerLen)) {
#ifdef _WITH_LOG  
        PrintLog((DEST,"RestoreBuffer Assertion failed."));
#endif 
        return NULL;
    }
	
#ifdef _WITH_LOG  
	if (bTransHeader)
		PrintLog((DEST,"%16.16s",pRestoredDataBuffer));
#endif 

    // return the resulting data length
    *pnRestoredDataLen = GiveRestDataLen(nDataLen);
	
	//turn off salt, we should have it already
	bSaltD = bTransHeader = false;

    return pLocalRestBuffer;
    //return pRestoredDataBuffer;
}


//
// Free the DataBuffer and TransBuffer than have been allocated
// in TransformBuffer and RestoreBuffer, using the method adapted
// to the used allocation method.
//
PLUGIN_API void FreeBuffer(BYTE* pBuffer)
{
    //PrintLog((DEST,"FreeBuffer."));
	
    if (pBuffer != NULL) 
        free(pBuffer);
	
    return;
}


// -----------------------------------------------------------------
// End of functions that must be exported
// -----------------------------------------------------------------



// -----------------------------------------------------------------
//  Plugin internal Config Dialog Box 
// -----------------------------------------------------------------


// Move the given window to the centre of the screen
// and bring it to the top.
void CentreWindow(HWND hwnd)
{
    RECT winrect, workrect;
    
    // Find how large the desktop work area is
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
    int workwidth = workrect.right -  workrect.left;
    int workheight = workrect.bottom - workrect.top;
    
    // And how big the window is
    GetWindowRect(hwnd, &winrect);
    int winwidth = winrect.right - winrect.left;
    int winheight = winrect.bottom - winrect.top;
	
    // Make sure it's not bigger than the work area
    winwidth = min(winwidth, workwidth);
    winheight = min(winheight, workheight);
	
    // Now centre it
    SetWindowPos(hwnd, 
        HWND_TOP,
        workrect.left + (workwidth-winwidth) / 2,
        workrect.top + (workheight-winheight) / 2,
        winwidth, winheight, 
        SWP_SHOWWINDOW);
    SetForegroundWindow(hwnd);
}


//
// Display the Plugin Config Dialog box
//
int DoDialog(void)
{
#ifdef _WITH_LOG  
    PrintLog((DEST,"DoDialog."));
#endif  
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), 
        NULL, (DLGPROC) ConfigDlgProc, (LONG) pPlugin);
}

//Display the plugin Error dialog
int DoError(void)
{
#ifdef _WITH_LOG  
    PrintLog((DEST,"DoError."));
#endif 
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 
        NULL, (DLGPROC) ErrorDlgProc, (LONG) pPlugin);
}


//
// Config Dialog box callback
//
BOOL CALLBACK ConfigDlgProc(HWND hwnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam )
{
    //PLUGINSTRUCT* _this = (PLUGINSTRUCT*) GetWindowLong(hwnd, GWL_USERDATA);

#ifdef _WITH_REGISTRY
    REGISTRY *m_pREGISTRY;
#endif
long iCryptVer=0;
DWORD keyLen = 0;


    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
	char output[OUTPUT_BUFFER_SIZE];
    HANDLE      hKeyFile = 0;
            // Init the various fields with the saved values if they exist.
            CentreWindow(hwnd);
			FindKey(PLUGIN_FILE, sDefaultKeyName, sVariable, &hKeyFile, output);
			CloseHandle(hKeyFile);
            SetDlgItemText(hwnd, IDC_STATUS, output  );

            SetDlgItemText(hwnd, IDC_CLIENT, pPlugin->szHKCUViewer  );
            SetDlgItemText(hwnd, IDC_SERVER_USER, pPlugin->szHKCUServer  );
            SetDlgItemText(hwnd, IDC_KEYLOCATION, pPlugin->szHKCUGenKey  );
            SetDlgItemText(hwnd, IDC_SERVER_SERVICE, pPlugin->szHKLMServer  );


#ifndef _WITH_REGISTRY
			//grey out key filename options for "no registry"
            EnableWindow(GetDlgItem(hwnd, IDC_CLIENT), false);
            EnableWindow(GetDlgItem(hwnd, IDC_SERVER_USER), false);
            EnableWindow(GetDlgItem(hwnd, IDC_KEYLOCATION), true);
            EnableWindow(GetDlgItem(hwnd, IDC_SERVER_SERVICE), false);
#else
            EnableWindow(GetDlgItem(hwnd, IDC_CLIENT), true);
            EnableWindow(GetDlgItem(hwnd, IDC_SERVER_USER), true);
            EnableWindow(GetDlgItem(hwnd, IDC_KEYLOCATION), true);
            EnableWindow(GetDlgItem(hwnd, IDC_SERVER_SERVICE), true);
#endif

			iCryptVer = GetCryptoVersion();
			//Grey out invalid options
			if (iCryptVer == 1) {
				EnableWindow(GetDlgItem(hwnd,IDC_RADIO1),false);
				EnableWindow(GetDlgItem(hwnd,IDC_RADIO2),false);
				EnableWindow(GetDlgItem(hwnd,IDC_RADIO3),false);
			}
			else {
				if (maxKeyLen == KEYLEN_128BIT) {
					CheckDlgButton(hwnd, IDC_RADIO2,BST_CHECKED);
					EnableWindow(GetDlgItem(hwnd,IDC_RADIO2),true);
					EnableWindow(GetDlgItem(hwnd,IDC_RADIO1),true);
					EnableWindow(GetDlgItem(hwnd,IDC_RADIO3),true);
				}
				else {
					if (maxKeyLen == KEYLEN_56BIT) {
						CheckDlgButton(hwnd, IDC_RADIO3,BST_CHECKED);
						EnableWindow(GetDlgItem(hwnd,IDC_RADIO2),false);
						EnableWindow(GetDlgItem(hwnd,IDC_RADIO1),true);
						EnableWindow(GetDlgItem(hwnd,IDC_RADIO3),true);
					}
					else {
						CheckDlgButton(hwnd, IDC_RADIO1,BST_CHECKED);
						EnableWindow(GetDlgItem(hwnd,IDC_RADIO1),true);
						EnableWindow(GetDlgItem(hwnd,IDC_RADIO2),false);
						EnableWindow(GetDlgItem(hwnd,IDC_RADIO3),false);
					}
				}
			}
            return TRUE;
        }
		
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            // Save the parameters in an ini file or registry for instance/
//            GetDlgItemText(hwnd, IDC_CLIENT, pPlugin->szHKCUViewer, KEYFILENAME_SIZE );
//            GetDlgItemText(hwnd, IDC_SERVER_USER, pPlugin->szHKCUServer, KEYFILENAME_SIZE );
//            GetDlgItemText(hwnd, IDC_KEYLOCATION, pPlugin->szHKCUGenKey, KEYFILENAME_SIZE );
//            GetDlgItemText(hwnd, IDC_SERVER_SERVICE, pPlugin->szHKLMServer, KEYFILENAME_SIZE );
			
#ifdef _WITH_REGISTRY
			//HKLM/Server
//            m_pREGISTRY = new REGISTRY(HKEY_LOCAL_MACHINE, MSRC4_KEY_NAME_SERVER, true);
//            m_pREGISTRY->WriteItem(pPlugin->szHKLMServer ,INDEXVAL_KEYFILE);
//            m_pREGISTRY->WriteItem(pPlugin->szHKCUGenKey ,INDEXVAL_KEYGEN);
//            delete m_pREGISTRY;
            
			//HKCU/Server
//            m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_SERVER, true);
//            m_pREGISTRY->WriteItem(pPlugin->szHKCUServer,INDEXVAL_KEYFILE);
//            m_pREGISTRY->WriteItem(pPlugin->szHKCUGenKey ,INDEXVAL_KEYGEN);
//            delete m_pREGISTRY;
			
			//HKCU/Viewer
//            m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_NAME_VIEWER, true);
//            m_pREGISTRY->WriteItem(pPlugin->szHKCUViewer,INDEXVAL_KEYFILE);
//            m_pREGISTRY->WriteItem(pPlugin->szHKCUGenKey ,INDEXVAL_KEYGEN);
//            delete m_pREGISTRY;
			
#endif
			
            EndDialog(hwnd, TRUE);
            return TRUE;
        case IDC_KEYGEN:

			GetDlgItemText(hwnd, IDC_KEYLOCATION, pPlugin->szHKCUGenKey , KEYFILENAME_SIZE );
			if (IsDlgButtonChecked(hwnd, IDC_RADIO2))
				keyLen = KEYLEN_128BIT;
			else {
				if (IsDlgButtonChecked(hwnd, IDC_RADIO3))
					keyLen = KEYLEN_56BIT;
				else 
					keyLen = KEYLEN_40BIT;
			}
			//Button Generate Key
			//Generates a file in c:\program files\ultravnc\new_rc4.key
			//you will need to copy it to the client AND the server as 
			//c:\program files\ultravnc\rc4.key
			
			//save the path in case they changed it!
#ifdef _WITH_REGISTRY
            m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_FILE, false);
            m_pREGISTRY->WriteItem(pPlugin->szHKCUGenKey, INDEXVAL_KEYGEN);
            delete m_pREGISTRY;
#endif
            if (!GenKey(pPlugin->szHKCUGenKey , keyLen))
			{
				strcpy(pPlugin->szDescription2, "An unrecoverable error occurred.");
				strcpy(pPlugin->szDescription,"GenerateKey Failed");
				DoError();
			}
			keyLen = KEYLEN_128BIT;  //default back to 128bit
            EndDialog(hwnd, TRUE);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return TRUE;
        }
        break;
		
		case WM_DESTROY:
			EndDialog(hwnd, FALSE);
			return TRUE;
    }
    return 0;
}


//
// Error Dialog box callback
//
BOOL CALLBACK ErrorDlgProc(HWND hwnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam )
{
//    PLUGINSTRUCT* _this = (PLUGINSTRUCT*) GetWindowLong(hwnd, GWL_USERDATA);
	
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Init the various fields with the saved values if they exist.
            CentreWindow(hwnd);
            SetDlgItemText(hwnd, ID_MSG2, pPlugin->szDescription2  );
            SetDlgItemText(hwnd, IDC_MSG, pPlugin->szDescription  );
            return TRUE;
        }
		
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            //
            EndDialog(hwnd, FALSE);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return TRUE;
        }
        break;
		
		case WM_DESTROY:
			EndDialog(hwnd, FALSE);
			return TRUE;
    }
    return 0;
}



// -----------------------------------------------------------------
// Others internal functions, some depending on what the Plugin does
// -----------------------------------------------------------------

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
// Calculate the len of the data after transformation and return it. 
// 
// MANDATORY: The calculation must be possible by
// ONLY knowing the source data length ! (=> forget compression algos...)
//
// Example:
// For 128bits key encryption, the typical calculation would be;
// Pad the DataBuffer so it is 16 bytes (128 bits) modulo 
//      nPad = (nDataLen % 16 == 0) ? 0 : (16 - (nDataLen % 16));
// Then add a 16 bytes to store the original buffer length (this way it's 
// still 16 bytes modulo) that will be necessary for decryption
//      *pnTransformedDataLen = nDataLen + nPad + 16;

int GiveTransDataLen(int nDataLen)
{
    int nTransDataLen = nDataLen; // STREAM encryption, the datalen remains unchanged

			//if the message had a header, it is SALT, add it in.
	if (bTransHeader)
		nTransDataLen = nDataLen + SALT_SIZE + IV_SIZE;

    return nTransDataLen;
}

//
// Calculate the len of the data after Restauration and return it. 
// 
// MANDATORY: The calculation must be possible by
// ONLY knowing the source data length ! (=> forget compression algos...)
//
int GiveRestDataLen(int nDataLen)
{
    int nRestDataLen = nDataLen; // STREAM encryption, the datalen remains unchanged

			//if the message had a header, it is SALT, add it in.
	if (bSaltD)
		nRestDataLen = nDataLen - (SALT_SIZE + IV_SIZE);

    return nRestDataLen;
}



//
// Allocate more space for the local transformation buffer if necessary
// and returns the pointer to this buffer
//
BYTE* CheckLocalTransBufferSize(int nBufferSize)
{
    if (nLocalTransBufferSize >= nBufferSize) return pLocalTransBuffer;
	
    BYTE *pNewBuffer = (BYTE *) malloc (nBufferSize + 256);
    if (pNewBuffer == NULL) 
    {
        return NULL;
    }
    if (pLocalTransBuffer != NULL)
        free(pLocalTransBuffer);
	
    pLocalTransBuffer = pNewBuffer;
    nLocalTransBufferSize = nBufferSize + 256;
	
    memset(pLocalTransBuffer, 0, nLocalTransBufferSize);
	
    return pLocalTransBuffer;
}


//
// Allocate more space for the local restoration buffer if necessary
// and returns the pointer to this buffer
//
BYTE* CheckLocalRestBufferSize(int nBufferSize)
{
    if (nLocalRestBufferSize >= nBufferSize) return pLocalRestBuffer;
	
    BYTE *pNewBuffer = (BYTE *) malloc (nBufferSize + 256);
    if (pNewBuffer == NULL) 
    {
        return NULL;
    }
    if (pLocalRestBuffer != NULL)
        free(pLocalRestBuffer);
	
    pLocalRestBuffer = pNewBuffer;
    nLocalRestBufferSize = nBufferSize + 256;
	
    memset(pLocalRestBuffer, 0, nLocalRestBufferSize);
	
    return pLocalRestBuffer;
}



//
// DLL Main Entry point  
// 
BOOL WINAPI DllMain( HINSTANCE hInst, 
					DWORD  ul_reason_for_call, 
					LPVOID lpReserved
					)
{
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH:
		hInstance = hInst;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
    }
    return TRUE;
}


