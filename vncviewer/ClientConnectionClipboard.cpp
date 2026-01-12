// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


#include "stdhdrs.h"
#include <shellapi.h>
#include <shlobj.h>
#include "vncviewer.h"
#include "ClientConnection.h"
#include "Exception.h"
extern char sz_C1[64];
extern char sz_C2[64];
extern char sz_C3[64];

// This file contains the code for getting text from, and putting text into
// the Windows clipboard.

//
// ProcessClipboardChange
// Called by ClientConnection::WndProc.
// We've been informed that the local clipboard has been updated.
// If it's text we want to send it to the server.
//

void ClientConnection::ProcessLocalClipboardChange()
{
	vnclog.Print(2, _T("Clipboard changed\n"));
	
	HWND hOwner = GetClipboardOwner();

	//adzm 2010-05-11 - Ignore clipboard while initializing (copying a password, for example, will end up sending a packet and causing a failure)
	if (!m_running)
	{
		vnclog.Print(2, _T("Ignore Clipboard while initializing!\n"));
		//m_initialClipboardSeen = true;
	}
	else if (m_settingClipboardViewer)
	{
		vnclog.Print(2, _T("Ignore Clipboard while setting viewer!\n"));
		//m_initialClipboardSeen = true;
	}
	else if (m_pFileTransfer && (m_pFileTransfer->m_fFileTransferRunning ||m_pFileTransfer->m_fFileUploadRunning || m_pFileTransfer->m_fFileDownloadRunning))
	{
		vnclog.Print(2, _T("Ignore Clipboard while File Transfer is buzy!\n"));
		//m_initialClipboardSeen = true;
	}
	else if (hOwner == m_hwndcn) {
		vnclog.Print(2, _T("We changed it - ignore!\n"));
	/*} else if (!m_initialClipboardSeen) {
		vnclog.Print(2, _T("Don't send initial clipboard!\n"));
		m_initialClipboardSeen = true;*/
	} else if (!m_opts->m_DisableClipboard && !m_opts->m_ViewOnly) {
		UpdateRemoteClipboard();
	}
	// Pass the message to the next window in clipboard viewer chain
	if (m_hwndNextViewer != NULL && m_hwndNextViewer != (HWND)INVALID_HANDLE_VALUE) {
		vnclog.Print(6, _T("Passing WM_DRAWCLIPBOARD to 0x%08x\n"), m_hwndNextViewer);
		// use SendNotifyMessage instead of SendMessage so misbehaving or hung applications
		// (like ourself before this) won't cause our thread to hang.
		::SendNotifyMessage(m_hwndNextViewer, WM_DRAWCLIPBOARD , 0,0); 
	} else {
		vnclog.Print(6, _T("No next window in chain; WM_DRAWCLIPBOARD will not be passed\n"), m_hwndNextViewer);
	}
}


