// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#ifndef FILETRANSFER_H__
#define FILETRANSFER_H__
#pragma once

#include <list>
#include <string>
#include "common/ZipUnzip/MiniZipNG.h"

#define CONFIRM_YES 1
#define CONFIRM_YESALL 2
#define CONFIRM_NO 3
#define CONFIRM_NOALL 4

#define FT_PROTO_VERSION_OLD 1  // <= RC18 UltraVNC Server "fOldFTPRotocole" version
#define FT_PROTO_VERSION_2   2  // base File Transfer Protocol
#define FT_PROTO_VERSION_3   3  // new File Transfer Protocol session messages
#define FT_PROTO_VERSION_4   4  // Adds: rfbADirInaccessible, rfbRDirContentUnicode/rfbADirUnicode

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
	enum : LPARAM
	{
		FT_LPARAM_UNREADABLE = 0x00000001
	};
	bool				m_fAbort;
    bool                m_fUserAbortedFileTransfer; // 21 April 2008 jdp 
	bool                m_fUserForcedAbortedFileTransfer; // 21 April 2008 jdp 
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
	wchar_t             m_szFTParamTitle[128];
	wchar_t             m_szFTParamComment[64];
	wchar_t             m_szFTParam[256];
	WCHAR               m_szFTParamW[256];
	wchar_t             m_szFTConfirmTitle[128];
	wchar_t             m_szFTConfirmComment[364];
	int					m_nConfirmAnswer;
	CMiniZipNG			*m_pMiniZipNG;     // Unicode-aware zip
	bool				m_fApplyToAll;
	bool				m_fShowApplyToAll;
	wchar_t				m_szDeleteButtonLabel[64];
	wchar_t				m_szNewFolderButtonLabel[64];
	wchar_t				m_szRenameButtonLabel[64];
	wchar_t				m_szRefreshButtonLabel[64];

	// adzm 2009-08-02
	char				m_szLastLocalPath[_MAX_PATH];
	char				m_szLastRemotePath[_MAX_PATH];
	WCHAR				m_szLastRemotePathW[_MAX_PATH];
	WCHAR				m_szHeadlessRemotePathW[MAX_PATH * 4];
	bool				m_fHeadlessUpload;
	int                 m_nLastLocalAttemptItem;
	char                m_szLastLocalAttemptName[MAX_PATH + 2];
	WCHAR               m_szLastLocalAttemptNameW[MAX_PATH + 2];
	int                 m_nLastRemoteAttemptItem;
	char                m_szLastRemoteAttemptName[MAX_PATH + 2];
	WCHAR               m_szLastRemoteAttemptNameW[MAX_PATH + 2];

	__int64				m_nnFileSize;
	DWORD				m_dwCurrentValue;
	DWORD				m_dwCurrentPercent;

	DWORD				m_dwStartTick;

	// File Sending (upload)
	HANDLE				m_hSrcFile;
	char				m_szSrcFileName[MAX_PATH * 4];
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
	WCHAR				m_szSrcFileNameW[MAX_PATH * 4];    // Unicode local src path for upload
	char				m_szDestFileName[MAX_PATH * 4];
	WCHAR				m_szDestFileNameW[MAX_PATH * 4]; // Unicode version for correct rename on all locales
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

    int                 m_ServerFTProtocolVersion;
    bool                m_fServerSupportsUnicode; // true when server sent FT_PROTO_VERSION_4+
	UINT					m_nBlockSize;

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
	void DoUploadFile(LPCWSTR szLocalPath, LPCWSTR szRemotePath);
   	virtual ~FileTransfer();
	static BOOL CALLBACK FileTransferDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK LFBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK RFBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	int DoFTParamDialog(LPWSTR szTitle, LPWSTR szComment);
	static BOOL CALLBACK FTParamDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	int DoFTConfirmDialog(LPWSTR szTitle, LPWSTR szComment);
	static BOOL CALLBACK FTConfirmDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static int CALLBACK ListViewLocalCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort); /*TAW*/
	static int CALLBACK ListViewRemoteCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort); /*TAW*/

	void DisableButtons(HWND hWnd, bool X = true);
	void EnableButtons(HWND hWnd);
    void CheckButtonState(HWND hWnd);

	bool SendFile(long lSize, UINT nLen);
	bool SendFileChunk();
	bool FinishFileReception();
	bool UnzipPossibleDirectory(LPCWSTR szFileName);
	bool SendFiles(long lSize, UINT nLen);
	bool OfferNextFile();
	void ListRemoteDrives(HWND hWnd, UINT nLen);
	void ProcessFileTransferMsg(void);
	void RequestPermission();
	bool TestPermission(long lSize, int nVersion);
	void AddFileToFileList(HWND hWnd, int nListId, WIN32_FIND_DATA& fd, bool fLocalSide, const WCHAR* pszUnicodeFileName = NULL);
	void RequestRemoteDirectoryContent(HWND hWnd, LPCWSTR szPath);
	void RequestRemoteDrives();
	void RequestRemoteFile(LPSTR szRemoteFileName);
	bool OfferLocalFileW(LPCWSTR szSrcFileNameW);
	bool OfferLocalFile(LPSTR szSrcFileName);
	int  ZipPossibleDirectory(LPSTR szSrcFileName);
	bool ReceiveFile(unsigned long lSize, UINT nLen);
	bool ReceiveFileChunk(UINT nLen, int nSize);
	bool FinishFileSending();
	bool AbortFileReception();
	bool ReceiveFiles(unsigned long lSize, UINT nLen);
	bool RequestNextFile();
	bool ReceiveDestinationFileChecksums(int nSize, UINT nLen);
	void HighlightTransferedFiles(HWND hSrcList, HWND hDstList);
	void PopulateRemoteListBox(HWND hWnd, UINT nLen, bool fUnicodeEntry = false);
	void ReceiveDirectoryItem(HWND hWnd, UINT nLen, bool fUnicodeEntry = false);
	void FinishDirectoryReception();
	bool IsShortcutFolder(LPCWSTR szPath);
	bool ResolvePossibleShortcutFolder(HWND hWnd, LPSTR szFolder);
	void PopulateLocalListBox(HWND hWnd, LPSTR szPath);
	void PopulateLocalListBoxW(HWND hWnd, LPCWSTR szPathW); // Unicode wrapper
	void ListDrives(HWND hWnd);
	void RequestRemoteDirectoryContentW(HWND hWnd, LPCWSTR szPathW); // Unicode wrapper
	void CreateRemoteDirectory(LPSTR szDir);
	void CreateRemoteDirectoryPath(LPCWSTR szFullPathW);
	void CreateRemoteDirectoryW(LPCWSTR szDirW); // Unicode wrapper
    void DeleteRemoteFile(std::string szFile);
	bool CreateRemoteDirectoryFeedback(long lSize, UINT nLen);
	bool DeleteRemoteFileFeedback(long lSize, UINT nLen);
	void RenameRemoteFileOrDirectory(LPSTR szCurrentName, LPSTR szNewName);
	void RenameRemoteFileOrDirectoryW(LPCWSTR szCurrentNameW, LPCWSTR szNewNameW); // Unicode wrapper
	bool RenameRemoteFileOrDirectoryFeedback(long lSize, UINT nLen);
	int  GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize);

	void SetTotalSize(HWND hwnd,DWORD dwTotalSize);
	void SetGauge(HWND hwnd,__int64 dwCount);
	void SetGlobalCount();
	void SetStatus(LPWSTR szStatus);
	void ShowFileTransferWindow(bool fVisible);
	bool IsDirectoryGetItW(WCHAR* szName, int size);
	bool IsDirectoryGetIt(char* szName, int size);
	bool GetSpecialFolderPathW(int nId, WCHAR* szPathW);
	bool GetSpecialFolderPath(int nId, char* szPath);
	void GetFriendlyFileSizeString(__int64 Size, char* szText, int size);
	bool MyGetFileSize(LPCWSTR szFilePath, ULARGE_INTEGER* n2FileSize);
	void InitListViewImagesList(HWND hListView);
    bool DeleteFileOrDirectory(WCHAR *srcpath); // Unicode path support
	bool FileOrFolderExists(HWND fileListWnd, std::wstring fileOrFolder);
    bool UsingOldProtocol() { return m_ServerFTProtocolVersion == FT_PROTO_VERSION_OLD; }
    void StartFTSession();
    void EndFTSession();

	void InitFTTimer();
	void KillFTTimer();
	static void CALLBACK fpTimer(UINT uID,	UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1,	DWORD_PTR dw2);
	static void CALLBACK fpTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static void TimerCallback(FileTransfer* ft);

	static __int64 GetFileSizeFromStringW(WCHAR* szSize);
	static __int64 GetFileSizeFromString(char* szSize);
	static FILETIME GetFileTimeFromStringW(WCHAR* szFileSystemTime);
	static FILETIME GetFileTimeFromString(char* szFileSystemTime);
private:
	int nDirZipRet;
	bool rfbFileHeaderRequested;
	bool rfbFileTransferOfferRequested;
	char szRemoteFileNameRequested[MAX_PATH * 3];

};

#endif // FILETRANSFER_H__



