/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2007 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com
//
////////////////////////////////////////////////////////////////////////////

#ifndef FILETRANSFER_H__
#define FILETRANSFER_H__
#pragma once

#include <list>
#include "ZipUnZip32/zipUnZip32.h"

#define CONFIRM_YES 1
#define CONFIRM_YESALL 2
#define CONFIRM_NO 3
#define CONFIRM_NOALL 4

#define FT_PROTO_VERSION_OLD 1  // <= RC18 server.. "fOldFTPRotocole" version
#define FT_PROTO_VERSION_2   2  // base ft protocol
#define FT_PROTO_VERSION_3   3  // new ft protocol session messages

typedef std::list<int> FilesList; // List of files indexes to be sent or received

class ClientConnection;

class FileTransfer  
{
public:
	CRITICAL_SECTION crit;
	// Props
	VNCviewerApp		*m_pApp; 
	ClientConnection	*m_pCC;
	HWND				hWnd;
	bool				m_fAbort;
    bool                m_fUserAbortedFileTransfer; // 21 April 2008 jdp 
	bool				m_fAborted;		// Async Reception file only
	int					m_nDeleteCount; // Grouped file deletion trick

	FilesList			m_FilesList;	// List of files indexes to be sent or received
	FilesList::iterator m_iFile;
	int					m_nFilesToTransfer;
	int					m_nFilesTransfered;
	bool				m_fFileCommandPending;
	bool				m_fFileTransferRunning;
	bool				m_fVisible;
	bool				m_fFTAllowed;
	int                 m_timer;
	bool				m_fFocusLocal;
	char                m_szFTParamTitle[128];
	char                m_szFTParamComment[64];
	char                m_szFTParam[256];
	char                m_szFTConfirmTitle[128];
	char                m_szFTConfirmComment[364];
	int					m_nConfirmAnswer;
	CZipUnZip32			*m_pZipUnZip;
	bool				m_fApplyToAll;
	bool				m_fShowApplyToAll;
	char				m_szDeleteButtonLabel[64];
	char				m_szNewFolderButtonLabel[64];
	char				m_szRenameButtonLabel[64];

	// adzm 2009-08-02
	char				m_szLastLocalPath[_MAX_PATH];

	__int64				m_nnFileSize;
	DWORD				m_dwCurrentValue;
	DWORD				m_dwCurrentPercent;

	DWORD				m_dwStartTick;

	// File Sending (upload)
	HANDLE				m_hSrcFile;
	char				m_szSrcFileName[MAX_PATH + 32];
	DWORD				m_dwNbBytesRead;
	__int64				m_dwTotalNbBytesRead;
	bool				m_fEof;
	bool				m_fFileUploadError;
	bool				m_fFileUploadRunning;
	bool				m_fSendFileChunk;
	bool				m_fCompress;
	char*				m_lpCSBuffer;
	int					m_nCSOffset;
	int					m_nCSBufferSize;

	// Directory list reception
	WIN32_FIND_DATA		m_fd;
	int					m_nFileCount;
	bool				m_fDirectoryReceptionRunning;
	char				m_szFileSpec[MAX_PATH + 64];


	// File reception (download)
	char				m_szDestFileName[MAX_PATH + 32];
	HANDLE				m_hDestFile;
	DWORD				m_dwNbReceivedPackets;
	DWORD				m_dwNbBytesWritten;
	__int64				m_dwTotalNbBytesWritten;
	__int64				m_dwTotalNbBytesNotReallyWritten;
	int					m_nPacketCount;
	bool				m_fPacketCompressed;
	bool				m_fFileDownloadRunning;
	bool				m_fFileDownloadError;
	char				m_szIncomingFileTime[18];

    int                 m_ServerFTProtocolVersion; // 8/6/2008 jdp 
	int					m_nBlockSize;

	int					m_nNotSent;

	DWORD				m_dwLastChunkTime;
	MMRESULT			m_mmRes; 
	UINT				m_timerID;

	bool				bSortDirectionsL[3];
	bool				bSortDirectionsR[3];

   	HMODULE				m_hRichEdit;     // 16 April 2008 jdp
    int                 m_maxHistExtent;

