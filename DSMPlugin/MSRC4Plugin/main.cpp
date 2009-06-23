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


#include "stdafx.h"
#define _WIN32_WINNT 0x0400

#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <wincrypt.h>       //windows crypto api
#include "main.h"
#include "logging.h"
//#include "MSRC4Plugin.h"
#include "crypto.h"
#include "utils.h"

int arProv[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
int iProv;
int MAX_Prov = 20;
char arCSP[][73] = {NULL,MS_DEF_PROV,MS_ENHANCED_PROV,MS_DEF_RSA_SIG_PROV,MS_DEF_RSA_SCHANNEL_PROV,MS_ENHANCED_RSA_SCHANNEL_PROV,MS_DEF_DSS_PROV,MS_DEF_DSS_DH_PROV  };
int MAX_CSP = 6;

#define MAX_LEN 256

char sDefaultGenKey[MAX_LEN];
long iWinVer=0;
long iCryptVer=0;

//char * logFile = "crypto.log";     //need to fix this...someday...

#define CRYPTO_DESCRIPTION  "crypto.exe, Mar 11 2005, Version 1.1.6"

int main(int argc, char* argv[])
{

if (argc >= 2) {


	LOGIT = false;

	PrintLog((DEST,"%s",WindowsName[WhatWindowsVer()]));
	WinVer();

	InitVars(CSP_NAME, &iWinVer, &iCryptVer, &MAXKEYLEN);

	if (WhatWindowsVer() >= WINXP)
		CSP_NAME[0] = '\0';
	else
		strcpy(CSP_NAME,MS_ENHANCED_PROV);

	DebugLog((DEST,"CSP=%s WinVer=%d CryptVer=%d MaxKeyLen=%d",CSP_NAME, iWinVer, iCryptVer, MAXKEYLEN));

	iProv = arProv[iProv];

	switch(argv[1][1])   // second character of the first parameter -e = 'e'
	{
	case 'e':
		Encrypt(argv[2],argv[3], argv[4]);
		break;
	case 'd':
		Decrypt(argv[2],argv[3], argv[4]);
		break;
	case 'k':
		CreateKey(argv[2], MAXKEYLEN);
		break;
	case 'l':
		ListProviders();
		break;
	case 'x':
		WriteDebugInfo();
		break;
	case 'v':
		Version();
		break;
	case 'r':
		LOGIT = true;
		DEBUGIT = true;
		if (argc == 3)
			DeleteContainer(argv[2]);
		else
			DeleteContainer(CTX);
		LOGIT = false;
		DEBUGIT = false;
		break;
	case 'c':
		LOGIT = true;
		DEBUGIT = true;
		if (argc == 3)
			CreateContainer(argv[2]);
		else
			CreateContainer(CTX);
		LOGIT = false;
		DEBUGIT = false;
		break;
	default:
		Usage();
		break;
	}
}
else
	Usage();

return(0);

}



int ListProviders (){
int i;
unsigned long cbName;
long iVer;
long rc = 0;
LOGIT = true;

iVer = GetCryptoVersion();
cbName = 100;

//PrintLog((DEST,"Default Provider %s",cspName));
PrintLog((DEST,"Crypto Version %d ",iVer));

//Windows OS before 2000 won't import ExponentOfOne key in a verify context.
if (iWinVer > WIN2000)
{
	CONTEXT_FLAG = VERIFY_CONTEXT_FLAG;
	szUserName[0] = '\0';
}
else
{
	CONTEXT_FLAG = NULL_CONTEXT_FLAG;
	strcpy(szUserName, CTX);
	CreateContainer(szUserName);
}

for (i=0;i<=MAX_CSP;i++) {

		strcpy(CSP_NAME,arCSP[i]);
	
	for (iProv=1;iProv<=MAX_Prov;iProv++) {

		PrintLog((DEST,"\r\n\r\nListing Provider '%s' %d ",CSP_NAME,arProv[iProv]));
		//printf("\r\n\r\nListing Provider %d\r\n",arProv[iProv]);
		unsigned long cbData;
		PROV_ENUMALGS_EX EnumAlgs;     //   Structure to hold information on 
		PROV_ENUMALGS EnumAlgsOld;     //   Structure to hold information on 
									   //   a supported algorithm
		DWORD dFlag = CRYPT_FIRST;     //   Flag indicating that the first
									   //   supported algorithm is to be
									   //   enumerated. Changed to 0 after the
									   //   first call to the function.
		HCRYPTPROV    hProvider = 0;

		//PrintLog((DEST,"Creating the Container "));
		//CreateContainer();

		PrintLog((DEST,"Acquiring the Crypto Context "));
		// Get handle for the default provider (use RSA encryption).
		PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, szUserName, CSP_NAME, arProv[iProv], CONTEXT_FLAG));
		rc = CryptAcquireContext(&hProvider, szUserName, CSP_NAME,  arProv[iProv], CONTEXT_FLAG);
		if (rc == FALSE)
		{
			PrintLog((DEST,"CryptAcquireContext |%d| |%s| |%s| |%d| |%d|",hProvider, szUserName, CSP_NAME, arProv[iProv], 0));
			rc = CryptAcquireContext(&hProvider, szUserName, CSP_NAME,  arProv[iProv], 0);
		}
		if (hProvider == 0){
				PrintLog((DEST,"Crypto could not acquire a Crypto Provider Context. "));
				continue;
		}
		else {

			cbData = sizeof(PROV_ENUMALGS_EX);

			if (CSP_NAME[0] == '\0')
			{
				printf("\r\nListing Provider 'Default' %d\r\n",arProv[iProv]);
				DebugLog((DEST,"\r\nListing Provider 'Default' %d\r\n",arProv[iProv]));
			}
			else
			{
				printf("\r\nListing Provider '%s' %d\r\n",CSP_NAME,arProv[iProv]);
				DebugLog((DEST,"\r\nListing Provider '%s' %d\r\n",CSP_NAME,arProv[iProv]));
			}

			PrintLog((DEST,"Listing Providers '%s' %d ",CSP_NAME,arProv[iProv]));
			printf("%-20.20s %-16.16s %-10.10s %-10.10s\r\n", "Algorithm Name", "Default Key len" , "Min len", "Max len"  );
			DebugLog((DEST,"%-20.20s %-16.16s %-10.10s %-10.10s\r\n", "Algorithm Name", "Default Key len" , "Min len", "Max len"  ));

 		    //DWORD dFlag = CRYPT_FIRST;     //   Flag indicating that the first
			if (iVer == 2)
			while( CryptGetProvParam(
				hProvider,          // handle to an open cryptographic provider
				PP_ENUMALGS_EX, 
				(BYTE *)&EnumAlgs,  // information on the next algorithm
				&cbData,            // number of bytes in the PROV_ENUMALGS_EX
				dFlag))             // flag to indicate whether this is a first or
									// subsequent algorithm supported by the
									// CSP.
			{
				printf("%-20.20s %-16d %-10d %-10d\r\n", EnumAlgs.szName  , EnumAlgs.dwDefaultLen , EnumAlgs.dwMinLen, EnumAlgs.dwMaxLen  );
				PrintLog((DEST,"%-20.20s %-16d %-10d %-10d", EnumAlgs.szName  , EnumAlgs.dwDefaultLen , EnumAlgs.dwMinLen, EnumAlgs.dwMaxLen  ));
				dFlag = 0;          // Set to 0 after the first call,
			} //  end of while loop. When all of the supported algorithms have
			  //  been enumerated, the function returns FALSE.
			else
			while( CryptGetProvParam(
				hProvider,          // handle to an open cryptographic provider
				PP_ENUMALGS, 
				(BYTE *)&EnumAlgsOld,  // information on the next algorithm
				&cbData,            // number of bytes in the PROV_ENUMALGS_EX
				dFlag))             // flag to indicate whether this is a first or
									// subsequent algorithm supported by the
									// CSP.
			{
				printf("%-20.20s %-16d \r\n", EnumAlgsOld.szName  , EnumAlgsOld.dwBitLen );
				PrintLog((DEST,"%-20.20s %-16d ", EnumAlgsOld.szName  , EnumAlgsOld.dwBitLen ));
				dFlag = 0;          // Set to 0 after the first call,
			} //  end of while loop. When all of the supported algorithms have
			  //  been enumerated, the function returns FALSE.

				PrintLog((DEST,"Completed listing Providers "));

				PrintLog((DEST,"Releasing the Crypto context "));
			   CryptReleaseContext(hProvider, 0);

			}
		}
	}
	return (0);

}


