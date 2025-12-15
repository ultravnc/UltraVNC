// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//


#include "logging.h"

HANDLE hLogFile;
long LOGIT = 0;	// Flag to turn on/off logging
long DEBUGIT = 0;
bool nameSet = false;
char logFile[LOGNAME_SIZE];

void SetLogFile(const char * logname)
{
	strcpy(logFile,logname);
}

int SetLogStatus(const char * sLogit)
{
	char *stop;
	int logit = 0;
	logit = strtol( sLogit, &stop, 10 );

	return logit;
}

int SetLogging(const char * logname, char * sLogit)
{

	SetLogFile(logname);

	return SetLogStatus(sLogit);
}

int SetLogging(const char * logname)
{
	char dsmdebug[BUFFER_SIZE];

	SetLogFile(logname);

	GetEnvVar("dsmdebug", dsmdebug, BUFFER_SIZE);
	PrintLog((DEST,"dsmdebug = %s",dsmdebug));

	return SetLogStatus(dsmdebug);
}

// Same as PrintIt, but checks a different variable
void DebugIt(const char * sMsg)
{
	long saveIt = 0;


	if (DEBUGIT)
	{
		saveIt = LOGIT;

		PrintIt(sMsg);

		LOGIT = saveIt;
	}

}

// Stupid error logging function. (Kinda nice actually!)
void PrintIt(const char * sMsg) 
{
	char logname[LOGNAME_SIZE];
	
	if (LOGIT)
	{
		
		DWORD dwLogBytes = 0;
		long lLastError;
		char tmpMsg[2048];
		char dbuffer [10];
		char tbuffer [10];
		
		// Kinda cool, get last error and write it out. Helps with the API stuff
		lLastError = GetLastError();
    
 //pgm 		_strdate( dbuffer );
 //pgm 		_strtime( tbuffer );

		// Always append...
		if (hLogFile == 0)
		{
			time_t ltime;

			if (strstr(logFile,".log")!=NULL)
			{
				strcpy(logname,logFile);
				nameSet = true;
			}

			if (nameSet == false)
			{
				time( &ltime );
				sprintf(logname,"%s%d.log",logFile,ltime);
				nameSet = true;
			}

			hLogFile = CreateFile(logname, GENERIC_WRITE, FILE_SHARE_READ, 
					NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			
			if (hLogFile == INVALID_HANDLE_VALUE)
			{
				time( &ltime );
				ltime++;
				sprintf(logname,"%s%d.log",logFile,ltime);
				hLogFile = CreateFile(logname, GENERIC_WRITE, FILE_SHARE_READ, 
					NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			}

			strcpy(logFile,logname);
			if (APPEND)
				SetFilePointer( hLogFile, 0, NULL, FILE_END );	// Append
			else 
				SetEndOfFile( hLogFile );	// Overwrite
		}

		if(_snprintf(tmpMsg, sizeof(tmpMsg),"%s %s : ",dbuffer, tbuffer)< 0)
			PrintLog((DEST,"_snprintf failed - tmpMsg too small"));

		WriteFile(hLogFile, tmpMsg, strlen(tmpMsg), &dwLogBytes, NULL); 
		WriteFile(hLogFile, sMsg, strlen(sMsg), &dwLogBytes, NULL); 
		
		if (lLastError < 0)
		{
			if (_snprintf(tmpMsg, sizeof(tmpMsg)," - Last Error = %d : ",lLastError) < 0)
				PrintLog((DEST,"_snprintf failed - tmpMsg too small"));
			WriteFile(hLogFile,tmpMsg,strlen(tmpMsg), &dwLogBytes,NULL);
		}
		WriteFile(hLogFile, "\r\n", 2, &dwLogBytes, NULL); 
		
		SetLastError(0);

		// Cheap way to clean up...
		if (strcmp(sMsg, "Shutting Down.") == 0)
			CloseHandle(hLogFile);
		
	}
	
}

