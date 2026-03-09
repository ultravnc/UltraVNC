// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// FileTransfer.cpp: implementation of the File Transfer class.

// sf@2002 - sf@2003 - sf@2004 - File Transfer
// This class handles all the File Transfer messages, events and procs, as well as the
// DialogBox which allows the user to browse Client ans Server disks-directories,
// and select some files to transfer between Client and Server.
//
// The GUI is very basic because I don't want to include MFC Classes in UltraVNC...
// I use only Windows SDK.
//
//
// The GUI is now quite bearable, but following modifs could be done one day or another
// - Add more columns to FileLists (File type, Attributes...)
// - Total progress should be based on total files' size instead of total number of files
// - Make the History persistent (file) so it's not lost each time the File Transfer Win is closed
// - Clean-up the code (duplicated parts, arrays and strings dimensions checks...)
// - Display the total files size in the currently displayed directory
// - Remember the current directories - Partially done: the File Transfer window can be minimized...


#include "stdhdrs.h"
#include "vncviewer.h"
#include "FileTransfer.h"
#include "Exception.h"
#include "commctrl.h"
#include "shlobj.h"
#ifdef _VCPKG
#include <zlib.h>
#include <zstd.h>
#else
#include "../zlib/zlib.h"
#include "../zstd/lib/zstd.h"
#endif
#include "Log.h"
#include <string>
#include <vector>
#include "common/win32_helpers.h"
#include "shlwapi.h"
#include "UltraVNCHelperFunctions.h"
#include <limits>

using namespace helper;
extern HINSTANCE m_hInstResDLL;

#pragma comment(lib, "Shlwapi.lib")

// [v1.0.2-jp1 fix] yak!'s File transfer patch
// Simply forward strchr() and strrchr() to _mbschr() and _mbsrchr() to avoid 0x5c problem, respectively.
// Probably, it is better to write forward functions internally.
#include <mbstring.h>
#define strchr(a, b) reinterpret_cast<char*>(_mbschr(reinterpret_cast<unsigned char*>(a), b))
#define strrchr(a, b) reinterpret_cast<char*>(_mbsrchr(reinterpret_cast<unsigned char*>(a), b))

// These strings contain all the translated File Transfer messages 
extern char sz_H1[64];
extern char sz_H2[64];
extern char sz_H3[128];
extern char sz_H4[64];
extern char sz_H5[64];
extern char sz_H6[64];
extern char sz_H7[64];
extern char sz_H8[64];
extern char sz_H9[64];
extern char sz_H10[64];
extern char sz_H11[64];
extern char sz_H12[64];
extern char sz_H13[64];
extern char sz_H14[64];
extern char sz_H15[64];
extern char sz_H16[64];
extern char sz_H17[64];
extern char sz_H18[64];
extern char sz_H19[64];
extern char sz_H20[64];
extern char sz_H21[64];
extern char sz_H22[64];
extern char sz_H23[64];
extern char sz_H24[64];
extern char sz_H25[64];
extern char sz_H26[64];
extern char sz_H27[64];
extern char sz_H28[64];
extern char sz_H29[64];
extern char sz_H30[64];
extern char sz_H31[64];
extern char sz_H32[64];
extern char sz_H33[64];
extern char sz_H34[64];
extern char sz_H35[64];
extern char sz_H36[64];
extern char sz_H37[64];
extern char sz_H38[128];
extern char sz_H39[64];
extern char sz_H40[64];
extern char sz_H41[64];
extern char sz_H42[64];
extern char sz_H43[128];
extern char sz_H44[64];
extern char sz_H45[64];
extern char sz_H46[128];
extern char sz_H47[64];
extern char sz_H48[64];
extern char sz_H49[64];
extern char sz_H50[64];
extern char sz_H51[64];
extern char sz_H52[64];
extern char sz_H53[64];
extern char sz_H54[64];
extern char sz_H55[64];
extern char sz_H56[64];
extern char sz_H57[64];

// Folder Transfer messages
extern char sz_H58[64];
extern char sz_H59[64];
extern char sz_H60[64];
extern char sz_H61[64];
extern char sz_H62[128];
extern char sz_H63[64];
extern char sz_H64[64];
extern char sz_H65[64];
extern char sz_H66[64];
extern char sz_H67[64];
extern char sz_H68[128];
extern char sz_H69[64];
extern char sz_H70[64];
extern char sz_H71[64];
extern char sz_H72[128];
extern char sz_H73[64];

// File/dir Rename messages
extern char sz_M1[64];
extern char sz_M2[64];
extern char sz_M3[64];
extern char sz_M4[64];
extern char sz_M5[64];
extern char sz_M6[64];
extern char sz_M7[64];
extern char sz_M8[64];

// 14 April 2008 jdp
extern char sz_H94[64];
extern char sz_H95[64];
extern char sz_H96[64];
extern char sz_H97[64];
extern char sz_H98[64];
extern char sz_H99[64];
extern char sz_E1[64];
extern char sz_E2[64];
extern char sz_H100[64];
extern char sz_H101[64];
extern char sz_H102[128];
#define CB_SETHORIZONTALEXTENT 0x015e
typedef BOOL (WINAPI *PGETDISKFREESPACEEX)(LPCSTR,PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);


#define FT_USE_MMTIMER

static FileTransfer* g_FileTransferSingleton = NULL;

HWND hFTWnd = 0;
static std::string make_temp_filename(const char *szRemoteFileName)
{
    // don't add prefix for directory transfers.
    char *pFileName = strrchr(const_cast<char *>(szRemoteFileName), '\\');
    if (pFileName != NULL)
        ++pFileName;
    else pFileName = const_cast<char *>(szRemoteFileName);
    if (strncmp(pFileName, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)) == 0)
        return szRemoteFileName;

    std::string tmpName(rfbPartialFilePrefix);
    tmpName += szRemoteFileName;
    return tmpName;
}

static std::string get_real_filename(const char *destFileName)
{
    std::string name (destFileName);
    std::string::size_type pos;

    pos = name.find(rfbPartialFilePrefix);
    if (pos != std::string::npos)
        name.erase(pos, sz_rfbPartialFilePrefix);
 
    return name;
}
bool FileTransfer::DeleteFileOrDirectory(WCHAR *srcpath)
{
    WCHAR path[MAX_PATH + 2]; // room for double null terminator required by SHFileOperationW
    memset(path, 0, sizeof path);
    wcsncpy_s(path, srcpath, MAX_PATH);

    SHFILEOPSTRUCTW op;
    memset(&op, 0, sizeof(SHFILEOPSTRUCTW));
    op.hwnd = hWnd;
    op.wFunc = FO_DELETE;
    op.pFrom  = path;
    op.fFlags =  FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_ALLOWUNDO;

    // MSDN says to not look at the error code, just treat 0 as SUCCESS, nonzero is failure.
    // Do not use GetLastError with the return values of this function.
    int result = SHFileOperationW(&op);
    if (result == 0)
        return true;

    // Fallback: SHFileOperationW can fail for reserved shell names or when Recycle Bin
    // is unavailable. Try direct deletion.
    DWORD attr = GetFileAttributesW(srcpath);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return RemoveDirectoryW(srcpath) != 0;
    return DeleteFileW(srcpath) != 0;
}

//
//
//
FileTransfer::FileTransfer(VNCviewerApp *l_pApp, ClientConnection *pCC)
{
//	vnclog.Print(0, _T("FileTransfer\n"));
	m_pApp	= l_pApp;
	m_pCC	= pCC;
	m_fAbort = false;
    m_fUserAbortedFileTransfer = false;
	m_fAborted = false;
	m_FilesList.clear();
	m_nFilesToTransfer = 0;
	m_nFilesTransfered = 0;
	m_fFileCommandPending = false;
	m_fFileTransferRunning = false;
	m_fFileDownloadRunning = false;
	m_fDirectoryReceptionRunning = false;
	m_fVisible = true;
	m_fFTAllowed = false;
	m_timer = 0;
	m_pZipUnZip = new CZipUnZip32();
	m_lpCSBuffer = NULL;
	m_nCSOffset = 0;
	m_nCSBufferSize = 0;
	m_nDeleteCount = 0;
	memset(m_szDeleteButtonLabel, 0, sizeof(m_szDeleteButtonLabel));
	memset(m_szNewFolderButtonLabel, 0, sizeof(m_szNewFolderButtonLabel));
	memset(m_szRenameButtonLabel, 0, sizeof(m_szRenameButtonLabel));
    m_ServerFTProtocolVersion = FT_PROTO_VERSION_2;
    m_fServerSupportsUnicode = false;
	m_nBlockSize = 8192;
	m_dwCurrentValue = 0;
	m_dwCurrentPercent = 0;
	m_fSendFileChunk = false;
    m_hSrcFile = INVALID_HANDLE_VALUE;
    m_hDestFile = INVALID_HANDLE_VALUE;
    m_fFileUploadRunning = false;
    m_fFileDownloadRunning = false;
    hWnd = 0;
    m_fFocusLocal = true;
    memset(m_szFTParamTitle, 0, sizeof m_szFTParamTitle);
    memset(m_szFTParamComment, 0, sizeof m_szFTParamComment);
    memset(m_szFTParam, 0, sizeof m_szFTParam);
    memset(m_szFTConfirmTitle, 0, sizeof m_szFTConfirmTitle);
    memset(m_szFTConfirmComment, 0, sizeof m_szFTConfirmComment);
    m_nConfirmAnswer = 0;
    m_fApplyToAll = false;
    m_fShowApplyToAll = true;
    m_nnFileSize = 0;
    memset(m_szSrcFileName, 0, sizeof m_szSrcFileName);
    memset(m_szSrcFileNameW, 0, sizeof m_szSrcFileNameW);
    memset(m_szDestFileNameW, 0, sizeof m_szDestFileNameW);
    m_fEof = false;
    m_fFileUploadError = false;
    m_fCompress = true;
    m_nFileCount = 0;
    memset(m_szFileSpec, 0, sizeof m_szFileSpec);
    memset(m_szDestFileName, 0, sizeof m_szDestFileName);
    m_dwNbReceivedPackets = 0;
    m_dwNbBytesRead = 0;
    m_dwNbBytesWritten = 0;
    m_dwTotalNbBytesRead = 0;
    m_dwTotalNbBytesWritten = 0;
    m_dwTotalNbBytesNotReallyWritten = 0;
    m_nPacketCount = 0;
    m_fPacketCompressed = false;
    m_fFileDownloadError = false;
    memset(m_szIncomingFileTime, 0, sizeof m_szIncomingFileTime);
    m_nNotSent = 0;
    m_dwLastChunkTime = 0;
    m_maxHistExtent = 0;
	m_mmRes = -1; 
	m_timerID = 0xFFFFFFFF;
	m_dwStartTick = GetTickCount();

	// adzm 2009-08-02
	memset(m_szLastLocalPath, 0, sizeof(m_szLastLocalPath));
	memset(m_szLastRemotePath, 0, sizeof(m_szLastRemotePath));
	memset(m_szLastRemotePathW, 0, sizeof(m_szLastRemotePathW));
	m_nLastLocalAttemptItem = -1;
	memset(m_szLastLocalAttemptName, 0, sizeof(m_szLastLocalAttemptName));
	m_nLastRemoteAttemptItem = -1;
	memset(m_szLastRemoteAttemptName, 0, sizeof(m_szLastRemoteAttemptName));

	for (int i = 0; i<3; i++)
	{
		bSortDirectionsL[i] = false;
		bSortDirectionsR[i] = false;
	}
    // 16 April 2008 jdp
    // load richedit so the path display can handly mbcs
    m_hRichEdit = LoadLibraryEx( "RICHED32.DLL", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!m_hRichEdit)
	{
		yesUVNCMessageBox(m_hInstResDLL, NULL, sz_E1, sz_E2, MB_ICONEXCLAMATION );
    }
	InitializeCriticalSection(&crit);
	rfbFileHeaderRequested = false;
	rfbFileTransferOfferRequested = false;
}

//
//
//
FileTransfer::~FileTransfer()
{
//	vnclog.Print(0, _T("nFileTransfer\n"));
	KillFTTimer();
	EnterCriticalSection(&crit);
	m_fFileCommandPending = false;
	m_fFileTransferRunning = false;
	m_FilesList.clear();
	if (m_pZipUnZip) delete m_pZipUnZip;
	if (m_lpCSBuffer != NULL) 
	{
		delete [] m_lpCSBuffer;
		m_lpCSBuffer = NULL;
	}
    // 16 April 2008 jdp
	if (m_hRichEdit != NULL) FreeLibrary(m_hRichEdit);
	LeaveCriticalSection(&crit);
	DeleteCriticalSection(&crit);
}


void FileTransfer::InitFTTimer()
{	
#ifdef FT_USE_MMTIMER
	if (m_mmRes != -1) return;

	m_fSendFileChunk = false;
	m_mmRes = timeSetEvent( 10, 0, (LPTIMECALLBACK)fpTimer, (DWORD_PTR)this, TIME_PERIODIC );
#else

	if (m_timerID != 0xFFFFFFFF) {
		return;
	}

	g_FileTransferSingleton = this;
	m_fSendFileChunk = false;
	m_timerID = SetTimer(NULL, 0, USER_TIMER_MINIMUM, (TIMERPROC)fpTimerProc);
#endif
}


void FileTransfer::KillFTTimer()
{
#ifdef FT_USE_MMTIMER
	if (m_mmRes!=-1) timeKillEvent(m_mmRes);
	m_mmRes = -1;
#else
	
	if (m_timerID == 0xFFFFFFFF) {
		return;
	}

	KillTimer(NULL, m_timerID);
	g_FileTransferSingleton = NULL;
	m_timerID = 0xFFFFFFFF;
#endif
}

void CALLBACK FileTransfer::fpTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (g_FileTransferSingleton != NULL) {
		TimerCallback(g_FileTransferSingleton);
	}
}

void CALLBACK FileTransfer::fpTimer(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	FileTransfer* ft = (FileTransfer *) dwUser;

	TimerCallback(ft);
}

void FileTransfer::TimerCallback(FileTransfer* ft)
{
	EnterCriticalSection(&ft->crit);
	if (!ft->m_fFileUploadRunning) 
		{
			LeaveCriticalSection(&ft->crit);
			return;
		}

	if (!ft->m_fSendFileChunk)
	{
		ft->m_fSendFileChunk = true;
		ft->SendFileChunk();
		ft->m_fSendFileChunk = false;
	}
	LeaveCriticalSection(&ft->crit);
}

//
//
//
void FileTransfer::ShowFileTransferWindow(bool fVisible)
{
//	vnclog.Print(0, _T("ShowFileTransferWindow\n"));
	bool bChanged = false;
	if (m_fVisible != fVisible) {
		bChanged = true;
	}

	ShowWindow(hWnd, fVisible ? SW_RESTORE : SW_MINIMIZE);
	if (fVisible) {
		SetForegroundWindow(hWnd);
	}
	// Put the File Transfer Windows always on Top if fullscreen
	if (fVisible && m_pCC->InFullScreenMode())
	{
		RECT Rect;
		GetWindowRect(hWnd, &Rect);
		SetWindowPos(hWnd, 
					HWND_TOPMOST,
					Rect.left,
					Rect.top,
					Rect.right - Rect.left,
					Rect.bottom - Rect.top,
					SWP_SHOWWINDOW);
	}

	m_fVisible = fVisible; // This enables screen updates to be processed in ClientConnection
	// Refresh screen view if File Transfer window has been hidden
	//adzm 2010-09 - all socket writes must remain on a single thread, but we only need an async request here
	if (bChanged && !fVisible)
		m_pCC->SendAppropriateFramebufferUpdateRequest(true);
}


//
// Simply the classic Windows message processing
//
bool PseudoYield(HWND hWnd)
{
//	vnclog.Print(0, _T("PseudoYield\n"));
	MSG msg;
	while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_CLOSE) 
            return FALSE;
	}
	return TRUE;
}



//
//	ProcessFileTransferMsg
//
//  Here we process all incoming FileTransferMsg stuff
//  coming from the server.
//  The server only sends File Transfer data when requested
//  by the client. Possible request are:
//
//  - Send the list of your drives
//  - Send the content of a directory
//  - Send a file
//  - Accept a file
//  - ...
// 
//  We use the main ClientConnection thread and its
//  rfb message reception loop.
//  This function is called by the rfb message processing thread.
//  Thus it's safe to call the ReadExact and ReadString 
//  functions in the functions that are called from here:
//  PopulateRemoteListBox, ReceiveFile
// 
void FileTransfer::ProcessFileTransferMsg(void)
{
//	vnclog.Print(0, _T("ProcessFileTransferMsg\n"));
	// Save encoder buffer state to prevent corruption during file transfer
	bool saved_fReadFromNetRectBuf = m_pCC->m_fReadFromNetRectBuf;
	int saved_nNetRectBufOffset = m_pCC->m_nNetRectBufOffset;
	int saved_nReadSize = m_pCC->m_nReadSize;
	
	// Ensure file transfer reads directly from socket, not from encoder buffers
	m_pCC->m_fReadFromNetRectBuf = false;
	m_pCC->m_nNetRectBufOffset = 0;
	
	rfbFileTransferMsg ft;
	m_pCC->ReadExact(((char *) &ft) + m_pCC->m_nTO, sz_rfbFileTransferMsg - m_pCC->m_nTO);

	switch (ft.contentType)
	{
	// Response to a rfbDirContentRequest request:
	// some directory data is received from the server
    case rfbFileTransferProtocolVersion:
        {
            int proto_ver = ft.contentParam;
            if ((proto_ver >= FT_PROTO_VERSION_OLD) && (proto_ver <= FT_PROTO_VERSION_4))
                m_ServerFTProtocolVersion = proto_ver;
            m_fServerSupportsUnicode = (m_ServerFTProtocolVersion >= FT_PROTO_VERSION_4);

			if (m_ServerFTProtocolVersion >= FT_PROTO_VERSION_3)
			{
				CARD32 serverBlockSize = Swap32IfLE(ft.size);
				if (serverBlockSize >= 4096 && serverBlockSize <= 1048576)
					m_nBlockSize = serverBlockSize;
			}
        }
        break;

	case rfbDirPacket:
		{
		// Strip the unicode flag to get the base content param
		const CARD16 rawParam = ft.contentParam;
		const bool fUnicodeEntry = (rawParam & rfbADirUnicode) != 0;
		const CARD16 baseParam  = rawParam & ~(CARD16)rfbADirUnicode;
		switch (baseParam)
		{
		// Response to a rfbRDrivesList request
		case rfbADrivesList:
			ListRemoteDrives(hWnd, Swap32IfLE(ft.length));
			m_fFileCommandPending = false;
			break;

		// Server signals the folder cannot be read (permission denied, media error, etc.)
		// Old servers never send this value; old viewers treat it as end-of-dir (graceful degradation).
		case rfbADirInaccessible:
			if (m_fDirectoryReceptionRunning)
			{
				FinishDirectoryReception();
				m_fFileCommandPending = false;
			}
			PopulateRemoteListBox(hWnd, 0); // nLen=0 triggers the "inaccessible" UI path
			break;

		// Response to a rfbRDirContent request 
		case rfbADirectory:
			if (nDirZipRet == 1)
			{
				SetStatus("Folder unzipped.");
				nDirZipRet = 0;
			}
			// fall through
		case rfbAFile:
			if (!m_fDirectoryReceptionRunning)
				PopulateRemoteListBox(hWnd, Swap32IfLE(ft.length), fUnicodeEntry);
			else
				ReceiveDirectoryItem(hWnd, Swap32IfLE(ft.length), fUnicodeEntry);
			break;
		default: // End-of-directory marker (contentParam==0) or unknown
			if (m_fDirectoryReceptionRunning)
			{
				FinishDirectoryReception();
				m_fFileCommandPending = false;
			}
			break;

		}
		}
		break;

	// In response to a rfbFileTransferRequest request
	// A file is received from the server.
	case rfbFileHeader:
		if (rfbFileHeaderRequested) {
			ReceiveFiles(Swap32IfLE(ft.size), Swap32IfLE(ft.length));
			rfbFileHeaderRequested = false;
		}
		break;

	// In response to a rfbFileTransferOffer request
	// The server can send the checksums of the destination file before sending a ack through
	// rfbFileAcceptHeader (only if the destination file already exists and is accessible)
	case rfbFileChecksums:
			ReceiveDestinationFileChecksums(Swap32IfLE(ft.size), Swap32IfLE(ft.length));
			m_pCC->SetRecvTimeout();
		break;

	// In response to a rfbFileTransferOffer request
	// A ack or nack is received from the server.
	case rfbFileAcceptHeader:
		if (rfbFileTransferOfferRequested) {
			SendFiles(Swap32IfLE(ft.size), Swap32IfLE(ft.length));
			rfbFileTransferOfferRequested = false;
		}
		break;

	// Response to a command
	case rfbCommandReturn:
		switch (ft.contentParam)
		{
		case rfbADirCreate:
			CreateRemoteDirectoryFeedback(Swap32IfLE(ft.size), Swap32IfLE(ft.length));
			m_fFileCommandPending = false;
			break;

		case rfbAFileDelete:
			DeleteRemoteFileFeedback(Swap32IfLE(ft.size), Swap32IfLE(ft.length));
			m_fFileCommandPending = false;
			break;

		case rfbAFileRename:
			RenameRemoteFileOrDirectoryFeedback(Swap32IfLE(ft.size), Swap32IfLE(ft.length));
			m_fFileCommandPending = false;
			break;
		}
		break;
		
	// Should never be handled here but in the File Transfer Loop
	case rfbFilePacket:
        m_pCC->SetRecvTimeout();
		ReceiveFileChunk(Swap32IfLE(ft.length), Swap32IfLE(ft.size));
		// adzm 2010-09
        m_pCC->SendKeepAlive(false, true);
		break;

	// Should never be handled here but in the File Transfer Loop
	case rfbEndOfFile:
		FinishFileReception();
		break;

	// Abort current File Transfer
	// For versions <= RC18 we also use it to test if we're allowed to use File Transfer on the server
	case rfbAbortFileTransfer:
		// AbortFileDownload();
		if (m_fFileDownloadRunning)
		{
			m_fFileDownloadError = true;
			FinishFileReception();
		}
		else
		{
			// We want the viewer to be backward compatible with UltraVNC Server running the old File Transfer Protocol
            m_ServerFTProtocolVersion = FT_PROTO_VERSION_OLD; // Old permission method -> it's a <= RC18 UltraVNC Server
			m_nBlockSize = 4096; // Old packet size value...
			ShowWindow(GetDlgItem(hWnd, IDC_RENAME_B), SW_HIDE);

			TestPermission(Swap32IfLE(ft.size), 0);
		}
		break;
	
	// New File Transfer handshaking/permission method (from RC19)
	case rfbFileTransferAccess:
		TestPermission(Swap32IfLE(ft.size), ft.contentParam);
		break;

	default:
		break;
	}
	
	// Restore encoder buffer state after file transfer processing
	m_pCC->m_fReadFromNetRectBuf = saved_fReadFromNetRectBuf;
	m_pCC->m_nNetRectBufOffset = saved_nNetRectBufOffset;
	m_pCC->m_nReadSize = saved_nReadSize;
}


//
// request File Transfer permission 
//
void FileTransfer::RequestPermission()
{
//	vnclog.Print(0, _T("RequestPermission\n"));

    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	// Versions <= RC18 method
	ft.contentType = rfbAbortFileTransfer; 
	// ft.contentParam = 0; 
	ft.contentParam = rfbFileTransferVersion; // Old viewer will send 0
	// New method can't be used yet as we want backward compatibility (new UltraVNC Viewer File Transfer must 
	// work with old UltraVNC Server File Transfer
	// ft.contentType = rfbFileTransferAccess; 
	// ft.contentParam = rfbFileTransferVersion;
	ft.length = 0;
	ft.size = 0;
    m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	return;
}

void FileTransfer::StartFTSession()
{
    if (m_ServerFTProtocolVersion < FT_PROTO_VERSION_3)
        return;

    rfbFileTransferMsg ft;
    memset (&ft, 0, sizeof ft);
    ft.type = rfbFileTransfer;
    ft.contentType = rfbFileTransferSessionStart;
    m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
}

void FileTransfer::EndFTSession()
{
    if (m_ServerFTProtocolVersion < FT_PROTO_VERSION_3 || !m_pCC->m_running)
        return;

    rfbFileTransferMsg ft;
    memset (&ft, 0, sizeof ft);
    ft.type = rfbFileTransfer;
    ft.contentType = rfbFileTransferSessionEnd;
    m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
}
//
// Free lParam (wstring*) memory for all items in a ListView, then clear it
// (0x00000001 == FT_LPARAM_UNREADABLE, kept as literal to avoid class-scope dependency)
//
static void FTListViewClear(HWND hWndList)
{
	int nCount = ListView_GetItemCount(hWndList);
	for (int i = 0; i < nCount; i++)
	{
		LVITEMW ItemW;
		memset(&ItemW, 0, sizeof(ItemW));
		ItemW.mask = LVIF_PARAM;
		ItemW.iItem = i;
		ItemW.iSubItem = 0;
		if (SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM)&ItemW))
		{
			std::wstring* p = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)0x00000001);
			delete p;
		}
	}
	ListView_DeleteAllItems(hWndList);
}

//
// Test if we are allowed to access File Transfer
//
bool FileTransfer::TestPermission(long lSize, int nVersion)
{
//	vnclog.Print(0, _T("TestPermission\n"));
	if (lSize == -1)
	{
		m_fFTAllowed = false;
		HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
		SendDlgItemMessage(hWnd, IDC_REMOTE_DRIVECB, LB_RESETCONTENT, 0, 0L);
		FTListViewClear(hWndRemoteList);
		SetDlgItemText(hWnd, IDC_CURR_REMOTE, sz_H1);
		SetDlgItemText(hWnd, IDC_REMOTE_STATUS, sz_H2);
		SetStatus(sz_H3);
		DisableButtons(hWnd, false);
		ShowWindow(GetDlgItem(hWnd, IDCANCEL), SW_SHOW);
		ShowWindow(GetDlgItem(hWnd, IDCANCEL2), SW_SHOW);
	}
	else
	{
		m_fFTAllowed = true;
        StartFTSession();
		RequestRemoteDrives();
		SetStatus(sz_H4);
	}

	return true;
}