// adzm - 2010-07 - Extended clipboard
void ClientConnection::UpdateRemoteClipboard(CARD32 overrideFlags)
{			
	// The clipboard should not be modified by more than one thread at once
	omni_mutex_lock l(m_clipMutex);
	
	if (m_clipboard.settings.m_bSupportsEx) {
		ClipboardData newClipboard;

		// Check for files first (for delayed rendering)
		bool hasFiles = newClipboard.LoadFiles(NULL);
		if (hasFiles && (m_clipboard.settings.m_remoteCaps & clipFiles)) {
			// Store files for later transfer and send notification
			m_clipboard.m_pendingFiles = newClipboard.m_files;
			m_clipboard.m_bFilesAvailable = true;
			m_clipboard.m_bNeedToNotifyFiles = true;
			vnclog.Print(6, _T("Clipboard has %d files available for transfer\n"), (int)newClipboard.m_files.size());
		} else {
			m_clipboard.m_bFilesAvailable = false;
			m_clipboard.m_bNeedToNotifyFiles = false;
		}

		if (newClipboard.Load(NULL)) {
			if (newClipboard.m_crc == m_clipboard.m_crc && overrideFlags == 0 && !m_clipboard.m_bNeedToNotifyFiles) {
				vnclog.Print(6, _T("Ignoring extended SendClientCutText due to identical data\n"));
				return;
			}

			m_clipboard.UpdateClipTextEx(newClipboard, overrideFlags);
			
			if (m_clipboard.m_bNeedToProvide) {
				m_clipboard.m_bNeedToProvide = false;
				int actualLen = m_clipboard.extendedClipboardDataMessage.GetDataLength();

				rfbClientCutTextMsg message;
				memset(&message, 0, sizeof(rfbClientCutTextMsg));
				message.type = rfbClientCutText;

				message.length = Swap32IfLE(-actualLen);
				
				//adzm 2010-09
				WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
				WriteExact((char*)(m_clipboard.extendedClipboardDataMessage.GetData()), m_clipboard.extendedClipboardDataMessage.GetDataLength());
			
				vnclog.Print(6, _T("Sent extended clipboard\n"));
			}

			m_clipboard.extendedClipboardDataMessage.Reset();

			if (m_clipboard.m_bNeedToNotify) {
				m_clipboard.m_bNeedToNotify = false;
				if (m_clipboard.settings.m_bSupportsEx) {

					int actualLen =m_clipboard.extendedClipboardDataNotifyMessage.GetDataLength();

					rfbClientCutTextMsg message;
					memset(&message, 0, sizeof(rfbClientCutTextMsg));
					message.type = rfbClientCutText;

					message.length = Swap32IfLE(-actualLen);

					WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
					WriteExact((char*)(m_clipboard.extendedClipboardDataNotifyMessage.GetData()), m_clipboard.extendedClipboardDataNotifyMessage.GetDataLength());

				}
				m_clipboard.extendedClipboardDataNotifyMessage.Reset();
			}

		} else {
			vnclog.Print(6, _T("Failed to load local clipboard!\n"));
		}

		// Send file notification if files are available (RDP-style delayed rendering)
		if (m_clipboard.m_bNeedToNotifyFiles && m_clipboard.m_bFilesAvailable) {
			m_clipboard.m_bNeedToNotifyFiles = false;
			SendClipboardFilesNotification();
		}
	} else {		
		vnclog.Print(6, _T("Checking clipboard...\n"));
		ClipboardHolder holder(m_hwndcn);
		if (holder.m_bIsOpen) {
			vnclog.Print(6, _T("Opened...\n"));
			HGLOBAL hglb = GetClipboardData(CF_TEXT); 
			if (hglb == NULL) {				
				vnclog.Print(6, _T("No CF_TEXT!\n"));
			} else {
				vnclog.Print(6, _T("Got CF_TEXT!\n"));
				LPSTR lpstr = (LPSTR) GlobalLock(hglb);  
				
				char *contents = new char[strlen(lpstr) + 1];
				char *unixcontents = new char[strlen(lpstr) + 1];
				strcpy_s(contents, strlen(lpstr) + 1, lpstr);
				GlobalUnlock(hglb);  		
				
				// Translate to Unix-format lines before sending
				int j = 0;
				for (int i = 0; contents[i] != '\0'; i++) {
					if (contents[i] != '\x0d') {
						unixcontents[j++] = contents[i];
					}
				}
				unixcontents[j] = '\0';
				try {
					SendClientCutText(unixcontents, strlen(unixcontents));
				} catch (WarningException &e) {
					vnclog.Print(0, _T("Exception while sending clipboard text : %s\n"), e.m_info);
					DestroyWindow(m_hwndcn);
				}
				delete [] contents; 
				delete [] unixcontents;
			}
		} 
	}
}

