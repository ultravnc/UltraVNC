// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// Clipboard.h

// adzm - July 2010
//
// Common classes for dealing with the clipboard, including serializing and deserializing compressed data, hashing and comparing, etc.
// Used by server and viewer.

#pragma once

#define VC_EXTRALEAN
//#include <stdhdrs.h>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <rdr/MemOutStream.h>
#include "rfb.h"

struct ExtendedClipboardDataMessage {
	ExtendedClipboardDataMessage();
	~ExtendedClipboardDataMessage();

	void Reset();
	void AddFlag(CARD32 flag);
	bool HasFlag(CARD32 flag);

	int GetMessageLength();	// does not include rfbExtendedClipboardData
	int GetDataLength();	// does include rfbExtendedClipboardData
	const BYTE* GetData();

	BYTE* GetBuffer();		// writable buffer
	int GetBufferLength();	// does include rfbExtendedClipboardData

	const BYTE* GetCurrentPos();

	rfbExtendedClipboardData* m_pExtendedData;

	void AppendInt(CARD32 val); // swaps if LE
	void AppendBytes(BYTE* pData, int length);
	void Advance(int len);
	CARD32 ReadInt();

	void EnsureBufferLength(int len, bool bGrowBeyond = true);

	int CountFormats();
	CARD32 GetFlags();

protected:

	int m_nInternalLength;
	BYTE* m_pCurrentPos;
	BYTE* m_pData;
};

struct ClipboardSettings {
	ClipboardSettings(CARD32 caps);

	static CARD32 defaultCaps;
	static CARD32 defaultViewerCaps;
	static CARD32 defaultServerCaps;

	static const UINT formatDIB;
	static const UINT formatHTML;
	static const UINT formatRTF;
	static const UINT formatUnicodeText;

	static const int defaultLimitText;
	static const int defaultLimitRTF;
	static const int defaultLimitHTML;
	static const int defaultLimitDIB;

	static const int defaultLimit;

	///////

	bool m_bSupportsEx;

	int m_nLimitText;
	int m_nLimitRTF;
	int m_nLimitHTML;
	int m_nLimitDIB;

	int m_nRequestedLimitText;
	int m_nRequestedLimitRTF;
	int m_nRequestedLimitHTML;
	int m_nRequestedLimitDIB;

	CARD32 m_myCaps;

	CARD32 m_remoteCaps; // messages and formats that will be handled by the remote application

	void PrepareCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage);

	void HandleCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage, bool bSetLimits);
};

struct ClipboardHolder {
	ClipboardHolder(HWND hwndOwner);

	~ClipboardHolder();

	bool m_bIsOpen;
};

// Structure to hold information about a single file in clipboard
struct ClipboardFileInfo {
	std::wstring fileName;		// Full path on local system
	std::wstring relativeName;	// Relative path for transfer
	DWORD fileSizeLow;
	DWORD fileSizeHigh;
	DWORD fileAttributes;
	FILETIME lastWriteTime;
};

struct ClipboardData {
	ClipboardData();

	~ClipboardData();

	DWORD m_crc;

	int m_lengthText;
	int m_lengthRTF;
	int m_lengthHTML;
	int m_lengthDIB;

	BYTE* m_pDataText;
	BYTE* m_pDataRTF;
	BYTE* m_pDataHTML;
	BYTE* m_pDataDIB;

	// Clipboard file transfer support
	bool m_bHasFiles;
	std::vector<ClipboardFileInfo> m_files;

	void FreeData();

	bool Load(HWND hwndOwner); // will return false on failure
	bool LoadFiles(HWND hwndOwner); // Load CF_HDROP file list

	bool Restore(HWND hwndOwner, ExtendedClipboardDataMessage& extendedClipboardDataMessage);
};

struct Clipboard {
	Clipboard(CARD32 caps);

	bool UpdateClipTextEx(ClipboardData& clipboardData, CARD32 overrideFlags = 0); // returns true if something changed

	ClipboardSettings settings;
	DWORD m_crc;
	std::string m_strLastCutText; // for non-extended clipboards

	bool m_bNeedToProvide;
	bool m_bNeedToNotify;
	bool m_bNeedToNotifyFiles;  // Files available for delayed rendering

	CARD32 m_notifiedRemoteFormats;

	// Clipboard file transfer - store files for delayed rendering
	std::vector<ClipboardFileInfo> m_pendingFiles;
	bool m_bFilesAvailable;

	ExtendedClipboardDataMessage extendedClipboardDataMessage;
	ExtendedClipboardDataMessage extendedClipboardDataNotifyMessage;
	ExtendedClipboardDataMessage extendedClipboardFileListMessage;
};

