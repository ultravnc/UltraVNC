//  Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//
//  Created by Sean E. Covel
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

//#include "stdafx.h"
#define _WIN32_WINNT 0x0410     //must be defined for the crypto api
#define _WIN32_WINDOWS 0x0410
#define WINVER 0x0400

#include <stdio.h>
#include <windows.h>
//#include <time.h>
//#include <tchar.h>


#include <wincrypt.h>       //windows crypto api
#include "crypto.h"
#ifdef _WITH_LOG  
	#include "logging.h"
#endif  
#include "utils.h"

int CSP_PROV =	PROV_RSA_FULL;	//Microsoft Basic/Enhanced provider
char DEFAULTKEY[KEYSize];

CHAR szUserName[100];         // Buffer to hold the name of the key container.
DWORD dwUserNameLen = 100;    // Length of the buffer.


char CSP_NAME[CSP_SIZE];

DWORD VERIFY_CONTEXT_FLAG = CRYPT_VERIFYCONTEXT;
DWORD MACHINE_CONTEXT_FLAG = CRYPT_MACHINE_KEYSET;	//Test for Win98 winvnc problem
DWORD NULL_CONTEXT_FLAG = 0;
DWORD CONTEXT_FLAG = CRYPT_VERIFYCONTEXT;

DWORD KEYLEN = KEYLEN_128BIT;
DWORD MAXKEYLEN = KEYLEN_128BIT;	//I'm going to do something with this some day...


BOOL GenKey(char * sDefaultGenKey, DWORD keyLen)
{
	
    //Generates the RC4 key and writes it to a file
    //The key contains information about the algorithm used and
    //the key length.
    char GkeyFile[KEYFILENAME_SIZE];
	
    const int IN_BUFFER_SIZE    = 2048;
    const int OUT_BUFFER_SIZE   = IN_BUFFER_SIZE + 64; // extra padding
	
    HANDLE     hGKeyFile = 0;
    BYTE       pbBuffer[OUT_BUFFER_SIZE];
    HCRYPTPROV    hGProvider = 0;
    HCRYPTKEY     hGKey = 0;
    HCRYPTKEY     hGExchangeKey = 0;
    DWORD         dwByteCount;
    DWORD         dwBytesWritten;
    long rc;

	long iWinVer=0;
	long iCryptVer=0;

	KEYLEN = keyLen;

	InitVars(CSP_NAME, &iWinVer, &iCryptVer, &MAXKEYLEN);

#ifdef _WITH_LOG  
    PrintLog((DEST,"GenKey. %d bit", (DWORD)(HIWORD(KEYLEN))));
	PrintLog((DEST,"OS is '%s'",WindowsName[iWinVer]));
	PrintLog((DEST,"Using provider |%s|",CSP_NAME));
	PrintLog((DEST,"Max Key Length %d",(DWORD)(HIWORD(MAXKEYLEN))));
#endif 

#ifdef _WITH_REGISTRY

    REGISTRY *m_pREGISTRY;
    m_pREGISTRY = new REGISTRY(HKEY_CURRENT_USER, MSRC4_KEY_FILE, false);
    m_pREGISTRY->ReadItem(GkeyFile, KEYFILENAME_SIZE, INDEXVAL_KEYGEN, NULL);
    delete m_pREGISTRY;
#else
    strcpy(GkeyFile,sDefaultGenKey);
#endif
    

	//Windows OS before 2000 won't import ExponentOfOne key in a verify context.
	if (iWinVer >= WIN2000)
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"Using NULL_CONTEXT"));
#endif  
		CONTEXT_FLAG = NULL_CONTEXT_FLAG;
		szUserName[0] = '\0';
	}
	else
	{
		//PrintLog((DEST,"Using VERIFY_CONTEXT"));
		//CONTEXT_FLAG = VERIFY_CONTEXT_FLAG;
#ifdef _WITH_LOG  
		PrintLog((DEST,"Using MACHINE_KEYSET"));
#endif  
		CONTEXT_FLAG = MACHINE_CONTEXT_FLAG;
		//CONTEXT_FLAG = VERIFY_CONTEXT_FLAG;
		strcpy(szUserName, CTX);
		CreateContainer(szUserName);
	}

    rc = 0;

	rc = PrepContext(iWinVer, &hGProvider);