//
// Receive all the files that are referenced in m_FilesList
//
bool FileTransfer::ReceiveFiles(unsigned long lSize, UINT nLen)
{
//	vnclog.Print(0, _T("ReceiveFiles\n"));
	// Receive the incoming file
	m_nFilesTransfered++;
	SetGlobalCount();
	if (!ReceiveFile(lSize, nLen))
		RequestNextFile();

	return true;
}

//
// A file has just been received
// Request the following one if any
//
bool FileTransfer::RequestNextFile()
{
//	vnclog.Print(0, _T("RequestNextFile\n"));

	SetGlobalCount();

	m_iFile++; // go to next file in the list of files to receive

	HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

	// If one more file has to be received, ask for it !
	if (m_iFile != m_FilesList.end() && !m_fAbort)
	{
		// Retrieve Unicode filename from lParam
		LVITEMW ItemW;
		memset(&ItemW, 0, sizeof(ItemW));
		ItemW.mask = LVIF_PARAM;
		ItemW.iItem = *m_iFile;
		ItemW.iSubItem = 0;
		SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
		std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);

		WCHAR szDstFileW[MAX_PATH];
		GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szDstFileW, MAX_PATH);
		if (!wcslen(szDstFileW)) return false; // no destination dir selected
		if (pNameW) wcscat_s(szDstFileW, pNameW->c_str());

		// Convert full Unicode path to UTF-8 for sending to server
		char szDstFileUTF8[MAX_PATH * 3];
		WideCharToMultiByte(CP_UTF8, 0, szDstFileW, -1, szDstFileUTF8, MAX_PATH * 3, NULL, NULL);
		RequestRemoteFile(szDstFileUTF8);
	}
	else // All the files have been processed and hopefully received
	{
		// Refresh the local list so new files are displayed and highlighted
		FTListViewClear(hWndLocalList);
		PopulateLocalListBox(hWnd, "");

		if (m_fAbort)
			SetStatus(sz_H5);
		else  if (!m_fFileDownloadError)
			SetStatus(sz_H6);

		EnableButtons(hWnd);

		ShowFileTransferWindow(true);
		Sleep(1000);
		//MessageBeep(-1);

		// Unlock 
		m_fFileCommandPending = false;
	}

	return true;
}


//
// Send all the files that are referenced in m_FilesList
//
bool FileTransfer::SendFiles(long lSize, UINT nLen)
{
	InitFTTimer(); // sf@2005

//	vnclog.Print(0, _T("SendFiles\n"));
	// Receive the incoming file
	m_nFilesTransfered++;
	SetGlobalCount();
	if (!SendFile(lSize, nLen))
		OfferNextFile();

	return true;
}


bool FileTransfer::OfferNextFile()
{
//	vnclog.Print(0, _T("OfferNextFile\n"));
	SetGlobalCount();

	m_iFile++; // go to next file in the list of files to send

	HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

	// If one more file has to be sent, offer it !
	if (m_iFile != m_FilesList.end() && !m_fAbort)
	{
		// Retrieve Unicode filename from lParam
		LVITEMW ItemW;
		memset(&ItemW, 0, sizeof(ItemW));
		ItemW.mask = LVIF_PARAM;
		ItemW.iItem = *m_iFile;
		ItemW.iSubItem = 0;
		SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
		std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);

		// Build full local path in Unicode
		WCHAR szSrcFileW[MAX_PATH];
		GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szSrcFileW, MAX_PATH);
		if (!wcslen(szSrcFileW)) return false; // no src dir selected
		if (pNameW) wcscat_s(szSrcFileW, pNameW->c_str());

		// Store Unicode path for OfferLocalFile (avoids CP_ACP corruption of Chinese filenames)
		wcscpy_s(m_szSrcFileNameW, szSrcFileW);
		// CP_ACP version for legacy code paths
		char szSrcFile[MAX_PATH];
		WideCharToMultiByte(CP_ACP, 0, szSrcFileW, -1, szSrcFile, MAX_PATH, NULL, NULL);

		if (!OfferLocalFile(szSrcFile))
		   SendFiles(-1, 0);
	}
	else // All the files have been processed and hopefully received
	{
		// Refresh the remote list so new files are displayed and highlighted
		FTListViewClear(hWndRemoteList);
		RequestRemoteDirectoryContent(hWnd, "");

		if (m_fAbort)
			SetStatus(sz_H7);
		else if (!m_fFileUploadError)
			SetStatus(sz_H6);

//		EnableButtons(hWnd);

		ShowFileTransferWindow(true);
		Sleep(1000);
		//MessageBeep(-1);
		if (!m_fAbort && nDirZipRet == 1)
			SetStatus("Decompressing folder(s). Please wait...");

		// Unlock 
		m_fFileCommandPending = false;

		KillFTTimer(); // sf@2005
	}

	return true;
}


//
// Format file size so it is user friendly to read
// 
void FileTransfer::GetFriendlyFileSizeString(__int64 Size, char* szText, int size)
{
	szText[0] = '\0';
	if( Size > (1024*1024*1024) )
	{
		__int64 lRest = (Size % (1024*1024*1024));
		Size /= (1024*1024*1024);
		_snprintf_s(szText, size, 256, "%u.%2.2lu Gb", (unsigned long)Size, (unsigned long)(lRest * 100 / 1024 / 1024 / 1024));
	}
	else if( Size > (1024*1024) )
	{
		unsigned long lRest = (Size % (1024*1024));
		Size /= (1024*1024);
		_snprintf_s(szText, size, 256, "%u.%2.2lu Mb", (unsigned long)Size, lRest * 100 / 1024 / 1024);
	}
	else if ( Size > 1024 )
	{
		unsigned long lRest = Size % (1024);
		Size /= 1024;
		_snprintf_s(szText, size, 256, "%u.%2.2lu Kb", (unsigned long)Size, lRest * 100 / 1024);
	}
	else
	{
		_snprintf_s(szText, size, 256, "%u bytes", (unsigned long)Size);
	}
}




//
// GetFileSize() doesn't handle files > 4GBytes...
// GetFileSizeEx() doesn't exist under Win9x...
// So let's write our own function.
// 
bool FileTransfer::MyGetFileSize(char* szFilePath, ULARGE_INTEGER *n2FileSize)
{
	WIN32_FIND_DATAW fdW;
	HANDLE ff;

	// Convert path to Unicode for proper Chinese character support
	WCHAR szFilePathW[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, szFilePath, -1, szFilePathW, MAX_PATH);

	SetErrorMode(SEM_FAILCRITICALERRORS); // No popup please !
	ff = FindFirstFileW(szFilePathW, &fdW);
	SetErrorMode( 0 );

	if (ff == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	FindClose(ff);

	(*n2FileSize).LowPart = fdW.nFileSizeLow;
	(*n2FileSize).HighPart = fdW.nFileSizeHigh;
	(*n2FileSize).QuadPart = (((__int64)fdW.nFileSizeHigh) << 32 ) + fdW.nFileSizeLow;
	
	return true;
}


//
// Add a file line to a ListView
// 
void FileTransfer::AddFileToFileList(HWND hWnd, int nListId, WIN32_FIND_DATA& fd, bool fLocalSide, const WCHAR* pszUnicodeFileName)
{
//	vnclog.Print(0, _T("AddFileToFileList\n"));
	HWND hWndList = GetDlgItem(hWnd, nListId);

	// Build Unicode filename: use pszUnicodeFileName if provided, else convert fd.cFileName from CP_ACP
	WCHAR szFileNameW[MAX_PATH + 2];
	if (pszUnicodeFileName)
		wcscpy_s(szFileNameW, pszUnicodeFileName);
	else
		MultiByteToWideChar(CP_ACP, 0, fd.cFileName, -1, szFileNameW, MAX_PATH);

	if (((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && wcscmp(szFileNameW, L"."))
		||
		(!wcscmp(szFileNameW, L".."))
	   )
	{
		// Build "[dirname]" display name in Unicode
		WCHAR szDirNameW[MAX_PATH + 4];
		WCHAR szPrefixW[4], szSuffixW[4];
		MultiByteToWideChar(CP_ACP, 0, rfbDirPrefix, -1, szPrefixW, 4);
		MultiByteToWideChar(CP_ACP, 0, rfbDirSuffix, -1, szSuffixW, 4);
		swprintf_s(szDirNameW, L"%s%s%s", szPrefixW, szFileNameW, szSuffixW);

		// Insert using Unicode message to support Chinese filenames regardless of dialog mode
		LVITEMW ItemW;
		memset(&ItemW, 0, sizeof(ItemW));
		ItemW.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		ItemW.iItem = 0;
		ItemW.iSubItem = 0;
		ItemW.iImage = 0;
		ItemW.pszText = szDirNameW;
		// Store heap-allocated Unicode filename in lParam (freed when list is cleared)
		ItemW.lParam = (LPARAM)(new std::wstring(szFileNameW));
		int nItem = (int)SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&ItemW);

		// Type subitem
		LVITEMW SubW;
		memset(&SubW, 0, sizeof(SubW));
		SubW.mask = LVIF_TEXT;
		SubW.iItem = nItem;
		SubW.iSubItem = 1;
		SubW.pszText = L"Folder";
		SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&SubW);

		// For remote items: server sets dwReserved0=rfbFD_INACCESSIBLE when subfolder can't be read
		if (!fLocalSide && wcscmp(szFileNameW, L"..") && fd.dwReserved0 == rfbFD_INACCESSIBLE)
		{
			LVITEMW flagItem;
			memset(&flagItem, 0, sizeof(flagItem));
			flagItem.mask = LVIF_PARAM;
			flagItem.iItem = nItem;
			flagItem.iSubItem = 0;
			SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM)&flagItem);
			flagItem.lParam |= FT_LPARAM_UNREADABLE;
			SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&flagItem);
		}

		// Proactively check if local folder is accessible
		if (fLocalSide && wcscmp(szFileNameW, L".."))
		{
			WCHAR szCurrPathW[MAX_PATH];
			GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szCurrPathW, MAX_PATH);
			if (wcslen(szCurrPathW) > 0)
			{
				WCHAR szProbeW[MAX_PATH + 4];
				swprintf_s(szProbeW, L"%s%s\\*", szCurrPathW, szFileNameW);
				WIN32_FIND_DATAW fdProbeW;
				SetErrorMode(SEM_FAILCRITICALERRORS);
				HANDLE hProbe = FindFirstFileW(szProbeW, &fdProbeW);
				SetErrorMode(0);
				if (hProbe == INVALID_HANDLE_VALUE)
				{
					LVITEMW flagItem;
					memset(&flagItem, 0, sizeof(flagItem));
					flagItem.mask = LVIF_PARAM;
					flagItem.iItem = nItem;
					flagItem.iSubItem = 0;
					SendMessageW(hWndList, LVM_GETITEMW, 0, (LPARAM)&flagItem);
					flagItem.lParam |= FT_LPARAM_UNREADABLE;
					SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&flagItem);
				}
				else
				{
					FindClose(hProbe);
				}
			}
		}
	}
	else if (wcscmp(szFileNameW, L".")) // Test actually Not necessary for remote list
	{
		// Insert file using Unicode message
		LVITEMW ItemW;
		memset(&ItemW, 0, sizeof(ItemW));
		ItemW.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		ItemW.iItem = 0;
		ItemW.iSubItem = 0;
		ItemW.iImage = 1;
		ItemW.pszText = szFileNameW;
		// Store heap-allocated Unicode filename in lParam (freed when list is cleared)
		ItemW.lParam = (LPARAM)(new std::wstring(szFileNameW));
		int nItem = (int)SendMessageW(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&ItemW);

		// Size
		__int64 Size = ( ((__int64)fd.nFileSizeHigh) << 32 ) + fd.nFileSizeLow;
		char szText[256];
		GetFriendlyFileSizeString(Size, szText, 256);
		WCHAR szTextW[256];
		MultiByteToWideChar(CP_ACP, 0, szText, -1, szTextW, 256);

		LVITEMW SubW;
		memset(&SubW, 0, sizeof(SubW));
		SubW.mask = LVIF_TEXT;
		SubW.iItem = nItem;
		SubW.iSubItem = 1;
		SubW.pszText = szTextW;
		SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&SubW);

		// Last Modif Time
		FILETIME LocalFileTime;
		FileTimeToLocalFileTime(&fd.ftLastWriteTime, &LocalFileTime);
		SYSTEMTIME FileTime;
		FileTimeToSystemTime(fLocalSide ? &LocalFileTime : &LocalFileTime, &FileTime);
		WCHAR szTimeW[64];
		swprintf_s(szTimeW, L"%2.2d/%2.2d/%4.4d %2.2d:%2.2d",
				FileTime.wMonth, FileTime.wDay, FileTime.wYear,
				FileTime.wHour, FileTime.wMinute);

		SubW.iSubItem = 2;
		SubW.pszText = szTimeW;
		SendMessageW(hWndList, LVM_SETITEMW, 0, (LPARAM)&SubW);
	}
}


//
// Select the new transfered files in the dest FileList so the user find them easely
//
void FileTransfer::HighlightTransferedFiles(HWND hSrcList, HWND hDstList)
{
//	vnclog.Print(0, _T("HighlightTransferedFiles\n"));
	if (m_FilesList.size() > 0)
	{
		TCHAR szSelectedFile[128];

		LVITEM Item;
		Item.mask = LVIF_TEXT;
		Item.iSubItem = 0;
		Item.pszText = szSelectedFile;
		Item.cchTextMax = 128;

		LVFINDINFO Info;
		Info.flags = LVFI_STRING;
		Info.psz = (LPSTR)szSelectedFile;

		for (m_iFile = m_FilesList.begin();
			m_iFile != m_FilesList.end();
			m_iFile++)
		{
			// Get the name of the file sent
			Item.iItem = *m_iFile;
			ListView_GetItem(hSrcList, &Item);

			// Find this file in the list and highlight it
			int nTheIndex = ListView_FindItem(hDstList, -1, &Info);
			if (nTheIndex > -1)
			{
				ListView_SetItemState(hDstList, nTheIndex, LVIS_SELECTED, LVIS_SELECTED);
				ListView_EnsureVisible(hDstList, nTheIndex, FALSE);
			}
		}	
		m_FilesList.clear();
	}

}


//
//
//
bool FileTransfer::IsShortcutFolder(LPSTR szPath)
{
//	vnclog.Print(0, _T("IsShortcutFolder\n"));
	// Todo: Cultures Translation
	char szGUIDir[64];

	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "My Documents", rfbDirSuffix);
	if (!_strnicmp(szPath, szGUIDir, strlen(szGUIDir)))
		return true;

	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Desktop", rfbDirSuffix);
	if (!_strnicmp(szPath, szGUIDir, strlen(szGUIDir)))
		return true;

	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Network Favorites", rfbDirSuffix);
	if (!_strnicmp(szPath, szGUIDir, strlen(szGUIDir)))
		return true;

	return false;
}


//
//
//
bool FileTransfer::ResolvePossibleShortcutFolder(HWND hWnd, LPSTR szFolder)
{
//	vnclog.Print(0, _T("ResolvePossibleShortcutFolder\n"));
	TCHAR szP[MAX_PATH];
	int nFolder = -1;

	char szGUIDir[64];

	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "My Documents", rfbDirSuffix);
	if (!_strnicmp(szFolder, szGUIDir, strlen(szGUIDir)))
		nFolder = CSIDL_PERSONAL;

	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Desktop", rfbDirSuffix);
	if (!_strnicmp(szFolder, szGUIDir, strlen(szGUIDir)))
		nFolder = CSIDL_DESKTOP;

	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Network Favorites", rfbDirSuffix);
	if (!_strnicmp(szFolder, szGUIDir, strlen(szGUIDir)))
		nFolder = CSIDL_NETHOOD;

	/*
	if (!strnicmp(szFolder, "(Net. Shares)", 9))
		nFolder = CSIDL_NETWORK;
	*/

	if (nFolder != -1)
	{
		// if (SHGetSpecialFolderPath(NULL, szP, nFolder, FALSE))
		if (GetSpecialFolderPath(nFolder, szP))
		{
			strcat_s(szP,"\\");
			{
				WCHAR szPW2[MAX_PATH];
				MultiByteToWideChar(CP_ACP, 0, szP, -1, szPW2, MAX_PATH);
				SetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szPW2);
			}
		}
		return true;
	} else {		
		strcpy_s(szP, szFolder);

		//PGM len of "[ C: ] - Local Disk" is always > 2

		memset(szGUIDir, '\0', 64); //PGM 

		strncpy_s(szGUIDir, szP, 2); //PGM 

		if (strcmp(szGUIDir, "[ ")!=0) //PGM 

		{ //PGM

			int len = strlen(szP);



			if (len > 2) {

				WCHAR szPW[MAX_PATH];
				MultiByteToWideChar(CP_ACP, 0, szP, -1, szPW, MAX_PATH);
				if (GetFileAttributesW(szPW) & FILE_ATTRIBUTE_DIRECTORY) {

					SetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szPW);

					return true;

				}

			}

		} //PGM

	}
	return false;
}


bool FileTransfer::GetSpecialFolderPath(int nId, char* szPath)
{
	LPITEMIDLIST pidl;

	if (SHGetSpecialFolderLocation(0, nId, &pidl) != NOERROR)
		return false;

	if (!SHGetPathFromIDList(pidl, szPath) )
		return false;

	return true;
}
//
// Populate the local machine listbox with files located in szPath
//
void FileTransfer::PopulateLocalListBox(HWND hWnd, LPSTR szPath)
{
//	vnclog.Print(0, _T("PopulateLocalListBox\n"));
	char ofDir[MAX_PATH];
	char ofDirT[MAX_PATH];
	int nSelected = -1;
	int nCount = 0;
	int nFileCount = 0;
	char szLocalStatus[128];


	HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

	ofDir[0] = '\0';
	ofDirT[0] = '\0';

	if (lstrlen(szPath) == 0)
	{
		nCount = ListView_GetItemCount(hWndLocalList);
		for (nSelected = 0; nSelected < nCount; nSelected++)
		{
			if(ListView_GetItemState(hWndLocalList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
			{
				// Get display text (e.g. "[subfolder]") from ListView using Unicode message
				WCHAR szDispW[MAX_PATH] = {0};
				LVITEMW ItemW;
				memset(&ItemW, 0, sizeof(ItemW));
				ItemW.mask = LVIF_TEXT;
				ItemW.iItem = nSelected;
				ItemW.iSubItem = 0;
				ItemW.pszText = szDispW;
				ItemW.cchTextMax = MAX_PATH;
				SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
				WideCharToMultiByte(CP_ACP, 0, szDispW, -1, ofDirT, MAX_PATH, NULL, NULL);
				m_nLastLocalAttemptItem = nSelected;
				strcpy_s(m_szLastLocalAttemptName, ofDirT);
				break;
			}
		}
		// If no item was selected (including empty list), treat as "no selection"
		if (nSelected == -1) nSelected = nCount;
	}
	else
	{
		// Usual shortcuts case
		if (ResolvePossibleShortcutFolder(hWnd, szPath))
		{
		}
		else
		{
			// General case
			szPath[6] = '\0';
			// szPath always contains a drive letter (X:) or (..)
			strcpy_s(ofDirT, szPath);
			// remember which item was attempted so we can mark it on error
			LVFINDINFO Info;
			memset(&Info, 0, sizeof(Info));
			Info.flags = LVFI_STRING;
			Info.psz = (LPSTR)ofDirT;
			m_nLastLocalAttemptItem = ListView_FindItem(hWndLocalList, -1, &Info);
			strcpy_s(m_szLastLocalAttemptName, ofDirT);
			// In the case of (..) we keep the current path intact
			char szUpDirMask[16];
			sprintf_s(szUpDirMask, "%s..%s", rfbDirPrefix, rfbDirSuffix);
			if (strcmp(ofDirT, szUpDirMask))
				SetDlgItemTextW(hWnd, IDC_CURR_LOCAL, L"");
		}
	}

	char szSavedParentPath[MAX_PATH] = {0}; // To restore path if folder is unreadable

	bool bDirectPath = false;
	if (nSelected == nCount || lstrlen(ofDirT) == 0)
	{
		GetDlgItemText(hWnd, IDC_CURR_LOCAL, ofDirT, sizeof(ofDirT));
		strcpy_s(szSavedParentPath, ofDirT); // Save current path
		if (strlen(ofDirT) == 0) return;
		// If IDC_CURR_LOCAL already holds a full path (e.g. pasted by user), use it directly
		if (ofDirT[0] != rfbDirPrefix[0] || ofDirT[1] != rfbDirPrefix[1])
		{
			strcpy_s(ofDir, ofDirT);
			strcat_s(ofDir, "*");
			bDirectPath = true;
		}
	}
	if (!bDirectPath)
	{
		bool bTmp = false; //PGM @ Advantig
		if (ofDirT[0] == rfbDirPrefix[0] && ofDirT[1] == rfbDirPrefix[1])
		{
			TCHAR szTmp[MAX_PATH]; //PGM @ Advantig
			GetDlgItemText(hWnd, IDC_CURR_LOCAL, szTmp, sizeof(szTmp)); //PGM @ Advantig
			if (strlen(szTmp) == 0 && strlen(ofDirT) > 10 ) //PGM @ Advantig
			{ //PGM @ Advantig
				if (ResolvePossibleShortcutFolder(hWnd, ofDirT)) //PGM @ Advantig
				{ //PGM @ Advantig
					bTmp = true; //PGM @ Advantig
				} //PGM @ Advantig
			} //PGM @ Advantig
			else //PGM @ Advantig
			{ //PGM @ Advantig
				strncpy_s(ofDir, ofDirT + 2, strlen(ofDirT) - 3); 
				ofDir[strlen(ofDirT) - 4] = '\0';
			} //PGM @ Advantig
		}
		else 
			return;

		GetDlgItemText(hWnd, IDC_CURR_LOCAL, ofDirT, sizeof(ofDirT));
		strcpy_s(szSavedParentPath, ofDirT); // Save parent path before modification
		if (!_stricmp(ofDir, ".."))
		{	
			char* p;
			if (strlen(ofDirT) < 1)
				return;
			ofDirT[strlen(ofDirT) - 1] = '\0';
			p = strrchr(ofDirT, '\\');
			if (p == NULL) return;
			*p = '\0';
		}
		else
			strcat_s(ofDirT, ofDir);
		if (!bTmp) //PGM @ Advantig
			strcat_s(ofDirT, "\\");
		{
			WCHAR ofDirTW2[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, ofDirT, -1, ofDirTW2, MAX_PATH);
			SetDlgItemTextW(hWnd, IDC_CURR_LOCAL, ofDirTW2);
		}
		strcpy_s(ofDir, ofDirT);
		strcat_s(ofDir, "*");
	}

	// Select the good drive in the drives combo box (the first time only)
	int nIndex = SendDlgItemMessage(hWnd, IDC_LOCAL_DRIVECB, CB_GETCURSEL, 0, 0L);
	if (nIndex == LB_ERR)
	{
	    char szDrive[5];
		strcpy_s(szDrive, rfbDirPrefix);
	    strncat_s(szDrive, ofDir, 2);
	    nIndex = SendDlgItemMessage(hWnd, IDC_LOCAL_DRIVECB, CB_FINDSTRING, -1, (LPARAM)(LPSTR)szDrive); 
	    SendDlgItemMessage(hWnd, IDC_LOCAL_DRIVECB, CB_SETCURSEL, nIndex, 0L);
	}

	sprintf_s(szLocalStatus, sz_H8); 
	SetDlgItemText(hWnd, IDC_LOCAL_STATUS, szLocalStatus);

	WIN32_FIND_DATA fd;
	WIN32_FIND_DATAW fdW;
	HANDLE ff;
	int bRet = 1;

	// Convert path to Unicode for proper Chinese character support
	WCHAR ofDirW[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, ofDir, -1, ofDirW, MAX_PATH);

	SetErrorMode(SEM_FAILCRITICALERRORS); // No popup please !
	ff = FindFirstFileW(ofDirW, &fdW);
	SetErrorMode( 0 );

	// Convert Unicode result back to ANSI (system code page)
	if (ff != INVALID_HANDLE_VALUE)
	{
		memset(&fd, 0, sizeof(WIN32_FIND_DATA));
		fd.dwFileAttributes = fdW.dwFileAttributes;
		fd.ftCreationTime = fdW.ftCreationTime;
		fd.ftLastAccessTime = fdW.ftLastAccessTime;
		fd.ftLastWriteTime = fdW.ftLastWriteTime;
		fd.nFileSizeHigh = fdW.nFileSizeHigh;
		fd.nFileSizeLow = fdW.nFileSizeLow;
		fd.dwReserved0 = fdW.dwReserved0;
		WideCharToMultiByte(CP_ACP, 0, fdW.cFileName, -1, fd.cFileName, MAX_PATH, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, fdW.cAlternateFileName, -1, fd.cAlternateFileName, 14, NULL, NULL);
	}

	if (ff == INVALID_HANDLE_VALUE)
	{
		sprintf_s(szLocalStatus, sz_H9); 
		SetDlgItemText(hWnd, IDC_LOCAL_STATUS, szLocalStatus);
		if (m_nLastLocalAttemptItem >= 0)
		{
			LVITEMW ItemW;
			memset(&ItemW, 0, sizeof(ItemW));
			ItemW.mask = LVIF_PARAM;
			ItemW.iItem = m_nLastLocalAttemptItem;
			ItemW.iSubItem = 0;
			if (SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW))
				ItemW.lParam |= FT_LPARAM_UNREADABLE;
			else
				ItemW.lParam = FT_LPARAM_UNREADABLE;
			SendMessageW(hWndLocalList, LVM_SETITEMW, 0, (LPARAM)&ItemW);
			ListView_RedrawItems(hWndLocalList, m_nLastLocalAttemptItem, m_nLastLocalAttemptItem);
			UpdateWindow(hWndLocalList);
		}
		// Restore saved parent path and re-populate it
		{
			WCHAR szSavedW[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, szSavedParentPath, -1, szSavedW, MAX_PATH);
			SetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szSavedW);
		}
		strcpy_s(ofDir, szSavedParentPath);
		strcat_s(ofDir, "*");
		MultiByteToWideChar(CP_ACP, 0, ofDir, -1, ofDirW, MAX_PATH);
		ff = FindFirstFileW(ofDirW, &fdW);
		if (ff == INVALID_HANDLE_VALUE)
			return; // Parent also unreadable, nothing we can do
		FTListViewClear(hWndLocalList);
		// Update m_szLastLocalPath to parent
		lstrcpy(m_szLastLocalPath, ofDir);
		int len = strlen(m_szLastLocalPath);
		if (len > 2) { // truncate off the *
			m_szLastLocalPath[len-1] = '\0';
		}
		// Fall through to populate parent folder
		// Convert Unicode result to ANSI (system code page)
		memset(&fd, 0, sizeof(WIN32_FIND_DATA));
		fd.dwFileAttributes = fdW.dwFileAttributes;
		fd.ftCreationTime = fdW.ftCreationTime;
		fd.ftLastAccessTime = fdW.ftLastAccessTime;
		fd.ftLastWriteTime = fdW.ftLastWriteTime;
		fd.nFileSizeHigh = fdW.nFileSizeHigh;
		fd.nFileSizeLow = fdW.nFileSizeLow;
		fd.dwReserved0 = fdW.dwReserved0;
		WideCharToMultiByte(CP_ACP, 0, fdW.cFileName, -1, fd.cFileName, MAX_PATH, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, fdW.cAlternateFileName, -1, fd.cAlternateFileName, 14, NULL, NULL);
	} else {
		FTListViewClear(hWndLocalList);
		// adzm 2009-08-02
		lstrcpy(m_szLastLocalPath, ofDir);
		int len = strlen(m_szLastLocalPath);
		if (len > 2) { // truncate off the *
			m_szLastLocalPath[len-1] = '\0';
		}
	}

	WCHAR fdWFileName[MAX_PATH];
	wcscpy_s(fdWFileName, fdW.cFileName);
	while (bRet != 0)
	{
		AddFileToFileList(hWnd, IDC_LOCAL_FILELIST, fd, true, fdWFileName);
		nFileCount++;
		if (!PseudoYield(GetParent(hWnd))) { FindClose(ff); return; }
		bRet = FindNextFileW(ff, &fdW);
		if (bRet)
		{
			wcscpy_s(fdWFileName, fdW.cFileName);
			memset(&fd, 0, sizeof(WIN32_FIND_DATA));
			fd.dwFileAttributes = fdW.dwFileAttributes;
			fd.ftCreationTime = fdW.ftCreationTime;
			fd.ftLastAccessTime = fdW.ftLastAccessTime;
			fd.ftLastWriteTime = fdW.ftLastWriteTime;
			fd.nFileSizeHigh = fdW.nFileSizeHigh;
			fd.nFileSizeLow = fdW.nFileSizeLow;
			fd.dwReserved0 = fdW.dwReserved0;
		}
	}

	FindClose(ff);

	sprintf_s(szLocalStatus, " > %d %s", nFileCount, sz_H58); 
	SetDlgItemText(hWnd, IDC_LOCAL_STATUS, szLocalStatus);

	// If some files have been received 
	HighlightTransferedFiles( hWndRemoteList, hWndLocalList);

}