	// Methods
	FileTransfer(VNCviewerApp *pApp, ClientConnection *pCC);
	int DoDialog();
   	virtual ~FileTransfer();
	static BOOL CALLBACK FileTransferDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK LFBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK RFBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	int DoFTParamDialog(LPSTR szTitle, LPSTR szComment);
	static BOOL CALLBACK FTParamDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	int DoFTConfirmDialog(LPSTR szTitle, LPSTR szComment);
	static BOOL CALLBACK FTConfirmDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static int CALLBACK ListViewLocalCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort); /*TAW*/
	static int CALLBACK ListViewRemoteCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort); /*TAW*/

	void DisableButtons(HWND hWnd, bool X = true);
	void EnableButtons(HWND hWnd);
    void CheckButtonState(HWND hWnd);

	bool SendFile(long lSize, int nLen);
	bool SendFileChunk();
	bool FinishFileReception();
	bool UnzipPossibleDirectory(LPSTR szFileName);
	bool SendFiles(long lSize, int nLen);
	bool OfferNextFile();
	void ListRemoteDrives(HWND hWnd, int nLen);
	void ProcessFileTransferMsg(void);
	void RequestPermission();
	bool TestPermission(long lSize, int nVersion);
	void AddFileToFileList(HWND hWnd, int nListId, WIN32_FIND_DATA& fd, bool fLocalSide);
	void RequestRemoteDirectoryContent(HWND hWnd, LPSTR szPath);
	void RequestRemoteDrives();
	void RequestRemoteFile(LPSTR szRemoteFileName);
	bool OfferLocalFile(LPSTR szSrcFileName);
	int  ZipPossibleDirectory(LPSTR szSrcFileName);
	bool ReceiveFile(unsigned long lSize, int nLen);
	bool ReceiveFileChunk(int nLen, int nSize);
	bool FinishFileSending();
	bool AbortFileReception();
	bool ReceiveFiles(unsigned long lSize, int nLen);
	bool RequestNextFile();
	bool ReceiveDestinationFileChecksums(int nSize, int nLen);
	void HighlightTransferedFiles(HWND hSrcList, HWND hDstList);
	void PopulateRemoteListBox(HWND hWnd, int nLen);
	void ReceiveDirectoryItem(HWND hWnd, int nLen);
	void FinishDirectoryReception();
	bool IsShortcutFolder(LPSTR szPath);
	bool ResolvePossibleShortcutFolder(HWND hWnd, LPSTR szFolder);
	void PopulateLocalListBox(HWND hWnd, LPSTR szPath);
	void ListDrives(HWND hWnd);
	void CreateRemoteDirectory(LPSTR szDir);
    void DeleteRemoteFile(std::string szFile);
	bool CreateRemoteDirectoryFeedback(long lSize, int nLen);
	bool DeleteRemoteFileFeedback(long lSize, int nLen);
	void RenameRemoteFileOrDirectory(LPSTR szCurrentName, LPSTR szNewName);
	bool RenameRemoteFileOrDirectoryFeedback(long lSize, int nLen);
	int  GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize);

	void SetTotalSize(HWND hwnd,DWORD dwTotalSize);
	void SetGauge(HWND hwnd,__int64 dwCount);
	void SetGlobalCount();
	void SetStatus(LPSTR szStatus);
	void ShowFileTransferWindow(bool fVisible);
	bool IsDirectoryGetIt(char* szName);
	bool GetSpecialFolderPath(int nId, char* szPath);
	void GetFriendlyFileSizeString(__int64 Size, char* szText);
	bool MyGetFileSize(char* szFilePath, ULARGE_INTEGER* n2FileSize);
	void InitListViewImagesList(HWND hListView);
    bool DeleteFileOrDirectory(TCHAR *srcpath); // 14 April 2008 jdp
	bool FileOrFolderExists(HWND fileListWnd, std::string fileOrFolder);
    bool UsingOldProtocol() { return m_ServerFTProtocolVersion == FT_PROTO_VERSION_OLD; }
    void StartFTSession();
    void EndFTSession();

	void InitFTTimer();
	void KillFTTimer();
	static void CALLBACK fpTimer(UINT uID,	UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1,	DWORD_PTR dw2);
	static void CALLBACK fpTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static void TimerCallback(FileTransfer* ft);

	static __int64 GetFileSizeFromString(char* szSize);
	static FILETIME GetFileTimeFromString(char* szFileSystemTime);

};

#endif // FILETRANSFER_H__