#ifdef _WITH_LOG  
	PrintLog((DEST,"Generating key file: %s",GkeyFile));
#endif 

    //open both the output file
	hGKeyFile = CreateFile(GkeyFile, GENERIC_WRITE, FILE_SHARE_READ, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	//check if the file created ok
	if (hGKeyFile == INVALID_HANDLE_VALUE)
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"Failed to create key file. "));
#endif  
		return false;
	}

#ifdef _WITH_LOG  
	PrintLog((DEST,"Importing ExponentOfOne KeyBlob"));
#endif

    //HCRYPTKEY ExponentOfOneKey = NULL;
    if( !CryptImportKey(hGProvider, PrivateKeyWithExponentOfOne, sizeof(PrivateKeyWithExponentOfOne), 0, 0, &hGExchangeKey))
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"Import ExponentOfOne Key failed.(GenKey)"));
#endif 
		return false;
	}
	
#ifdef _WITH_LOG  
	PrintLog((DEST,"Generating random key blob"));
#endif 
	// Generate a random key blob
	if (! CryptGenKey(hGProvider, CALG_RC4, CRYPT_EXPORTABLE + KEYLEN, &hGKey))
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"CryptGenKey failed. " ));
#endif  
		return false;
	}
	
    // The first call to ExportKey with NULL gets the key size.
	dwByteCount=0;
	if (! CryptExportKey(hGKey, hGExchangeKey, SIMPLEBLOB, 0,  NULL, &dwByteCount))
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"CryptExportKey failed. " ));
#endif  
		return false;
	}
	
	if (dwByteCount == 0)
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"CryptKeyLen == 0."));
#endif 
		return false;
	}
	
#ifdef _WITH_LOG  
	PrintLog((DEST,"Export the key blob"));
#endif  
	//Generate the actual key
	rc = CryptExportKey(hGKey, hGExchangeKey, SIMPLEBLOB, 0, pbBuffer, 
		&dwByteCount);
    if (rc == 0)
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"CryptExportKey2 failed. "));
#endif  
		return false;
	}
	
	//assertion
	if (dwByteCount != BLOBSIZE)
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"CryptKeyLen != BLOBSIZE."));
#endif  
		return false;
	}
	
	// Write the "bitness" of the key.
	if (KEYLEN == 0x00800000)
	{
#ifdef _WITH_LOG  
		DebugLog((DEST,"128 bit key."));
#endif  
		WriteFile(hGKeyFile, "128 bit", 7, &dwBytesWritten, NULL);
	}
	else {
			if (KEYLEN == 0x00380000)
			{
#ifdef _WITH_LOG  
				DebugLog((DEST,"56 bit key."));
#endif  
				WriteFile(hGKeyFile, " 56 bit", 7, &dwBytesWritten, NULL);
			}
			else
			{
#ifdef _WITH_LOG  
				DebugLog((DEST,"40 bit key."));
#endif  
				WriteFile(hGKeyFile, " 40 bit", 7, &dwBytesWritten, NULL);
			}
	}

	// Write size of key blob
	WriteFile(hGKeyFile, &dwByteCount, sizeof(dwByteCount), &dwBytesWritten, NULL);
	
	//Write key blob itself
	WriteFile(hGKeyFile, pbBuffer, dwByteCount, &dwBytesWritten, NULL);
	
	// Clean up: release handles
	CryptDestroyKey(hGKey);
	CryptDestroyKey(hGExchangeKey);
	
	//We’re finished using the CSP handle, so we must release it. We close the input and output files, and we’re finished.
	CryptReleaseContext(hGProvider, 0);
	
    CloseHandle(hGKeyFile);

	return true;
}