//
// Request the contents of a remote directory
//
void FileTransfer::RequestRemoteDirectoryContent(HWND hWnd, LPSTR szPath)
{
//	vnclog.Print(0, _T("RequestRemoteDirectoryContent\n"));
	if (!m_fFTAllowed)
	{
		m_fFileCommandPending = false;
		return;
	}

	char ofDir[MAX_PATH * 3];   // UTF-8 path to send (up to 3 bytes per char)
	char ofDirT[MAX_PATH];      // ANSI display path used for IDC_CURR_REMOTE
	WCHAR ofDirW[MAX_PATH];     // Unicode folder name (raw, no brackets)
	WCHAR ofDirTW[MAX_PATH];    // Unicode full path (for IDC_CURR_REMOTE)
	int nSelected = -1;
	int nCount = 0;
	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

	ofDir[0] = '\0';
	ofDirT[0] = '\0';
	ofDirW[0] = L'\0';
	ofDirTW[0] = L'\0';

	if (lstrlen(szPath) == 0)
	{
		nCount = ListView_GetItemCount(hWndRemoteList);
		for (nSelected = 0; nSelected < nCount; nSelected++)
		{
			if(ListView_GetItemState(hWndRemoteList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
			{
				// Get raw Unicode filename from lParam for server path
				LVITEMW ItemLP;
				memset(&ItemLP, 0, sizeof(ItemLP));
				ItemLP.mask = LVIF_PARAM;
				ItemLP.iItem = nSelected;
				ItemLP.iSubItem = 0;
				SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemLP);
				// Also get display text (e.g. "[subfolder]" or "[C:]") for ANSI display/legacy
				WCHAR szDispW[MAX_PATH] = {0};
				LVITEMW ItemW;
				memset(&ItemW, 0, sizeof(ItemW));
				ItemW.mask = LVIF_TEXT;
				ItemW.iItem = nSelected;
				ItemW.iSubItem = 0;
				ItemW.pszText = szDispW;
				ItemW.cchTextMax = MAX_PATH;
				SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
				WideCharToMultiByte(CP_ACP, 0, szDispW, -1, ofDirT, MAX_PATH, NULL, NULL);
				m_nLastRemoteAttemptItem = nSelected;
				strcpy_s(m_szLastRemoteAttemptName, ofDirT);
				GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, m_szLastRemotePathW, _MAX_PATH);
				WideCharToMultiByte(CP_ACP, 0, m_szLastRemotePathW, -1, m_szLastRemotePath, _MAX_PATH, NULL, NULL);

				std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemLP.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
				if (pNameW)
				{
					wcscpy_s(ofDirW, pNameW->c_str());
				}
				else
				{
					// lParam=0: drive or shortcut item (e.g. "[C:]", "[My Documents]").
					// Strip brackets from ofDirT, build the Unicode path directly.
					size_t tlen = strlen(ofDirT);
					size_t prefLen = strlen(rfbDirPrefix);
					size_t sufLen  = strlen(rfbDirSuffix);
					if (tlen > prefLen + sufLen && ofDirT[0] == rfbDirPrefix[0])
					{
						char szName[MAX_PATH];
						size_t nameLen = tlen - prefLen - sufLen;
						strncpy_s(szName, ofDirT + prefLen, nameLen);
						szName[nameLen] = '\0';
						strcat_s(szName, "\\");
						MultiByteToWideChar(CP_ACP, 0, szName, -1, ofDirTW, MAX_PATH);
						{
						WCHAR szNameW[MAX_PATH];
						MultiByteToWideChar(CP_ACP, 0, szName, -1, szNameW, MAX_PATH);
						SetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szNameW);
					}
						WideCharToMultiByte(CP_UTF8, 0, ofDirTW, -1, ofDir, MAX_PATH * 3, NULL, NULL);
						// Jump directly to send
						goto send_request;
					}
				}
				break;
			}
		}
		// If no item was selected (including empty list), treat as "no selection"
		if (nSelected == -1) nSelected = nCount;
	}
	else
	{
		if (!IsShortcutFolder(szPath))
			szPath[6] = '\0';
		// szPath always contains a drive letter (X:) or (..)
		strcpy_s(ofDirT, szPath);
		MultiByteToWideChar(CP_ACP, 0, ofDirT, -1, ofDirTW, MAX_PATH);
		// remember which item was attempted so we can mark it on error
		LVFINDINFO Info;
		memset(&Info, 0, sizeof(Info));
		Info.flags = LVFI_STRING;
		Info.psz = (LPSTR)ofDirT;
		m_nLastRemoteAttemptItem = ListView_FindItem(hWndRemoteList, -1, &Info);
		strcpy_s(m_szLastRemoteAttemptName, ofDirT);
		// In the case of (..) we keep the current path intact
		char szUpDirMask[16];
		sprintf_s(szUpDirMask, "%s..%s", rfbDirPrefix, rfbDirSuffix);
		// Save current path BEFORE clearing, so we can restore it if the folder is inaccessible
		GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, m_szLastRemotePathW, _MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, m_szLastRemotePathW, -1, m_szLastRemotePath, _MAX_PATH, NULL, NULL);
		if (strcmp(ofDirT, szUpDirMask))
			SetDlgItemTextW(hWnd, IDC_CURR_REMOTE, L"");
	}

	bool bDirectPath = false;
	if (nSelected == nCount || lstrlen(ofDirT) == 0)
	{
		GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, ofDirTW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, ofDirTW, -1, ofDirT, MAX_PATH, NULL, NULL);
		if (wcslen(ofDirTW) == 0)
		{
			m_fFileCommandPending = false;
			return;
		}
		// If IDC_CURR_REMOTE already holds a full path (e.g. pasted by user), send it directly
		if (ofDirT[0] != rfbDirPrefix[0] || ofDirT[1] != rfbDirPrefix[1])
		{
			// Convert Unicode full path to UTF-8 for server
			WideCharToMultiByte(CP_UTF8, 0, ofDirTW, -1, ofDir, MAX_PATH * 3, NULL, NULL);
			bDirectPath = true;
		}
	}
	if (!bDirectPath)
	{
		// Strip brackets from ANSI ofDirT to get folder name for .. detection
		if (ofDirT[0] == rfbDirPrefix[0] && ofDirT[1] == rfbDirPrefix[1])
		{
			strncpy_s(ofDir, ofDirT + 2, strlen(ofDirT) - 3);
			ofDir[strlen(ofDirT) - 4] = '\0';
			// Also strip brackets from Unicode: use ofDirW (raw name from lParam) if set,
			// else strip from ofDirTW
			if (ofDirW[0] == L'\0')
			{
				// fallback: strip from ofDirTW (e.g. when szPath was provided)
				size_t lenW = wcslen(ofDirTW);
				if (lenW > 3)
				{
					wcsncpy_s(ofDirW, ofDirTW + 1, lenW - 2);
					ofDirW[lenW - 2] = L'\0';
				}
			}
		}
		else
		{
			m_fFileCommandPending = false;
			return;
		}

		// Get current remote path in Unicode
		GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, ofDirTW, MAX_PATH);
		if (!_stricmp(ofDir, ".."))
		{
			if (wcslen(ofDirTW) < 1) return;
			ofDirTW[wcslen(ofDirTW) - 1] = L'\0'; // strip trailing backslash
			WCHAR* p = wcsrchr(ofDirTW, L'\\');
			if (p == NULL)
			{
				m_fFileCommandPending = false;
				return;
			}
			*p = L'\0';
		}
		else
			wcscat_s(ofDirTW, ofDirW);
		wcscat_s(ofDirTW, L"\\");
		// Update IDC_CURR_REMOTE display using Unicode directly (avoids CP_ACP corruption)
		SetDlgItemTextW(hWnd, IDC_CURR_REMOTE, ofDirTW);
		// Build ofDir as UTF-8 for server
		WideCharToMultiByte(CP_UTF8, 0, ofDirTW, -1, ofDir, MAX_PATH * 3, NULL, NULL);
	}

	// Todo: In case of shortcuts dir, do a translation here !

	// Select the good drive in the drives combo box (the first time only)
	int nIndex = SendDlgItemMessage(hWnd, IDC_REMOTE_DRIVECB, CB_GETCURSEL, 0, 0L);
	if (nIndex == LB_ERR)
	{
	    char szDrive[5];
		strcpy_s(szDrive, rfbDirPrefix);
	    strncat_s(szDrive, ofDir, 2);
	    nIndex = SendDlgItemMessage(hWnd, IDC_REMOTE_DRIVECB, CB_FINDSTRING, -1, (LPARAM)(LPSTR)szDrive);
	    SendDlgItemMessage(hWnd, IDC_REMOTE_DRIVECB, CB_SETCURSEL, nIndex, 0L);
	}

	// Don't clear list here - wait until we know folder is readable (in PopulateRemoteListBox)
	//ListView_DeleteAllItems(hWndRemoteList);

	send_request:
	// Send directory content request. When Unicode server: send UTF-8 path.
	// When old server: ofDir is still ANSI (szPath came in as ANSI from drive CB).
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	ft.contentType = rfbDirContentRequest;
    ft.contentParam = m_fServerSupportsUnicode
        ? (CARD16)(rfbRDirContent | rfbRDirContentUnicode)
        : (CARD16)rfbRDirContent;
	ft.length = Swap32IfLE((UINT32)strlen(ofDir));
	//adzm 2010-09
    m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	m_pCC->WriteExact((char *)ofDir, strlen(ofDir));

	return;
}


//
// Populate the remote machine listbox with files received from server
//
void FileTransfer::PopulateRemoteListBox(HWND hWnd, UINT nLen, bool fUnicodeEntry)
{
//	vnclog.Print(0, _T("PopulateRemoteListBox\n"));
	char szRemoteStatus[128];

	// If the distant media is not browsable for some reason
	if (nLen == 0)
	{
		sprintf_s(szRemoteStatus, sz_H10); 
		SetDlgItemText(hWnd, IDC_REMOTE_STATUS, szRemoteStatus);
		HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
		if (m_nLastRemoteAttemptItem >= 0)
		{
			LVITEMW ItemW;
			memset(&ItemW, 0, sizeof(ItemW));
			ItemW.mask = LVIF_PARAM;
			ItemW.iItem = m_nLastRemoteAttemptItem;
			ItemW.iSubItem = 0;
			if (SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW))
				ItemW.lParam |= FT_LPARAM_UNREADABLE;
			else
				ItemW.lParam = FT_LPARAM_UNREADABLE;
			SendMessageW(hWndRemoteList, LVM_SETITEMW, 0, (LPARAM)&ItemW);
			ListView_RedrawItems(hWndRemoteList, m_nLastRemoteAttemptItem, m_nLastRemoteAttemptItem);
			UpdateWindow(hWndRemoteList);
		}
		// Restore previous path so user can navigate back
		if (m_szLastRemotePathW[0] != L'\0')
			SetDlgItemTextW(hWnd, IDC_CURR_REMOTE, m_szLastRemotePathW);
		else if (strlen(m_szLastRemotePath) > 0)
		{
			WCHAR szLastW[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, m_szLastRemotePath, -1, szLastW, MAX_PATH);
			SetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szLastW);
		}
		// Don't clear list - keep showing parent folder contents
		// User stays in parent folder with error message
		m_fFileCommandPending = false;
		return;
	}

	// Folder is readable, clear list and populate
	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
	FTListViewClear(hWndRemoteList);

	// sf@2004 - Read the returned Directory full path
	if (nLen > 1 && !UsingOldProtocol())
	{
		char szPath[MAX_PATH * 3];
		if ((nLen+1) > (int)sizeof(szPath)) return;
		m_pCC->ReadString(szPath, nLen);
		// szPath is UTF-8 (server sends back the UTF-8 path it received).
		// Decode to Unicode and store via SetDlgItemTextW so Chinese paths display correctly.
		WCHAR szPathW[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, szPath, -1, szPathW, MAX_PATH);
		SetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szPathW);
	}

	sprintf_s(szRemoteStatus, sz_H11); 
	SetDlgItemText(hWnd, IDC_REMOTE_STATUS, szRemoteStatus);

	// The dir in the current packet
	memset(&m_fd, '\0', sizeof(WIN32_FIND_DATA));

	m_nFileCount = 0;
	m_fDirectoryReceptionRunning = true;

	// File Transfer Backward compatibility DIRTY hack for DSMPlugin mode...
	if (UsingOldProtocol() && m_pCC->m_fUsePlugin && !m_pCC->m_fPluginStreamingIn)
	{
		m_pCC->m_nTO = 0;
		ProcessFileTransferMsg();
	}

	//ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_REMOTE_FILELIST));

}


//
//
//
void FileTransfer::ReceiveDirectoryItem(HWND hWnd, UINT nLen, bool fUnicodeEntry)
{
//	vnclog.Print(0, _T("ReceiveDirectoryItem\n"));
	if (!m_fDirectoryReceptionRunning) return;

	WCHAR szUnicodeFileName[MAX_PATH] = {0};
	if (fUnicodeEntry)
	{
		// WIN32_FIND_DATAW is larger than m_szFileSpec; use a local buffer
		if (nLen > sizeof(WIN32_FIND_DATAW)) return;
		WIN32_FIND_DATAW fdW;
		memset(&fdW, 0, sizeof(fdW));
		m_pCC->ReadString((char *)&fdW, nLen);
		memset(&m_fd, 0, sizeof(WIN32_FIND_DATA));
		m_fd.dwFileAttributes  = fdW.dwFileAttributes;
		m_fd.ftCreationTime    = fdW.ftCreationTime;
		m_fd.ftLastAccessTime  = fdW.ftLastAccessTime;
		m_fd.ftLastWriteTime   = fdW.ftLastWriteTime;
		m_fd.nFileSizeHigh     = fdW.nFileSizeHigh;
		m_fd.nFileSizeLow      = fdW.nFileSizeLow;
		m_fd.dwReserved0       = fdW.dwReserved0;
		m_fd.cFileName[0] = '\0';
		wcscpy_s(szUnicodeFileName, fdW.cFileName);
	}
	else
	{
		if ((nLen + 1) > sizeof(m_szFileSpec)) return;
		// Read the File/Directory full info
		m_pCC->ReadString((char *)m_szFileSpec, nLen);
		memset(&m_fd, '\0', sizeof(WIN32_FIND_DATA));
		memcpy(&m_fd, m_szFileSpec, nLen);
	}
	// Pass Unicode filename directly for Unicode entries so display is correct on all Windows locales
	AddFileToFileList(hWnd, IDC_REMOTE_FILELIST, m_fd, false, fUnicodeEntry ? szUnicodeFileName : NULL);
	m_nFileCount++;

	// PseudoYield(pFileTransfer->hWnd);
	if (!PseudoYield(GetParent(hWnd))) return;

	// File Transfer Backward compatibility DIRTY hack for DSMPlugin mode...
	if (UsingOldProtocol() && m_pCC->m_fUsePlugin && !m_pCC->m_fPluginStreamingIn)
	{
		m_pCC->m_nTO = 0;
		ProcessFileTransferMsg();
	}

}


//
//
//
void FileTransfer::FinishDirectoryReception()
{
//	vnclog.Print(0, _T("FinishDirectoryReception\n"));
	if (!m_fDirectoryReceptionRunning) return;

	HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

	char szRemoteStatus[128];
	sprintf_s(szRemoteStatus, " > %d %s", m_nFileCount, sz_H58); 
	SetDlgItemText(hWnd, IDC_REMOTE_STATUS, szRemoteStatus);

	// If some files have been sent to remote side highlight them 
	HighlightTransferedFiles(hWndLocalList, hWndRemoteList);
	// UpdateWindow(hWnd);

	m_fDirectoryReceptionRunning = false;
    EnableButtons(hWnd);
    CheckButtonState(hWnd);

}

//
// request the list of remote drives 
//
void FileTransfer::RequestRemoteDrives()
{
//	vnclog.Print(0, _T("RequestRemoteDrives\n"));
	if (!m_fFTAllowed) return;

	// TODO : hook error !
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	ft.contentType = rfbDirContentRequest;
    ft.contentParam = rfbRDrivesList; // List of Remote Drives please
	ft.length = 0;
    m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);

	return;
}


//
// Fill the Remote FilesList and remote drives combo box 
//
void FileTransfer::ListRemoteDrives(HWND hWnd, UINT nLen)
{
//	vnclog.Print(0, _T("ListRemoteDrives\n"));
	TCHAR szDrivesList[256]; // Format when filled : "C:t<NULL>D:t<NULL>....Z:t<NULL><NULL>"
	TCHAR szDrive[4];
	TCHAR szTheDrive[128];
	TCHAR szType[32];
	UINT nIndex = 0;

	if ((nLen + 1) > sizeof(szDrivesList)) return;

	m_pCC->ReadString((char *)szDrivesList, nLen);

	HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

	SendDlgItemMessage(hWnd, IDC_REMOTE_DRIVECB, LB_RESETCONTENT, 0, 0L);
	FTListViewClear(hWndRemoteList);
	SetDlgItemText(hWnd, IDC_CURR_REMOTE, "");

	// Fill the tree with the remote drives
	while (nIndex < nLen - 3)
	{
		strcpy_s(szDrive, szDrivesList + nIndex);
		nIndex += 4;

		// Get the type of drive
		switch (szDrive[2])
		{
		case 'l':
			sprintf_s(szType, "%s", "Local Disk");
			break;
		case 'f':
			sprintf_s(szType, "%s", "Removable");
			break;
		case 'c':
			sprintf_s(szType, "%s", "CD-ROM");
			break;
		case 'n':
			sprintf_s(szType, "%s", "Network");
			break;
		default:
			sprintf_s(szType, "%s", "Unknown");
			break;
		}

		szDrive[2] = '\0'; // remove the type char
		sprintf_s(szTheDrive, "%s%s%s", rfbDirPrefix, szDrive, rfbDirSuffix);

		LVITEM Item;
		Item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		Item.iItem = 0;
		Item.iSubItem = 0;
		Item.iImage = 2;
		Item.pszText = szTheDrive;
		Item.lParam = 0;
		int nItem = ListView_InsertItem(hWndRemoteList, &Item);
		
		Item.mask = LVIF_TEXT;
		Item.iItem = nItem;
		Item.iSubItem = 1;
		Item.pszText = szType;
		ListView_SetItem(hWndRemoteList, &Item);

		// Prepare it for Combo Box and add it
		strcat_s(szTheDrive, " - ");
		strcat_s(szTheDrive, szType);

		SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szTheDrive); 

	}

	// List the usual shorcuts

	if (!UsingOldProtocol())
	{
	char szGUIDir[64];

	// MyDocuments
	LVITEM Item;
	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = 0;
	Item.iSubItem = 0;
	Item.lParam = 0;
	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "My Documents", rfbDirSuffix);
	Item.pszText = szGUIDir; // Todo: Fr/De
	ListView_InsertItem(hWndRemoteList, &Item);
	SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szGUIDir); 

	// Desktop
	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = 0;
	Item.iSubItem = 0;
	Item.lParam = 0;
	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Desktop", rfbDirSuffix);
	Item.pszText = szGUIDir; // Todo: Fr/De
	ListView_InsertItem(hWndRemoteList, &Item);
	SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szGUIDir); 

	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = 0;
	Item.iSubItem = 0;
	Item.lParam = 0;
	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Network Favorites", rfbDirSuffix);
	Item.pszText = szGUIDir; // Todo: Fr/De
	ListView_InsertItem(hWndRemoteList, &Item);
	SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szGUIDir); 
	}

	SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_SETCURSEL, -1, 0);
}


//
// List local drives
//
void FileTransfer::ListDrives(HWND hWnd)
{
//	vnclog.Print(0, _T("ListDrives\n"));
	TCHAR szDrivesList[256]=""; // Format when filled : "C:\<NULL>D:\<NULL>....Z:\<NULL><NULL>"
	TCHAR szDrive[4];
	TCHAR szTheDrive[32];
	TCHAR szType[32];
	UINT nType = 0;
	DWORD dwLen;
	DWORD nIndex = 0;
	dwLen = GetLogicalDriveStrings(256, szDrivesList);

	HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
	SendDlgItemMessage(hWnd, IDC_LOCAL_DRIVECB, LB_RESETCONTENT, 0, 0L);

	FTListViewClear(hWndLocalList);
	SetDlgItemTextW(hWnd, IDC_CURR_LOCAL, L"");

	// Parse the list of drives
	while (nIndex < dwLen - 3)
	{
		strcpy_s(szDrive, szDrivesList + nIndex);
		nIndex += 4;
		szDrive[2] = '\0'; // remove the '\'
		sprintf_s(szTheDrive, "%s%s%s", rfbDirPrefix, szDrive, rfbDirSuffix);

		// szName[0] = '\0';
		szType[0] = '\0';

		strcat_s(szDrive, "\\");

		// GetVolumeInformation(szDrive, szName, sizeof(szName), NULL, NULL, NULL, NULL, NULL);

		// Get infos on the Drive (type and Name)
		nType = GetDriveType(szDrive);
		switch (nType)
		{
        case DRIVE_FIXED:
			sprintf_s(szType, "%s", "Local Disk");
			break;
		case DRIVE_REMOVABLE:
			sprintf_s(szType, "%s", "Removable");
			break;
        case DRIVE_CDROM:
			sprintf_s(szType, "%s", "CD-ROM");
			break;
        case DRIVE_REMOTE:
			sprintf_s(szType, "%s", "Network");
			break;
		default:
			sprintf_s(szType, "%s", "Unknown");
			break;
		}

		// Add it to the ListView
		LVITEM Item;
		Item.mask = LVIF_TEXT | LVIF_IMAGE;
		Item.iItem = 0;
		Item.iSubItem = 0;
		Item.iImage = 2;
		Item.pszText = szTheDrive;
		int nItem = ListView_InsertItem(hWndLocalList, &Item);
		
		Item.mask = LVIF_TEXT;
		Item.iItem = nItem;
		Item.iSubItem = 1;
		Item.pszText = szType;
		ListView_SetItem(hWndLocalList, &Item);

		// Prepare it for Combo Box and add it
		strcat_s(szTheDrive, " - ");
		strcat_s(szTheDrive, szType);

		SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szTheDrive); 
	}

	// List the usual shorcuts

	char szGUIDir[64];

	// MyDocuments
	LVITEM Item;
	Item.mask = LVIF_TEXT;
	Item.iItem = 0;
	Item.iSubItem = 0;
	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "My Documents", rfbDirSuffix);
	Item.pszText = szGUIDir; // Todo: Fr/De
	ListView_InsertItem(hWndLocalList, &Item);
	SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szGUIDir); 

	// Desktop
	Item.mask = LVIF_TEXT;
	Item.iItem = 0;
	Item.iSubItem = 0;
	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Desktop", rfbDirSuffix);
	Item.pszText = szGUIDir; // Todo: Fr/De
	ListView_InsertItem(hWndLocalList, &Item);
	SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szGUIDir); 

	// Network Favorites
	Item.mask = LVIF_TEXT;
	Item.iItem = 0;
	Item.iSubItem = 0;
	sprintf_s(szGUIDir, "%s%s%s", rfbDirPrefix, "Network Favorites", rfbDirSuffix);
	Item.pszText = szGUIDir; // Todo: Fr/De
	ListView_InsertItem(hWndLocalList, &Item);
	SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_ADDSTRING, 0, (LPARAM)szGUIDir); 

	SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_SETCURSEL, -1, 0);
}