// adzm - 2010-07 - Extended clipboard
void ClientConnection::UpdateRemoteClipboardCaps(bool bSavePreferences)
{
	omni_mutex_lock l(m_clipMutex);
	if (!m_clipboard.settings.m_bSupportsEx) return;

	ExtendedClipboardDataMessage extendedClipboardDataMessage;
	
	if (m_opts->m_DisableClipboard || m_opts->m_ViewOnly) {
		// messages and formats that we can handle
		extendedClipboardDataMessage.m_pExtendedData->flags = Swap32IfLE(clipCaps | clipText | clipRTF | clipHTML | clipDIB);

		// now include our limits in order of enum value
		extendedClipboardDataMessage.AppendInt(0);
		extendedClipboardDataMessage.AppendInt(0);
		extendedClipboardDataMessage.AppendInt(0);
		extendedClipboardDataMessage.AppendInt(0);
	} else {
		if (bSavePreferences) {
			SaveClipboardPreferences();
		}
		m_clipboard.settings.PrepareCapsPacket(extendedClipboardDataMessage);
	}

	int actualLen = extendedClipboardDataMessage.GetDataLength();

	rfbClientCutTextMsg message;
	memset(&message, 0, sizeof(rfbClientCutTextMsg));
	message.type = rfbClientCutText;

	message.length = Swap32IfLE(-actualLen);
	
	//adzm 2010-09
	WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact((char*)(extendedClipboardDataMessage.GetData()), extendedClipboardDataMessage.GetDataLength());
}

void ClientConnection::RequestRemoteClipboard()
{
	if (!m_clipboard.settings.m_bSupportsEx) return;

	ExtendedClipboardDataMessage extendedClipboardDataMessage;

	int actualLen = extendedClipboardDataMessage.GetDataLength();
	extendedClipboardDataMessage.AddFlag(clipRequest | clipText | clipRTF | clipHTML | clipDIB);

	rfbClientCutTextMsg message;
	memset(&message, 0, sizeof(rfbClientCutTextMsg));
	message.type = rfbClientCutText;

	message.length = Swap32IfLE(-actualLen);
	
	//adzm 2010-09
	WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact((char*)(extendedClipboardDataMessage.GetData()), extendedClipboardDataMessage.GetDataLength());
}

// We've read some text from the remote server, and
// we need to copy it into the local clipboard.
// Called by ClientConnection::ReadServerCutText()
// adzm - 2010-07 - Extended clipboard
void ClientConnection::UpdateLocalClipboard(char *buf, int len)
{	
	if (m_opts->m_DisableClipboard || m_opts->m_ViewOnly)
		return;

	// Copy to wincontents replacing LF with CR-LF
	char *wincontents = new char[len * 2 + 1];

	int j = 0;;
	for (int i = 0; buf[i] != 0; i++, j++) {
		if (buf[i] == '\x0a') {
			wincontents[j++] = '\x0d';
			len++;
		}
		wincontents[j] = buf[i];
	}
	wincontents[j] = '\0';

    // The clipboard should not be modified by more than one thread at once
    {
        omni_mutex_lock l(m_clipMutex);
		ClipboardHolder holder(m_hwndcn);

		if (!holder.m_bIsOpen) {
			vnclog.Print(2, "UpdateLocalClipboard: Failed to open clipboard! Last error 0x%08x", GetLastError());
			delete [] wincontents;
			return;
        }
        if (! ::EmptyClipboard()) {
			vnclog.Print(2, "UpdateLocalClipboard: Failed to empty clipboard! Last error 0x%08x", GetLastError());
			delete [] wincontents;
			return;
        }
			
		int finalLen = strlen(wincontents) + 1;

        // Allocate a global memory object for the text. 
        HGLOBAL hglbCopy = GlobalAlloc(GMEM_DDESHARE, finalLen); // in bytes
        if (hglbCopy != NULL) { 
	        // Lock the handle and copy the text to the buffer.  
	        LPTSTR lptstrCopy = (LPTSTR) GlobalLock(hglbCopy); 
			memcpy(lptstrCopy, wincontents, finalLen); // in bytes
	        lptstrCopy[finalLen - 1] = 0;    // null character 
	        GlobalUnlock(hglbCopy);          // Place the handle on the clipboard.  
			
			m_clipboard.m_strLastCutText = wincontents;

	        SetClipboardData(CF_TEXT, hglbCopy); 
        }
		
        delete [] wincontents;
    }
}