long GetCryptoVersion() {
//Determine the Crypto API version

BYTE pbData[1000];
unsigned long cbData;
HCRYPTPROV    hProvider = 0;
BYTE *ptr = NULL;
DWORD version;
long rc = 0;

#ifdef _WITH_LOG  
	PrintLog((DEST,"GetCryptoVersion."));
	PrintLog((DEST,"Acquiring the Crypto Context."));

	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, NULL, NULL, PROV_RSA_FULL, CONTEXT_FLAG));
#endif  

// Get handle for the default provider (use RSA encryption).
CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL , CONTEXT_FLAG );

if (hProvider == 0){
#ifdef _WITH_LOG  
		PrintLog((DEST,"Crypto could not acquire a Crypto Provider Context. "));
#endif  
		return 0;
}
else {

#ifdef _WITH_LOG  
	PrintLog((DEST,"Crypto acquired a Crypto Provider Context. "));
#endif  
	cbData = 1000;

	rc = CryptGetProvParam(
				hProvider,          // handle to an open cryptographic provider
				PP_VERSION,			//get VERSION information
				(BYTE *)&pbData,  // information on the version
				&cbData,            // number of bytes 
				0);       // flag for enumeration functions
	if (rc != TRUE)
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"CryptGetProvParam failed to return the PP_VERSION.  %d",rc));
#endif 
		return 0;
	}
}

if(!CryptReleaseContext(hProvider, 0)) {
#ifdef _WITH_LOG  
    PrintLog((DEST,"Error during CryptReleaseContext."));
#endif 
	return 0;
}

ptr = pbData;

version = *(DWORD *)ptr;

//version - M = major version, mm = minor version...
//  0x00000101 = Version 1.1

					//       Mmm        Mmm
if (version < 511)	//0x00000100-0x000001FF (0-511)
	return 1;

					//       Mmm        Mmm
if (version < 767)  //0x00000200-0x000002FF  (512-767)
	return 2;

return 0;

}


BOOL InitVars(char *szCSPName, long *iWinVer, long *iCryptVer, DWORD * iMaxKey) {

//Get the windows version, crypto version, best crypto provider, and best bit-depth supported.

HCRYPTPROV    hProvider = 0;                // crypto provider

	*iWinVer = 0;
	*iCryptVer = 0;
	*iMaxKey = 0;

	*iWinVer = WhatWindowsVer();
	
#ifdef _WITH_LOG  
	PrintLog((DEST,"InitVars"));
#endif 

	//*iCryptVer 
	*iCryptVer = GetCryptoVersion();

	if (*iCryptVer == 1) {
		//Version 1 doesn't support anything usefull for finding bit depth and whatnot...
		//I think you only see this on 95 anyway...
		//*** Only on 95 without the High Encryption pack ***
		//not much works with version 1.
		szCSPName[0] = '\0';	//default
		*iMaxKey = KEYLEN_40BIT;
	}
	else
	{
		//Version 2 has better features, but we don't want to waste a lot of time...

		if (*iWinVer>= WINXP) {	//XP and up come with 128bit out of the box.
			szCSPName[0] = '\0';	//default
			*iMaxKey = KEYLEN_128BIT;
		}
		else {	
			
			if (*iWinVer == WINNT) {  //NT does not support GetDefaultProvider function...
				//look for MS_ENHANCED_PROV

#ifdef _WITH_LOG  
	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT));
#endif 
				CryptAcquireContext(&hProvider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  				if (hProvider != 0){
					strcpy(szCSPName,MS_ENHANCED_PROV);
					*iMaxKey = KEYLEN_128BIT;
				}
				else {
					szCSPName[0] = '\0';
					*iMaxKey = KEYLEN_40BIT;
				}
			   CryptReleaseContext(hProvider, 0);
			}
			else {	//98, 98SE, ME, 2000
				//We gotta look and see what we have available...
				//I was going to use GetDefaultProvider, but it returns MS Base Provider
				//Even when the Enhanced provider is available...
#ifdef _WITH_LOG  
	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT));
#endif 
				CryptAcquireContext(&hProvider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  				if (hProvider != 0){
					strcpy(szCSPName, MS_ENHANCED_PROV);
					*iMaxKey = KEYLEN_128BIT;
				}
				else {
					szCSPName[0] = '\0';
					*iMaxKey = KEYLEN_40BIT;
				}
			   CryptReleaseContext(hProvider, 0);
			}
		}
	}


	//copy the values to the global variables
	strcpy(CSP_NAME, szCSPName);
	MAXKEYLEN = *iMaxKey;

	return true;
}