//
// Set gauge max value
//
void FileTransfer::SetTotalSize(HWND hWnd, DWORD dwTotalSize)
{
//	vnclog.Print(0, _T("SetTotalSize\n"));	
	SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, (WPARAM)0, (LPARAM)0L);
	SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, m_nBlockSize));
}

//
// Set gauge value
//
void FileTransfer::SetGauge(HWND hWnd, __int64 dwCount)
{
//	vnclog.Print(0, _T("SetGauge\n"));
	DWORD dwSmallerCount = (DWORD)(dwCount / m_nBlockSize);
	DWORD dwSmallerFileSize = (DWORD)(m_nnFileSize / m_nBlockSize);
	if (dwSmallerFileSize == 0) dwSmallerFileSize = 1;
	DWORD dwValue = (DWORD)( (((__int64)(dwSmallerCount) * m_nBlockSize / dwSmallerFileSize)));
	if (dwValue != m_dwCurrentValue)
	{
		SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, dwValue, 0);
		m_dwCurrentValue = dwValue;
	}

	DWORD dwPercent = (DWORD)(((__int64)(dwSmallerCount) * 100 / dwSmallerFileSize));
	if (dwPercent != m_dwCurrentPercent)
	{
		// adzm - Include the speed and kb total
		char szPercentAndSpeed[255];
		DWORD dwMsElapsed = GetTickCount() - m_dwStartTick;
		double dKbTotal = double(dwCount) / 1024;
		double dKbps = (dKbTotal / (double(dwMsElapsed) / 1000));
		sprintf_s(szPercentAndSpeed, "%d%% (%4.0f kb @ ~%4.0f kb/s)", dwPercent, dKbTotal, dKbps);
		SetDlgItemText(hWnd, IDC_PERCENT, szPercentAndSpeed);
		m_dwCurrentPercent = dwPercent;
	}
}


//
// Display global progress (ratio files transfered/Total Files To transfer)
//
void FileTransfer::SetGlobalCount()
{
//	vnclog.Print(0, _T("SetGlobalCount\n"));
	char szGlobal[64];
	
	sprintf_s(szGlobal,
			"File %d/%d",
			m_nFilesTransfered,
			m_nFilesToTransfer
			);
	SetDlgItemText(hWnd, IDC_GLOBAL_STATUS, szGlobal);
}


//
// Display current status and add it to the history combo box
//
void FileTransfer::SetStatus(LPSTR szStatus)
{
	if (strlen(szStatus) > (512 + 256 - 1))
		szStatus[768] = '\0';

	//	vnclog.Print(0, _T("SetStatus\n"));
	// time_t lTime;
	char dbuffer [9];
	char tbuffer [9];

	char szHist[800];

	SetDlgItemText(hWnd, IDC_STATUS, szStatus);
	_tzset();
	// time(&lTime);
	_strdate_s(dbuffer);
	_strtime_s(tbuffer);
	sprintf_s(szHist, " > %s %s - %s", dbuffer, tbuffer/*ctime(&lTime)*/, szStatus);
	{
		COMBOBOXINFO cbi;
		cbi.cbSize = sizeof cbi;

		GetComboBoxInfo(GetDlgItem(hWnd, IDC_HISTORY_CB), &cbi);
		HDC hdc = GetDC(cbi.hwndList);
		RECT rc = {0, 0, 0, 0};
        DrawText(hdc, szHist, -1, &rc, DT_CALCRECT|DT_SINGLELINE);
        ReleaseDC(cbi.hwndList, hdc);
        int dx = rc.right - rc.left;
        m_maxHistExtent = maximum(m_maxHistExtent, dx);
        SendDlgItemMessage(hWnd, IDC_HISTORY_CB, CB_SETHORIZONTALEXTENT, m_maxHistExtent, 0L);
    }
	LRESULT Index = SendMessage(GetDlgItem(hWnd, IDC_HISTORY_CB), CB_ADDSTRING, 0, (LPARAM)szHist); 
	SendMessage(GetDlgItem(hWnd, IDC_HISTORY_CB), CB_SETCURSEL, (WPARAM)Index, (LPARAM)0);		
}


//
// Request a file
//
void FileTransfer::RequestRemoteFile(LPSTR szRemoteFileName)
{

//	vnclog.Print(0, _T("RequestRemoteFile\n"));
	if (!m_fFTAllowed) return;

	// Ensure Backward File Transfer compatibility (Directory reception)....
	if (UsingOldProtocol())
	{
		char* p1 = strrchr(szRemoteFileName, '\\') + 1;
		char* p2 = strrchr(szRemoteFileName, rfbDirSuffix[0]);
		if (
			p1[0] == rfbDirPrefix[0] && p1[1] == rfbDirPrefix[1]  // Check dir prefix
			&& p2[0] == rfbDirSuffix[0] && p2[1] == rfbDirSuffix[1] && p2 != NULL && p1 < p2  // Check dir suffix
			) //
		{
			// p1 = strrchr(szRemoteFileName, '\\') + 1;
			char szDirectoryName[MAX_PATH];
			strcpy_s(szDirectoryName, p1 + 2); // Skip dir prefix (2 chars)
			szDirectoryName[strlen(szDirectoryName) - 2] = '\0'; // Remove dir suffix (2 chars)
			*p1 = '\0';
			strcat_s(szRemoteFileName, MAX_PATH, "("),
			strcat_s(szRemoteFileName, MAX_PATH, szDirectoryName);
			strcat_s(szRemoteFileName, MAX_PATH, ")");
		}
	}

	// TODO : hook error !
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	ft.contentType = rfbFileTransferRequest;
    ft.contentParam = 0;
	
	// szRemoteFileName is already UTF-8 encoded (callers use Unicode->UTF-8 conversion)
	// For old servers without Unicode support, convert UTF-8 back to CP_ACP
	char szFileNameToSend[MAX_PATH * 3];
	if (m_fServerSupportsUnicode)
	{
		// Already UTF-8, send as-is
		strcpy_s(szFileNameToSend, szRemoteFileName);
	}
	else
	{
		// Old server: convert UTF-8 -> Unicode -> CP_ACP
		WCHAR szFileNameW[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, szRemoteFileName, -1, szFileNameW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, szFileNameW, -1, szFileNameToSend, MAX_PATH * 3, NULL, NULL);
	}
	
	ft.length = Swap32IfLE(strlen(szFileNameToSend));
	ft.size = (m_pCC->kbitsPerSecond > 2048) ? Swap32IfLE(0) : Swap32IfLE(1); // 1 means "Enable compression" 
	//adzm 2010-09
    m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
    m_pCC->WriteExact((char *)szFileNameToSend, strlen(szFileNameToSend));
	strncpy_s(szRemoteFileNameRequested, szFileNameToSend, strlen(szFileNameToSend));
	rfbFileHeaderRequested = true;
	return;
}

bool endsWith(const char* str, const char* ending) {
	if (str == nullptr || ending == nullptr)
		return false;

	size_t strLength = strlen(str);
	size_t endingLength = strlen(ending);

	if (endingLength > strLength)
		return false;

	return (strcmp(str + strLength - endingLength, ending) == 0);
}

//
// Receive a file
//
bool FileTransfer::ReceiveFile(unsigned long lSize, UINT nLen)
{
//	vnclog.Print(0, _T("ReceiveFile\n"));
	if (!m_fFTAllowed) return false;

	rfbFileTransferMsg ft;
	ft.type = rfbFileTransfer;
	ft.contentType = rfbFileHeader;
	ft.size = Swap32IfLE(0);
	ft.length = Swap32IfLE(0);

	char *szRemoteFileName = new char [nLen+1];
	if (szRemoteFileName == NULL) return false;
	memset(szRemoteFileName, 0, nLen+1);

	// Read in the Name of the file to copy (remote full name !)
	m_pCC->ReadExact(szRemoteFileName, nLen);

	if (nLen > MAX_PATH)
		szRemoteFileName[MAX_PATH] = '\0';
	else
		szRemoteFileName[nLen] = '\0';

	char szStatus[MAX_PATH + 256];

	// sf@2004 - The file size can be wrong for huge files (>4Gb)
	// idealy we should pass another param (sizeH) in the rfbFileTransfer msg (same thing
	// for the Date/Time) but we want to maintain backward compatibility between all Ultra V1 RC version..
	// So instead we pass the additionnal High size param after the received string...of the current msg
	// Parse the FileTime and isolate filename
	CARD32 sizeH = 0;
	if (!UsingOldProtocol())
	{
		CARD32 sizeHtmp;
		m_pCC->ReadExact((char*)&sizeHtmp, sizeof(CARD32));
		sizeH = Swap32IfLE(sizeHtmp);
	}
    // 5/2/2008 moved jdp so that the entire packet is read
	// If lSize = -1 (0xFFFFFFFF) that means that the Src file on the remote machine
	// could not be opened for some reason (locked, doesn't exits any more...)
	if ((UsingOldProtocol() && lSize == 0xFFFFFFFFu)  ||
        (!UsingOldProtocol() && lSize == 0xFFFFFFFFu && sizeH == 0xFFFFFFFFu))
	{
		sprintf_s(szStatus, " %s < %s > %s", sz_H12, szRemoteFileName, sz_H13);
		// SetDlgItemText(pFileTransfer->hWnd, IDC_STATUS, szStatus);
		SetStatus(szStatus);
		delete [] szRemoteFileName;
        m_fFileDownloadError = true;
		return false;
	}

	// Get the current path (destination path) in Unicode
	WCHAR szDestDirW[MAX_PATH];
	GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szDestDirW, MAX_PATH);
	// Also store as CP_ACP for legacy uses below
	WideCharToMultiByte(CP_ACP, 0, szDestDirW, -1, m_szDestFileName, sizeof(m_szDestFileName), NULL, NULL);

	char *p = strrchr(szRemoteFileName, ',');
	if (p == NULL)
		m_szIncomingFileTime[0] = '\0';
	else 
	{
		strcpy_s(m_szIncomingFileTime, p+1);
		*p = '\0';
	}

    char  displayName[MAX_PATH + 32];
    {
        const char* pSlash = strrchr(szRemoteFileName, '\\');
        sprintf_s(displayName, "%s%s", m_szDestFileName, pSlash ? pSlash + 1 : szRemoteFileName);
    }
	// Check the free space on local destination drive
	bool fErr = false;
	ULARGE_INTEGER lpFreeBytesAvailable = { 0, 0 };
	ULARGE_INTEGER lpTotalBytes;		
	ULARGE_INTEGER lpTotalFreeBytes;
	unsigned long dwFreeKBytes=0;
	char *szDestPath = new char [strlen(m_szDestFileName) + 1];
	memset(szDestPath, 0, strlen(m_szDestFileName) + 1);
	strcpy_s(szDestPath, strlen(m_szDestFileName) + 1, m_szDestFileName);
	{ char *pbs = strrchr(szDestPath, '\\'); if (pbs) *pbs = '\0'; }

	//security requested filename must be the received filename
	if (strcmp(szRemoteFileName, szRemoteFileNameRequested) != NULL) {
		if (!endsWith(szRemoteFileName, ".zip"))
			return false;
	}

    // only check root folder on drive, in case we have no permissions on dest folder
    if (szDestPath[1] == ':')
        szDestPath[3] = 0;

	PGETDISKFREESPACEEX pGetDiskFreeSpaceEx;
	pGetDiskFreeSpaceEx = (PGETDISKFREESPACEEX)GetProcAddress( GetModuleHandle("kernel32.dll"),"GetDiskFreeSpaceExA");
	if (pGetDiskFreeSpaceEx)
						{

						if (!pGetDiskFreeSpaceEx((LPCTSTR)szDestPath,
							&lpFreeBytesAvailable,
							&lpTotalBytes,
							&lpTotalFreeBytes)) fErr = true;
						}
	delete [] szDestPath;
	dwFreeKBytes  = (unsigned long) (Int64ShraMod32(lpFreeBytesAvailable.QuadPart, 10));
	if (dwFreeKBytes < (unsigned long)(lSize / 1000)) fErr = true;

	bool fErrNoFileName = false;
	char *plbs = strrchr(szRemoteFileName, '\\');
	if (plbs == NULL)
	{ 
		fErrNoFileName = true;
		fErr = true;
	}
	else if (plbs[1] == '\0')
	{
		fErrNoFileName = true;
		fErr = true;
	}

	if (fErr)
	{
		if (!fErrNoFileName)
			sprintf_s(szStatus, " %s < %s >",sz_H14, displayName); 
		else
			sprintf_s(szStatus, " %s < %s > %s",sz_H14, "Invalid remote file name", sz_H13); 

		SetStatus(szStatus);
		delete [] szRemoteFileName;
		// Tell the server to cancel the transfer
		ft.size = Swap32IfLE(-1);
		if (UsingOldProtocol())
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg);
		else
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
        m_fFileDownloadError = true;
		return false;
	}


    // Build destination filename: decode UTF-8 remote filename to Unicode, append to local dir
    {
        // Decode the UTF-8 remote filename to Unicode
        WCHAR szRemoteFileNameW[MAX_PATH];
        MultiByteToWideChar(CP_UTF8, 0, szRemoteFileName, -1, szRemoteFileNameW, MAX_PATH);
        // Get just the filename part (after last backslash)
        const WCHAR* pFilePartW = wcsrchr(szRemoteFileNameW, L'\\');
        if (pFilePartW) pFilePartW++; else pFilePartW = szRemoteFileNameW;
        // Apply temp filename logic on the Unicode filename
        char szFilePartUTF8[MAX_PATH * 3];
        WideCharToMultiByte(CP_UTF8, 0, pFilePartW, -1, szFilePartUTF8, MAX_PATH * 3, NULL, NULL);
        std::string tempName = make_temp_filename(szFilePartUTF8);
        WCHAR szTempNameW[MAX_PATH];
        MultiByteToWideChar(CP_UTF8, 0, tempName.c_str(), -1, szTempNameW, MAX_PATH);
        // Build full Unicode dest path and store as member for FinishFileReception
        wcscpy_s(m_szDestFileNameW, szDestDirW);
        wcscat_s(m_szDestFileNameW, szTempNameW);
        // Also store CP_ACP version for legacy code that reads m_szDestFileName
        WideCharToMultiByte(CP_ACP, 0, m_szDestFileNameW, -1, m_szDestFileName, sizeof(m_szDestFileName), NULL, NULL);
        m_nnFileSize = (((__int64)(sizeH)) << 32) + lSize;
        char szFFS[96];
        GetFriendlyFileSizeString(m_nnFileSize, szFFS, 96);
        sprintf_s(szStatus, " %s < %s > (%s) <<<",
            sz_H15, displayName, szFFS);
        SetStatus(szStatus);
        SetTotalSize(hWnd, lSize);
        SetGauge(hWnd, 0);
        UpdateWindow(hWnd);
        // Create the local Destination file using Unicode path directly
        m_hDestFile = CreateFileW(m_szDestFileNameW,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
    }

	// sf@2004 - Delta Transfer
	bool fAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);

	if (m_hDestFile == INVALID_HANDLE_VALUE)
	{
		sprintf_s(szStatus, " %s < %s > %s", sz_H12, displayName, sz_H16);
		SetStatus(szStatus);
		CloseHandle(m_hDestFile);
		delete [] szRemoteFileName;
        m_fFileDownloadError =  true;

		// Tell the server to cancel the transfer
		ft.size = Swap32IfLE(-1);
		if (UsingOldProtocol())
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg);
		else
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
		return false;
	}

	delete [] szRemoteFileName;

	m_pCC->CheckFileChunkBufferSize(m_nBlockSize + 1024);

	// sf@2004 - Delta Transfer
	if (fAlreadyExists && !UsingOldProtocol())
	{
		// DWORD dwFileSize = GetFileSize(m_hDestFile, NULL); 
		ULARGE_INTEGER n2FileSize;
		bool bSizeOk = MyGetFileSize(m_szDestFileName, &n2FileSize); 
		// if (dwFileSize != 0xFFFFFFFF)
		if (bSizeOk)
		{
			unsigned long long nCSBufferSize = (4 * (unsigned long long)(n2FileSize.QuadPart / m_nBlockSize)) + 1024;
			if (nCSBufferSize > std::numeric_limits<size_t>::max())			
				return false;
			
			std::unique_ptr<char[]> lpCSBuff = std::make_unique<char[]>(static_cast<size_t>(nCSBufferSize));
			if (lpCSBuff)
			{
				int nCSBufferLen = GenerateFileChecksums(m_hDestFile, lpCSBuff.get(), static_cast<int>(nCSBufferSize)
				);
				if (nCSBufferLen != -1)
				{
					//sprintf_s(szStatus, " Sending %d bytes of file checksums to remote machine. Please Wait...", nCSBufferSize); 
					//SetStatus(szStatus);

					rfbFileTransferMsg ftm;
					ftm.type = rfbFileTransfer;
					ftm.contentType = rfbFileChecksums;
					ftm.size = Swap32IfLE(nCSBufferSize);
					ftm.length = Swap32IfLE(nCSBufferLen);
					//adzm 2010-09
					m_pCC->WriteExactQueue((char*)&ftm, sz_rfbFileTransferMsg, rfbFileTransfer);
					m_pCC->WriteExactQueue((char*)lpCSBuff.get(), nCSBufferLen);
				}
			}
			
		}
	}

	// Tell the server that the transfer can start
	ft.size = Swap32IfLE(lSize); 
	if (UsingOldProtocol())
		m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg);
	else
		m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);

	// DWORD dwNbPackets = (DWORD)(lSize / m_nBlockSize);
	m_dwNbReceivedPackets = 0;
	m_dwNbBytesWritten = 0;
	m_dwTotalNbBytesWritten = 0;
	m_dwTotalNbBytesNotReallyWritten = 0;
	m_nPacketCount = 0;
	m_fPacketCompressed = true;

	m_fFileDownloadError = false;
	m_fFileDownloadRunning = true;

	m_dwStartTick = GetTickCount();

	// File Transfer Backward compatibility DIRTY hack for DSMPlugin mode...
	if (UsingOldProtocol() && m_pCC->m_fUsePlugin && !m_pCC->m_fPluginStreamingIn)
	{
		m_pCC->m_nTO = 0;
		ProcessFileTransferMsg();
	}

	return true;
}


//
// Receive incoming file chunk
//
bool FileTransfer::ReceiveFileChunk(UINT nLen, int nSize)
{
//	vnclog.Print(0, _T("ReceiveFileChunk\n"));
	if (!m_fFileDownloadRunning) return false;

	if (m_fFileDownloadError)
		FinishFileReception();

	BOOL fRes = true;
	bool fAlreadyHere = (nSize == 2);
	m_fPacketCompressed = true; // sf@2005 - This missing line was causing RC19 file reception bug...

	if (nLen > m_pCC->m_filechunkbufsize) return false;

	// sf@2004 - Delta Transfer - Empty packet
	if (fAlreadyHere) 
	{
		DWORD dwPtr = SetFilePointer(m_hDestFile, nLen, NULL, FILE_CURRENT); 
		if (dwPtr == 0xFFFFFFFF)
			fRes = false;
	}
	else
	{
		m_pCC->ReadExact((char *)m_pCC->m_filechunkbuf, nLen);

		if (nSize == 0) 
			m_fPacketCompressed = false;
		
		unsigned int nRawBytes = m_nBlockSize + 1024;

		if (m_fPacketCompressed)
		{
			// Decompress incoming data
			m_pCC->CheckFileZipBufferSize(nRawBytes);
			int nRetU = uncompress(	(unsigned char*)m_pCC->m_filezipbuf,// Dest 
									(unsigned long *)&nRawBytes,// Dest len
									(unsigned char*)m_pCC->m_filechunkbuf,		// Src
									nLen		// Src len
								 );							    

			if (nRetU != 0)
			{
				// vnclog.Print(0, _T("uncompress error in ReceiveFile: %d\n"), nRet);
				m_fFileDownloadError = true;
				// break;
			}
			Sleep(5);
		}

		fRes = WriteFile(m_hDestFile,
							m_fPacketCompressed ? m_pCC->m_filezipbuf : m_pCC->m_filechunkbuf,
							m_fPacketCompressed ? nRawBytes : nLen,
							&m_dwNbBytesWritten,
							NULL);
	}

	if (!fRes)
	{
		// TODO: Ask the server to stop the transfer
		m_fFileDownloadError = true;
	}

	m_dwTotalNbBytesWritten += (fAlreadyHere ? nLen : m_dwNbBytesWritten);
	m_dwTotalNbBytesNotReallyWritten += (fAlreadyHere ? nLen : 0);
	m_dwNbReceivedPackets++;

	// Refresh of the progress bar
	SetGauge(hWnd, m_dwTotalNbBytesWritten);
	PseudoYield(GetParent(hWnd));

	// We still support the *dirty* old "Abort" method (for backward compatibility with UltraVNC Server <= RC18)
	if (UsingOldProtocol())
	{
		// Every 10 packets, test if the transfer must be stopped
		m_nPacketCount++;
		if (m_nPacketCount > 10)
		{
			m_fAborted = true;
			rfbFileTransferMsg ft;
			ft.type = rfbFileTransfer;
			ft.contentType = rfbFilePacket;
			ft.contentParam = 0;
			ft.length = 0;

			if (m_fAbort || m_fFileDownloadError)
			{
				// Ask the server to stop the transfer
				ft.size = Swap32IfLE(-1);
				m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg/*, rfbFileTransfer*/);
			}
			else
			{
				// Tell the server to continue the transfer
				ft.size = Swap32IfLE(0);
				m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg/*, rfbFileTransfer*/);
			}
			m_nPacketCount = 0;
		}
	}
	else // New v2 File Transfer Protocol
	{
		// Now abort the File Transfer if required by the user
		if (m_fAbort && !m_fAborted)
		{
			m_fAborted = true;
			rfbFileTransferMsg ft;
			ft.type = rfbFileTransfer;
			ft.contentType = rfbAbortFileTransfer;
			ft.contentParam = rfbFileTransferVersion;
			ft.length = 0;
			ft.size = 0;
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
		}
	}

	// File Transfer Backward compatibility DIRTY hack for DSMPlugin mode...
	if (UsingOldProtocol() && m_pCC->m_fUsePlugin && !m_pCC->m_fPluginStreamingIn)
	{
		m_pCC->m_nTO = 0;
		ProcessFileTransferMsg();
	}
	return true;

}


