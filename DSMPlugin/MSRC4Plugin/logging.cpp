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

#include "logging.h"

HANDLE hLogFile;
long LOGIT = 0;	//flag to turn on/off logging
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

//same as PrintIt, but checks a different variable
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

//stupid error logging function.  (Kinda nice actually!)
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
		
		//kinda cool, get last error and write it out.  Helps with the API stuff
		lLastError = GetLastError();
    
 //pgm 		_strdate( dbuffer );
 //pgm 		_strtime( tbuffer );

		//always append...
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
				SetFilePointer( hLogFile, 0, NULL, FILE_END );	//append
			else 
				SetEndOfFile( hLogFile );	//overwrite
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

		//cheap way to clean up...
		if (strcmp(sMsg, "Shutting Down.") == 0)
			CloseHandle(hLogFile);
		
	}
	
}