int ResetCrypto(HCRYPTKEY hKey)
{
  DWORD dwByteCount = 1;
  CHAR temp[] = "a";		//decrypt/encrypt some junk.

	//reset the crypto context to prepare for the next connection
	//true means "this is the last bunch of data, so reset"
    return CryptDecrypt(hKey, 0, true, 0, (BYTE *)temp, &dwByteCount);

}


int PrepContext(int iWinVer, HCRYPTKEY * hProvider)
{
#ifdef _WITH_LOG  
		PrintLog((DEST,"PrepContext"));
#endif 
	//Windows OS before 2000 won't import ExponentOfOne key in a verify context.
	if (iWinVer >= WIN2000)
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"Using VERIFY_CONTEXT"));
#endif  
		CONTEXT_FLAG = VERIFY_CONTEXT_FLAG;
		szUserName[0] = '\0';
	}
	else
	{
		//PrintLog((DEST,"Using NULL_CONTEXT"));
		//CONTEXT_FLAG = NULL_CONTEXT_FLAG;
#ifdef _WITH_LOG  
		PrintLog((DEST,"Using MACHINE_KEYSET"));
#endif 
		CONTEXT_FLAG = MACHINE_CONTEXT_FLAG;
		//CONTEXT_FLAG = NULL_CONTEXT_FLAG;
		strcpy(szUserName, CTX);
		CreateContainer(szUserName);
	}

#ifdef _WITH_LOG  
	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, szUserName, CSP_NAME, CSP_PROV, CONTEXT_FLAG));
#endif  
    // Get handle for the default provider (use RSA encryption).
    if (! CryptAcquireContext(hProvider, szUserName, CSP_NAME, CSP_PROV, CONTEXT_FLAG)) {
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"CryptAcquireContext failed...retrying."));
			PrintLog((DEST,"Using NULL Context flag."));
			PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, szUserName, CSP_NAME, CSP_PROV, NULL_CONTEXT_FLAG));
#endif 
			if (! CryptAcquireContext(hProvider, szUserName, CSP_NAME, CSP_PROV, NULL_CONTEXT_FLAG)) {
#ifdef _WITH_LOG  
				PrintLog((DEST,"Unable to Acquire a Crypto context."));
#endif 
				return -1;
			}

		}
    }

	return 0;

}

void CleanupCryptoKey(HCRYPTKEY hKey)
{

    CryptDestroyKey(hKey);

}

void CleanupCryptoContext(HCRYPTPROV hProvider)
{
    CryptReleaseContext(hProvider, 0);

}