//
// Finish File Download
//
bool FileTransfer::FinishFileReception()
{
//	vnclog.Print(0, _T("FinishFileReception\n"));
	if (!m_fFileDownloadRunning) return false;

	m_fFileDownloadRunning = false;
    m_pCC->SetRecvTimeout(0);
	// adzm 2010-09
    m_pCC->SendKeepAlive(false, true);

	// sf@2004 - Delta transfer
	SetEndOfFile(m_hDestFile);

	// TODO : check dwNbReceivedPackets and dwTotalNbBytesWritten or test a checksum
	FlushFileBuffers(m_hDestFile);

    std::string realName = get_real_filename(m_szDestFileName);
    
	char szStatus[512 + 256];
	if (m_fFileDownloadError)
		sprintf_s(szStatus, " %s < %s > %s", sz_H19,realName.c_str(),sz_H20);
	else
		// sprintf_s(szStatus, " %s < %s > %s - %u bytes", sz_H17, m_szDestFileName,sz_H18, (m_dwTotalNbBytesWritten - m_dwTotalNbBytesNotReallyWritten));  // Testing
		sprintf_s(szStatus, " %s < %s > %s", sz_H17, realName.c_str(),sz_H18);

	SetStatus(szStatus);

	// Set the DestFile Time Stamp
	if (strlen(m_szIncomingFileTime))
	{
		FILETIME DestFileTime;
		SYSTEMTIME FileTime;
		FileTime.wMonth  = atoi(m_szIncomingFileTime);
		FileTime.wDay    = atoi(m_szIncomingFileTime + 3);
		FileTime.wYear   = atoi(m_szIncomingFileTime + 6);
		FileTime.wHour   = atoi(m_szIncomingFileTime + 11);
		FileTime.wMinute = atoi(m_szIncomingFileTime + 14);
		FileTime.wMilliseconds = 0;
		FileTime.wSecond = 0;
		SystemTimeToFileTime(&FileTime, &DestFileTime);
		// ToDo: hook error
		SetFileTime(m_hDestFile, &DestFileTime, &DestFileTime, &DestFileTime);
	}

	CloseHandle(m_hDestFile);

	// sf@2004 - Delta Transfer - Now we can keep the existing file data :)
	if (m_fFileDownloadError && (UsingOldProtocol() || m_fUserAbortedFileTransfer)) {
		DeleteFileW(m_szDestFileNameW);
	}

	// sf@2003 - Directory Transfer trick
	// If the file is an Ultra Directory Zip we unzip it here and we delete the
	// received file
	// Todo: make a better free space check (above) in this particular case. The free space must be at least
	// 3 times the size of the directory zip file (this zip file is ~50% of the real directory size) 

    // hide the stop button
    ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B2), SW_HIDE);
	// Convert m_szDestFileNameW to CP_ACP only for UnzipPossibleDirectory (ANSI function)
	bool bWasDir = UnzipPossibleDirectory(m_szDestFileName);
    ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B2), SW_SHOW);

    if (!m_fFileDownloadError && !bWasDir)
    {
		// Build real (final) Unicode filename by stripping the !UVNCPFT- prefix from m_szDestFileNameW
		WCHAR realNameW[MAX_PATH + 32];
		wcscpy_s(realNameW, m_szDestFileNameW);
		{
			// find and remove rfbPartialFilePrefix in Unicode path
			WCHAR prefixW[32];
			MultiByteToWideChar(CP_ACP, 0, rfbPartialFilePrefix, -1, prefixW, 32);
			WCHAR *pos = wcsstr(realNameW, prefixW);
			if (pos)
				wmemmove(pos, pos + wcslen(prefixW), wcslen(pos + wcslen(prefixW)) + 1);
		}
        if (!::MoveFileExW(m_szDestFileNameW, realNameW, MOVEFILE_REPLACE_EXISTING))
       {
            // failure. Updated status
            sprintf_s(szStatus, " %s < %s > %s", sz_H12, realName.c_str(), sz_H16);
            SetStatus(szStatus);
        }
    }

	// SetStatus(szStatus);
	UpdateWindow(hWnd);

	// Sound notif
	// MessageBeep(-1);

	// Request the next file in the list
	RequestNextFile();
	return true;
}


//
// Unzip possible directory
// Todo: handle unzip error correctly...
//
bool FileTransfer::UnzipPossibleDirectory(LPSTR szFileName)
{
//	vnclog.Print(0, _T("UnzipPossibleDirectory\n"));
	if (!m_fFileDownloadError 
		&& 
		!strncmp(strrchr(szFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix))
	   )
	{
		char szStatus[512 + 256];
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH]; // Todo: improve this (size) 
		strcpy_s(szPath, szFileName);
		// Todo: improve all this (p, p2, p3 NULL test or use a standard substring extraction function)
		char *p = strrchr(szPath, '\\') + 1; 
		char *p2 = strchr(p, '-') + 1; // rfbZipDirectoryPrefix MUST have a "-" at the end...
		strcpy_s(szDirName, p2);
		char *p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat_s(szPath, szDirName);

		// Create the Directory
		sprintf_s(szStatus, " %s < %s > %s", sz_H59 , szDirName, sz_H60); 
		SetStatus(szStatus);

		bool fUnzip = m_pZipUnZip->UnZipDirectory(szPath, szFileName);
		if (fUnzip)
			sprintf_s(szStatus, " %s < %s > %s", sz_H61, szDirName, sz_H18);
		else
			sprintf_s(szStatus, " %s < %s >. %s", sz_H62, szDirName, sz_H63);
		SetStatus(szStatus);
		DeleteFile(szFileName);
        return true;
	}						
	return false;
}

//
// Finish File Download
//
bool FileTransfer::AbortFileReception()
{
//	vnclog.Print(0, _T("AbortFileReception\n"));
	if (!m_fFileDownloadRunning) return false;

	m_fFileDownloadError = true;
	FlushFileBuffers(m_hDestFile);
	char szStatus[512 + 256];
	sprintf_s(szStatus, " %s < %s > %s", sz_H19, m_szDestFileName,sz_H20); 

	m_fFileDownloadRunning = false;

	return true;
}

//
// Offer a file
//
bool FileTransfer::OfferLocalFile(LPSTR szSrcFileName)
{
//	vnclog.Print(0, _T("OfferLocalFile\n"));
	if (!m_fFTAllowed) return false;

	char szStatus[512 + 256];

	strcpy_s(m_szSrcFileName, szSrcFileName);

	// sf@2003 - Directory Transfer trick
	// The File to transfer is actually a directory, so we must Zip it recursively and send
	// the resulting zip file (it will be recursively unzipped on server side once
	// the transfer is done)
	nDirZipRet = ZipPossibleDirectory(m_szSrcFileName);
	if (nDirZipRet == -1)
    {
        m_fFileUploadError = true;
		return false;
    }
	if (nDirZipRet == 1)
	{
		// ZipPossibleDirectory updated m_szSrcFileName to a zip path (always ASCII-safe temp path)
		MultiByteToWideChar(CP_ACP, 0, m_szSrcFileName, -1, m_szSrcFileNameW, MAX_PATH + 32);
	}

	// Open local src file using Unicode path (m_szSrcFileNameW) to support Chinese filenames
	// If m_szSrcFileNameW is empty (legacy call), fall back to CP_ACP conversion
	if (m_szSrcFileNameW[0] == L'\0')
		MultiByteToWideChar(CP_ACP, 0, m_szSrcFileName, -1, m_szSrcFileNameW, MAX_PATH + 32);
	m_hSrcFile = CreateFileW(
							m_szSrcFileNameW,		
							GENERIC_READ,		
							FILE_SHARE_READ,	
							NULL,				
							OPEN_EXISTING,		
							FILE_FLAG_SEQUENTIAL_SCAN,	
							NULL
							);				

	if (m_hSrcFile == INVALID_HANDLE_VALUE)
	{
		sprintf_s(szStatus, " %s < %s >", sz_H21, m_szSrcFileName); 
		SetStatus(szStatus);
        m_fFileUploadError = true;

		return false;
	}

	// Size of src file - use Unicode path for correct size retrieval
	ULARGE_INTEGER n2SrcSize;
	WIN32_FIND_DATAW fdW2;
	HANDLE ffSize = FindFirstFileW(m_szSrcFileNameW, &fdW2);
	bool bSize = (ffSize != INVALID_HANDLE_VALUE);
	if (bSize) { FindClose(ffSize); n2SrcSize.LowPart = fdW2.nFileSizeLow; n2SrcSize.HighPart = fdW2.nFileSizeHigh; n2SrcSize.QuadPart = (((__int64)fdW2.nFileSizeHigh)<<32)+fdW2.nFileSizeLow; }
	if (!bSize)
	{
		sprintf_s(szStatus, " %s < %s >", sz_H21, m_szSrcFileName);
		SetStatus(szStatus);
		CloseHandle(m_hSrcFile);
        m_fFileUploadError = true;
		return false;
	}

	char szFFS[96];
	GetFriendlyFileSizeString(n2SrcSize.QuadPart, szFFS, 96);
	sprintf_s(szStatus, " %s < %s > (%s) >>>", sz_H22, m_szSrcFileName , szFFS); 

	SetStatus(szStatus);
	m_nnFileSize = n2SrcSize.QuadPart;
	SetTotalSize(hWnd, (DWORD)m_nnFileSize); // In bytes
	SetGauge(hWnd, 0); // In bytes
	UpdateWindow(hWnd);

	// Add the File Time Stamp to the filename
	FILETIME SrcFileModifTime; 
	BOOL fRes = GetFileTime(m_hSrcFile, NULL, NULL, &SrcFileModifTime);
	if (!fRes)
	{
		sprintf_s(szStatus, " %s < %s >", sz_H23, m_szSrcFileName); 
		SetStatus(szStatus);
		CloseHandle(m_hSrcFile);
        m_fFileUploadError = true;
		return false;
	}

	CloseHandle(m_hSrcFile);

	// Build destination path in Unicode to handle Chinese remote paths correctly.
	WCHAR szDstFileNameW[MAX_PATH + 32];
	GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szDstFileNameW, MAX_PATH);
	if (!wcslen(szDstFileNameW)) return false; // no destination dir selected
	// Append local filename from Unicode source path (avoids CP_ACP corruption)
	{
		const WCHAR* pSrcBasenameW = wcsrchr(m_szSrcFileNameW, L'\\');
		if (pSrcBasenameW) pSrcBasenameW++; else pSrcBasenameW = m_szSrcFileNameW;
		wcscat_s(szDstFileNameW, pSrcBasenameW);
	}
	// Convert full Unicode dest path to CP_ACP for legacy szDstFileName
	TCHAR szDstFileName[MAX_PATH + 32];
	WideCharToMultiByte(CP_ACP, 0, szDstFileNameW, -1, szDstFileName, MAX_PATH + 32, NULL, NULL);

	char szSrcFileTime[18];
	// sf@2003
	// For now, we've made the choice off displaying all the files 
	// off client AND server sides converted in clients local
	// time only. We keep file time as it is before transfering the file (absolute time)
	/*
	FILETIME LocalFileTime;
	FileTimeToLocalFileTime(&SrcFileModifTime, &LocalFileTime);
	*/
	SYSTEMTIME FileTime;
	FileTimeToSystemTime(/*&LocalFileTime*/&SrcFileModifTime, &FileTime);
	sprintf_s(szSrcFileTime,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d",
			FileTime.wMonth,
			FileTime.wDay,
			FileTime.wYear,
			FileTime.wHour,
			FileTime.wMinute
			);
	// Append timestamp to both ANSI and Unicode dest filename
	strcat_s(szDstFileName, ",");
	strcat_s(szDstFileName, szSrcFileTime);
	{
		WCHAR szTimestampW[32];
		MultiByteToWideChar(CP_ACP, 0, ",", -1, szTimestampW, 32);
		wcscat_s(szDstFileNameW, MAX_PATH + 32, szTimestampW);
		MultiByteToWideChar(CP_ACP, 0, szSrcFileTime, -1, szTimestampW, 32);
		wcscat_s(szDstFileNameW, MAX_PATH + 32, szTimestampW);
	}

	// sf@2004 - Delta Transfer
	if (m_lpCSBuffer != NULL) 
	{
		delete [] m_lpCSBuffer;
		m_lpCSBuffer = NULL;
	}
	m_nCSOffset = 0;
	m_nCSBufferSize = 0;

	// Send the FileTransferMsg with rfbFileTransferOffer
	// So the server creates the appropriate new file on the other side
    rfbFileTransferMsg ft;

    ft.type = rfbFileTransfer;
	ft.contentType = rfbFileTransferOffer;
    ft.contentParam = 0;
    ft.size = Swap32IfLE(n2SrcSize.LowPart); // File Size in bytes
	
	// Convert filename to UTF-8 if server supports Unicode
	char szDstFileNameToSend[MAX_PATH * 3 + 32]; // UTF-8 can be up to 3x longer + timestamp
	if (m_fServerSupportsUnicode)
	{
		// szDstFileNameW already has the correct Unicode path (remote dir + local basename + timestamp)
		WideCharToMultiByte(CP_UTF8, 0, szDstFileNameW, -1, szDstFileNameToSend, MAX_PATH * 3 + 32, NULL, NULL);
	}
	else
	{
		// Old server: send as-is
		strcpy_s(szDstFileNameToSend, szDstFileName);
	}
	
	ft.length = Swap32IfLE(strlen(szDstFileNameToSend));
	//adzm 2010-09
    m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	m_pCC->WriteExactQueue((char *)szDstFileNameToSend, strlen(szDstFileNameToSend));
	rfbFileTransferOfferRequested = true;
	if (!UsingOldProtocol())
	{
		CARD32 sizeH = Swap32IfLE(n2SrcSize.HighPart);
		m_pCC->WriteExact((char *)&sizeH, sizeof(CARD32));
	}
	else
	{
		m_pCC->FlushWriteQueue();
	}

    // show the stop button
    ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B2), SW_SHOW);
    m_pCC->SetSendTimeout();
	return true;
}


//
// Zip a possible directory
//
int FileTransfer::ZipPossibleDirectory(LPSTR szSrcFileName)
{
//	vnclog.Print(0, _T("ZipPossibleDirectory\n"));
	char* p1 = strrchr(szSrcFileName, '\\') + 1;
	char* p2 = strrchr(szSrcFileName, rfbDirSuffix[0]);
	if (
		p1[0] == rfbDirPrefix[0] && p1[1] == rfbDirPrefix[1]  // Check dir prefix
		&& p2[1] == rfbDirSuffix[1] && p2 != NULL && p1 < p2  // Check dir suffix
		) //
	{
		// sf@2004 - Improving Directory Transfer: Avoids ReadOnly media problem
		char szDirZipPath[MAX_PATH];
		char szWorkingDir[MAX_PATH];
		::GetTempPath(MAX_PATH,szWorkingDir); //PGM Use Windows Temp folder
		if (szWorkingDir == NULL) //PGM 
		{ //PGM
			if (GetModuleFileName(NULL, szWorkingDir, MAX_PATH))
			{
				char* p = strrchr(szWorkingDir, '\\');
				if (p == NULL)
					return -1;
				*(p+1) = '\0';
			}
			else
			{
				return -1;
			}
		}//PGM

		char szStatus[512 + 256];
		char szPath[MAX_PATH];
		char szDirectoryName[MAX_PATH];
		strcpy_s(szPath, szSrcFileName);
		p1 = strrchr(szPath, '\\') + 1;
		strcpy_s(szDirectoryName, p1 + 2); // Skip dir prefix (2 chars)
		szDirectoryName[strlen(szDirectoryName) - 2] = '\0'; // Remove dir suffix (2 chars)
		*p1 = '\0';
		if ((strlen(szPath) + strlen(rfbZipDirectoryPrefix) + strlen(szDirectoryName) + 4) > (MAX_PATH - 1)) return false;
		// sprintf_s(szSrcFileName, "%s%s%s%s", szPath, rfbZipDirectoryPrefix, szDirectoryName, ".zip"); 
		sprintf_s(szDirZipPath, "%s%s%s%s", szWorkingDir, rfbZipDirectoryPrefix, szDirectoryName, ".zip"); 
		strcat_s(szPath, szDirectoryName);
		strcpy_s(szDirectoryName, szPath);
		if (strlen(szDirectoryName) > (MAX_PATH - 4)) return -1;
		strcat_s(szDirectoryName, "\\*.*");
		sprintf_s(szStatus, " %s < %s > %s", sz_H64, szPath, sz_H65);
		SetStatus(szStatus);
		bool fZip = m_pZipUnZip->ZipDirectory(szPath, szDirectoryName, szDirZipPath/*szSrcFileName*/, true);
		if (fZip)
			sprintf_s(szStatus, " %s < %s > %s", sz_H66, szPath, sz_H67);
		else
			sprintf_s(szStatus, " %s < %s >. %s", sz_H68, szPath, sz_H69);
		SetStatus(szStatus);
		if (!fZip) return -1;
		strcpy_s(szSrcFileName, 292, szDirZipPath);
		return 1;
	}
	else
		return 0;
}

//
// sf@2004 - Delta Transfer
// Destination file already exists
// The server sends the checksums of this file in one shot.
// 
bool FileTransfer::ReceiveDestinationFileChecksums(int nSize, UINT nLen)
{
//	vnclog.Print(0, _T("ReceiveDestinationFileChecksums\n"));
	m_lpCSBuffer = new char [nLen+1]; //nSize
	if (m_lpCSBuffer == NULL) 
	{
		return false;
	}

	// char szStatus[255];
	// sprintf_s(szStatus, " Receiving %d bytes of file checksums from remote machine. Please wait...", nLen); 
	// SetStatus(szStatus);

	memset(m_lpCSBuffer, '\0', nLen+1); // nSize

	m_pCC->ReadExact((char *)m_lpCSBuffer, nLen);
	m_nCSBufferSize = nLen;

	return true;
}


//
//  SendFile 
// 
bool FileTransfer::SendFile(long lSize, UINT nLen)
{
//	vnclog.Print(0, _T("SendFile\n"));
	if (!m_fFTAllowed) return false;
	if (nLen == 0) return false; // Used when the local file could no be open in OfferLocalFile

	char *szRemoteFileName = new char [nLen+1];
	if (szRemoteFileName == NULL) return false;
	memset(szRemoteFileName, 0, nLen+1);

	// Read in the Name of the file to copy (remote full name !)
	m_pCC->ReadExact(szRemoteFileName, nLen);

	if (nLen > MAX_PATH)
		szRemoteFileName[MAX_PATH] = '\0';
	else
		szRemoteFileName[nLen] = '\0';
	
	char szStatus[MAX_PATH + 256];

	// If lSize = -1 (0xFFFFFFFF) that means that the Dst file on the remote machine
	// could not be created for some reason (locked..)
	if (lSize == -1)
	{
		sprintf_s(szStatus, " %s < %s > %s",sz_H12, get_real_filename(szRemoteFileName).c_str(),sz_H24);
		SetStatus(szStatus);
		sprintf_s(szStatus, " %s < %s > %s", sz_H25,get_real_filename(szRemoteFileName).c_str(),sz_H26);
		SetStatus(szStatus);
        m_fFileUploadError = true;

		delete [] szRemoteFileName;
		return false;
	}

	delete [] szRemoteFileName;

	// Open src file using Unicode path from m_szSrcFileNameW (set in OfferLocalFile)
	if (m_szSrcFileNameW[0] == L'\0')
		MultiByteToWideChar(CP_ACP, 0, m_szSrcFileName, -1, m_szSrcFileNameW, MAX_PATH + 32);
	m_hSrcFile = CreateFileW(
							m_szSrcFileNameW,		
							GENERIC_READ,		
							FILE_SHARE_READ,	
							NULL,				
							OPEN_EXISTING,		
							FILE_FLAG_SEQUENTIAL_SCAN,	
							NULL
							);				

	if (m_hSrcFile == INVALID_HANDLE_VALUE)
	{
		sprintf_s(szStatus, " %s < %s >", sz_H21, m_szSrcFileName); 
		SetStatus(szStatus);

        m_fFileUploadError = true;
		return false;
	}

	m_fFileUploadError = false;
	m_dwNbBytesRead = 0;
	m_dwTotalNbBytesRead = 0;
	m_fEof = false;

	// If the connection speed is > 2048 Kbit/s, no need to compress.
	m_fCompress = (m_pCC->kbitsPerSecond <= 2048);

	m_fFileUploadRunning = true;
    m_fFileUploadError = false;
	//m_dwLastChunkTime = timeGetTime();
	m_dwLastChunkTime = GetTickCount();
	m_dwStartTick = GetTickCount();
	// m_nNotSent = 0;
	// SendFileChunk();
	return true;

}


//
// Send the next file packet (upload)
// This function sends chunks for up to 50ms, then returns to allow UI processing.
// The timer calls this periodically. No more tight infinite loop.
// 
bool FileTransfer::SendFileChunk()
{
	DWORD dwStartTime = GetTickCount();
	const DWORD dwMaxTimePerCall = 50; // Max 50ms per call to maintain responsiveness

	while (true)
	{
		m_dwLastChunkTime = GetTickCount();

		if (!m_fFileUploadRunning)
			return false;

		if (m_fEof || m_fFileUploadError)
		{
			FinishFileSending();
			return true;
		}

		if (m_fAbort)
		{
			m_fFileUploadError = true;
			FinishFileSending();
			return false;
		}

		m_pCC->CheckFileChunkBufferSize(m_nBlockSize + 1024);

		int nRes = ReadFile(m_hSrcFile, m_pCC->m_filechunkbuf, m_nBlockSize, &m_dwNbBytesRead, NULL);
		if (!nRes && m_dwNbBytesRead != 0)
		{
			m_fFileUploadError = true;
			return false;
		}

		if (nRes && m_dwNbBytesRead == 0)
		{
			m_fEof = true;
			return true; // Next timer tick will call FinishFileSending
		}

		// sf@2004 - Delta Transfer
		bool fAlreadyThere = false;
		unsigned long nCS = 0;
		// if Checksums are available for this file
		if (m_lpCSBuffer != NULL)
		{
			if (m_nCSOffset < m_nCSBufferSize)
			{
				memcpy(&nCS, &m_lpCSBuffer[m_nCSOffset], 4);
				if (nCS != 0)
				{
					m_nCSOffset += 4;
					unsigned long cs = adler32(0L, Z_NULL, 0);
					cs = adler32(cs, m_pCC->m_filechunkbuf, (int)m_dwNbBytesRead);
					if (cs == nCS)
						fAlreadyThere = true;
				}
			}
		}

		if (fAlreadyThere)
		{
			// Send the FileTransferMsg with empty rfbFilePacket
			rfbFileTransferMsg ft;
			ft.type = rfbFileTransfer;
			ft.contentType = rfbFilePacket;
			ft.size = Swap32IfLE(2); // Means "Empty packet"// Swap32IfLE(nCS); 
			ft.length = Swap32IfLE(m_dwNbBytesRead);
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
			// m_nNotSent += m_dwNbBytesRead;
		}
		else
		{
			// Compress the data
			// (Compressed data can be longer if it was already compressed)
			unsigned int nMaxCompSize = m_nBlockSize + 1024; // TODO: Improve this...
			bool fCompressed = false;
			if (m_fCompress && !UsingOldProtocol())
			{
				m_pCC->CheckFileZipBufferSize(nMaxCompSize);
				int nRetC = compress((unsigned char*)(m_pCC->m_filezipbuf),
											(unsigned long*)&nMaxCompSize,	
											(unsigned char*)m_pCC->m_filechunkbuf,
											m_dwNbBytesRead
											);
				if (nRetC != 0)
				{
					// Todo: send data uncompressed instead
					m_fFileUploadError = true;
					return false;
				}					
				fCompressed = true;
			}

			// If data compressed is larger, we're presumably dealing with already compressed data.
			if (nMaxCompSize > m_dwNbBytesRead)
				fCompressed = false;

			// Send the FileTransferMsg with rfbFilePacket
			rfbFileTransferMsg ft;
			ft.type = rfbFileTransfer;
			ft.contentType = rfbFilePacket;
			ft.size = fCompressed ? Swap32IfLE(1) : Swap32IfLE(0); 
			ft.length = fCompressed ? Swap32IfLE(nMaxCompSize) : Swap32IfLE(m_dwNbBytesRead);
			//adzm 2010-09
			if(UsingOldProtocol())
				m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg);
			else
				m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);

			if (fCompressed)
				m_pCC->WriteExact((char *)m_pCC->m_filezipbuf, nMaxCompSize);
			else
				m_pCC->WriteExact((char *)m_pCC->m_filechunkbuf, m_dwNbBytesRead);
		}

		m_dwTotalNbBytesRead += m_dwNbBytesRead;

		// Refresh progress bar periodically
		SetGauge(hWnd, m_dwTotalNbBytesRead);

		// Check if we've been running too long - yield for UI responsiveness
		if (GetTickCount() - dwStartTime >= dwMaxTimePerCall)
			break;
	} // end while

	return true;
}


bool FileTransfer::FinishFileSending()
{
//	vnclog.Print(0, _T("FinishSendFile\n"));
	if (!m_fFileUploadRunning) return false;

	m_fFileUploadRunning = false;
    m_pCC->SetSendTimeout(0);

	char szStatus[512 + 256];

	CloseHandle(m_hSrcFile);
	
	if ( !m_fFileUploadError || m_fEof)
	{
		rfbFileTransferMsg ft;

		ft.type = rfbFileTransfer;
		ft.contentType = rfbEndOfFile;
		if (UsingOldProtocol())
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg);
		else
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
		sprintf_s(szStatus, " %s < %s > %s", sz_H17, m_szSrcFileName, sz_H27/*, (int)((lTotalComp * 100) / dwTotalNbBytesWritten), fCompress ? "C" : "N"*//*, szDstFileName*/); 
	}
	else // Error during File Transfer loop
	{
		rfbFileTransferMsg ft;
		ft.type = rfbFileTransfer;
		ft.contentType = rfbAbortFileTransfer;
		ft.contentParam = rfbFileTransferVersion;
		ft.length = 0;
		ft.size = 0;
        if (UsingOldProtocol())
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg);
		else
			m_pCC->WriteExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
		sprintf_s(szStatus, " %s < %s > %s", sz_H19, m_szSrcFileName, sz_H28); 
	}

	// If the transfered file is a Directory zip, we delete it locally, whatever the result of the transfer
	if (!strncmp(strrchr(m_szSrcFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)))
	{
		WCHAR szSrcFileNameW[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, m_szSrcFileName, -1, szSrcFileNameW, MAX_PATH);
		DeleteFileW(szSrcFileNameW);
		if (!m_fFileUploadError)
		{
			char szDirectoryName[MAX_PATH];
			char *p = strrchr(m_szSrcFileName, '\\');
			char *p1 = strchr(p, '-');
			strcpy_s(szDirectoryName, p1 + 1);
			szDirectoryName[strlen(szDirectoryName) - 4] = '\0'; // Remove '.zip'
			// sprintf_s(szStatus, " %s < %s > %s - Not really sent: %ld", sz_H66, szDirectoryName, sz_H70, m_nNotSent);
			sprintf_s(szStatus, " %s < %s > %s", sz_H66, szDirectoryName, sz_H70);
		}
	}

	SetStatus(szStatus);
	UpdateWindow(hWnd);

    // hide the stop button
    ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B2), SW_HIDE);
	// Sound notif
	//MessageBeep(-1);

	// Send next file in the list, if any
	OfferNextFile();

	// if (nRet) return true; else return false;
	// return !m_fFileUploadError;
	return true;
}