void Usage(){

printf("\n\nUsage: crypto -h" \
	   "\n\nList Crypto providers: crypto -l" \
	   "\n\nGenerate Key: crypto -k [keyfile]" \
	   "\n\nEncrypt File: crypto -e [infile] [outfile] [keyfile]" \
	   "\n\nDecrypt File: crypto -d [infile] [outfile] [keyfile]" \
	   "\n\nDebug Info (crypto.log): crypto -x" \
	   "\n\nRemove key container: crypto -r <optional container name>" \
	   "\n\nCreate key container: crypto -c <optional container name>" \
	   "\n\nVersion: crypto -v" \
	   "\n\n");

}


void Version(){

printf("\n\n%s\n\n", CRYPTO_DESCRIPTION);

}

int CreateKey(char * keyFile, DWORD keyLen) {

strncpy(sDefaultGenKey,keyFile, MAX_LEN);

GenKey(keyFile, keyLen);

return 0;

}



int Encrypt(char * inFile, char * outFile, char * keyFile) {	
	const IN_BUFFER_SIZE    = 2048;
	const OUT_BUFFER_SIZE   = IN_BUFFER_SIZE + 64; // extra padding
	HANDLE     hInFile; 
	HANDLE     hOutFile;
	HANDLE	   hKeyFile;
    BYTE       pbBuffer[OUT_BUFFER_SIZE];
	BOOL          finished;
    HCRYPTPROV    hProvider = 0;
    HCRYPTKEY     hKey = 0, hExchangeKey = 0;
    DWORD         dwByteCount, dwBytesWritten;
	long	rc=0;


     // Open infile and create outfile.
     hInFile = CreateFile(inFile, GENERIC_READ, FILE_SHARE_READ, 
         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
     hOutFile = CreateFile(outFile, GENERIC_WRITE, FILE_SHARE_READ, 
         NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);


		DebugLog((DEST,"PrepContext"));
	 PrepContext(iWinVer, &hProvider);

     hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	

	DebugLog((DEST,"ImportCryptKey"));
	ImportCryptKey(hProvider, &hKey, hKeyFile);

DWORD	dwCount = 11;
BYTE pbIV[11];

//CryptGenRandom(hProvider, dwCount, pbIV);

strcpy((char *)pbIV,"12345678901");

if(!CryptSetKeyParam(hKey, KP_SALT, pbIV, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}

 
  // Now, read data in, encrypt it, and write encrypted data to output.
          do 
          {
               ReadFile(hInFile, pbBuffer, IN_BUFFER_SIZE,&dwByteCount,NULL);
               finished = (dwByteCount < IN_BUFFER_SIZE);
               rc = CryptEncrypt(hKey, 0, finished, 0, pbBuffer, &dwByteCount,  
                         OUT_BUFFER_SIZE);
				PrintLog((DEST,"Finished = %d ",finished));
               WriteFile(hOutFile, pbBuffer, dwByteCount, &dwBytesWritten,
                         NULL);
          } while (!finished);

   //And we’re finished
   //almost. All we need to do now is clean up. We’re finished with both keys at this point, so let’s delete them.

   // Clean up: release handles, close files.
	CleanupCryptoKey(hExchangeKey);
    CleanupCryptoKey(hKey);
   //We’re finished using the CSP handle, so we must release it. We close the input and output files, and we’re finished.

   CleanupCryptoContext(hProvider);
   CloseHandle(hInFile);
   CloseHandle(hOutFile);
   return (0);
}


int Decrypt(char * inFile, char * outFile, char * keyFile) {	

// note: the IN_BUFFER_SIZE used here is purposely an even multiple of the
// various possible encryption block sizes, namely 1, 8, and 64 bytes.
const IN_BUFFER_SIZE    = 64 * 100; // needs to be multiple of block size.

	HANDLE      hInFile; 
	HANDLE		hOutFile;
	HANDLE		hKeyFile;
     BYTE        pbBuffer[IN_BUFFER_SIZE];
     BOOL        finished;
     HCRYPTPROV  hProvider = 0;
    HCRYPTKEY     hKey = 0, hExchangeKey = 0;
     DWORD       dwByteCount, dwBytesWritten;

	 //As in the Encrypt demo, we first need to get a handle to the default CSP.

     // Get handle for the default provider (use RSA encryption).
		DebugLog((DEST,"PrepContext"));
	 PrepContext(iWinVer, &hProvider);

     // Open infile and create outfile.
     hInFile = CreateFile(inFile, GENERIC_READ, FILE_SHARE_READ, 
         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
     hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, 
         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
     hOutFile = CreateFile(outFile, GENERIC_WRITE, FILE_SHARE_READ, 
         NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

//We now read in, in the same order as they were written, the key blob size and then the key blob itself. 

   // Import Key blob into "CSP"
	DebugLog((DEST,"ImportCryptKey"));
	ImportCryptKey(hProvider, &hKey, hKeyFile);

DWORD	dwCount = 11;
BYTE pbIV[11];

//CryptGenRandom(hProvider, dwCount, pbIV);

strcpy((char *)pbIV,"12345678901");

if(!CryptSetKeyParam(hKey, KP_SALT, pbIV, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}

//Next, we read encrypted data in, decrypt it using CryptDecrypt, and write the decrypted data to our output file. Like the CryptEncrypt function, CryptDecrypt takes a “finished” Boolean flag to tell it when we’re sending it the last buffer to decrypt.

     // Read data in, encrypt it, and write encrypted data to output file.
     do 
     {
          ReadFile(hInFile, pbBuffer, IN_BUFFER_SIZE, &dwByteCount,NULL);
          finished = (dwByteCount < IN_BUFFER_SIZE);
          CryptDecrypt(hKey, 0, finished, 0, pbBuffer, &dwByteCount);
          WriteFile(hOutFile, pbBuffer,dwByteCount,&dwBytesWritten,NULL);
     } while (!finished);
//We’re finished, so we now delete our key, release the CSP handle, and close the input and output files.

     // Clean up: release handles, close files.
	 CleanupCryptoKey(hExchangeKey);
     CleanupCryptoKey(hKey);

	 CleanupCryptoContext(hProvider);

     CloseHandle(hInFile);
     CloseHandle(hOutFile);
     CloseHandle(hKeyFile);
     return(0);
}


int WriteDebugInfo ()
{
//do some "crypto stuff" and log the output to a file for debugging
	
	const IN_BUFFER_SIZE    = 2048;
	const OUT_BUFFER_SIZE   = IN_BUFFER_SIZE + 64; // extra padding
	HANDLE	   hKeyFile = 0;
    BYTE       pbBuffer[OUT_BUFFER_SIZE];
	BOOL          finished = 1;
    HCRYPTPROV    hProvider = 0;
    HCRYPTKEY     hKey = 0, hExchangeKey = 0;
    DWORD         dwByteCount;
	long	rc=0;
	char * keyFile = "crypto.key";
	const char *  test = "This is a test...";
	char sProgramFiles[KEYFILENAME_SIZE];

/////////////////////////////
DWORD dwMode;
BYTE pbData[16];
DWORD dwCount;
DWORD i;
//////////////////////////////

	LOGIT = true;
	DEBUGIT = true;


	PrintLog((DEST,"%s",WindowsName[WhatWindowsVer()]));
	WinVer();

	InitVars(CSP_NAME, &iWinVer, &iCryptVer, &MAXKEYLEN);

	DebugLog((DEST,"CSP=%s WinVer=%d CryptVer=%d MaxKeyLen=%d",CSP_NAME, iWinVer, iCryptVer, MAXKEYLEN));

	DebugLog((DEST,"Testing some environment variables."));
	GetEnvVar(PROGRAMFILES, sProgramFiles, BufSize);
	GetEnvVar("temp", sProgramFiles, BufSize);
	GetEnvVar("path", sProgramFiles, BufSize);

	CreateKey(keyFile, MAXKEYLEN);

	DebugLog((DEST,"PrepContext"));
	PrepContext(iWinVer, &hProvider);

	hKeyFile = CreateFile(keyFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	DebugLog((DEST,"ImportCryptKey"));
	ImportCryptKey(hProvider, &hKey, hKeyFile);

	CloseHandle(hKeyFile);

////////////////////////////////////////////////////////
// Read the cipher mode.
dwCount = sizeof(DWORD);
if(!CryptGetKeyParam(hKey, KP_MODE, (BYTE *)&dwMode, &dwCount, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}

// Print out the cipher mode.
printf("Default cipher mode: %d\n", dwMode);

// Read the salt.
dwCount = 16;
if(!CryptGetKeyParam(hKey, KP_SALT, pbData, &dwCount, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}
// Print out the initialization vector.
printf("Default IV:");
for(i=0;i<dwCount;i++) printf("%2.2x ",pbData[i]);
printf("\n");

// Read the initialization vector.
dwCount = 16;
if(!CryptGetKeyParam(hKey, KP_IV, pbData, &dwCount, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}
// Print out the initialization vector.
printf("Default IV:");
for(i=0;i<dwCount;i++) printf("%2.2x ",pbData[i]);
printf("\n");

// Set the cipher mode.
dwMode = CRYPT_MODE_OFB;
if(!CryptSetKeyParam(hKey, KP_MODE, (BYTE *)&dwMode, 0)) {
    printf("Error %x during CryptSetKeyParam!\n", GetLastError());
    return 1;
}

// Read the cipher mode.
dwCount = sizeof(DWORD);
if(!CryptGetKeyParam(hKey, KP_MODE, (BYTE *)&dwMode, &dwCount, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}

// Print out the cipher mode.
printf("Default cipher mode: %d\n", dwMode);

dwCount = 16;
BYTE pbIV[17];

CryptGenRandom(hProvider, dwCount, pbIV);

if(!CryptSetKeyParam(hKey, KP_IV, pbIV, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}

// Read the initialization vector.
dwCount = 16;
if(!CryptGetKeyParam(hKey, KP_IV, pbData, &dwCount, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}
// Print out the initialization vector.
printf("Default IV:");
for(i=0;i<dwCount;i++) printf("%2.2x ",pbData[i]);
printf("\n");

if(!CryptSetKeyParam(hKey, KP_SALT, pbIV, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}

// Read the salt.
dwCount = 16;
if(!CryptGetKeyParam(hKey, KP_SALT, pbData, &dwCount, 0)) {
    printf("Error %x during CryptGetKeyParam!\n", GetLastError());
    return 1;
}
// Print out the initialization vector.
printf("Default IV:");
for(i=0;i<dwCount;i++) printf("%2.2x ",pbData[i]);
printf("\n");

////////////////////////////////////////////

	strcpy((char *)pbBuffer, test);
	DebugLog((DEST,"%s",(char *)pbBuffer));

	dwByteCount = strlen((char *)pbBuffer);
	rc = CryptEncrypt(hKey, 0, finished, 0, pbBuffer, &dwByteCount, OUT_BUFFER_SIZE);
	DebugLog((DEST,"Encrypted buffer"));

	rc = CryptDecrypt(hKey, 0, finished, 0, pbBuffer, &dwByteCount);
	DebugLog((DEST,"Decrypted buffer"));

	DebugLog((DEST,"%s",(char *)pbBuffer));

	if (strcmp((char *)pbBuffer,test) ==0)
	{
		DebugLog((DEST,"Encrypt/Decrypt Succeeded"));
	}
	else
	{
		DebugLog((DEST,"Encrypt/Decrypt Failed"));
	}

	CleanupCryptoKey(hExchangeKey);
    CleanupCryptoKey(hKey);
	CleanupCryptoContext(hProvider);

	DebugLog((DEST,"Listing Providers."));
	ListProviders();
   
   return(0);
}