int CreateDerivedCryptKey(HCRYPTPROV hProvider, HCRYPTKEY * hKey, char* password)
{

	HCRYPTHASH hHash = 0;
	DWORD dwLength;


	// Create a hash object.
	if(!CryptCreateHash(hProvider, CALG_MD5, 0, 0, &hHash)) {
#ifdef _WITH_LOG  
	    PrintLog((DEST,"Error creating hash provider"));
#endif 
        return -1;
	}

	// Hash the password string.
	dwLength = strlen(password);
	if(!CryptHashData(hHash, (BYTE *)password, dwLength, 0)) {
#ifdef _WITH_LOG  
	    PrintLog((DEST,"Error hashing data"));
#endif 
        return -1;
	}
 

	// Create a block cipher session key based on the hash of the password.
	if(!CryptDeriveKey(hProvider, CALG_RC4, hHash, CRYPT_EXPORTABLE, hKey)) {
#ifdef _WITH_LOG  
	    PrintLog((DEST,"Error creating derived key"));
#endif 
        return -1;
	}


	if(hHash != 0) CryptDestroyHash(hHash);

    return 0;

}



int ImportCryptKey(HCRYPTPROV hProvider, HCRYPTKEY * hKey, HANDLE hKeyFile)
				   
{
    const int      IN_BUFFER_SIZE    = 2048;
    const int      OUT_BUFFER_SIZE   = IN_BUFFER_SIZE + 64; // extra padding
    BYTE        pbBuffer[OUT_BUFFER_SIZE];
    DWORD       dwByteCount = 0, dwBytesWritten = 0;
	HCRYPTKEY	hExchangeKey;
	char		bitNess[8];

#ifdef _WITH_LOG  
            PrintLog((DEST,"Reading KeyBlob"));
#endif 
				//read in "bitness"

			if (! ReadFile(hKeyFile,bitNess,7,&dwBytesWritten,NULL)) {
#ifdef _WITH_LOG  
                PrintLog((DEST,"Reading BLOB size failed"));
#endif 
                return -1;
			}

#ifdef _WITH_LOG  
			DebugLog((DEST,"Key bits is %s",bitNess));
#endif  

            // Read in key blob size
            if (! ReadFile(hKeyFile,&dwByteCount,sizeof(dwByteCount),&dwBytesWritten,NULL)) {
#ifdef _WITH_LOG  
                PrintLog((DEST,"Reading BLOB size failed"));
#endif 
                return -1;
            }

			if (dwByteCount <= OUT_BUFFER_SIZE)
			{
				//read in the key blob itself from input file.
				if (! ReadFile(hKeyFile, pbBuffer, dwByteCount, &dwBytesWritten, NULL)) {
#ifdef _WITH_LOG  
					PrintLog((DEST,"Reading BLOB failed"));
#endif 
					return -1;
				}
			}
			else
			{
#ifdef _WITH_LOG  
				PrintLog((DEST,"Possible buffer overrun"));
#endif 
				return -1;
			}

#ifdef _WITH_LOG  
    PrintLog((DEST,"Importing ExponentOfOne KeyBlob"));
#endif 

	if( !CryptImportKey(hProvider, PrivateKeyWithExponentOfOne, sizeof(PrivateKeyWithExponentOfOne), 0, 0, &hExchangeKey))
	{
#ifdef _WITH_LOG  
        PrintLog((DEST,"Import ExponentOfOne Key failed. (SetParams)"));
#endif 
		return -1;
    }

#ifdef _WITH_LOG  
    PrintLog((DEST,"Importing KEY KeyBlob"));
#endif 

    //now, we convert the key blob back into a key (internally to the CSP), with the call to CryptImportKey.
    if (! CryptImportKey(hProvider, (const BYTE *)pbBuffer, dwByteCount, hExchangeKey, 0, hKey)) {
#ifdef _WITH_LOG  
        PrintLog((DEST,"Error importing key."));
#endif 
        return -1;
    }

	CleanupCryptoKey(hExchangeKey);

	return 0;
}