//
// Request the creation of a directory on the remote machine
//
void FileTransfer::CreateRemoteDirectory(LPSTR szDir)
{
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	ft.contentType = rfbCommand;
    ft.contentParam = rfbCDirCreate;
	ft.size = 0;
	
	// szDir is already UTF-8 when server supports Unicode (caller converts Unicode->UTF-8)
	char szDirToSend[MAX_PATH * 3];
	if (m_fServerSupportsUnicode)
	{
		strcpy_s(szDirToSend, szDir);
	}
	else
	{
		// Old server: convert UTF-8 -> Unicode -> CP_ACP
		WCHAR szDirW[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, szDir, -1, szDirW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, szDirW, -1, szDirToSend, MAX_PATH * 3, NULL, NULL);
	}
	
	ft.length = Swap32IfLE(strlen(szDirToSend));
	//adzm 2010-09
    m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	m_pCC->WriteExact((char *)szDirToSend, strlen(szDirToSend));
	return;
}


//
// Request the deletion of a file on the remote machine
//
void FileTransfer::DeleteRemoteFile(std::string szFile)
{
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	ft.contentType = rfbCommand;
    ft.contentParam = rfbCFileDelete;
	ft.size = 0;
	
	// szFile is already UTF-8 when server supports Unicode (caller converts Unicode->UTF-8)
	char szFileToSend[MAX_PATH * 3];
	if (m_fServerSupportsUnicode)
	{
		strcpy_s(szFileToSend, szFile.c_str());
	}
	else
	{
		// Old server: convert UTF-8 -> Unicode -> CP_ACP
		WCHAR szFileW[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, szFile.c_str(), -1, szFileW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, szFileW, -1, szFileToSend, MAX_PATH * 3, NULL, NULL);
	}
	
    size_t len = strlen(szFileToSend);
	ft.length = Swap32IfLE(len);
	//adzm 2010-09
    m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	m_pCC->WriteExact((char *)szFileToSend, len);
	return;
}

//
// Request the renaming of a file or a directory
// 
void FileTransfer::RenameRemoteFileOrDirectory(LPSTR szCurrentName, LPSTR szNewName)
{
	// szCurrentName and szNewName are already UTF-8 when server supports Unicode
	char szMsgContentToSend[(2 * MAX_PATH * 3) + 1];
	if (m_fServerSupportsUnicode)
	{
		// Already UTF-8, concatenate directly
		if (strlen(szCurrentName) + strlen(szNewName) + 2 > sizeof(szMsgContentToSend)) return;
		sprintf_s(szMsgContentToSend, "%s*%s", szCurrentName, szNewName);
	}
	else
	{
		// Old server: convert UTF-8 -> Unicode -> CP_ACP, then build message
		if (strlen(szCurrentName) > MAX_PATH || strlen(szNewName) > MAX_PATH) return;
		char szCurrentACP[MAX_PATH], szNewACP[MAX_PATH];
		WCHAR szW[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, szCurrentName, -1, szW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, szW, -1, szCurrentACP, MAX_PATH, NULL, NULL);
		MultiByteToWideChar(CP_UTF8, 0, szNewName, -1, szW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, szW, -1, szNewACP, MAX_PATH, NULL, NULL);
		sprintf_s(szMsgContentToSend, "%s*%s", szCurrentACP, szNewACP);
	}
	
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
	ft.contentType = rfbCommand;
    ft.contentParam = rfbCFileRename; // or rfbCDirRename ...
	ft.size = 0;
	ft.length = Swap32IfLE(strlen(szMsgContentToSend));
	//adzm 2010-09
    m_pCC->WriteExactQueue((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	m_pCC->WriteExact((char *)szMsgContentToSend, strlen(szMsgContentToSend));
	return;
}

//
// Server's response to a directory creation command
//
bool FileTransfer::CreateRemoteDirectoryFeedback(long lSize, UINT nLen)
{
	char *szRemoteName = new char [nLen+1];
	if (szRemoteName == NULL) return false;
	memset(szRemoteName, 0, nLen+1);
	m_pCC->ReadExact(szRemoteName, nLen);

	if (nLen > MAX_PATH)
		szRemoteName[MAX_PATH] = '\0';
	else
		szRemoteName[nLen] = '\0';
	
	char szStatus[MAX_PATH + 256];

	if (lSize == -1)
	{
		sprintf_s(szStatus, "%s < %s > %s", sz_H29,szRemoteName,sz_H30); 
		SetStatus(szStatus);
		delete [] szRemoteName;
		return false;
	}

	sprintf_s(szStatus, "%s < %s > %s",sz_H31, szRemoteName,sz_H32); 
	SetStatus(szStatus);
	// Refresh the remote list
	FTListViewClear(GetDlgItem(hWnd, IDC_REMOTE_FILELIST));
	RequestRemoteDirectoryContent(hWnd, "");

	delete [] szRemoteName;
	return true;
}


//
// Server's response to a File Deletion command
//
bool FileTransfer::DeleteRemoteFileFeedback(long lSize, UINT nLen)
{
	char *szRemoteName = new char [nLen+1];
	if (szRemoteName == NULL) return false;
	memset(szRemoteName, 0, nLen+1);
	m_pCC->ReadExact(szRemoteName, nLen);

	if (nLen > MAX_PATH)
		szRemoteName[MAX_PATH] = '\0';
	else
		szRemoteName[nLen] = '\0';

	// Decode UTF-8 server response to ACP for status display
	char szDisplayName[MAX_PATH * 3] = {0};
	if (m_fServerSupportsUnicode)
	{
		WCHAR szW[MAX_PATH]; szW[0] = L'\0';
		MultiByteToWideChar(CP_UTF8, 0, szRemoteName, -1, szW, MAX_PATH);
		WideCharToMultiByte(CP_ACP, 0, szW, -1, szDisplayName, MAX_PATH * 3, NULL, NULL);
	}
	else
		strcpy_s(szDisplayName, szRemoteName);
	
	char szStatus[MAX_PATH + 256];

    bool isDir = IsDirectoryGetIt(szRemoteName, nLen+1);
	if (lSize == -1)
	{
        sprintf_s(szStatus, "%s < %s > %s", isDir ? sz_H99: sz_H33, szDisplayName, sz_H30);
		SetStatus(szStatus);
		delete [] szRemoteName;
		return false;
	}
    sprintf_s(szStatus, "%s < %s > %s", isDir ? sz_H31 : sz_H17, szDisplayName, sz_H34);
	SetStatus(szStatus);
	// Refresh the remote list
	if (--m_nDeleteCount == 0)
    {
		FTListViewClear(GetDlgItem(hWnd, IDC_REMOTE_FILELIST));
	    RequestRemoteDirectoryContent(hWnd, "");
    }

	delete [] szRemoteName;
	return true;
}


//
// Server's response to a File Deletion command
//
bool FileTransfer::RenameRemoteFileOrDirectoryFeedback(long lSize, UINT nLen)
{
	if (nLen <= 0) return false;
	if (nLen > ((2 * MAX_PATH))) return false;

	char *szContent = new char [nLen+1];
	if (szContent == NULL) return false;
	memset(szContent, 0, nLen+1);
	m_pCC->ReadExact(szContent, nLen);
	szContent[nLen] = '\0';

	char szStatus[(2 * MAX_PATH) + 1 + 200];

	char *p = strrchr(szContent, '*');
	if (p==NULL)
	{
		sprintf_s(szStatus, " %s < %s > %s", sz_M5, "selected file", sz_H30); 
		SetStatus(szStatus);
		delete [] szContent;
		return false;
	}

	char szOldName[(2 * MAX_PATH) + 1];
	char szCurrentName[(2 * MAX_PATH) + 1];

	strcpy_s(szCurrentName, p + 1); 
	*p = '\0';
	strcpy_s(szOldName, szContent);

	if (lSize == -1)
	{
		sprintf_s(szStatus, " %s < %s > %s", sz_M5, szOldName, sz_H30); 
		SetStatus(szStatus);
		delete [] szContent;
		return false;
	}

	sprintf_s(szStatus, " %s < %s > %s < %s >", sz_M8 , szOldName, sz_M7, szCurrentName); 
	SetStatus(szStatus);
	// Refresh the remote list
	FTListViewClear(GetDlgItem(hWnd, IDC_REMOTE_FILELIST));
	RequestRemoteDirectoryContent(hWnd, "");

	delete [] szContent;
	return true;
}


//
// sf@2004 - Delta Transfer
// Create the checksums buffer of an open file
//
int FileTransfer::GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize)
{
	bool fEof = false;
	bool fError = false;
	DWORD dwNbBytesRead = 0;
	int nCSBufferOffset = 0;

	char* lpBuffer = new char[m_nBlockSize];
	if (!lpBuffer)
		return -1;

	while (!fEof)
	{
		if (!ReadFile(hFile, lpBuffer, m_nBlockSize, &dwNbBytesRead, NULL))
		{
			fError = true;
			break;
		}

		if (dwNbBytesRead == 0)
		{
			fEof = true;
		}
		else
		{
			if (nCSBufferOffset + 4 > nCSBufferSize)
			{
				fError = true;
				break;
			}

			unsigned long cs = adler32(0L, Z_NULL, 0);
			cs = adler32(cs, (unsigned char*)lpBuffer, (int)dwNbBytesRead);
			memcpy(lpCSBuffer + nCSBufferOffset, &cs, 4);
			nCSBufferOffset += 4;
		}
	}

	if (SetFilePointer(hFile, 0L, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		fError = true;
	}

	delete[] lpBuffer;

	return fError ? -1 : nCSBufferOffset;
}


bool FileTransfer::IsDirectoryGetIt(char* szName, int size)
{
	char szWork[MAX_PATH];
	strcpy_s(szWork, szName);
	if (szWork[0] == rfbDirPrefix[0] && szWork[1] == rfbDirPrefix[1])
	{
		strncpy_s(szName, size, szWork + 2, strlen(szWork) - 3); 
		szName[strlen(szWork) - 4] = '\0';
		return true;
	}	
	return false;
}

int FileTransfer::DoDialog()
{
	extern HINSTANCE m_hInstResDLL;
 	return DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_FILETRANSFER_DLG), 
		NULL, (DLGPROC) FileTransferDlgProc, (LONG_PTR) this);
}


//
// 2006 - Resizable File Transfer Window mod - By Roytam1 & and KP774
//
void FTAdjustLeft(LPRECT lprc)
{
	int cx = lprc->right - lprc->left - GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	if(cx < 611)
	{
		lprc->left = lprc->right - 611 - GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	}
}


void FTAdjustTop(LPRECT lprc)
{
	int cy = lprc->bottom - lprc->top - GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	if(cy < 429)
	{
		lprc->top = lprc->bottom - 429 - GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	}
}



void FTAdjustRight(LPRECT lprc)
{
	int cx = lprc->right - lprc->left - GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	if(cx < 611)
	{
		lprc->right = lprc->left + 611 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	}
}



void FTAdjustBottom(LPRECT lprc)
{
	int cy = lprc->bottom - lprc->top - GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	if(cy < 429)
	{
		lprc->bottom = lprc->top + 429 + GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	}
}


// sf@2006
void FTAdjustFileNameColumns(HWND hWnd)
{

	RECT rc;
	GetWindowRect(GetDlgItem(hWnd, IDC_LOCAL_FILELIST), &rc);
	int w = rc.right - rc.left;
	int cw = w - (70 + 100 + 25);
	if (cw < 120) cw = 120;

	LVCOLUMN Column;
	Column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER|LVCF_SUBITEM;
	Column.fmt = LVCFMT_LEFT;
	Column.cx = cw;
	Column.pszText = "Name";
	Column.iSubItem = 0;
	Column.iOrder = 0;

	HWND hWndList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
	ListView_SetColumn(hWndList, 0, &Column);
	hWndList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
	ListView_SetColumn(hWndList, 0, &Column);
}

//
// Subclass proc for IDC_CURR_LOCAL and IDC_CURR_REMOTE RichEdit path bars.
// Intercepts Enter key to trigger navigation to the typed/pasted path.
//
static LRESULT CALLBACK PathEditSubclassProc(HWND hEdit, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uMsg == WM_GETDLGCODE)
    {
        // Claim Enter key so IsDialogMessage doesn't swallow it; leave Tab alone
        LRESULT lr = DefSubclassProc(hEdit, uMsg, wParam, lParam);
        MSG *pMsg = (MSG *)lParam;
        if (pMsg && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
            return lr | DLGC_WANTMESSAGE;
        return lr;
    }
    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN)
    {
        HWND hDlg = GetParent(hEdit);
        FileTransfer *_this = (FileTransfer *)helper::SafeGetWindowUserData<FileTransfer>(hDlg);
        if (_this && !_this->m_fFileCommandPending)
        {
            char szPath[MAX_PATH + 1]; // +1 to safely append trailing backslash
            GetWindowText(hEdit, szPath, MAX_PATH);
            szPath[MAX_PATH] = '\0';
            // Ensure trailing backslash
            size_t len = strlen(szPath);
            if (len > 0 && szPath[len - 1] != '\\' && len < MAX_PATH)
            {
                szPath[len] = '\\';
                szPath[len + 1] = '\0';
            }
            if (uIdSubclass == IDC_CURR_LOCAL)
            {
                // Set the path into the control, then call with "" so it reads from IDC_CURR_LOCAL
                {
                    WCHAR szPathW[MAX_PATH + 1];
                    MultiByteToWideChar(CP_ACP, 0, szPath, -1, szPathW, MAX_PATH + 1);
                    SetDlgItemTextW(hDlg, IDC_CURR_LOCAL, szPathW);
                }
                _this->PopulateLocalListBox(hDlg, "");
            }
            else // IDC_CURR_REMOTE
            {
                _this->m_fFileCommandPending = true;
                SetWindowText(hEdit, szPath);
                _this->RequestRemoteDirectoryContent(hDlg, "");
            }
        }
        return 0; // swallow Enter
    }
    return DefSubclassProc(hEdit, uMsg, wParam, lParam);
}