void ClientConnection::SaveClipboardPreferences()
{
	omni_mutex_lock l(m_clipMutex);

	{
		DWORD dwClipboardPrefs = 0;
		if (m_clipboard.settings.m_nLimitText > 0) {
			dwClipboardPrefs |= clipText;
		}
		if (m_clipboard.settings.m_nLimitRTF > 0) {
			dwClipboardPrefs |= clipRTF;
		}
		if (m_clipboard.settings.m_nLimitHTML > 0) {
			dwClipboardPrefs |= clipHTML;
		}
		//ofnInit();
		vnclog.Print(1, "Saving to %s\n", m_opts->getDefaultOptionsFileName());
		char buf[32];
		sprintf_s(buf, "%d", dwClipboardPrefs);
		WritePrivateProfileString("connection", "ClipboardPrefs", buf, m_opts->getDefaultOptionsFileName());
	}
}

bool ClientConnection::LoadClipboardPreferences()
{
	omni_mutex_lock l(m_clipMutex);
	DWORD dwClipboardPrefs = 0;
//	ofnInit();
	vnclog.Print(1, "Saving to %s\n", m_opts->getDefaultOptionsFileName());
	dwClipboardPrefs = clipText | clipRTF | clipHTML;
	dwClipboardPrefs = GetPrivateProfileInt("connection", "ClipboardPrefs", dwClipboardPrefs, m_opts->getDefaultOptionsFileName());
	dwClipboardPrefs |= clipText;
	if (!(dwClipboardPrefs & clipRTF)) {
		m_clipboard.settings.m_nLimitRTF = 0;
	}
	if (!(dwClipboardPrefs & clipHTML)) {
		m_clipboard.settings.m_nLimitHTML = 0;
	}
	return true;
}

// Clipboard file transfer - RDP-style delayed rendering
// Send notification that files are available in clipboard
void ClientConnection::SendClipboardFilesNotification()
{
	if (!m_clipboard.settings.m_bSupportsEx) return;
	if (!(m_clipboard.settings.m_remoteCaps & clipFiles)) return;
	if (m_clipboard.m_pendingFiles.empty()) return;

	vnclog.Print(6, _T("Sending clipboard files notification (%d files)\n"), (int)m_clipboard.m_pendingFiles.size());

	ExtendedClipboardDataMessage notifyMessage;
	notifyMessage.AddFlag(clipNotify | clipFiles);

	int actualLen = notifyMessage.GetDataLength();

	rfbClientCutTextMsg message;
	memset(&message, 0, sizeof(rfbClientCutTextMsg));
	message.type = rfbClientCutText;
	message.length = Swap32IfLE(-actualLen);

	WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact((char*)(notifyMessage.GetData()), notifyMessage.GetDataLength());
}