int GetKeyLen(HCRYPTKEY hKey)
{

	char		pKeyLN[20];
    BYTE		pbDataBuf[20];
	DWORD		pdwDataLen		  = 20;
	int keyLen = 0;

	//check the imported key's length
			CryptGetKeyParam(hKey, KP_KEYLEN, pbDataBuf, &pdwDataLen, 0);

			if (_snprintf(pKeyLN, sizeof(pKeyLN),"%2.2x",pbDataBuf[0]) < 0)
			{  
#ifdef _WITH_LOG  
					PrintLog((DEST,"_snprintf failed - pKeyLN too small"));
#endif 
			}  

			if (strcmp(pKeyLN,"80")==0)
			{
				keyLen = 128;
#ifdef _WITH_LOG  
				PrintLog((DEST,"Imported Key is 128bit"));
#endif  
			}
			if (strcmp(pKeyLN,"28")==0)
			{
				keyLen = 40;
#ifdef _WITH_LOG  
				PrintLog((DEST,"Imported Key is 40bit"));
#endif 
			}

			if (strcmp(pKeyLN,"38")==0)
			{
				keyLen = 56;
#ifdef _WITH_LOG  
				PrintLog((DEST,"Imported Key is 56bit"));
#endif 
			}

			return keyLen;
}


BOOL CreateContainer(char * container) 
{ 
	//--------------------------------------------------------------------
	// Verify and correct the Key Container and base keys if needed, otherwise, does nothing
	
	// Code from MSDN example
	
	HCRYPTPROV hCryptProv = 0;        // handle for the cryptographic provider context
	HCRYPTKEY hCKey;               // public/private key handle

#ifdef _WITH_LOG  
    PrintLog((DEST,"CreateContainer %s", container));
	
	
	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hCryptProv, container, CSP_NAME, CSP_PROV, 0));
#endif 

	// CryptAcquireContext. Try to open the key container
	if(CryptAcquireContext(
		&hCryptProv,               // handle to the CSP
		container,                  // container name 
		CSP_NAME,                      // use the default provider
		CSP_PROV,             // provider type
		0))                        // flag values
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"A crypto context with the %s key container already exists.", szUserName));
#endif  
	}
	else
	{ 
		//--------------------------------------------------------------------
		// Some sort of error occurred in acquiring the context.
		//probably didn't exist yet.
		// Create a new key container. 
		
#ifdef _WITH_LOG  
	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hCryptProv, container, CSP_NAME, CSP_PROV, CRYPT_NEWKEYSET));
#endif  

		if(CryptAcquireContext(
			&hCryptProv, 
			container, 
			CSP_NAME, 
			CSP_PROV, 
			CRYPT_NEWKEYSET)) 
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"A new key container has been created."));
#endif 
		}
		else
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"Could not create a new key container."));
#endif 
			return false;
		}
	} // end else