//
//
//
BOOL CALLBACK FileTransfer::FileTransferDlgProc(  HWND hWnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam ) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. But we can get a pseudo-this from the parameter to 
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:
    FileTransfer *_this = helper::SafeGetWindowUserData<FileTransfer>(hWnd);

	switch (uMsg)
	{
	case WM_TIMER:
		{
			// We have to wait for NetBuf flush
			// !!!: Not necessary anymore - Pb solved on server side.
			// DWORD lTime = timeGetTime();
			// DWORD lLastTime = _this->m_pCC->m_lLastRfbRead;
			// DWORD lDelta = abs(timeGetTime() - _this->m_pCC->m_lLastRfbRead);
			if (true && wParam != 100/*meGetTime() - _this->m_pCC->m_lLastRfbRead) > 1000 */)
			{
				_this->m_fFileCommandPending = true;
				_this->RequestPermission();
				if (KillTimer(hWnd, _this->m_timer))
					_this->m_timer = 0;
			}
			else
				_this->m_pCC->SendKeepAlive(false, true);
		break;
		}

	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            helper::SafeSetWindowUserData(hWnd, lParam);

            FileTransfer *l_this = (FileTransfer *) lParam;
			SetTimer(hWnd, 100, 5000, NULL);
            // CentreWindow(hWnd);
			l_this->hWnd = hWnd;
			hFTWnd = hWnd;

			// Window always on top if Fullscreen On
			if (l_this->m_pCC->InFullScreenMode())
			{
				RECT Rect;
				GetWindowRect(hWnd, &Rect);
				SetWindowPos(hWnd, 
							HWND_TOPMOST,
							Rect.left,
							Rect.top,
							Rect.right - Rect.left,
							Rect.bottom - Rect.top,
							SWP_SHOWWINDOW);
			}

			SetForegroundWindow(hWnd);

			// Set the title 
			const long lTitleBufSize=256;			
			char szRemoteName[lTitleBufSize];
			char szTitle[lTitleBufSize];
			if (_snprintf_s(szRemoteName, 127 ,"%s", l_this->m_pCC->m_desktopName) < 0 )
			{
				szRemoteName[128-4]='.';
				szRemoteName[128-3]='.';
				szRemoteName[128-2]='.';
				szRemoteName[128-1]=0x00;
			}	
			_snprintf_s(szTitle, lTitleBufSize-1," %s < %s>  -  UltraVNC", sz_H35,szRemoteName);
			SetWindowText(hWnd, szTitle);

			// Create all the columns of the Files ListViews
			LVCOLUMN Column;
			Column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER|LVCF_SUBITEM;
			Column.fmt = LVCFMT_LEFT;
			Column.cx = 166;
			Column.pszText = "Name";
			Column.iSubItem = 0;
			Column.iOrder = 0;
			HWND hWndList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			ListView_InsertColumn(hWndList, 0, &Column);
			hWndList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
			ListView_InsertColumn(hWndList, 0, &Column);

			Column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER|LVCF_SUBITEM;
			Column.fmt = LVCFMT_LEFT;
			Column.cx = 70;
			Column.pszText = "Size";
			Column.iSubItem = 1;
			Column.iOrder = 1;
			hWndList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			ListView_InsertColumn(hWndList, 1, &Column);
			hWndList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
			ListView_InsertColumn(hWndList, 1, &Column);

			Column.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER|LVCF_SUBITEM;
			Column.fmt = LVCFMT_LEFT;
			Column.cx = 100;
			Column.pszText = "Modified";
			Column.iSubItem = 2;
			Column.iOrder = 2;
			hWndList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			ListView_InsertColumn(hWndList, 2, &Column);
			hWndList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
			ListView_InsertColumn(hWndList, 2, &Column);
			
			// Make the selection bar full width in the ListViews
			hWndList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
			hWndList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT); 

			// Create Icons List of ListViews
			l_this->InitListViewImagesList(GetDlgItem(hWnd, IDC_LOCAL_FILELIST));
			l_this->InitListViewImagesList(GetDlgItem(hWnd, IDC_REMOTE_FILELIST));
			
			// Create the status bar
			HWND hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD, sz_H36 , hWnd, IDC_STATUS);

			// Populate the Local listboxes with local drives
			l_this->ListDrives(hWnd);

			// adzm 2009-08-02 - Still list drives above in case this is incorrect, also to populate the dropdown combo
			if (lstrlen(l_this->m_szLastLocalPath) > 0) { 
				WCHAR szLastLocalPathW[MAX_PATH];
				MultiByteToWideChar(CP_ACP, 0, l_this->m_szLastLocalPath, -1, szLastLocalPathW, MAX_PATH);
				if (GetFileAttributesW(szLastLocalPathW) & FILE_ATTRIBUTE_DIRECTORY) {
					// let's try to use the last path
					l_this->PopulateLocalListBox(hWnd, l_this->m_szLastLocalPath); //PGM
				}
			}

			// Populate the remote listboxes with remote drives if allowed by the server
			if (false/*!_this->m_pCC->m_pEncrypt->IsEncryptionEnabled()*/)
			{
				/*
				if (!_this->m_fFileCommandPending)
				{
					_this->m_fFileCommandPending = true;
					// RequestRemoteDrives(_this);
					RequestPermission(_this);
					SetStatus(_this, " Connecting. Please Wait...");
				}
				*/
			}
			else
			{
				l_this->m_timer = SetTimer( hWnd, 3333,  1000, NULL);
				l_this->SetStatus(sz_H37);
			}

			// Subclass path edit controls so Enter key navigates to typed/pasted path
			SetWindowSubclass(GetDlgItem(hWnd, IDC_CURR_LOCAL),  PathEditSubclassProc, IDC_CURR_LOCAL,  0);
			SetWindowSubclass(GetDlgItem(hWnd, IDC_CURR_REMOTE), PathEditSubclassProc, IDC_CURR_REMOTE, 0);

			// Save original (translated) buttons labels 
			if (strlen(l_this->m_szDeleteButtonLabel) == 0)
			{
//				char szLbl[64];
				HWND hB = GetDlgItem(hWnd, IDC_DELETE_B);
				GetWindowText(hB, (LPSTR)l_this->m_szDeleteButtonLabel, 64);
			}

			if (strlen(l_this->m_szNewFolderButtonLabel) == 0)
			{
//				char szLbl[64];
				HWND hB = GetDlgItem(hWnd, IDC_NEWFOLDER_B);
				GetWindowText(hB, (LPSTR)l_this->m_szNewFolderButtonLabel, 64);
			}

			if (strlen(l_this->m_szRenameButtonLabel) == 0)
			{
//				char szLbl[64];
				HWND hB = GetDlgItem(hWnd, IDC_RENAME_B);
				GetWindowText(hB, (LPSTR)l_this->m_szRenameButtonLabel, 64);
			}

            l_this->CheckButtonState(hWnd);
            return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			return TRUE;

		case IDCANCEL:
            _this->EndFTSession();
			EndDialog(hWnd, FALSE);
			return TRUE;

		// Send selected files
		case IDC_UPLOAD_B:
			{
			if (_this->m_fFileCommandPending) break;

			int nSelected = -1;
			int nCount = 0;
			HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

			// If no destination is set, nothing to do.
			WCHAR szDstFileW[MAX_PATH + 32];
			GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szDstFileW, MAX_PATH + 32);
			if (!wcslen(szDstFileW)) break;

			// Build [..] mask for up-dir detection
			WCHAR szUpDirMaskW[16];
			WCHAR szPrefixW[4], szSuffixW[4];
			MultiByteToWideChar(CP_ACP, 0, rfbDirPrefix, -1, szPrefixW, 4);
			MultiByteToWideChar(CP_ACP, 0, rfbDirSuffix, -1, szSuffixW, 4);
			swprintf_s(szUpDirMaskW, L"%s..%s", szPrefixW, szSuffixW);

			// Get all selected files, check if they already exist on remote side
			_this->m_FilesList.clear();
			nCount = ListView_GetItemCount(hWndLocalList);
			_this->m_nConfirmAnswer = CONFIRM_YES;
			for (nSelected = 0; nSelected < nCount; nSelected++)
			{
				if(ListView_GetItemState(hWndLocalList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
				{
					// Get Unicode filename from lParam
					LVITEMW ItemW;
					memset(&ItemW, 0, sizeof(ItemW));
					ItemW.mask = LVIF_PARAM;
					ItemW.iItem = nSelected;
					SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
					std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
					if (!pNameW) continue;
					const WCHAR* szSelW = pNameW->c_str();

					if (_wcsicmp(szSelW, szUpDirMaskW) != 0)
					{
						bool fDirectory = (szSelW[0] == szPrefixW[0] && szSelW[1] == szPrefixW[1]);
						// Convert to CP_ACP for FileOrFolderExists (ANSI comparison)
						char szSelA[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, szSelW, -1, szSelA, MAX_PATH, NULL, NULL);
						if (_this->FileOrFolderExists(hWndRemoteList, szSelA))
						{
							if (_this->m_nConfirmAnswer == CONFIRM_YES || _this->m_nConfirmAnswer == CONFIRM_NO)
							{
								char szMes[MAX_PATH + 96];
								if (fDirectory)
									sprintf_s(szMes, "%s < %s >\n%s", sz_H71, szSelA, sz_H72);
								else
									sprintf_s(szMes, "%s < %s >\n%s", sz_H17, szSelA, sz_H38);
								_this->DoFTConfirmDialog(fDirectory ? sz_H101 : sz_H39, _T(szMes));
								if (_this->m_nConfirmAnswer == CONFIRM_NO)
									continue;
								if (_this->m_nConfirmAnswer == CONFIRM_NOALL)
									break;
							}
						}
						_this->m_FilesList.push_back(nSelected);
					}
				}
			}
			if (_this->m_FilesList.size() == 0) break;

			// Display Status
			char szLocalStatus[128];
			sprintf_s(szLocalStatus, " > %d %s", _this->m_FilesList.size(), sz_H40);
			SetDlgItemText(hWnd, IDC_LOCAL_STATUS, szLocalStatus);
			sprintf_s(szLocalStatus, "%s %d %s", sz_H41,_this->m_FilesList.size(),sz_H42);
			_this->SetStatus(szLocalStatus);

			_this->m_nFilesTransfered = 0;
			_this->m_nFilesToTransfer = _this->m_FilesList.size();

			// Disable buttons
			_this->DisableButtons(_this->hWnd);
			ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B), SW_SHOW);
			ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B2), SW_SHOW);

			// Get the first selected file Unicode filename from lParam
			_this->m_iFile = _this->m_FilesList.begin();
			{
				LVITEMW ItemW0;
				memset(&ItemW0, 0, sizeof(ItemW0));
				ItemW0.mask = LVIF_PARAM;
				ItemW0.iItem = *_this->m_iFile;
				SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW0);
				std::wstring* pNameW0 = reinterpret_cast<std::wstring*>(ItemW0.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);

				WCHAR szCurrLocalW[MAX_PATH];
				GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szCurrLocalW, MAX_PATH);
				if (!wcslen(szCurrLocalW)) break;
				if (pNameW0) wcscat_s(szCurrLocalW, pNameW0->c_str());

				// Store full Unicode local path for use in OfferLocalFile
				wcscpy_s(_this->m_szSrcFileNameW, szCurrLocalW);
				// CP_ACP version for legacy code paths that don't handle Chinese
				char szCurrLocal[MAX_PATH];
				WideCharToMultiByte(CP_ACP, 0, szCurrLocalW, -1, szCurrLocal, MAX_PATH, NULL, NULL);

				_this->m_fFileCommandPending = true;
				_this->m_fAbort = false;
				_this->m_fAborted = false;
				_this->m_fUserAbortedFileTransfer = false;
				if (!_this->OfferLocalFile(szCurrLocal))
					_this->SendFiles(-1, 0);
			}

			}
			break;

		// Receive selected files
		case IDC_DOWNLOAD_B:
			{
			if (_this->m_fFileCommandPending) break;
			{
			int nSelected = -1;
			int nCount = 0;
			HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

			// If no dst dir is selected, nothing to do
			char szCurrLocal[MAX_PATH];
			GetDlgItemText(hWnd, IDC_CURR_LOCAL, szCurrLocal, sizeof(szCurrLocal));
			if (!strlen(szCurrLocal)) break;

			// Build [..] mask for up-dir detection
			WCHAR szUpDirMaskW[16];
			WCHAR szPrefixW[4], szSuffixW[4];
			MultiByteToWideChar(CP_ACP, 0, rfbDirPrefix, -1, szPrefixW, 4);
			MultiByteToWideChar(CP_ACP, 0, rfbDirSuffix, -1, szSuffixW, 4);
			swprintf_s(szUpDirMaskW, L"%s..%s", szPrefixW, szSuffixW);

			// Get all selected files, check if they already exist on local side
			_this->m_FilesList.clear();
			nCount = ListView_GetItemCount(hWndRemoteList);
			_this->m_nConfirmAnswer = CONFIRM_YES;
			for (nSelected = 0; nSelected < nCount; nSelected++)
			{
				if(ListView_GetItemState(hWndRemoteList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
				{
					// Get Unicode filename from lParam
					LVITEMW ItemW;
					memset(&ItemW, 0, sizeof(ItemW));
					ItemW.mask = LVIF_PARAM;
					ItemW.iItem = nSelected;
					SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
					std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
					if (!pNameW) continue;
					const WCHAR* szSelW = pNameW->c_str();

					if (_wcsicmp(szSelW, szUpDirMaskW) != 0)
					{
						bool fDirectory = (szSelW[0] == szPrefixW[0] && szSelW[1] == szPrefixW[1]);
						char szSelA[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, szSelW, -1, szSelA, MAX_PATH, NULL, NULL);
						if (_this->FileOrFolderExists(hWndLocalList, szSelA))
						{
							if (_this->m_nConfirmAnswer == CONFIRM_YES || _this->m_nConfirmAnswer == CONFIRM_NO)
							{
								char szMes[MAX_PATH + 96];
								if (fDirectory)
									sprintf_s(szMes, "%s < %s >\n\n%s", sz_H73, szSelA, sz_H72);
								else
									sprintf_s(szMes, "%s < %s >\n\n%s", sz_H17, szSelA, sz_H43);
								_this->DoFTConfirmDialog(fDirectory ? sz_H101 : sz_H39, _T(szMes));
								if (_this->m_nConfirmAnswer == CONFIRM_NO)
									continue;
								if (_this->m_nConfirmAnswer == CONFIRM_NOALL)
									break;
							}
						}
						_this->m_FilesList.push_back(nSelected);
					}
				}
			}
			if (_this->m_FilesList.size() == 0) break;

			// Display Status
			char szRemoteStatus[128];
			sprintf_s(szRemoteStatus, " > %d %s", _this->m_FilesList.size(), sz_H40);
			SetDlgItemText(hWnd, IDC_REMOTE_STATUS, szRemoteStatus);
			sprintf_s(szRemoteStatus, "%s %d %s ", sz_H44, _this->m_FilesList.size(), sz_H45);
			_this->SetStatus(szRemoteStatus);

			_this->m_nFilesTransfered = 0;
			_this->m_nFilesToTransfer = _this->m_FilesList.size();

			// Disable buttons
			_this->DisableButtons(_this->hWnd);
			ShowWindow(GetDlgItem(_this->hWnd, IDC_ABORT_B), SW_SHOW);
			ShowWindow(GetDlgItem(_this->hWnd, IDC_ABORT_B2), SW_SHOW);

			// Get first file Unicode from lParam, build UTF-8 path for server
			_this->m_iFile = _this->m_FilesList.begin();
			{
				LVITEMW ItemW0;
				memset(&ItemW0, 0, sizeof(ItemW0));
				ItemW0.mask = LVIF_PARAM;
				ItemW0.iItem = *_this->m_iFile;
				SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW0);
				std::wstring* pNameW0 = reinterpret_cast<std::wstring*>(ItemW0.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);

				WCHAR szDstFileW[MAX_PATH + 32];
				GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szDstFileW, MAX_PATH + 32);
				if (!wcslen(szDstFileW)) break;
				if (pNameW0) wcscat_s(szDstFileW, pNameW0->c_str());

				char szDstFileUTF8[MAX_PATH * 3];
				WideCharToMultiByte(CP_UTF8, 0, szDstFileW, -1, szDstFileUTF8, MAX_PATH * 3, NULL, NULL);

				_this->m_fFileCommandPending = true;
				_this->m_fAbort = false;
				_this->m_fAborted = false;
				_this->m_fUserAbortedFileTransfer = false;
				_this->RequestRemoteFile(szDstFileUTF8);
			}
			}
			} 
			break;

		case IDC_LOCAL_ROOTB:
			{
			char ofDir[MAX_PATH];
			int nSelected = SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_GETCURSEL, 0, 0); 
			if (nSelected == -1) break;
			SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_GETLBTEXT, (WPARAM)nSelected, (LPARAM)ofDir); 
			//ofDir[4] = '\0'; // Hum...
			_this->PopulateLocalListBox(hWnd, ofDir);
			}
			break;

		case IDC_REMOTE_ROOTB:
			if (!_this->m_fFileCommandPending)
			{
				// _this->m_fFileCommandPending = true; // Move to after nSelected test
				char ofDir[MAX_PATH];
				int nSelected = SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_GETCURSEL, 0, 0); 
				if (nSelected == -1) break;
				_this->m_fFileCommandPending = true; // Moved // PGM @ Advantig 
				SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_GETLBTEXT, (WPARAM)nSelected, (LPARAM)ofDir); 
				//ofDir[4] = '\0'; // Hum...
				_this->RequestRemoteDirectoryContent(hWnd, ofDir);					
			}
			break;

		case IDC_LOCAL_UPB:
			{
			char szUpDirMask[16];
			sprintf_s(szUpDirMask, "%s..%s", rfbDirPrefix, rfbDirSuffix);
			_this->PopulateLocalListBox(hWnd, szUpDirMask);
			}
			break;

		case IDC_REMOTE_UPB:
			if (!_this->m_fFileCommandPending)
			{
				_this->m_fFileCommandPending = true;
				char szUpDirMask[16];
				sprintf_s(szUpDirMask, "%s..%s", rfbDirPrefix, rfbDirSuffix);
				_this->RequestRemoteDirectoryContent(hWnd, szUpDirMask);					
			}
			break;

		case IDC_ABORT_B:
			_this->m_fAbort = true;
            _this->m_fUserAbortedFileTransfer = true;
			ShowWindow(GetDlgItem(_this->hWnd, IDC_ABORT_B), SW_HIDE);
			break;
		
		case IDCANCEL2:
		case IDC_ABORT_B2:
			_this->m_fAbort = true;
			_this->m_fUserAbortedFileTransfer = true;
			_this->m_fUserForcedAbortedFileTransfer = true;
			closesocket(_this->m_pCC->m_sock);
			ShowWindow(GetDlgItem(_this->hWnd, IDC_ABORT_B), SW_HIDE);
			break;

		case IDC_DELETE_B:
			// Delete Local File
			if (_this->m_fFocusLocal)
			{
				HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);

				char szMes[MAX_PATH + 96];
				int nSelected = -1;

				WCHAR szCurrLocalW[MAX_PATH];
				GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szCurrLocalW, MAX_PATH);
				if (!wcslen(szCurrLocalW)) break;

				int nCount = ListView_GetItemCount(hWndLocalList);
				_this->m_nConfirmAnswer = CONFIRM_YES;
				for (nSelected = 0; nSelected < nCount; nSelected++)
				{
					if(ListView_GetItemState(hWndLocalList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
					{
						LVITEMW ItemW;
						memset(&ItemW, 0, sizeof(ItemW));
						ItemW.mask = LVIF_PARAM;
						ItemW.iItem = nSelected;
						SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
						std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
						if (!pNameW) continue;
						const WCHAR* szSelW = pNameW->c_str();
						bool isDir = (szSelW[0] == L'[' && szSelW[1] != L'.');

						GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szCurrLocalW, MAX_PATH);
						WCHAR szFullPathW[MAX_PATH];
						wcscpy_s(szFullPathW, szCurrLocalW);
						wcscat_s(szFullPathW, szSelW);

						char szSelA[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, szSelW, -1, szSelA, MAX_PATH, NULL, NULL);
						char szFullPathA[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, szFullPathW, -1, szFullPathA, MAX_PATH, NULL, NULL);

						if (_this->m_nConfirmAnswer == CONFIRM_YES || _this->m_nConfirmAnswer == CONFIRM_NO)
						{
							sprintf_s(szMes, "%s\n\n< %s > ?\n", isDir ? sz_H95 : sz_H48, szSelA);
							_this->DoFTConfirmDialog(isDir ? sz_H94 : sz_H47, _T(szMes));
							if (_this->m_nConfirmAnswer == CONFIRM_NO)
								continue;
							if (_this->m_nConfirmAnswer == CONFIRM_NOALL)
								break;
						}
						if (!_this->DeleteFileOrDirectory(szFullPathW))
						{
							sprintf_s(szMes, "%s < %s >", isDir ? sz_H97 : sz_H49, szFullPathA);
							_this->SetStatus(szMes);
							break;
						}
						sprintf_s(szMes, "%s < %s > %s", isDir ? sz_H31 : sz_H17, szFullPathA, sz_H50);
						_this->SetStatus(szMes);
					}
				}
				// Refresh the Local List view
				FTListViewClear(GetDlgItem(hWnd, IDC_LOCAL_FILELIST));
				_this->PopulateLocalListBox(hWnd, "");
			}
			else // Delete remote file
			{
				if (_this->m_fFileCommandPending) break;
				HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);

				char szMes[MAX_PATH + 96];
				int nSelected = -1;

				WCHAR szCurrRemoteW[MAX_PATH];
				GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szCurrRemoteW, MAX_PATH);
				if (!wcslen(szCurrRemoteW)) break;

				int nCount = ListView_GetItemCount(hWndRemoteList);
				_this->m_nConfirmAnswer = CONFIRM_YES;
				_this->m_nDeleteCount = 0;
				std::vector<std::string> pathsToDelete; // UTF-8 paths
				for (nSelected = 0; nSelected < nCount; nSelected++)
				{
					if(ListView_GetItemState(hWndRemoteList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
					{
						LVITEMW ItemW;
						memset(&ItemW, 0, sizeof(ItemW));
						ItemW.mask = LVIF_PARAM;
						ItemW.iItem = nSelected;
						SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
						std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
						if (!pNameW) continue;
						const WCHAR* szSelW = pNameW->c_str();
						bool isDir = (szSelW[0] == L'[' && szSelW[1] != L'.');

						GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szCurrRemoteW, MAX_PATH);
						WCHAR szFullPathW[MAX_PATH];
						wcscpy_s(szFullPathW, szCurrRemoteW);
						wcscat_s(szFullPathW, szSelW);

						char szSelA[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, szSelW, -1, szSelA, MAX_PATH, NULL, NULL);

						if (_this->m_nConfirmAnswer == CONFIRM_YES || _this->m_nConfirmAnswer == CONFIRM_NO)
						{
							sprintf_s(szMes, "%s\n\n< %s > ?\n", isDir ? sz_H96 : sz_H51, szSelA);
							_this->DoFTConfirmDialog(isDir ? sz_H94 : sz_H47, _T(szMes));
							if (_this->m_nConfirmAnswer == CONFIRM_NO)
								continue;
							if (_this->m_nConfirmAnswer == CONFIRM_NOALL)
								break;
						}
						_this->m_fFileCommandPending = true;
						// Convert Unicode full path to UTF-8 for server
						char szFullPathUTF8[MAX_PATH * 3];
						WideCharToMultiByte(CP_UTF8, 0, szFullPathW, -1, szFullPathUTF8, MAX_PATH * 3, NULL, NULL);
						_this->m_nDeleteCount++;
						pathsToDelete.push_back(std::string(szFullPathUTF8));
					}
				}
				for (auto& path : pathsToDelete)
				{
					_this->DeleteRemoteFile(path.c_str());
					::UpdateWindow(_this->hWnd);
				}
			}
			break;

		case IDC_NEWFOLDER_B:
			// Create Local Folder
			if (_this->m_fFocusLocal)
			{
				char szMes[MAX_PATH + 96];
				WCHAR szCurrLocalW[MAX_PATH];
				GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szCurrLocalW, MAX_PATH);
				if (!wcslen(szCurrLocalW)) break;
				memset(_this->m_szFTParam, '\0', sizeof(_this->m_szFTParam));
				_this->m_szFTParamW[0] = L'\0';
				_this->DoFTParamDialog(sz_H100,sz_H52);
				if (_this->m_szFTParamW[0] == L'\0' || (wcslen(szCurrLocalW) + wcslen(_this->m_szFTParamW)) > 248)
					break;
				WCHAR szFullLocalW[MAX_PATH];
				wcscpy_s(szFullLocalW, szCurrLocalW);
				wcscat_s(szFullLocalW, _this->m_szFTParamW);
				char szFolderName[MAX_PATH];
				_snprintf_s(szFolderName, MAX_PATH, "%s%s%s", rfbDirPrefix, _this->m_szFTParam, rfbDirSuffix);
				szFolderName[MAX_PATH - 1] = 0;
				if (_this->FileOrFolderExists(GetDlgItem(hWnd, IDC_LOCAL_FILELIST), szFolderName))
				{
					char szCurrLocalA[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, szFullLocalW, -1, szCurrLocalA, MAX_PATH, NULL, NULL);
					sprintf_s(szMes, "%s < %s >: %s", sz_H53, szCurrLocalA, sz_H102);
					_this->SetStatus(szMes);
					break;
				}
				char szCurrLocalA[MAX_PATH];
				WideCharToMultiByte(CP_ACP, 0, szFullLocalW, -1, szCurrLocalA, MAX_PATH, NULL, NULL);
				if (!CreateDirectoryW(szFullLocalW, NULL))
				{
					sprintf_s(szMes, "%s < %s >", sz_H53, szCurrLocalA);
					_this->SetStatus(szMes);
					break;
				}

				sprintf_s(szMes, "%s < %s > %s", sz_H54, szCurrLocalA, sz_H55);
				_this->SetStatus(szMes);

				// Refresh the Local List view
				FTListViewClear(GetDlgItem(hWnd, IDC_LOCAL_FILELIST));
				_this->PopulateLocalListBox(hWnd, "");

			}
			else // Create Remote Folder
			{
				if (_this->m_fFileCommandPending) break;
				WCHAR szCurrRemoteW[MAX_PATH];
				GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szCurrRemoteW, MAX_PATH);
				if (!wcslen(szCurrRemoteW)) break;
				memset(_this->m_szFTParam, '\0', sizeof(_this->m_szFTParam));
				_this->m_szFTParamW[0] = L'\0';
				_this->DoFTParamDialog(sz_H100,sz_H56);
				if (_this->m_szFTParamW[0] == L'\0' || (wcslen(szCurrRemoteW) + wcslen(_this->m_szFTParamW)) > 248)
					break;

				char szFolderName[MAX_PATH];
                _snprintf_s(szFolderName, MAX_PATH, "%s%s%s", rfbDirPrefix, _this->m_szFTParam, rfbDirSuffix);
                szFolderName[MAX_PATH - 1] = 0;
				if (_this->FileOrFolderExists(GetDlgItem(hWnd, IDC_REMOTE_FILELIST), szFolderName))
				{
					char szCurrRemoteA[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, szCurrRemoteW, -1, szCurrRemoteA, MAX_PATH, NULL, NULL);
                    char szStatus[MAX_PATH + 128];
					sprintf_s(szStatus, "%s < %s > %s: %s", sz_H29, szCurrRemoteA, sz_H30, sz_H102);
					_this->SetStatus(szStatus);
					break;
				}

				_this->m_fFileCommandPending = true;
				{
					// Build full Unicode path and convert to UTF-8 for server
					wcscat_s(szCurrRemoteW, _this->m_szFTParamW);
					char szCurrRemoteUTF8[MAX_PATH * 3];
					WideCharToMultiByte(CP_UTF8, 0, szCurrRemoteW, -1, szCurrRemoteUTF8, MAX_PATH * 3, NULL, NULL);
					_this->CreateRemoteDirectory(szCurrRemoteUTF8);
				}
			}
			break;

		case IDC_RENAME_B:
		{
		if (_this->m_fFocusLocal)
		{
			HWND hWndLocalList = GetDlgItem(hWnd, IDC_LOCAL_FILELIST);
			int nCount = ListView_GetSelectedCount(hWndLocalList);
			if (nCount == 0 || nCount > 1)
			{
				yesUVNCMessageBox(m_hInstResDLL, _this->hWnd, sz_M1, sz_M2, MB_ICONINFORMATION);
				break; 
			}

			char szMes[MAX_PATH + 96];
			WCHAR szCurrLocalW[MAX_PATH];
			GetDlgItemTextW(hWnd, IDC_CURR_LOCAL, szCurrLocalW, MAX_PATH);
			if (!wcslen(szCurrLocalW)) break;

			nCount = ListView_GetItemCount(hWndLocalList);
			for (int nSelected = 0; nSelected < nCount; nSelected++)
			{
				if (ListView_GetItemState(hWndLocalList, nSelected, LVIS_SELECTED) & LVIS_SELECTED)
				{
					LVITEMW ItemW;
					memset(&ItemW, 0, sizeof(ItemW));
					ItemW.mask = LVIF_PARAM;
					ItemW.iItem = nSelected;
					SendMessageW(hWndLocalList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
					std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
					if (!pNameW) break;
					const WCHAR* szSelW = pNameW->c_str();
					char szSelA[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, szSelW, -1, szSelA, MAX_PATH, NULL, NULL);
					_this->IsDirectoryGetIt(szSelA, MAX_PATH);
					strcpy_s(_this->m_szFTParam, szSelA);
					_this->m_szFTParamW[0] = L'\0';
					_this->DoFTParamDialog(sz_M3, sz_M4);
					if (_this->m_szFTParamW[0] == L'\0' || (wcslen(szCurrLocalW) + wcslen(_this->m_szFTParamW)) > 248)
						break;
					TCHAR szFolderName[MAX_PATH];
					_snprintf_s(szFolderName, MAX_PATH, "%s%s%s", rfbDirPrefix, _this->m_szFTParam, rfbDirSuffix);
					szFolderName[MAX_PATH - 1] = 0;
					if ((_this->FileOrFolderExists(GetDlgItem(hWnd, IDC_LOCAL_FILELIST), szFolderName)) ||
					   (_this->FileOrFolderExists(GetDlgItem(hWnd, IDC_LOCAL_FILELIST), _this->m_szFTParam)))
					{
						sprintf_s(szMes, "%s < %s >: %s", sz_M5, szSelA, sz_H102);
						_this->SetStatus(szMes);
						break;
					}
					WCHAR szOldPathW[MAX_PATH], szNewPathW[MAX_PATH];
					wcscpy_s(szOldPathW, szCurrLocalW);
					wcscat_s(szOldPathW, szSelW);
					wcscpy_s(szNewPathW, szCurrLocalW);
					wcscat_s(szNewPathW, _this->m_szFTParamW);
					char szOldPathA[MAX_PATH], szNewPathA[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, szOldPathW, -1, szOldPathA, MAX_PATH, NULL, NULL);
					WideCharToMultiByte(CP_ACP, 0, szNewPathW, -1, szNewPathA, MAX_PATH, NULL, NULL);
					if (!MoveFileW(szOldPathW, szNewPathW))
					{
						sprintf_s(szMes, "%s < %s >", sz_M5, szOldPathA);
						_this->SetStatus(szMes);
						break;
					}
					sprintf_s(szMes, "%s < %s > %s < %s >", sz_M6, szOldPathA, sz_M7, szNewPathA);
					_this->SetStatus(szMes);
				}
			}
			FTListViewClear(GetDlgItem(hWnd, IDC_LOCAL_FILELIST));
			_this->PopulateLocalListBox(hWnd, "");
		}
		else // Rename Remote
		{
			if (_this->m_fFileCommandPending) break;
			HWND hWndRemoteList = GetDlgItem(hWnd, IDC_REMOTE_FILELIST);
			int nCount = ListView_GetSelectedCount(hWndRemoteList);
			if (nCount == 0 || nCount > 1)
			{
				yesUVNCMessageBox(m_hInstResDLL, _this->hWnd, sz_M1, sz_M2, MB_ICONINFORMATION);
				break; 
			}

			WCHAR szCurrRemoteW[MAX_PATH];
			GetDlgItemTextW(hWnd, IDC_CURR_REMOTE, szCurrRemoteW, MAX_PATH);
			if (!wcslen(szCurrRemoteW)) break;

			nCount = ListView_GetItemCount(hWndRemoteList);
			for (int nSel = 0; nSel < nCount; nSel++)
			{
				if (ListView_GetItemState(hWndRemoteList, nSel, LVIS_SELECTED) & LVIS_SELECTED)
				{
					LVITEMW ItemW;
					memset(&ItemW, 0, sizeof(ItemW));
					ItemW.mask = LVIF_PARAM;
					ItemW.iItem = nSel;
					SendMessageW(hWndRemoteList, LVM_GETITEMW, 0, (LPARAM)&ItemW);
					std::wstring* pNameW = reinterpret_cast<std::wstring*>(ItemW.lParam & ~(LPARAM)FT_LPARAM_UNREADABLE);
					if (!pNameW) break;
					const WCHAR* szSelW = pNameW->c_str();
					char szSelA[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, szSelW, -1, szSelA, MAX_PATH, NULL, NULL);
					_this->IsDirectoryGetIt(szSelA, MAX_PATH);
					strcpy_s(_this->m_szFTParam, szSelA);
					_this->m_szFTParamW[0] = L'\0';
					_this->DoFTParamDialog(sz_M3, sz_M4);
					if (_this->m_szFTParamW[0] == L'\0'
						|| wcschr(_this->m_szFTParamW, L'*') != NULL
						|| (wcslen(szCurrRemoteW) + wcslen(_this->m_szFTParamW)) > 248)
						break;
					TCHAR szFolderName[MAX_PATH];
					_snprintf_s(szFolderName, MAX_PATH, "%s%s%s", rfbDirPrefix, _this->m_szFTParam, rfbDirSuffix);
					szFolderName[MAX_PATH - 1] = 0;
					if ((_this->FileOrFolderExists(GetDlgItem(hWnd, IDC_REMOTE_FILELIST), szFolderName)) ||
					   (_this->FileOrFolderExists(GetDlgItem(hWnd, IDC_REMOTE_FILELIST), _this->m_szFTParam)))
					{
						char szMes[MAX_PATH + 256];
						sprintf_s(szMes, "%s < %s > %s: %s", sz_M5, szSelA, sz_H30, sz_H102);
						_this->SetStatus(szMes);
						break;
					}
					WCHAR szOldPathW[MAX_PATH], szNewPathW[MAX_PATH];
					wcscpy_s(szOldPathW, szCurrRemoteW);
					wcscat_s(szOldPathW, szSelW);
					wcscpy_s(szNewPathW, szCurrRemoteW);
					wcscat_s(szNewPathW, _this->m_szFTParamW);
					char szOldPathUTF8[MAX_PATH * 3], szNewPathUTF8[MAX_PATH * 3];
					WideCharToMultiByte(CP_UTF8, 0, szOldPathW, -1, szOldPathUTF8, MAX_PATH * 3, NULL, NULL);
					WideCharToMultiByte(CP_UTF8, 0, szNewPathW, -1, szNewPathUTF8, MAX_PATH * 3, NULL, NULL);
					_this->m_fFileCommandPending = true;
					_this->RenameRemoteFileOrDirectory(szOldPathUTF8, szNewPathUTF8);
				}
			}
		}
		}
		break;

	case IDC_LOCAL_DRIVECB: 
            switch (HIWORD(wParam)) 
            { 
	        case CBN_SELCHANGE:
				{
				char ofDir[MAX_PATH];
				int nSelected = SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_GETCURSEL, 0, 0); 
				if (nSelected == -1) break;
				SendMessage(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), CB_GETLBTEXT, (WPARAM)nSelected, (LPARAM)ofDir); 
				_this->PopulateLocalListBox(hWnd, ofDir);
				break;
				}
		}
		break;

	case IDC_REMOTE_DRIVECB:
            switch (HIWORD(wParam)) 
            { 
	        case CBN_SELCHANGE: 
				{
				char ofDir[MAX_PATH];
				int nSelected = SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_GETCURSEL, 0, 0); 
				if (nSelected == -1) break;
				SendMessage(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), CB_GETLBTEXT, (WPARAM)nSelected, (LPARAM)ofDir); 
				_this->RequestRemoteDirectoryContent(hWnd, ofDir);					
				break;
				}
		}
		break;

	case IDC_HIDE_B:
		_this->ShowFileTransferWindow(false);
		return TRUE;

	}
	break;


	case WM_SYSCOMMAND:
		switch (LOWORD(wParam))
		{
		case SC_RESTORE:
			_this->ShowFileTransferWindow(true);
			return TRUE;

		case SC_MINIMIZE:
			SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(IDC_HIDE_B,0),0);
			return FALSE;

		}
		break;

	case WM_SIZING:

		LPRECT lprc;
		lprc = (LPRECT)lParam;
		switch(wParam)
        {
		case WMSZ_TOPLEFT:
			FTAdjustTop(lprc);
			FTAdjustLeft(lprc);

		case WMSZ_TOP:
			FTAdjustTop(lprc);

		case WMSZ_TOPRIGHT:
			FTAdjustTop(lprc);
			FTAdjustRight(lprc);

		case WMSZ_LEFT:
			FTAdjustLeft(lprc);

		case WMSZ_RIGHT:
			FTAdjustRight(lprc);

		case WMSZ_BOTTOMLEFT:
			FTAdjustBottom(lprc);
			FTAdjustLeft(lprc);

		case WMSZ_BOTTOM:
			FTAdjustBottom(lprc);

		case WMSZ_BOTTOMRIGHT:
			FTAdjustBottom(lprc);
			FTAdjustRight(lprc);
		}
		return TRUE;


	case WM_SIZE:

		int cx;
		int cy;
		int icx;
		int icy;
		int icyb;
		int lf_an;
		int rg_an;
		RECT rc;
		RECT rcLMStatic;
		RECT rcUpload;
		RECT rcStatus;
		RECT rcHistory;
		RECT rcProgress;
		RECT rcRoot;
		int buttonUploadWidth;
		int buttonUploadHeight;
		int buttonStatusWidth;
		int buttonStatusHeight;
		int buttonHistoryWidth;
		int buttonHistoryHeight;
		int buttonProgressWidth;
		int buttonProgressHeight;
		int buttonRootWidth;
		int buttonRootHeight;
		int foced_closed;
		int topbuttonsHeight;
		int buttonLMStaticWidth;

		if(wParam == SIZE_MINIMIZED)
		{
			break;
		}

		GetWindowRect(GetDlgItem(hWnd, IDC_UPLOAD_B), &rcUpload);
		GetWindowRect(GetDlgItem(hWnd, IDC_LOCAL_STATUS), &rcStatus);
		GetWindowRect(GetDlgItem(hWnd, IDC_HISTORY_CB), &rcHistory);
		GetWindowRect(GetDlgItem(hWnd, IDC_PROGRESS), &rcProgress);
		GetWindowRect(GetDlgItem(hWnd, IDC_LOCAL_ROOTB), &rcRoot);
		GetWindowRect(GetDlgItem(hWnd, IDC_LM_STATIC), &rcLMStatic);

		buttonUploadWidth = rcUpload.right - rcUpload.left;
		buttonUploadHeight = rcUpload.bottom - rcUpload.top;

		buttonStatusWidth = rcStatus.right - rcStatus.left;
		buttonStatusHeight = rcStatus.bottom - rcStatus.top;

		buttonHistoryWidth = rcHistory.right - rcHistory.left;
		buttonHistoryHeight = rcHistory.bottom - rcHistory.top;

		buttonProgressWidth = rcProgress.right - rcProgress.left;
		buttonProgressHeight = rcProgress.bottom - rcProgress.top;

		buttonRootWidth = rcRoot.right - rcRoot.left;
		buttonRootHeight = rcRoot.bottom - rcRoot.top;

		buttonLMStaticWidth = rcLMStatic.right - rcLMStatic.left;

		foced_closed = buttonStatusHeight + buttonHistoryHeight + buttonProgressHeight + 12;
		topbuttonsHeight = buttonUploadHeight * 2 + 12;

		cx = LOWORD(lParam);	//Client Width
		cy = HIWORD(lParam);	//Client Height
		icy = cy- topbuttonsHeight - (buttonUploadHeight * 4); 
		lf_an=(cx - buttonUploadWidth)/2 -12;

		GetWindowRect(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), &rc);
		icx = lf_an - (buttonRootWidth + 4) * 2 - buttonLMStaticWidth - 4;
		rg_an = buttonUploadWidth + 20;
		

		//Left
		
		MoveWindow(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB),              4,       4, icx,  rc.bottom - rc.top, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_LM_STATIC),            4+icx+7,       4, buttonLMStaticWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_LOCAL_ROOTB),    4+icx+7+ buttonLMStaticWidth +7,       4, buttonRootWidth, buttonRootHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_LOCAL_UPB), 4+icx+7+ buttonLMStaticWidth +7+ buttonRootWidth +4,       4, buttonRootWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_CURR_LOCAL),                 4, buttonUploadHeight + 4, lf_an, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_LOCAL_FILELIST),             4, (buttonUploadHeight + 4)*2, lf_an, icy, TRUE);

		//Right
		GetWindowRect(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), &rc);
		MoveWindow(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB),               lf_an+ rg_an,       4, icx,   rc.bottom - rc.top, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_RM_STATIC),              lf_an+ rg_an +icx+7,       4, buttonLMStaticWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_REMOTE_ROOTB),     lf_an+ rg_an +icx+7+ buttonLMStaticWidth +7,       4, buttonRootWidth, buttonRootHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_REMOTE_UPB),  lf_an+ rg_an +icx+7+ buttonLMStaticWidth +7+ buttonRootWidth +4,       4, buttonRootWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_CURR_REMOTE),                  lf_an+ rg_an, buttonUploadHeight + 4, lf_an, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_REMOTE_FILELIST),              lf_an+ rg_an, (buttonUploadHeight + 4) * 2, lf_an, icy, TRUE);
		

		//Bottom
		icyb = icy + 7 + (buttonUploadHeight + 4) * 2;
		MoveWindow(GetDlgItem(hWnd, IDC_LOCAL_STATUS), 4, icyb, lf_an, buttonStatusHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_REMOTE_STATUS), lf_an + rg_an, icyb, lf_an, buttonStatusHeight, TRUE);

		MoveWindow(GetDlgItem(hWnd, IDC_HS_STATIC), 8, icyb + 7 + buttonStatusHeight, buttonUploadWidth, buttonStatusHeight, TRUE);
		GetWindowRect(GetDlgItem(hWnd, IDC_HISTORY_CB), &rc);
		MoveWindow(GetDlgItem(hWnd, IDC_HISTORY_CB), buttonUploadWidth + 4, icyb + 7 + buttonStatusHeight, cx - buttonUploadWidth * 4,  rc.bottom-rc.top, TRUE);

		MoveWindow(GetDlgItem(hWnd, IDC_PR_STATIC), 8, icyb + 7 + buttonStatusHeight * 2, buttonUploadWidth, buttonProgressHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_PROGRESS), buttonUploadWidth + 4, icyb + 7 + buttonStatusHeight * 2, cx - buttonUploadWidth *4, buttonProgressHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_PERCENT), cx - buttonUploadWidth * 2 - 12 , icyb + 7 + buttonStatusHeight * 2, buttonUploadWidth, buttonProgressHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_GLOBAL_STATUS), cx - buttonUploadWidth * 1 - 18, icyb + 7 + buttonStatusHeight * 2, buttonUploadWidth, buttonProgressHeight, TRUE);
		
		

		GetWindowRect(GetDlgItem(hWnd, IDC_STATUS), &rc);
		MoveWindow(GetDlgItem(hWnd, IDC_STATUS),                     0, cy-(rc.bottom-rc.top),     cx,  rc.bottom-rc.top, TRUE);

		//Center
		
		MoveWindow(GetDlgItem(hWnd, IDC_UPLOAD_B),     lf_an+10+2, icy - (buttonUploadHeight + 4) * 12, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_DOWNLOAD_B),   lf_an+10+2, icy - (buttonUploadHeight + 4) * 11, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_ABORT_B),      lf_an+10+2, icy - (buttonUploadHeight + 4) * 10, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_ABORT_B2),	   lf_an+10+2, icy - (buttonUploadHeight + 4) * 9, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_DELETE_B),     lf_an+10+2, icy - (buttonUploadHeight + 4) * 7, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_NEWFOLDER_B),  lf_an+10+2, icy - (buttonUploadHeight + 4) * 6, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_RENAME_B),     lf_an+10+2, icy - (buttonUploadHeight + 4) * 5, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDC_HIDE_B),       lf_an+10+2, icy - (buttonUploadHeight + 4) * 2, buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDCANCEL),         lf_an+10+2, icy - (buttonUploadHeight + 4), buttonUploadWidth, buttonUploadHeight, TRUE);
		MoveWindow(GetDlgItem(hWnd, IDCANCEL2),        lf_an+10+2, icy, buttonUploadWidth, buttonUploadHeight, TRUE);
		InvalidateRect(hWnd, NULL, FALSE);

		FTAdjustFileNameColumns(hWnd); // sf@2006

		return TRUE;


	// Messages from ListViews
	case WM_NOTIFY:
		{
			LPNMLISTVIEW lpNmlv = (LPNMLISTVIEW) lParam;
			LV_DISPINFO *pLvdi = (LV_DISPINFO*) lParam;


			switch(lpNmlv->hdr.code)
			{
			case HDN_ITEMCLICK:
				return TRUE;

			case NM_CUSTOMDRAW:
				{
					LPNMLVCUSTOMDRAW lpNmlvcd = (LPNMLVCUSTOMDRAW)lParam;
					if (lpNmlvcd->nmcd.hdr.hwndFrom != GetDlgItem(hWnd, IDC_LOCAL_FILELIST) &&
						lpNmlvcd->nmcd.hdr.hwndFrom != GetDlgItem(hWnd, IDC_REMOTE_FILELIST))
					{
						break;
					}

					switch (lpNmlvcd->nmcd.dwDrawStage)
					{
					case CDDS_PREPAINT:
						SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
						return TRUE;
					case CDDS_ITEMPREPAINT:
						{
							LVITEMW ItemW;
							memset(&ItemW, 0, sizeof(ItemW));
							ItemW.mask = LVIF_PARAM;
							ItemW.iItem = (int)lpNmlvcd->nmcd.dwItemSpec;
							ItemW.iSubItem = 0;
							if (SendMessageW(lpNmlvcd->nmcd.hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&ItemW))
							{
								if ((ItemW.lParam & FT_LPARAM_UNREADABLE) == FT_LPARAM_UNREADABLE)
								{
									lpNmlvcd->clrText = RGB(255, 0, 0);
									SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
									return TRUE;
								}
							}
							SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
							return TRUE;
						}
					}
				}
				break;

			case NM_SETFOCUS:
				if (lpNmlv->hdr.hwndFrom == GetDlgItem(hWnd, IDC_LOCAL_FILELIST))
				{
					_this->m_fFocusLocal = true;
                    _this->CheckButtonState(hWnd);
					char szTxt[64];
					HWND hB = GetDlgItem(hWnd, IDC_DELETE_B);
					sprintf_s(szTxt, "<- %s", _this->m_szDeleteButtonLabel);
					SetWindowText(hB, szTxt);
					hB = GetDlgItem(hWnd, IDC_NEWFOLDER_B);
					sprintf_s(szTxt, "<- %s", _this->m_szNewFolderButtonLabel);
					SetWindowText(hB, szTxt);
					hB = GetDlgItem(hWnd, IDC_RENAME_B);
					sprintf_s(szTxt, "<- %s", _this->m_szRenameButtonLabel);
					SetWindowText(hB, szTxt);
				}

				if (lpNmlv->hdr.hwndFrom == GetDlgItem(hWnd, IDC_REMOTE_FILELIST))
				{
					_this->m_fFocusLocal = false;
                    _this->CheckButtonState(hWnd);
					char szTxt[64];
					HWND hB = GetDlgItem(hWnd, IDC_DELETE_B);
					sprintf_s(szTxt, "%s ->", _this->m_szDeleteButtonLabel);
					SetWindowText(hB, szTxt);
					hB = GetDlgItem(hWnd, IDC_NEWFOLDER_B);
					sprintf_s(szTxt, "%s ->", _this->m_szNewFolderButtonLabel);
					SetWindowText(hB, szTxt);
					hB = GetDlgItem(hWnd, IDC_RENAME_B);
					sprintf_s(szTxt, "%s ->", _this->m_szRenameButtonLabel);
					SetWindowText(hB, szTxt);
				}

				return TRUE;

			case NM_DBLCLK:
				if (lpNmlv->hdr.hwndFrom == GetDlgItem(hWnd, IDC_LOCAL_FILELIST))
				{
					_this->PopulateLocalListBox(hWnd, "");
                    _this->CheckButtonState(hWnd);
				}

				if (lpNmlv->hdr.hwndFrom == GetDlgItem(hWnd, IDC_REMOTE_FILELIST))
				{
					if (!_this->m_fFileCommandPending)
					{
						_this->m_fFileCommandPending = true;
						_this->RequestRemoteDirectoryContent(hWnd, "");
					}
				}
				return TRUE;

			// sf@2006 - 2007 - Added in the ability to sort on the headers of the ListView
			// Modif initiated by TAW. Thanks !
			case LVN_COLUMNCLICK:
				if (lpNmlv->hdr.hwndFrom == GetDlgItem(hWnd, IDC_LOCAL_FILELIST))
				{
					int c = (LPARAM)lpNmlv->iSubItem;
					_this->bSortDirectionsL[c] = !(_this->bSortDirectionsL[c]);
					::SendMessage(lpNmlv->hdr.hwndFrom,
								LVM_FIRST+81 /*LVM_SORTITEMSEX*/,
								(WPARAM)(LPARAM)(_this->bSortDirectionsL[c] ? c+1 : ((c+1) * -1)),
								(LPARAM)(PFNLVCOMPARE)ListViewLocalCompareProc);
				}

				if (lpNmlv->hdr.hwndFrom == GetDlgItem(hWnd, IDC_REMOTE_FILELIST))
				{
					int c = (LPARAM)lpNmlv->iSubItem;
					_this->bSortDirectionsR[c] = !(_this->bSortDirectionsR[c]);
					::SendMessage(lpNmlv->hdr.hwndFrom,
								LVM_FIRST+81 /*LVM_SORTITEMSEX*/,
								(WPARAM)(LPARAM)(_this->bSortDirectionsR[c] ? c+1+10 : ((c+1+10) * -1)),
								(LPARAM)(PFNLVCOMPARE)ListViewLocalCompareProc);
				}
				return TRUE;
			}
		}
		break;


	case WM_DESTROY:
		if (_this->m_timer != 0) KillTimer(hWnd, _this->m_timer);
		KillTimer(hWnd, 100);
		EndDialog(hWnd, FALSE);
		return TRUE;

	case WM_SHOWWINDOW:
		// adzm - fix for window not restoring when hidden via show desktop, win+d, etc
		if (!wParam) {
			_this->m_fVisible = false;
		}
		break;

	}
	/*
	// Process File Transfer asynchronous Send Packet Message
	if (uMsg == FileTransferSendPacketMessage)
	{
		_this->SendFileChunk();
		return 0;
	}
	*/

	return 0;
}