// Send the file list metadata when requested
void ClientConnection::SendClipboardFileList()
{
	if (!m_clipboard.settings.m_bSupportsEx) return;
	if (m_clipboard.m_pendingFiles.empty()) return;

	vnclog.Print(6, _T("Sending clipboard file list (%d files)\n"), (int)m_clipboard.m_pendingFiles.size());

	ExtendedClipboardDataMessage fileListMessage;
	fileListMessage.AddFlag(clipProvide | clipFiles);

	// Add sub-action: file list
	fileListMessage.AppendInt(clipFileList);

	// Add file count
	fileListMessage.AppendInt((CARD32)m_clipboard.m_pendingFiles.size());

	// Add each file entry
	for (size_t i = 0; i < m_clipboard.m_pendingFiles.size(); i++) {
		const ClipboardFileInfo& fileInfo = m_clipboard.m_pendingFiles[i];

		// Convert relative name to UTF-8
		int utf8Len = WideCharToMultiByte(CP_UTF8, 0, fileInfo.relativeName.c_str(), -1, NULL, 0, NULL, NULL);
		std::string utf8Name(utf8Len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, fileInfo.relativeName.c_str(), -1, &utf8Name[0], utf8Len, NULL, NULL);
		utf8Name.resize(utf8Len - 1); // Remove null terminator

		// File index
		fileListMessage.AppendInt((CARD32)i);
		// File size (low, high)
		fileListMessage.AppendInt(fileInfo.fileSizeLow);
		fileListMessage.AppendInt(fileInfo.fileSizeHigh);
		// File attributes
		fileListMessage.AppendInt(fileInfo.fileAttributes);
		// Last write time (low, high)
		fileListMessage.AppendInt(fileInfo.lastWriteTime.dwLowDateTime);
		fileListMessage.AppendInt(fileInfo.lastWriteTime.dwHighDateTime);
		// File name length and name
		fileListMessage.AppendInt((CARD32)utf8Name.length());
		fileListMessage.AppendBytes((BYTE*)utf8Name.c_str(), (int)utf8Name.length());
	}

	int actualLen = fileListMessage.GetDataLength();

	rfbClientCutTextMsg message;
	memset(&message, 0, sizeof(rfbClientCutTextMsg));
	message.type = rfbClientCutText;
	message.length = Swap32IfLE(-actualLen);

	WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact((char*)(fileListMessage.GetData()), fileListMessage.GetDataLength());
}

// Send file contents when requested
void ClientConnection::SendClipboardFileContents(CARD32 fileIndex, CARD64 offset, CARD32 length)
{
	if (!m_clipboard.settings.m_bSupportsEx) return;
	if (fileIndex >= m_clipboard.m_pendingFiles.size()) {
		vnclog.Print(0, _T("Invalid file index %d requested\n"), fileIndex);
		return;
	}

	const ClipboardFileInfo& fileInfo = m_clipboard.m_pendingFiles[fileIndex];
	
	vnclog.Print(6, _T("Sending file contents: index=%d, offset=%lld, length=%d\n"), fileIndex, offset, length);

	// Open the file
	HANDLE hFile = CreateFileW(fileInfo.fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		vnclog.Print(0, _T("Failed to open file for clipboard transfer\n"));
		return;
	}

	// Seek to offset
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = offset;
	if (!SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN)) {
		CloseHandle(hFile);
		vnclog.Print(0, _T("Failed to seek in file for clipboard transfer\n"));
		return;
	}

	// Read the data
	std::vector<BYTE> fileData(length);
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, fileData.data(), length, &bytesRead, NULL)) {
		CloseHandle(hFile);
		vnclog.Print(0, _T("Failed to read file for clipboard transfer\n"));
		return;
	}
	CloseHandle(hFile);

	// Build response message
	ExtendedClipboardDataMessage contentsMessage;
	contentsMessage.AddFlag(clipProvide | clipFiles);

	// Add sub-action: file contents
	contentsMessage.AppendInt(clipFileContents);

	// Add file contents response header
	contentsMessage.AppendInt(fileIndex);
	contentsMessage.AppendInt((CARD32)(offset & 0xFFFFFFFF));
	contentsMessage.AppendInt((CARD32)(offset >> 32));
	contentsMessage.AppendInt(bytesRead);

	// Add file data
	contentsMessage.AppendBytes(fileData.data(), bytesRead);

	int actualLen = contentsMessage.GetDataLength();

	rfbClientCutTextMsg message;
	memset(&message, 0, sizeof(rfbClientCutTextMsg));
	message.type = rfbClientCutText;
	message.length = Swap32IfLE(-actualLen);

	WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact((char*)(contentsMessage.GetData()), contentsMessage.GetDataLength());
}