/*
	//--------------------------------------------------------------------
	// A cryptographic context with a key container is available. Get the
	// name of the key container. 
	if(CryptGetProvParam(
		hCryptProv,               // handle to the CSP
		PP_CONTAINER,             // get the key container name 
		(BYTE *)szUserName,       // pointer to the key container name
		&dwUserNameLen,           // length of name, preset to 100
		0)) 
	{
		PrintLog((DEST,"A crypto context has been acquired and the name on the key container is %s",szUserName));
	}
	else
	{
		// An error occurred while getting the key container name.
		PrintLog((DEST,"A context was acquired or created, but an error occurred getting the key container name."));
		return false;
	} 
*/
	
	//--------------------------------------------------------------------
	// A context with a key container is available.
	// Attempt to get the handle to the key exchange key. 
	
	if(CryptGetUserKey(
		hCryptProv,                     // handle to the CSP
		AT_SIGNATURE,                   // key specification
		&hCKey))                         // handle to the key
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"A signature key is available."));
#endif 
	}
	else
	{
		if(GetLastError() == NTE_NO_KEY) 
		{
			//----------------------------------------------------------------
			// The error was that there is a container but no key.
			
			// Create a signature key pair. 
			
#ifdef _WITH_LOG  
			PrintLog((DEST,"The signature key does not exist."));
			PrintLog((DEST,"Create a signature key pair.")); 
#endif 
			if(CryptGenKey(
				hCryptProv,
				AT_SIGNATURE,
				0,
				&hCKey)) 
			{
#ifdef _WITH_LOG  
				PrintLog((DEST,"Created a signature key pair."));
#endif 
			}
			else
			{
#ifdef _WITH_LOG  
				PrintLog((DEST,"Error occurred creating a signature key.")); 
#endif 
				return false;
			}
		}
		else
		{
#ifdef _WITH_LOG  
        PrintLog((DEST,"An error other than NTE_NO_KEY getting signature key."));
#endif 
		return false;
		}
	} // end if
	
	//PrintLog((DEST,"A signature key pair existed, or one was created."));
	
	// Destroy the signature key.
	
	if(hCKey)
	{
		if(!(CryptDestroyKey(hCKey)))
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"Error during CryptDestroyKey"));
#endif 
			return false;
		}
	} 
	
	// Next, check the exchange key. 
	if(CryptGetUserKey(
		hCryptProv,
		AT_KEYEXCHANGE,
		&hCKey)) 
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"An exchange key exists. "));
#endif
	}
	else
	{
		// Check to determine whether an exchange key needs to be created.
		if(GetLastError()==NTE_NO_KEY) 
		{ 
			// Create a key exchange key pair.
#ifdef _WITH_LOG  
			PrintLog((DEST,"The exchange key does not exist."));
			PrintLog((DEST,"Attempting to create an exchange key pair."));
#endif 
			if(CryptGenKey(
				hCryptProv,
				AT_KEYEXCHANGE,
				0,
				&hCKey)) 
			{
#ifdef _WITH_LOG  
				PrintLog((DEST,"Exchange key pair created."));
#endif 
			}
			else
			{
#ifdef _WITH_LOG  
				PrintLog((DEST,"Error occurred attempting to create an exchange key."));
#endif 
				return false;
			}
		}
		else
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"An error other than NTE_NO_KEY occurred."));
#endif 
			return false;
		}
	}
	
	//PrintLog((DEST,"An exchange key pair existed, or one was created."));
	// Destroy the session key.
	
	if(hCKey)
	{
		if(!(CryptDestroyKey(hCKey)))
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"Error during CryptDestroyKey"));
#endif 
			return false;
		}
	}
	
	// Release the CSP.
	
	if(hCryptProv)
	{
		if(!(CryptReleaseContext(hCryptProv,0)))
		{
#ifdef _WITH_LOG  
			PrintLog((DEST,"Error during CryptReleaseContext"));
#endif 
			return false;
		}
	} 
	

#ifdef _WITH_LOG  
	PrintLog((DEST,"Key Container is ready for use."));
#endif 

	return true;

}



BOOL DeleteContainer(char * container) 
{ 
	//--------------------------------------------------------------------
	// Delete the Key Container (it will get re-created next time the plugin is used)
	

	HCRYPTPROV hCryptProv = 0;        // handle for the cryptographic provider context
	
#ifdef _WITH_LOG  
    PrintLog((DEST,"DeleteContainer %s", container));
	
	
	PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hCryptProv, container, CSP_NAME, CSP_PROV, CRYPT_DELETEKEYSET));
#endif 
	// CryptAcquireContext. 
	  if(CryptAcquireContext(
		&hCryptProv,               // handle to the CSP
		container,                  // container name 
		CSP_NAME,                      // use the default provider
		CSP_PROV,             // provider type
		CRYPT_DELETEKEYSET))                        // flag values
	{
#ifdef _WITH_LOG  
		PrintLog((DEST,"The %s key container has been deleted.", container));
#endif
	}
	else
	{ 
		//--------------------------------------------------------------------
		// Some sort of error occurred in acquiring the context. 
#ifdef _WITH_LOG  
		PrintLog((DEST,"Could not delete container %s.", container));
#endif 
		return false;
	} // end else

	return true;
}