//
// Params acquisition Dialog Box
// 
int FileTransfer::DoFTParamDialog(LPSTR szTitle, LPSTR szComment)
{
	extern HINSTANCE m_hInstResDLL;
	strcpy_s(m_szFTParamTitle, szTitle);
	strcpy_s(m_szFTParamComment, szComment);
	return DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_FTPARAM_DLG), hWnd, (DLGPROC) FTParamDlgProc, (LONG_PTR) this);
}


BOOL CALLBACK FileTransfer::FTParamDlgProc(  HWND hwnd,  UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    FileTransfer *_this = helper::SafeGetWindowUserData<FileTransfer>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            helper::SafeSetWindowUserData(hwnd, lParam);
			_this = (FileTransfer *) lParam;
			//CentreWindow(hwnd);

			// Set Title
			SetWindowText(hwnd, _this->m_szFTParamTitle);
			// Set Comment
			SetDlgItemText(hwnd, IDC_FTPARAMCOMMENT, _this->m_szFTParamComment);
			//S et param initial value
			SetDlgItemText(hwnd, IDC_FTPARAM_EDIT, _this->m_szFTParam);

			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
        case IDC_FTPARAM_EDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                size_t length = GetWindowTextLength(GetDlgItem(hwnd, IDC_FTPARAM_EDIT));
                bool stringOk = length > 0;

                if (stringOk)
                {
                    std::string text;
                    text.resize(length+1);
                    GetDlgItemText(hwnd,  IDC_FTPARAM_EDIT, &*text.begin(), length+1);

                    std::string::size_type pos;
                    pos = text.find_first_not_of(" \t\n\r");
                    if (pos == std::string::npos || pos >= strlen(text.c_str()))
                       stringOk = false;
                }

                EnableWindow(GetDlgItem(hwnd, IDOK), stringOk);
            }
            break;

		case IDOK:
			{
				GetDlgItemTextW(hwnd, IDC_FTPARAM_EDIT, _this->m_szFTParamW, 256);
				WideCharToMultiByte(CP_ACP, 0, _this->m_szFTParamW, -1, _this->m_szFTParam, 256, NULL, NULL);
				EndDialog(hwnd, TRUE);
				return TRUE;
			}
		case IDCANCEL:
			_this->m_szFTParamW[0] = L'\0';
			strcpy_s(_this->m_szFTParam, "");
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
// Params acquisition Dialog Box
// 
int FileTransfer::DoFTConfirmDialog(LPSTR szTitle, LPSTR szComment)
{
	extern HINSTANCE m_hInstResDLL;
	strcpy_s(m_szFTConfirmTitle, szTitle);
	strcpy_s(m_szFTConfirmComment, szComment);
	return DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_FTCONFIRM_DLG), hWnd, (DLGPROC) FTConfirmDlgProc, (LONG_PTR) this);
}


BOOL CALLBACK FileTransfer::FTConfirmDlgProc(  HWND hwnd,  UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    FileTransfer *_this = helper::SafeGetWindowUserData<FileTransfer>(hwnd);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            helper::SafeSetWindowUserData(hwnd, lParam);
			_this = (FileTransfer *) lParam;
			//CentreWindow(hwnd);

			// Set Title
			SetWindowText(hwnd, _this->m_szFTConfirmTitle);
			// Set Comment
			SetDlgItemText(hwnd, IDC_FTPCONFIRMCOMMENT, _this->m_szFTConfirmComment);

			// Todo: Init buttons labels with corrsponding culture (En, Fr, De...)
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_YES_B:
				_this->m_nConfirmAnswer = CONFIRM_YES;
				EndDialog(hwnd, TRUE);
				return TRUE;

		case IDC_YESALL_B:
				_this->m_nConfirmAnswer = CONFIRM_YESALL;
				EndDialog(hwnd, TRUE);
				return TRUE;

		case IDC_NO_B:
				_this->m_nConfirmAnswer = CONFIRM_NO;
				EndDialog(hwnd, TRUE);
				return TRUE;

		case IDC_NOALL_B:
				_this->m_nConfirmAnswer = CONFIRM_NOALL;
				EndDialog(hwnd, TRUE);
				return TRUE;

		case IDOK:
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
//
//
void FileTransfer::DisableButtons(HWND hWnd, bool X)
{
	ShowWindow(GetDlgItem(hWnd, IDC_UPLOAD_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_DOWNLOAD_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_DELETE_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_NEWFOLDER_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_RENAME_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDCANCEL), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDCANCEL2), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_HIDE_B), SW_SHOW);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_FILELIST), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_ROOTB), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_UPB), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_FILELIST), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_ROOTB), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_UPB), FALSE);

	// Disable Close Window in in title bar
	if (X == true) {
		HMENU hMenu = GetSystemMenu(hWnd, 0);
		int nCount = GetMenuItemCount(hMenu);
		EnableMenuItem(hMenu, nCount - 1, MF_DISABLED | MF_GRAYED | MF_BYPOSITION);
		EnableMenuItem(hMenu, nCount - 2, MF_DISABLED | MF_GRAYED | MF_BYPOSITION);
	}
	DrawMenuBar(hWnd);

}
void FileTransfer::CheckButtonState(HWND hWnd)
{
//    bool bEnable = m_fFocusLocal && || 
    bool bEnable;
    bool localDirSet, remoteDirSet;

    localDirSet = GetWindowTextLength(GetDlgItem(hWnd, IDC_CURR_LOCAL)) > 0;
    remoteDirSet = GetWindowTextLength(GetDlgItem(hWnd, IDC_CURR_REMOTE)) > 0;
    bEnable = m_fFocusLocal ?localDirSet : remoteDirSet;

	EnableWindow(GetDlgItem(hWnd, IDC_UPLOAD_B), localDirSet && remoteDirSet);
	EnableWindow(GetDlgItem(hWnd, IDC_DOWNLOAD_B), localDirSet && remoteDirSet);
	EnableWindow(GetDlgItem(hWnd, IDC_DELETE_B), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_NEWFOLDER_B), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_RENAME_B), bEnable);
//	EnableWindow(GetDlgItem(hWnd, IDCANCEL), bEnable);
//	EnableWindow(GetDlgItem(hWnd, IDC_HIDE_B), bEnable);

}

//
//
//
void FileTransfer::EnableButtons(HWND hWnd)
{
	ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_ABORT_B2), SW_HIDE);
	ShowWindow(GetDlgItem(hWnd, IDC_UPLOAD_B), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_DOWNLOAD_B), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDCANCEL), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDCANCEL2), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_DELETE_B), SW_SHOW);
	ShowWindow(GetDlgItem(hWnd, IDC_NEWFOLDER_B), SW_SHOW);
	if (!UsingOldProtocol())
		ShowWindow(GetDlgItem(hWnd, IDC_RENAME_B), SW_SHOW);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_FILELIST), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_DRIVECB), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_ROOTB), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_LOCAL_UPB), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_FILELIST), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_DRIVECB), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_ROOTB), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_REMOTE_UPB), TRUE);
	// Disable Close Window in in title bar
	HMENU hMenu = GetSystemMenu(hWnd, 0);
	int nCount = GetMenuItemCount(hMenu);
	EnableMenuItem(hMenu, nCount-1, MF_ENABLED | MF_BYPOSITION);
	EnableMenuItem(hMenu, nCount-2, MF_ENABLED | MF_BYPOSITION);
	DrawMenuBar(hWnd);

}



void FileTransfer::InitListViewImagesList(HWND hListView)
{
    HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_MASK, 2, 2); 

    // Load icons from main executable, not language DLL
    extern HINSTANCE hInstance;
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIR)); 
    ImageList_AddIcon(hImageList, hIcon);  
    DestroyIcon(hIcon); 

    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FILE)); 
    ImageList_AddIcon(hImageList, hIcon); 
    DestroyIcon(hIcon); 

    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRIVE)); 
    ImageList_AddIcon(hImageList, hIcon); 
    DestroyIcon(hIcon); 

    ListView_SetImageList(hListView, hImageList, LVSIL_SMALL); 
}


// sf@2007 - Column sorting comparizon function
// Modif initiated by TAW. Thanks !
int CALLBACK FileTransfer::ListViewLocalCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int iResult=0;
	bool bSortDirection = true;
	bool bRemoteList = false;
	TCHAR szBuf1[255], szBuf2[255];

	if (lParamSort < 0)
	{
		bSortDirection = false;
		lParamSort *= -1;
	}
	if (lParamSort >=10)
	{
		lParamSort -=10;
		bRemoteList = true;
	}
	lParamSort -= 1;
	
	HWND hWndList = GetDlgItem(hFTWnd, bRemoteList ? IDC_REMOTE_FILELIST : IDC_LOCAL_FILELIST);

	ListView_GetItemText(hWndList, lParam1, lParamSort, szBuf1, sizeof(szBuf1));
    ListView_GetItemText(hWndList, lParam2, lParamSort, szBuf2, sizeof(szBuf2));

	switch(lParamSort)
	{
	case 0: //Sort by name
		iResult = lstrcmpi(szBuf1, szBuf2);
		break;
	case 1: //Sort by size
		{
		iResult = lstrcmpi(szBuf1, szBuf2);
		__int64 s1 = GetFileSizeFromString(szBuf1);
		__int64 s2 = GetFileSizeFromString(szBuf2);
		iResult = ((s1 >= s2) ? 1 : -1);
		}
		break;
	case 2: //Sort by date
		{
		FILETIME ft1 = GetFileTimeFromString(szBuf1);
		FILETIME ft2 = GetFileTimeFromString(szBuf2);
		iResult = CompareFileTime(&ft1, &ft2);
		}
		break;
		}

	if (bSortDirection == false)
		iResult *= -1;

	return(iResult);
}


__int64 FileTransfer::GetFileSizeFromString(char* szSize)
{
	__int64 Size = 0;
	unsigned long m, r;
	char ts0 = szSize[strlen(szSize)-1];
	if (ts0 == 'r') return -1;
	char ts1 = szSize[strlen(szSize)-2];
	if (ts1 != 'e')
		sscanf_s(szSize, "%d.%02d", &m, &r);
	else
		sscanf_s(szSize, "%d", &m);

	switch (ts1)
	{
		case 'G':
			Size = (__int64)(m*(__int64)(1024*1024*1024)) + (__int64)(r*(__int64)(1024*1024*1024/100));
			break;
		case 'M':
			Size = (__int64)(m*(__int64)(1024*1024)) + (__int64)(r*(__int64)(1024*1024/100));
			break;
		case 'K':
			Size = (__int64)(m*(__int64)(1024)) + (__int64)(r*1024/100);
			break;
		case 'e':
			Size = (__int64)m;
			break;
		default:
			Size = 0;
			break;
	}
	return Size;
}


FILETIME FileTransfer::GetFileTimeFromString(char* szFileSystemTime)
{
	SYSTEMTIME FileSystemTime;
	FILETIME LocalFileTime;
	int m,d,y,h,mn;

	if (strlen(szFileSystemTime) == 0)
	{
		m = d = y = h = mn = 0;
	}
	else
	{
		sscanf_s(szFileSystemTime,"%2d/%2d/%4d %2d:%2d",
				&m,
				&d,
				&y,
				&h,
				&mn
				);
	}
	FileSystemTime.wMonth = (WORD)m;
	FileSystemTime.wDay = (WORD)d;
	FileSystemTime.wYear = (WORD)y;
	FileSystemTime.wHour = (WORD)h;
	FileSystemTime.wMinute = (WORD)mn;
	FileSystemTime.wSecond = 0;
	FileSystemTime.wMilliseconds = 0;
	FileSystemTime.wDayOfWeek = 0;
	SystemTimeToFileTime(&FileSystemTime, &LocalFileTime);
	return LocalFileTime;
}
bool FileTransfer::FileOrFolderExists(HWND fileListWnd, std::string fileOrFolder)
{
	LVFINDINFO Info;
	Info.flags = LVFI_STRING;
	Info.psz = (LPSTR)fileOrFolder.c_str();
	int nTheIndex = ListView_FindItem(fileListWnd, -1, &Info);
	return nTheIndex > -1;
}