// Handle incoming file request from server
void ClientConnection::HandleClipboardFileRequest(ExtendedClipboardDataMessage& extendedDataMessage)
{
	// Read the sub-action
	CARD32 subAction = extendedDataMessage.ReadInt();

	if (subAction == clipFileList) {
		// Server is requesting the file list
		SendClipboardFileList();
	}
	else if (subAction == clipFileContents) {
		// Server is requesting file contents
		CARD32 fileIndex = extendedDataMessage.ReadInt();
		CARD32 offsetLow = extendedDataMessage.ReadInt();
		CARD32 offsetHigh = extendedDataMessage.ReadInt();
		CARD32 length = extendedDataMessage.ReadInt();

		CARD64 offset = ((CARD64)offsetHigh << 32) | offsetLow;
		SendClipboardFileContents(fileIndex, offset, length);
	}
}

// Set up local clipboard for delayed rendering when server has files
void ClientConnection::SetupLocalClipboardForRemoteFiles()
{
	// Set up delayed rendering on the local clipboard
	if (OpenClipboard(m_hwndcn))
	{
		EmptyClipboard();
		SetClipboardData(CF_HDROP, NULL);  // Delayed rendering
		CloseClipboard();
		vnclog.Print(6, _T("Set up delayed rendering for remote files\n"));
	}
}

// Handle file data received from server
void ClientConnection::HandleClipboardFileData(ExtendedClipboardDataMessage& extendedDataMessage)
{
	CARD32 subAction = extendedDataMessage.ReadInt();

	if (subAction == clipFileList) {
		m_remoteFiles.clear();
		CARD32 fileCount = extendedDataMessage.ReadInt();
		vnclog.Print(6, _T("Received file list with %d files from server\n"), fileCount);

		for (CARD32 i = 0; i < fileCount; i++) {
			ClipboardFileInfo fileInfo;
			CARD32 fileIndex = extendedDataMessage.ReadInt();
			fileInfo.fileSizeLow = extendedDataMessage.ReadInt();
			fileInfo.fileSizeHigh = extendedDataMessage.ReadInt();
			fileInfo.fileAttributes = extendedDataMessage.ReadInt();
			fileInfo.lastWriteTime.dwLowDateTime = extendedDataMessage.ReadInt();
			fileInfo.lastWriteTime.dwHighDateTime = extendedDataMessage.ReadInt();
			CARD32 nameLen = extendedDataMessage.ReadInt();

			std::string utf8Name(nameLen, '\0');
			const BYTE* nameData = extendedDataMessage.GetCurrentPos();
			memcpy(&utf8Name[0], nameData, nameLen);
			extendedDataMessage.Advance(nameLen);

			int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Name.c_str(), -1, NULL, 0);
			fileInfo.relativeName.resize(wideLen);
			MultiByteToWideChar(CP_UTF8, 0, utf8Name.c_str(), -1, &fileInfo.relativeName[0], wideLen);
			fileInfo.relativeName.resize(wideLen - 1);

			m_remoteFiles.push_back(fileInfo);
		}

		if (!m_remoteFiles.empty()) {
			WCHAR tempPath[MAX_PATH];
			GetTempPathW(MAX_PATH, tempPath);
			m_clipboardFileTempDir = tempPath;
			m_clipboardFileTempDir += L"UltraVNC_Clipboard\\";
			CreateDirectoryW(m_clipboardFileTempDir.c_str(), NULL);

			RequestRemoteFileContents(0, 0, 65536);
		}
	}
	else if (subAction == clipFileContents) {
		CARD32 fileIndex = extendedDataMessage.ReadInt();
		CARD32 offsetLow = extendedDataMessage.ReadInt();
		CARD32 offsetHigh = extendedDataMessage.ReadInt();
		CARD32 dataLen = extendedDataMessage.ReadInt();

		if (fileIndex < m_remoteFiles.size()) {
			ClipboardFileInfo& fileInfo = m_remoteFiles[fileIndex];
			std::wstring fullPath = m_clipboardFileTempDir + fileInfo.relativeName;

			size_t lastSlash = fullPath.rfind(L'\\');
			if (lastSlash != std::wstring::npos) {
				CreateDirectoryW(fullPath.substr(0, lastSlash).c_str(), NULL);
			}

			HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_WRITE, 0, NULL,
				(offsetLow == 0 && offsetHigh == 0) ? CREATE_ALWAYS : OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile != INVALID_HANDLE_VALUE) {
				if (offsetLow != 0 || offsetHigh != 0) {
					LARGE_INTEGER liOffset;
					liOffset.LowPart = offsetLow;
					liOffset.HighPart = offsetHigh;
					SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN);
				}

				const BYTE* fileData = extendedDataMessage.GetCurrentPos();
				DWORD bytesWritten;
				WriteFile(hFile, fileData, dataLen, &bytesWritten, NULL);
				CloseHandle(hFile);

				fileInfo.fileName = fullPath;

				CARD64 currentOffset = ((CARD64)offsetHigh << 32) | offsetLow;
				CARD64 fileSize = ((CARD64)fileInfo.fileSizeHigh << 32) | fileInfo.fileSizeLow;
				CARD64 nextOffset = currentOffset + dataLen;

				if (nextOffset < fileSize) {
					RequestRemoteFileContents(fileIndex, nextOffset, 65536);
				}
				else if (fileIndex + 1 < m_remoteFiles.size()) {
					RequestRemoteFileContents(fileIndex + 1, 0, 65536);
				}
				else {
					SetLocalClipboardWithReceivedFiles();
				}
			}
		}
	}
}

// Request file contents from server
void ClientConnection::RequestRemoteFileContents(CARD32 fileIndex, CARD64 offset, CARD32 length)
{
	ExtendedClipboardDataMessage requestMessage;
	requestMessage.AddFlag(clipRequest | clipFiles);
	requestMessage.AppendInt(clipFileContents);
	requestMessage.AppendInt(fileIndex);
	requestMessage.AppendInt((CARD32)(offset & 0xFFFFFFFF));
	requestMessage.AppendInt((CARD32)(offset >> 32));
	requestMessage.AppendInt(length);

	int actualLen = requestMessage.GetDataLength();
	rfbClientCutTextMsg message;
	memset(&message, 0, sizeof(rfbClientCutTextMsg));
	message.type = rfbClientCutText;
	message.length = Swap32IfLE(-actualLen);

	WriteExactQueue((char*)&message, sz_rfbClientCutTextMsg, rfbClientCutText);
	WriteExact((char*)(requestMessage.GetData()), requestMessage.GetDataLength());
}

// Set local clipboard with received files
void ClientConnection::SetLocalClipboardWithReceivedFiles()
{
	if (m_remoteFiles.empty()) return;

	size_t totalSize = sizeof(DROPFILES);
	for (const auto& file : m_remoteFiles) {
		totalSize += (file.fileName.length() + 1) * sizeof(WCHAR);
	}
	totalSize += sizeof(WCHAR);

	HGLOBAL hGlobal = GlobalAlloc(GHND, totalSize);
	if (hGlobal == NULL) return;

	DROPFILES* pDropFiles = (DROPFILES*)GlobalLock(hGlobal);
	if (pDropFiles == NULL) {
		GlobalFree(hGlobal);
		return;
	}

	pDropFiles->pFiles = sizeof(DROPFILES);
	pDropFiles->fWide = TRUE;

	WCHAR* pFileList = (WCHAR*)((BYTE*)pDropFiles + sizeof(DROPFILES));
	for (const auto& file : m_remoteFiles) {
		wcscpy(pFileList, file.fileName.c_str());
		pFileList += file.fileName.length() + 1;
	}
	*pFileList = L'\0';

	GlobalUnlock(hGlobal);

	if (OpenClipboard(m_hwndcn)) {
		EmptyClipboard();
		SetClipboardData(CF_HDROP, hGlobal);
		CloseClipboard();
		vnclog.Print(6, _T("Set local clipboard with %d received files\n"), (int)m_remoteFiles.size());
	}
	else {
		GlobalFree(hGlobal);
	}
}
