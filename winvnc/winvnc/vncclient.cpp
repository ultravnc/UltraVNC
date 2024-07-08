/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2015 D. R. Commander. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// vncClient.cpp

// The per-client object. This object takes care of all per-client stuff,
// such as socket input and buffering of updates.

// vncClient class handles the following functions:
// - Recieves requests from the connected client and
//   handles them
// - Handles incoming updates properly, using a vncBuffer
//   object to keep track of screen changes
// It uses a vncBuffer and is passed the vncDesktop and
// vncServer to communicate with.

// Includes
#include "stdhdrs.h"
#include <omnithread.h>
#include <string>
#include <sstream>
#include "resource.h"

// Custom
#include "vncserver.h"
#include "vncclient.h"
#include "vsocket.h"
#include "vncdesktop.h"
#include "rfbRegion.h"
#include "vncbuffer.h"
#include "vncpasswd.h"
#include "vncacceptdialog.h"
#include "vnckeymap.h"
#include "vncmenu.h"

#include "rfb/dh.h"
#include "vncauth.h"

#ifdef _VCPKG
#include <zlib.h>
#include <zstd.h>
#else
#include "../zlib/zlib.h"
#include "../zstd/lib/zstd.h"
#endif

#include "mmsystem.h" // sf@2002
#include "sys/types.h"
#include "sys/stat.h"

#include <string>
#include <iterator>
#include <shlobj.h>
#include "vncOSVersion.h"
#include "common/win32_helpers.h"
#include "uvncUiAccess.h"
#include "VirtualDisplay.h"
#include<map>
#include "SettingsManager.h"
#include "credentials.h"
using namespace std;

#pragma comment(lib, "mpr.lib") //for getting full mapped drive

#define DWEXTRA_VNC_REMOTE  0x564e4300

bool isDirectoryTransfer(const char* szFileName);

int getinfo(char mytext[1024]);
int calc_updates = 0;
int old_calc_updates = 0;
extern bool PreConnect;
int PreConnectID = 0;

// take a full path & file name, split it, prepend prefix to filename, then merge it back
static std::string make_temp_filename(const char* szFullPath)
{
	// don't add prefix for directory transfers.
	if (isDirectoryTransfer(szFullPath))
		return szFullPath;

	std::string tmpName(szFullPath);
	std::string::size_type pos = tmpName.rfind('\\');
	if (pos != std::string::npos)
		tmpName.insert(pos + 1, rfbPartialFilePrefix);

	return tmpName;
}

std::string get_real_filename(std::string name)
{
	std::string::size_type pos;

	pos = name.find(rfbPartialFilePrefix);
	if (pos != std::string::npos)
		name.erase(pos, sz_rfbPartialFilePrefix);

	return name;
}

// #include "rfb.h"
bool DeleteFileOrDirectory(TCHAR* srcpath)
{
	TCHAR path[MAX_PATH + 1]; // room for extra null; SHFileOperation requires double null terminator
	memset(path, 0, sizeof path);

	_tcsncpy_s(path, srcpath, MAX_PATH);
	path[_tcslen(srcpath) + 1] = 0;

	SHFILEOPSTRUCT op;
	memset(&op, 0, sizeof(SHFILEOPSTRUCT));
	op.wFunc = FO_DELETE;
	op.pFrom = path;
	op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

	int result = SHFileOperation(&op);
	// MSDN says to not look at the error code, just treat 0 as SUCCESS, nonzero is failure.
	// Do not use GetLastError with the return values of this function.

	return result == 0;
}

bool replaceFile(const char* src, const char* dst)
{
	DWORD dwFileAttribs;
	bool status;

	dwFileAttribs = GetFileAttributes(dst);
	// make the file read/write if it's read only.
	if (dwFileAttribs != INVALID_FILE_ATTRIBUTES && dwFileAttribs & FILE_ATTRIBUTE_READONLY)
		SetFileAttributes(dst, dwFileAttribs & ~FILE_ATTRIBUTE_READONLY);

	status = ::MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING) ? true : false;

	// restore orginal file attributes, if we have them. We won't have them if
	// the destination file didn't exist prior to the copy/move.
	if (dwFileAttribs != INVALID_FILE_ATTRIBUTES)
		SetFileAttributes(dst, dwFileAttribs);

	return status;
}
#include "Localization.h" // Act : add localization on messages
typedef BOOL(WINAPI* PGETDISKFREESPACEEX)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

// vncClient update thread class

//	[v1.0.2-jp1 fix]
//	yak!'s File transfer patch
//	Simply forward strchr() and strrchr() to _mbschr() and _mbsrchr() to avoid 0x5c problem, respectively.
//	Probably, it is better to write forward functions internally.
#include <mbstring.h>
#define strchr(a, b) reinterpret_cast<char*>(_mbschr(reinterpret_cast<unsigned char*>(a), b))
#define strrchr(a, b) reinterpret_cast<char*>(_mbsrchr(reinterpret_cast<unsigned char*>(a), b))

// 31 January 2008 jdp
std::string AddDirPrefixAndSuffix(const char* name)
{
	std::string dirname(rfbDirPrefix);
	dirname += name;
	dirname += rfbDirSuffix;

	return dirname;
}

bool isDirectory(const char* name)
{
	struct _stat statbuf;

	_stat(name, &statbuf);
	return (statbuf.st_mode & _S_IFDIR) == _S_IFDIR;
}
bool isDirectoryTransfer(const char* szFileName)
{
	return (strncmp(strrchr(const_cast<char*>(szFileName), '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)) == 0);
}

void GetZippedFolderPathName(const char* szZipFile, char* path)
{
	// input lookes like: "c:\temp\!UVNCDIR-folder.zip"
	// output should look like "c:\temp\folder"

	const char* zip_ext = ".zip";
	std::string folder_path(szZipFile);
	std::string::size_type pos;

	// remove "!UVNCDIR-"
	pos = folder_path.find(rfbZipDirectoryPrefix);
	if (pos != std::string::npos)
		folder_path.replace(pos, strlen(rfbZipDirectoryPrefix), "");

	// remove ".zip"
	pos = folder_path.rfind(zip_ext);
	if (pos != std::string::npos)
		folder_path.replace(pos, strlen(zip_ext), "");

	// remove "[ "
	pos = folder_path.find(rfbDirPrefix);
	if (pos != std::string::npos)
		folder_path.replace(pos, strlen(rfbDirPrefix), "");

	// remove " ]"
	pos = folder_path.rfind(rfbDirSuffix);
	if (pos != std::string::npos)
		folder_path.replace(pos, strlen(rfbDirSuffix), "");

	std::copy(folder_path.begin(), folder_path.end(), path);
	path[folder_path.size()] = 0; // terminate the string
}

void SplitTransferredFileNameAndDate(char* szFileAndDate, char* filetime)
{
	char* p = strrchr(szFileAndDate, ',');
	if (p == NULL)
	{
		if (filetime)
			*filetime = '\0';
	}
	else
	{
		if (filetime)
			strcpy_s(filetime, 324, p + 1);
		*p = '\0';
	}
}

/*
 * File transfer event hooks
 *
 * The following functions are called from various points in the File Transfer
 * process. They are notification hooks; the hook function cannot affect the
 * file operation. One possible use of these hooks is to implement auditing
 * of file operations on the server side.
 *
 * Note that uploads are transfers from the server to the viewer and a
 * download is a transfer from the viewer to the server.
 *
 * The hook will have to use the state stored in the vncClient object
 * to figure out what was done in most cases. For those cases where it
 * is not possible, the hook is passed enough information about the event.
 *
 */

 // called at the successful start of a file upload
void vncClient::FTUploadStartHook()
{
}

// called when an abort is received from the viewer. it is assumed that it was
// due to the user cancelling the transfer
void vncClient::FTUploadCancelledHook()
{
}

// Called when an upload fails for any reason: network failure, out of disk space, insufficient privileges etc.
void vncClient::FTUploadFailureHook()
{
}

// Called after the file is successfully uploaded. At this point the File Transfer is complete.
void vncClient::FTUploadCompleteHook()
{
}

// called at the successful start of a file download
void vncClient::FTDownloadStartHook()
{
}

// called when an abort is received from the viewer. it is assumed that it was
// due to the user cancelling the transfer
void vncClient::FTDownloadCancelledHook()
{
}

// Called when an dowload fails for any reason: network failure, out of disk space, insufficient privileges etc.
void vncClient::FTDownloadFailureHook()
{
}

// Called after the file is successfully downloaded. At this point the File Transfer is complete.
void vncClient::FTDownloadCompleteHook()
{
}

// Called when a new folder is created.
void vncClient::FTNewFolderHook(std::string name)
{
}

// called when a folder is created
void vncClient::FTDeleteHook(std::string name, bool isDir)
{
}

// called when a file or folder is renamed.
void vncClient::FTRenameHook(std::string oldName, std::string newname)
{
}

BOOL
vncClientUpdateThread::Init(vncClient* client)
{
	vnclog.Print(LL_INTINFO, VNCLOG("init update thread\n"));
	m_client = client;
	omni_mutex_lock l(m_client->GetUpdateLock(), 80);
	m_signal = new omni_condition(&m_client->GetUpdateLock());
	m_sync_sig = new omni_condition(&m_client->GetUpdateLock());
	m_active = TRUE;
	m_enable = m_client->m_disable_protocol == 0;
	if (m_signal && m_sync_sig) {
		start_undetached();
		return TRUE;
	}
	return FALSE;
}

vncClientUpdateThread::~vncClientUpdateThread()
{
	if (m_signal) delete m_signal;
	if (m_sync_sig) delete m_sync_sig;
	vnclog.Print(LL_INTINFO, VNCLOG("update thread gone\n"));
	m_client->m_updatethread = NULL;
}

void
vncClientUpdateThread::Trigger()
{
	// ALWAYS lock client UpdateLock before calling this!
	// Only trigger an update if protocol is enabled
	if (m_client->m_disable_protocol == 0) {
		m_signal->signal();
	}
}

void
vncClientUpdateThread::Kill()
{
	vnclog.Print(LL_INTINFO, VNCLOG("kill update thread\n"));

	omni_mutex_lock l(m_client->GetUpdateLock(), 81);
	m_active = FALSE;
	m_signal->signal();
}

void
vncClientUpdateThread::get_time_now(unsigned long* abs_sec, unsigned long* abs_nsec)
{
	static int days_in_preceding_months[12]
		= { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	static int days_in_preceding_months_leap[12]
		= { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

	SYSTEMTIME st;

	GetSystemTime(&st);
	*abs_nsec = st.wMilliseconds * 1000000;

	// this formula should work until 1st March 2100

	DWORD days = ((st.wYear - 1970) * 365 + (st.wYear - 1969) / 4
		+ ((st.wYear % 4)
			? days_in_preceding_months[st.wMonth - 1]
			: days_in_preceding_months_leap[st.wMonth - 1])
		+ st.wDay - 1);

	*abs_sec = st.wSecond + 60 * (st.wMinute + 60 * (st.wHour + 24 * days));
}
void
vncClientUpdateThread::EnableUpdates(BOOL enable)
{
	// ALWAYS call this with the UpdateLock held!
	if (enable) {
		vnclog.Print(LL_INTINFO, VNCLOG("enable update thread\n"));
	}
	else {
		vnclog.Print(LL_INTINFO, VNCLOG("disable update thread\n"));
	}

	m_enable = enable;
	m_signal->signal();
	//unsigned long now_sec, now_nsec;
	//get_time_now(&now_sec, &now_nsec);

	// give bad results with java
	//if (enable)
	if (!m_sync_sig->wait(5000))
		vnclog.Print(LL_INTINFO, VNCLOG("wait timeout\n"));
	/*if  (m_sync_sig->timedwait(now_sec+1,0)==0)
		{
//			m_signal->signal();
			vnclog.Print(LL_INTINFO, VNCLOG("thread timeout\n"));
		} */
	vnclog.Print(LL_INTINFO, VNCLOG("enable/disable synced\n"));
}

extern bool g_DesktopThread_running;

void*
vncClientUpdateThread::run_undetached(void* arg)
{
	rfb::SimpleUpdateTracker update;
	rfb::Region2D clipregion;
	// adzm - 2010-07 - Extended clipboard
	//char *clipboard_text = 0;
	update.enable_copyrect(true);
	BOOL send_palette = FALSE;
	const int UPDATE_INTERVAL = 40;
	first_run = true;

	vnclog.Print(LL_INTINFO, VNCLOG("starting update thread\n"));
	//Make sure we never can get locked by the initail m_initial_update) wait loop
	//After 5 sec cont.
	while (g_DesktopThread_running && m_client->cl_connected)
	{
		{
			m_client->m_incr_rgn.assign_union(clipregion);
			omni_mutex_lock l(m_client->GetUpdateLock(), 82);
			// We block as long as updates are disabled, or the client
			// isn't interested in them, unless this thread is killed.

			if (first_run) {
				while (m_active && (!m_enable || (
					m_client->m_update_tracker.get_changed_region().intersect(m_client->m_incr_rgn).is_empty() &&
					m_client->m_update_tracker.get_copied_region().intersect(m_client->m_incr_rgn).is_empty() &&
					m_client->m_update_tracker.get_cached_region().intersect(m_client->m_incr_rgn).is_empty() &&
					// adzm - 2010-07 - Extended clipboard
					!(m_client->m_clipboard.m_bNeedToProvide || m_client->m_clipboard.m_bNeedToNotify) &&
					!m_client->m_cursor_pos_changed // nyama/marscha - PointerPos
					))) {
					// Issue the synchronisation signal, to tell other threads
					// where we have got to
					m_sync_sig->broadcast();
					// Wait to be kicked into action
					m_signal->wait();
					first_run = false;
				}
			}
			else {
				while (m_active && (!m_enable || (
					m_client->m_update_tracker.get_changed_region().intersect(m_client->m_incr_rgn).is_empty() &&
					m_client->m_update_tracker.get_copied_region().intersect(m_client->m_incr_rgn).is_empty() &&
					m_client->m_update_tracker.get_cached_region().intersect(m_client->m_incr_rgn).is_empty() &&
					!m_client->m_encodemgr.IsCursorUpdatePending() &&
					// adzm - 2010-07 - Extended clipboard
					!(m_client->m_clipboard.m_bNeedToProvide || m_client->m_clipboard.m_bNeedToNotify) &&
					!m_client->m_NewSWUpdateWaiting &&
					!m_client->m_cursor_pos_changed // nyama/marscha - PointerPos
					))) {
					// Issue the synchronisation signal, to tell other threads
					// where we have got to
					m_sync_sig->broadcast();
					do {
						if (!m_client->cl_connected) return 0;
						if (m_signal->wait(UPDATE_INTERVAL * 100) == false) {
							//do forcefull update after 4 seconds
							m_client->TriggerUpdate();
							m_client->TriggerUpdateThread();
						}
						else
							break;
					} while (g_DesktopThread_running);
				}
			}
			// If the thread is being killed then quit
			if (!m_active)
				break;
			clipregion = m_client->m_incr_rgn;
			m_client->m_incr_rgn.clear();

			// sf@2002
			// New scale requested, we do it before sending the next Update
			if (m_client->fNewScale)
			{
				// Send the new framebuffer size to the client
				rfb::Rect ViewerSize = m_client->m_encodemgr.m_buffer->GetViewerSize();

				// Copyright (C) 2001 - Harakan software
				if (m_client->m_fPalmVNCScaling)
				{
					rfb::Rect ScreenSize = m_client->m_encodemgr.m_buffer->GetSize();
					rfbPalmVNCReSizeFrameBufferMsg rsfb = { 0 };

					rsfb.type = rfbPalmVNCReSizeFrameBuffer;
					rsfb.desktop_w = Swap16IfLE(ScreenSize.br.x);
					rsfb.desktop_h = Swap16IfLE(ScreenSize.br.y);
					rsfb.buffer_w = Swap16IfLE(ViewerSize.br.x);
					rsfb.buffer_h = Swap16IfLE(ViewerSize.br.y);
					omni_mutex_lock l(m_client->GetUpdateLock(), 82);
					m_client->m_socket->SendExact((char*)&rsfb,
						sz_rfbPalmVNCReSizeFrameBufferMsg,
						rfbPalmVNCReSizeFrameBuffer);
				}
				else // eSVNC-UltraVNC Scaling
				{
					rfbResizeFrameBufferMsg rsmsg;
					memset(&rsmsg, 0, sizeof(rfbResizeFrameBufferMsg));
					rsmsg.type = rfbResizeFrameBuffer;
					rsmsg.framebufferWidth = Swap16IfLE(ViewerSize.br.x);
					rsmsg.framebufferHeigth = Swap16IfLE(ViewerSize.br.y);
					omni_mutex_lock l(m_client->GetUpdateLock(), 82);
					m_client->m_socket->SendExact((char*)&rsmsg,
						sz_rfbResizeFrameBufferMsg,
						rfbResizeFrameBuffer);
					m_client->m_ScaledScreen = m_client->m_encodemgr.m_buffer->GetViewerSize();
					m_client->m_nScale = m_client->m_encodemgr.m_buffer->GetScale();
				}

				m_client->m_encodemgr.m_buffer->ClearCache();
				m_client->fNewScale = false;
				m_client->m_fPalmVNCScaling = false;

				// return 0;
			}

			// Has the palette changed?
			send_palette = m_client->m_palettechanged;
			m_client->m_palettechanged = FALSE;

			// Get the update details from the update tracker
			m_client->m_update_tracker.flush_update(update, clipregion);

			m_client->m_cursor_update_pending = m_client->m_encodemgr.WasCursorUpdatePending();

			if (!m_client->m_cursor_update_sent && !m_client->m_cursor_update_pending) {
				if (m_client->m_mousemoved) {
					// Re-render its old location
					m_client->m_oldmousepos = m_client->m_oldmousepos.intersect(m_client->m_ScaledScreen); // sf@2002
					if (!m_client->m_oldmousepos.is_empty())
						update.add_changed(m_client->m_oldmousepos);

					// And render its new one
					m_client->m_encodemgr.m_buffer->GetMousePos(m_client->m_oldmousepos);
					m_client->m_oldmousepos = m_client->m_oldmousepos.intersect(m_client->m_ScaledScreen);
					if (!m_client->m_oldmousepos.is_empty())
						update.add_changed(m_client->m_oldmousepos);
					m_client->m_mousemoved = FALSE;
				}
			}

			// SEND THE CLIPBOARD
			// If there is clipboard text to be sent then send it
			// Also allow in loopbackmode
			// Loopback mode with VNC Viewer will cause a loping
			// But ssh is back working
			if (!m_client->m_fFileSessionOpen) {
				bool bShouldFlush = false;
				omni_mutex_lock l(m_client->GetUpdateLock(), 82);
				// adzm - 2010-07 - Extended clipboard
				// send any clipboard data that should be sent automatically
				if (m_client->m_clipboard.m_bNeedToProvide) {
					m_client->m_clipboard.m_bNeedToProvide = false;
					if (m_client->m_clipboard.settings.m_bSupportsEx) {
						int actualLen = m_client->m_clipboard.extendedClipboardDataMessage.GetDataLength();

						rfbServerCutTextMsg message;
						memset(&message, 0, sizeof(rfbServerCutTextMsg));
						message.type = rfbServerCutText;

						message.length = Swap32IfLE(-actualLen);

						bShouldFlush = true;

						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						if (!m_client->SendRFBMsgQueue(rfbServerCutText,
							(BYTE*)&message, sizeof(message))) {
							m_client->m_socket->Close();
						}
						if (!m_client->m_socket->SendExactQueue((char*)(m_client->m_clipboard.extendedClipboardDataMessage.GetData()), m_client->m_clipboard.extendedClipboardDataMessage.GetDataLength()))
							m_client->m_socket->Close();
					}
					else {
						rfbServerCutTextMsg message;
						memset(&message, 0, sizeof(rfbServerCutTextMsg));
						const char* cliptext = m_client->m_clipboard.m_strLastCutText.c_str();
						char* unixtext = new char[m_client->m_clipboard.m_strLastCutText.length() + 1];

						// Replace CR-LF with LF - never send CR-LF on the wire,
						// since Unix won't like it
						int unixpos = 0;
						size_t cliplen = strlen(cliptext);
						for (unsigned int x = 0; x < cliplen; x++) {
							if (cliptext[x] != '\x0d') {
								unixtext[unixpos] = cliptext[x];
								unixpos++;
							}
						}
						unixtext[unixpos] = 0;

						message.length = Swap32IfLE(strlen(unixtext));

						bShouldFlush = true;

						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						if (!m_client->SendRFBMsgQueue(rfbServerCutText,
							(BYTE*)&message, sizeof(message))) {
							m_client->m_socket->Close();
						}
						if (!m_client->m_socket->SendExactQueue(unixtext, (const VCard)strlen(unixtext)))
							m_client->m_socket->Close();
						delete[] unixtext;
					}
					m_client->m_clipboard.extendedClipboardDataMessage.Reset();
				}

				// adzm - 2010-07 - Extended clipboard
				// notify of any other formats
				if (m_client->m_clipboard.m_bNeedToNotify) {
					m_client->m_clipboard.m_bNeedToNotify = false;
					if (m_client->m_clipboard.settings.m_bSupportsEx) {
						int actualLen = m_client->m_clipboard.extendedClipboardDataNotifyMessage.GetDataLength();

						rfbServerCutTextMsg message;
						memset(&message, 0, sizeof(rfbServerCutTextMsg));
						message.type = rfbServerCutText;

						message.length = Swap32IfLE(-actualLen);

						//adzm 2010-09 - minimize packets. SendExact flushes the queue.Queue

						bShouldFlush = true;

						if (!m_client->SendRFBMsgQueue(rfbServerCutText,
							(BYTE*)&message, sizeof(message))) {
							m_client->m_socket->Close();
						}
						if (!m_client->m_socket->SendExact((char*)(m_client->m_clipboard.extendedClipboardDataNotifyMessage.GetData()), m_client->m_clipboard.extendedClipboardDataNotifyMessage.GetDataLength()))
							m_client->m_socket->Close();
					}
					m_client->m_clipboard.extendedClipboardDataNotifyMessage.Reset();
				}

				if (bShouldFlush)
					m_client->m_socket->ClearQueue();
			}

			// SEND AN UPDATE
			// We do this without holding locks, to avoid network problems
			// stalling the server.

			// Update the client palette if necessary

			if (send_palette)
				m_client->SendPalette();

			//add extra check to avoid buffer/encoder sync problems
			if ((m_client->m_encodemgr.m_scrinfo.framebufferHeight == m_client->m_encodemgr.m_buffer->m_scrinfo.framebufferHeight) &&
				(m_client->m_encodemgr.m_scrinfo.framebufferWidth == m_client->m_encodemgr.m_buffer->m_scrinfo.framebufferWidth) &&
				(m_client->m_encodemgr.m_scrinfo.format.bitsPerPixel == m_client->m_encodemgr.m_buffer->m_scrinfo.format.bitsPerPixel &&
					m_client->initialCapture_done)) {
				if (settings->getMaxCpu() == 100)
					m_client->sendingUpdate = true;
				if (m_client->SendUpdate(update)) {
					clipregion.clear();
#ifdef _DEBUG
					static DWORD sNotifyLastCopy1 = GetTickCount();
					DWORD now = GetTickCount();;
					OutputDevMessage("==================== SendUpdate %4d =======================", now - sNotifyLastCopy1);
					sNotifyLastCopy1 = now;
#endif
				}
				m_client->sendingUpdate = false;
			}
			//else
				//clipregion.clear();
		}//end omni_mutex_lock l(m_client->GetUpdateLock(),82);
		yield();
	}

	vnclog.Print(LL_INTINFO, VNCLOG("stopping update thread\n"));
	return 0;
}

vncClientThread::~vncClientThread()
{
	if (m_client != NULL && !m_deleted)
		delete m_client;
#ifdef _Gii
#ifdef _USE_DLL
	if (win8dllHandle) FreeLibrary(win8dllHandle);
#endif
	if (point_status) delete[] point_status;
#endif
}

BOOL
vncClientThread::Init(vncClient* client, vncServer* server, VSocket* socket, BOOL auth, BOOL shared)
{
	// Save the server pointer and window handle
	m_deleted = false;
	m_server = server;
	m_socket = socket;
	m_client = client;
	m_auth = auth;
	m_shared = shared;
	m_autoreconnectcounter_quit = false;
	m_client->m_Autoreconnect = m_server->AutoReconnect();
	m_server->AutoReconnect(false);

	m_AutoReconnectPort = m_server->AutoReconnectPort();
	strcpy_s(m_szAutoReconnectAdr, m_server->AutoReconnectAdr());
	strcpy_s(m_szAutoReconnectId, m_server->AutoReconnectId());
#ifdef _Gii
	point_status = NULL;
	nr_points = 0;
#endif
	// Start the thread
	start();

	return TRUE;
}

BOOL
vncClientThread::InitVersion()
{
	bool repeaterkeepaliveloop = true;
	rfbProtocolVersionMsg protocol_ver{};
	while (repeaterkeepaliveloop)
	{
		// adzm 2010-09
		repeaterkeepaliveloop = false;
		m_major = 0;
		m_minor = 0;

		protocol_ver[12] = 0;
		if (strcmp(m_client->ProtocolVersionMsg, "0.0.0.0") == 0)
		{
			// Generate the server's protocol version
			// RDV 2010-6-10
			rfbProtocolVersionMsg protocolMsg{};
			sprintf_s((char*)protocolMsg, 13,
				rfbProtocolVersionFormat,
				rfbProtocolMajorVersion,
				rfbProtocolMinorVersion);
			// adzm 2010-08
			bool bRetry = true;
			bool bReady = false;
			int nRetry = 0;
			while (!bReady && bRetry) {
				// RDV 2010-6-10
				int Send_OK = 0;
				int Recv_OK = 0;
				vnclog.Print(LL_STATE, VNCLOG("Send protocolMsg\n"));
				Send_OK = m_socket->SendExact((char*)&protocolMsg, sz_rfbProtocolVersionMsg);
				if (Send_OK == 1)
				{
					vnclog.Print(LL_STATE, VNCLOG("Send_OK\n"));
					Recv_OK = m_socket->ReadExact((char*)&protocol_ver, sz_rfbProtocolVersionMsg);
				}
				// Send our protocol version, and get the client's protocol version
				if (!Send_OK || !Recv_OK) {
					if (!Recv_OK) vnclog.Print(LL_STATE, VNCLOG("!Send_OK || !Recv_OK\n"));
					bReady = false;
					// we need to reconnect!

					Sleep(std::min(nRetry * 1000, 30000));

					if (TryReconnect()) {
						// reconnect if in SC mode and not already using AutoReconnect
						bRetry = true;
						nRetry++;
					}
					else {
						bRetry = false;
					}
				}
				else {
					bReady = true;
				}
			}

			if (!bReady) {
				return FALSE;
			}
		}
		else memcpy(protocol_ver, m_client->ProtocolVersionMsg, sz_rfbProtocolVersionMsg);
		//We let the repeater send a invalid sz_rfbProtocolVersionMsg, that start with "REP"
		//Old serverversion return FALSE; and close repeater connection
		//Using an old repeater this doesn't make a difference

		if (strncmp(protocol_ver, "REP", 3) == 0)
		{
			vnclog.Print(LL_STATE, VNCLOG("Keepalive received\n"));
			repeaterkeepaliveloop = true;
		}
	}

	// sf@2006 - Trying to fix neverending authentication bug - Check if this is RFB protocol
	if (strncmp(protocol_ver, "RFB", 3) != 0)
		return FALSE;

	// Check viewer's the protocol version
	sscanf_s((char*)&protocol_ver, rfbProtocolVersionFormat, &m_major, &m_minor);
	if (m_major != rfbProtocolMajorVersion)
		return FALSE;

	m_ms_logon = settings->getRequireMSLogon();
	vnclog.Print(LL_INTINFO, VNCLOG("m_ms_logon set to %s"), m_ms_logon ? "true" : "false");
	m_client->SetUltraViewer(false);
	if ((m_minor >= 7) && m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL) {
		m_socket->SetPluginStreamingIn();
		m_socket->SetPluginStreamingOut();
	}

	return TRUE;
}

#ifdef _Gii
BOOL
vncClientThread::InitGiiVersion()
{
	BOOL bReady = FALSE;
	if (true)
	{
		// Generate the server's Gii protocol version
		rfbGIIServerVersionMsg rfbGIIServerVersion{};
		rfbGIIServerVersion.header.messageType = rfbGIIMessage;
		rfbGIIServerVersion.header.subType = rfbGIIVersionMessage; //big endian
		//rfbGIIServerVersion.header.subType = 1;
		rfbGIIServerVersion.header.length = 4;
		rfbGIIServerVersion.header.length = Swap16IfLE(rfbGIIServerVersion.header.length);
		rfbGIIServerVersion.maxVersion = rfbGIIMaxVersion;
		rfbGIIServerVersion.maxVersion = Swap16IfLE(rfbGIIServerVersion.maxVersion);
		rfbGIIServerVersion.minVersion = rfbGIIMinVersion;
		rfbGIIServerVersion.minVersion = Swap16IfLE(rfbGIIServerVersion.minVersion);
		// Send our protocol version, and get the client's protocol version
		if (!m_socket->SendExactQueue((char*)&rfbGIIServerVersion, sz_rfbGIIServerVersionMsg))
		{
			vnclog.Print(LL_INTINFO, VNCLOG("Write GII Protocol Version failed"));
			bReady = FALSE;
		}
		else
		{
			vnclog.Print(LL_INTINFO, VNCLOG("Write GII Protocol Version"));
			bReady = TRUE;
		}
	}
	else
	{
		vnclog.Print(LL_INTINFO, VNCLOG("Touch Support Disabled -> DOESN'T Write GII Protocol Version"));
	}
	return bReady;
}
#endif

// RDV 2010-4-10
// Ask user Permission Accept/Reject
// Interactive, destop depended
#ifndef SC_20
BOOL
vncClientThread::FilterClients_Ask_Permission()
{
	// Verify the peer host name against the AuthHosts string
	vncServer::AcceptQueryReject verified;
	if (m_auth) {
		verified = vncServer::aqrAccept;
	}
	else {
		verified = m_server->VerifyHost(m_socket->GetPeerName(false));
	}

	// If necessary, query the connection with a timed dialog
	char username[UNLEN + 1];
	if (!processHelper::CurrentUser(username, sizeof(username))) return false;
	if ((strcmp(username, "") != 0) || settings->getQueryIfNoLogon()) // marscha@2006 - Is AcceptDialog required even if no user is logged on
	{
		if (verified == vncServer::aqrQuery) {
			// 10 Dec 2008 jdp reject/accept all incoming connections if the workstation is locked
			if (processHelper::IsWSLocked() && !settings->getQueryIfNoLogon()) {
				if (m_server->QueryAcceptLocked())
					verified = m_server->QueryAccept() == 1 ? vncServer::aqrAccept : vncServer::aqrReject;
				else
					//m_queryaccept==2, new method to allow accept on locked user
					verified = vncServer::aqrAccept;
			}
			else {
				vncAcceptDialog* acceptDlg = new vncAcceptDialog(settings->getQueryTimeout(), m_server->QueryAccept() == 1, m_socket->GetPeerName(true), m_client->infoMsg, settings->getNotification());
				if (acceptDlg == NULL) {
					if (m_server->QueryAccept() == 1)
						verified = vncServer::aqrAccept;
					else
						verified = vncServer::aqrReject;
				}
				else if (!(acceptDlg->DoDialog()))
					verified = vncServer::aqrReject;
			}
		}
	}

	if (verified == vncServer::aqrReject) {
		return FALSE;
	}
	return TRUE;
}
#endif // SC_20

// RDV 2010-4-10
// Filter Blacklisted are refused connection
// Not interactive, not destop depended
BOOL
vncClientThread::FilterClients_Blacklist()
{
	// Verify the peer host name against the AuthHosts string
	vncServer::AcceptQueryReject verified;
	if (m_auth) {
		verified = vncServer::aqrAccept;
	}
	else {
		verified = m_server->VerifyHost(m_socket->GetPeerName(false));
	}

	if (verified == vncServer::aqrReject) {
		return FALSE;
	}
	return TRUE;
}

BOOL vncClientThread::CheckEmptyPasswd()
{
	vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());
	// By default we disallow passwordless workstations!
	if ((strlen(plain) == 0) && settings->getAuthRequired())
	{
		vnclog.Print(LL_CONNERR, VNCLOG("no password specified for server - client rejected\n"));
		SendConnFailed("This server does not have a valid password enabled."
			"Until a password is set, incoming connections cannot be accepted.");
		return FALSE;
	}
	return TRUE;
}

BOOL
vncClientThread::CheckLoopBack()
{
	// By default we filter out local loop connections, because they're pointless
	if (!settings->getAllowLoopback())
	{
		char* localname = _strdup(m_socket->GetSockName());
		char* remotename = _strdup(m_socket->GetPeerName(false));

		// Check that the local & remote names are different!
		if ((localname != NULL) && (remotename != NULL))
		{
			BOOL ok = _stricmp(localname, remotename) != 0;

			if (localname != NULL)
				free(localname);

			if (remotename != NULL)
				free(remotename);

			if (!ok)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("loopback connection attempted - client rejected\n"));
				SendConnFailed("Local loop-back connections are disabled.");
				return FALSE;
			}
		}
	}
	else
	{
		char* localname = _strdup(m_socket->GetSockName());
		char* remotename = _strdup(m_socket->GetPeerName(false));

		// Check that the local & remote names are different!
		if ((localname != NULL) && (remotename != NULL))
		{
			BOOL ok = _stricmp(localname, remotename) != 0;

			if (localname != NULL)
				free(localname);

			if (remotename != NULL)
				free(remotename);

			if (!ok)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("loopback connection attempted - client accepted\n"));
				m_client->m_IsLoopback = true;
			}
		}
	}
	return TRUE;
}

//WARNING  USING THIS FUNCTION AT A WRONG PLACE
//SEND  0 ---> rfbVncAuthOK
void vncClientThread::SendConnFailed(const char* szMessage)
{
	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	if (m_minor >= 7)
	{
		// 0 = Failure
		CARD8 value = 0;
		if (!m_socket->SendExactQueue((char*)&value, sizeof(value)))
			return;
	}
	else
	{
		CARD32 auth_val = Swap32IfLE(rfbConnFailed);
		if (!m_socket->SendExactQueue((char*)&auth_val, sizeof(auth_val)))
			return;
	}
	CARD32 errlen = Swap32IfLE(strlen(szMessage));
	if (!m_socket->SendExactQueue((char*)&errlen, sizeof(errlen)))
		return;
	m_socket->SendExact(szMessage, (const VCard)strlen(szMessage));
}

void vncClientThread::LogAuthResult(bool success, bool isconnected)
{
#ifndef SC_20
	if (!success)
	{
		vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));
		typedef BOOL(*LogeventFn)(char* machine);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
		{
			char* p = strrchr(szCurrentDir, '\\');
			*p = '\0';
			strcat_s(szCurrentDir, "\\logging.dll");
		}
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
		{
			Logevent = (LogeventFn)GetProcAddress(hModule, "LOGFAILED");
			Logevent((char*)m_client->GetClientNameName());
			FreeLibrary(hModule);
		}
	}
	else
	{
		typedef BOOL(*LogeventFn)(char* machine, int clientId, bool isinteractive);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
		{
			char* p = strrchr(szCurrentDir, '\\');
			*p = '\0';
			strcat_s(szCurrentDir, "\\logging.dll");
		}
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
		{
			if (!isconnected) {
				Logevent = (LogeventFn)GetProcAddress(hModule, "LOGCONN");
			} else {
				Logevent = (LogeventFn)GetProcAddress(hModule, "LOGLOGON");
			}
			Logevent((char*)m_client->GetClientNameName(), m_client->GetClientId(), m_client->m_keyboardenabled && m_client->m_pointerenabled);
			FreeLibrary(hModule);
		}
	}
#endif // SC_20
}

BOOL
vncClientThread::InitAuthenticate()
{
	vnclog.Print(LL_INTINFO, "Entered InitAuthenticate\n");
	// RDV 2010-4-10
	// Split Filter in desktop in/depended
	if (!FilterClients_Blacklist())
	{
		SendConnFailed("Your connection has been rejected.");
		return FALSE;
	}
	if (!CheckEmptyPasswd()) return FALSE;
	if (!CheckLoopBack()) return FALSE;

	//adzm 2010-09 - Do the actual authentication
	if (m_minor >= 7) {
		std::vector<CARD8> current_auth;
		if (!AuthenticateClient(current_auth, false)) {
			return FALSE;
		}
	}
	else {
		// RDV 2010-4-10
#ifndef SC_20
		if (!FilterClients_Ask_Permission())
		{
			SendConnFailed("Your connection has been rejected.");
			return FALSE;
		}
#endif // SC_20
		if (!AuthenticateLegacyClient(true)) {
			return FALSE;
		}
	}

	if (m_server->getNumberViewers() > settings->getMaxViewers() - 1)
	{
		if (settings->getMaxViewerSetting() == 0) {
			SendConnFailed("Max viewers reached");
			return FALSE;
		}
		else {
			m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());
			m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());
			m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());
			m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());
			m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());
			m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());

			m_server->GetClient(m_server->getOldestViewer())->Kill();
			m_client->forceBlacklist = true;
		}
	}

	// Read the client's initialisation message
	rfbClientInitMsg client_ini{};
	if (!m_socket->ReadExact((char*)&client_ini, sz_rfbClientInitMsg))
		return FALSE;

	if (client_ini.flags & clientInitExtraMsgSupport) {
		rfbClientInitExtraMsg msg{};
		if (!m_socket->ReadExact((char*)&msg, sz_rfbClientInitExtraMsg))
			return FALSE;
		if (msg.textLength < 0 || msg.textLength > 254)
			return FALSE;
		if (!m_socket->ReadExact(m_client->infoMsg, msg.textLength))
			return FALSE;
	}

#ifndef SC_20
	// Check the FilterClients thing after final auth
	if (strlen(m_client->infoMsg) > 0)
	{
		typedef BOOL(*LogeventFn)(char* info);
		LogeventFn Logevent = NULL;
		char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
		{
			char* p = strrchr(szCurrentDir, '\\');
			*p = '\0';
			strcat_s(szCurrentDir, "\\logging.dll");
		}
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
		{
			Logevent = (LogeventFn)GetProcAddress(hModule, "LOGEXTRAINFO");
			
			if (Logevent)
				Logevent((char*)m_client->infoMsg);
			FreeLibrary(hModule);
		}
	}
	if ((m_minor >= 7) && !FilterClients_Ask_Permission())
	{
		vnclog.Print(LL_CLIENTS, VNCLOG("Your connection has been rejected.\n"));
		return FALSE;
	}
#endif // SC_20

	// If the client wishes to have exclusive access then remove other clients
	if (settings->getConnectPriority() == 3 && !m_shared)
	{
		// Existing
		if (m_server->AuthClientCount() > 0)
		{
			vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
			return FALSE;
		}
	}
	// adzm 2010-09
	if (!(client_ini.flags & clientInitShared) && !m_shared)
	{
		// Which client takes priority, existing or incoming?
		if (settings->getConnectPriority() < 1)
		{
			// Incoming
			vnclog.Print(LL_INTINFO, VNCLOG("non-shared connection - disconnecting old clients\n"));
			m_server->KillAuthClients();
		}
		else if (settings->getConnectPriority() > 1)
		{
			// Existing
			if (m_server->AuthClientCount() > 0)
			{
				vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
				return FALSE;
			}
		}
	}

	vnclog.Print(LL_CLIENTS, VNCLOG("Leaving InitAuthenticate\n"));
	// Tell the server that this client is ok
	return m_server->Authenticated(m_client->GetClientId());
}

BOOL vncClientThread::AuthenticateClient(std::vector<CARD8>& current_auth, bool connected)
{
	// adzm 2010-09 - Gather all authentication types we support
	std::vector<CARD8> auth_types;

	const bool bUseSessionSelect = false;

	// obviously needs to be one that we suggested in the first place
	bool bSecureVNCPluginActive = std::find(current_auth.begin(), current_auth.end(), rfbUltraVNC_SecureVNCPluginAuth_new) != current_auth.end();
	bool bSCPromptActive = std::find(current_auth.begin(), current_auth.end(), rfbUltraVNC_SCPrompt) != current_auth.end();
	bool bSessionSelectActive = std::find(current_auth.begin(), current_auth.end(), rfbUltraVNC_SessionSelect) != current_auth.end();

	if (current_auth.empty()) {
		// send the UltraVNC auth type to identify ourselves as an UltraVNC Server, but only initially
		auth_types.push_back(rfbUltraVNC);
		auth_types.push_back(rfbClientInitExtraMsgSupportNew);
	}	

	// encryption takes priority over everything, for now at least.
	// would be useful to have a host list to configure these settings.
	// Include the SecureVNCPluginAuth type for those that support it but are not UltraVNC Viewers
	if (!bSecureVNCPluginActive && m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL)
	{
		auth_types.push_back(rfbUltraVNC_SecureVNCPluginAuth_new);
		auth_types.push_back(rfbUltraVNC_SecureVNCPluginAuth);
	}
	else if ((settings->getScPrompt() || settings->getScExit()) && !bSCPromptActive)
	{
		// adzm 2010-10 - Add the SCPrompt pseudo-auth
		auth_types.push_back(rfbUltraVNC_SCPrompt);
	}
	else if (bUseSessionSelect)
	{
		// adzm 2010-10 - Add the SessionSelect pseudo-auth
		auth_types.push_back(rfbUltraVNC_SessionSelect);
	}		
	else
	{
		vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());

		if (!m_auth && m_ms_logon)
		{
			auth_types.push_back(rfbUltraVNC_MsLogonIIAuth);
		}
		else
		{
			if (m_auth || (strlen(plain) == 0))
			{
				auth_types.push_back(rfbNoAuth);
			}
			else {
				auth_types.push_back(rfbVncAuth);
			}
		}
	}

	// adzm 2010-09 - Send the auths
	{
		CARD8 authCount = (CARD8)auth_types.size();
		if (!m_socket->SendExactQueue((const char*)&authCount, sizeof(authCount)))
			return FALSE;

		for (std::vector<CARD8>::iterator auth_it = auth_types.begin(); auth_it != auth_types.end(); auth_it++)
		{
			CARD8 authType = *auth_it;
			if (!m_socket->SendExactQueue((const char*)&authType, sizeof(authType)))
				return FALSE;
		}
		if (!m_socket->ClearQueue())
			return FALSE;
	}
	// read the accepted auth type
	CARD8 auth_accepted = rfbInvalidAuth;
	if (!m_socket->ReadExact((char*)&auth_accepted, sizeof(auth_accepted)))
		return FALSE;

	// obviously needs to be one that we suggested in the first place
	if (std::find(auth_types.begin(), auth_types.end(), auth_accepted) == auth_types.end()) {
		return FALSE;
	}

	// MS-Logon I never seems to be used anymore -- the old code would say if (m_ms_logon) AuthMsLogon (II) else AuthVnc
	// and within AuthVnc would be if (m_ms_logon) { /* MS-Logon code */ }. THat could never be hit since the first case
	// would always match!

	// Authenticate the connection, if required
	BOOL auth_success = FALSE;
	BOOL auth_is_mslogon = FALSE;
	BOOL version_warning = FALSE;
	std::string auth_message;
	switch (auth_accepted)
	{
	case rfbUltraVNC:
		m_client->SetUltraViewer(true);
		auth_success = true;
		break;
	case rfbUltraVNC_SecureVNCPluginAuth_new:
		auth_success = AuthSecureVNCPlugin(auth_message);
		break;
	case rfbUltraVNC_SecureVNCPluginAuth:
		auth_success = AuthSecureVNCPlugin_old(auth_message);
		auth_success = 0;
		version_warning = 1;
		break;
	case rfbUltraVNC_MsLogonIIAuth:
		auth_success = AuthMsLogon(auth_message);
		if (auth_success) {
			auth_is_mslogon = TRUE;
		}
		break;
	case rfbVncAuth:
		auth_success = AuthVnc(auth_message);
		break;
	case rfbNoAuth:
		auth_success = TRUE;
		break;
	case rfbUltraVNC_SCPrompt:
		// adzm 2010-10 - Do the SCPrompt auth
		auth_success = AuthSCPrompt(auth_message);
		break;
	case rfbUltraVNC_SessionSelect:
		// adzm 2010-10 - Do the SessionSelect auth
		auth_success = AuthSessionSelect(auth_message);
		break;
	default:
		auth_success = FALSE;
		break;
	}
	// Log authentication success or failure
	if (!auth_is_mslogon) {
		LogAuthResult(auth_success ? true : false, connected);
	}

	// Return the result
	CARD32 auth_result = rfbVncAuthFailed;
	if (auth_success == 1) {
		current_auth.push_back(auth_accepted);

		// continue the authentication if MS-Logon is enabled. any method of authentication should
		// work out fine with this method. Currently we limit ourselves to only one layer beyond
		// the plugin to avoid deep recursion, but that can easily be changed if necessary.
		if (m_ms_logon && auth_accepted == rfbUltraVNC_SecureVNCPluginAuth_new && m_socket->GetIntegratedPlugin()) {
			auth_result = rfbVncAuthContinue;
		}
		else if (auth_accepted == rfbUltraVNC) {
			auth_result = rfbVncAuthContinue;
		}
		else if ((settings->getScPrompt() || settings->getScExit()) && !bSCPromptActive) {
			auth_result = rfbVncAuthContinue;
		}
		else if (bUseSessionSelect && !bSessionSelectActive) {
			auth_result = rfbVncAuthContinue;
		}		
		else {
			auth_result = rfbVncAuthOK;
		}
	}

	// RDV 2010-4-10
	// This is a good spot for asking the user permission Accept/Reject
	// Only when auth_result == rfbVncAuthOK, in all other cases it isn't needed
	// adzm 2010-10 - This was causing failure with DSM plugin, since this is
	// pretty much the same as another auth type we'll just move it into that code
	// instead. So see the AuthSCPrompt function.

	CARD32 auth_result_msg = Swap32IfLE(auth_result);
	if (!m_socket->SendExactQueue((char*)&auth_result_msg, sizeof(auth_result_msg)))
		return FALSE;

	//adzm 2010-09 - Set handshake complete if integrated plugin finished auth
	/*if ((auth_success || auth_accepted == rfbUltraVNC_SecureVNCPluginAuth) && (auth_accepted == rfbUltraVNC_SecureVNCPluginAuth_new || auth_accepted == rfbUltraVNC_SecureVNCPluginAuth) && m_socket->GetIntegratedPlugin()) {
		m_socket->GetIntegratedPlugin()->SetHandshakeComplete();
	}*/
	if ((auth_success || auth_accepted == rfbUltraVNC_SecureVNCPluginAuth) && (auth_accepted == rfbUltraVNC_SecureVNCPluginAuth) && m_socket->GetIntegratedPlugin()) {
		m_socket->GetIntegratedPlugin()->SetHandshakeComplete();
	}

	// Send a failure reason
	if (!auth_success && !version_warning) {
		if (auth_message.empty()) {
			auth_message = "authentication rejected";
		}
		CARD32 auth_message_length = Swap32IfLE(auth_message.length());
		if (!m_socket->SendExactQueue((char*)&auth_message_length, sizeof(auth_message_length)))
			return FALSE;
		if (!m_socket->SendExact(auth_message.c_str(), (const VCard)auth_message.length()))
			return FALSE;
		return FALSE;
	}

	else if (!auth_success) {
		if (auth_message.empty()) {
			auth_message = "Server require a higher viewer version.";
		}
		CARD32 auth_message_length = Swap32IfLE(auth_message.length());
		if (!m_socket->SendExactQueue((char*)&auth_message_length, sizeof(auth_message_length)))
			return FALSE;
		if (!m_socket->SendExact(auth_message.c_str(), (const VCard)auth_message.length()))
			return FALSE;
		return FALSE;
	}

	if (!m_socket->ClearQueue())
		return FALSE;

	if (auth_success && auth_result == rfbVncAuthContinue) {
		if (!AuthenticateClient(current_auth, true)) {
			return FALSE;
		}
	}

	if (auth_success) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

BOOL vncClientThread::AuthenticateLegacyClient(bool isconnected)
{
	vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());

	CARD32 auth_type = rfbInvalidAuth;

	if (m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled() && m_socket->GetIntegratedPlugin() != NULL)
	{
		auth_type = rfbLegacy_SecureVNCPlugin;
	}
	else if (m_ms_logon)
	{
		auth_type = rfbLegacy_MsLogon;
	}
	else if (strlen(plain) > 0)
	{
		auth_type = rfbVncAuth;
	}
	else if (m_auth || (strlen(plain) == 0))
	{
		auth_type = rfbNoAuth;
	}

	// abort if invalid
	if (auth_type == rfbInvalidAuth) {
		SendConnFailed("unable to determine legacy authentication method");
		return FALSE;
	}

	// RDV 2010-4-10
	// CARD32, byte swap is needed
	auth_type = Swap32IfLE(auth_type);
	// adzm 2010-09 - Send the single auth type
	if (!m_socket->SendExact((const char*)&auth_type, sizeof(auth_type)))
		return FALSE;
	// RDV 2010-4-10
	// CARD32, reset original
	auth_type = Swap32IfLE(auth_type);

	// Authenticate the connection, if required
	BOOL auth_success = FALSE;
	BOOL auth_is_mslogon = FALSE;

	std::string auth_message;
	switch (auth_type)
	{
	case rfbLegacy_SecureVNCPlugin:
		auth_success = AuthSecureVNCPlugin(auth_message);
		// adzm 2010-11 - Legacy 1.0.8.2 special build will continue here with MS-Logon
		if (auth_success && m_ms_logon) {
			CARD32 auth_result_msg = Swap32IfLE(rfbLegacy_MsLogon);
			if (!m_socket->SendExact((char*)&auth_result_msg, sizeof(auth_result_msg)))
				return FALSE;

			//adzm 2010-09 - Set handshake complete if integrated plugin finished auth
			if (auth_success && auth_type == rfbLegacy_SecureVNCPlugin && m_socket->GetIntegratedPlugin()) {
				m_socket->GetIntegratedPlugin()->SetHandshakeComplete();
			}

			auth_type = rfbLegacy_MsLogon;
			auth_success = AuthMsLogon(auth_message);
		}
		break;
	case rfbLegacy_MsLogon:
		auth_success = AuthMsLogon(auth_message);
		if (auth_success) {
			auth_is_mslogon = TRUE;
		}
		break;
	case rfbVncAuth:
		auth_success = AuthVnc(auth_message);
		break;
	case rfbNoAuth:
		auth_success = TRUE;
		break;
	default:
		auth_success = FALSE;
		break;
	}

	// Log authentication success or failure
	if (!auth_is_mslogon) {
		LogAuthResult(auth_success ? true : false, false);
	}

	// Return the result
	CARD32 auth_result = rfbVncAuthFailed;
	if (auth_success) {
		auth_result = rfbVncAuthOK;
	}
	//proto 6.2.1 np securityresult for none 3.3 and 3.7
	if (auth_type != rfbNoAuth)
	{
		CARD32 auth_result_msg = Swap32IfLE(auth_result);
		if (!m_socket->SendExact((char*)&auth_result_msg, sizeof(auth_result_msg)))
			return FALSE;
	}

	//adzm 2010-09 - Set handshake complete if integrated plugin finished auth
	if (auth_success && auth_type == rfbLegacy_SecureVNCPlugin && m_socket->GetIntegratedPlugin()) {
		m_socket->GetIntegratedPlugin()->SetHandshakeComplete();
	}

	if (auth_success) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// must SetHandshakeComplete after sending auth result!
BOOL vncClientThread::AuthSecureVNCPlugin(std::string& auth_message)
{
	bool bPassphrase = false;
	vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());
	ConfigHelper ConfigHelpervar(settings->getDSMPluginConfig());
	if (strlen(ConfigHelpervar.m_szPassphrase) > 0)
	{
		bPassphrase = true;
		ConfigHelpervar.SetConfigHelper(ConfigHelpervar.m_dwFlags, "");
		m_socket->SetDSMPluginConfig(ConfigHelpervar.m_szConfig);
	}
	else
	{
		m_socket->SetDSMPluginConfig(settings->getDSMPluginConfig());
	}

	BOOL auth_ok = FALSE;

	const char* plainPassword = plain;
	/*if (!m_ms_logon && plainPassword && strlen(plainPassword) > 0) {
		m_socket->GetIntegratedPlugin()->SetPasswordData((const BYTE*)plainPassword, strlen(plainPassword));
	}*/

	int nSequenceNumber = 0;
	bool bSendChallenge = true;
	do
	{
		BYTE* pChallenge = NULL;
		int nChallengeLength = 0;
		if (!m_socket->GetIntegratedPlugin()->GetChallenge(pChallenge, nChallengeLength, nSequenceNumber)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			auth_message = m_socket->GetIntegratedPlugin()->GetLastErrorString();
			return FALSE;
		}

		WORD wChallengeLength = (WORD)nChallengeLength;

		if (!m_socket->SendExactQueue((const char*)&wChallengeLength, sizeof(wChallengeLength))) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		if (!m_socket->SendExact((const char*)pChallenge, nChallengeLength)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		char passphraseused = bPassphrase;
		if (m_ms_logon || strlen(plainPassword) == 0) passphraseused = 2;
		if (!m_socket->SendExact((const char*)&passphraseused, 1)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
		WORD wResponseLength = 0;
		if (!m_socket->ReadExact((char*)&wResponseLength, sizeof(wResponseLength))) {
			return FALSE;
		}
		if (wResponseLength > 2024) return FALSE;
		BYTE* pResponseData = new BYTE[wResponseLength];
		ZeroMemory(pResponseData, wResponseLength);
		if (!m_socket->ReadExact((char*)pResponseData, wResponseLength)) {
			delete[] pResponseData;
			return FALSE;
		}

		if (!m_socket->GetIntegratedPlugin()->HandleResponse(pResponseData, (int)wResponseLength, nSequenceNumber, bSendChallenge)) {
			auth_message = m_socket->GetIntegratedPlugin()->GetLastErrorString();
			auth_ok = FALSE;
			bSendChallenge = false;
		}
		else if (!bSendChallenge) {
			auth_ok = TRUE;
		}

		delete[] pResponseData;

		m_socket->GetIntegratedPlugin()->SetHandshakeComplete();

		if (!m_ms_logon && strlen(plainPassword) != 0)
		{
			if (!m_socket->ReadExact((char*)&wResponseLength, sizeof(wResponseLength))) {
				return FALSE;
			}
			if (wResponseLength > 2024) return FALSE;
			pResponseData = new BYTE[wResponseLength];

			if (!m_socket->ReadExact((char*)pResponseData, wResponseLength)) {
				delete[] pResponseData;
				return FALSE;
			}
			if (bPassphrase == false)
			{
				if (memcmp(plain, pResponseData, strlen(plain))) auth_ok = false;
			}
			else if (memcmp(ConfigHelpervar.m_szPassphrase, pResponseData, strlen(ConfigHelpervar.m_szPassphrase))) auth_ok = false;
			delete[] pResponseData;
		}
		nSequenceNumber++;
	} while (bSendChallenge);

	if (auth_ok) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// must SetHandshakeComplete after sending auth result!
BOOL vncClientThread::AuthSecureVNCPlugin_old(std::string& auth_message)
{
	vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());
	m_socket->SetDSMPluginConfig(settings->getDSMPluginConfig());
	BOOL auth_ok = FALSE;

	const char* plainPassword = plain;

	if (!m_ms_logon && plainPassword && strlen(plainPassword) > 0) {
		m_socket->GetIntegratedPlugin()->SetPasswordData((const BYTE*)plainPassword, (int)strlen(plainPassword));
	}

	int nSequenceNumber = 0;
	bool bSendChallenge = true;
	do
	{
		BYTE* pChallenge = NULL;
		int nChallengeLength = 0;
		if (!m_socket->GetIntegratedPlugin()->GetChallenge(pChallenge, nChallengeLength, nSequenceNumber)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			auth_message = m_socket->GetIntegratedPlugin()->GetLastErrorString();
			return FALSE;
		}

		WORD wChallengeLength = (WORD)nChallengeLength;

		if (!m_socket->SendExactQueue((const char*)&wChallengeLength, sizeof(wChallengeLength))) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		if (!m_socket->SendExact((const char*)pChallenge, nChallengeLength)) {
			m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
			return FALSE;
		}

		m_socket->GetIntegratedPlugin()->FreeMemory(pChallenge);
		WORD wResponseLength = 0;
		if (!m_socket->ReadExact((char*)&wResponseLength, sizeof(wResponseLength))) {
			return FALSE;
		}

		BYTE* pResponseData = new BYTE[wResponseLength];

		if (!m_socket->ReadExact((char*)pResponseData, wResponseLength)) {
			delete[] pResponseData;
			return FALSE;
		}

		if (!m_socket->GetIntegratedPlugin()->HandleResponse(pResponseData, (int)wResponseLength, nSequenceNumber, bSendChallenge)) {
			auth_message = m_socket->GetIntegratedPlugin()->GetLastErrorString();
			auth_ok = FALSE;
			bSendChallenge = false;
		}
		else if (!bSendChallenge) {
			auth_ok = TRUE;
		}

		delete[] pResponseData;
		nSequenceNumber++;
	} while (bSendChallenge);

	if (auth_ok) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// marscha@2006: Try to better hide the windows password.
// I know that this is no breakthrough in modern cryptography.
// It's just a patch/kludge/workaround.
BOOL
vncClientThread::AuthMsLogon(std::string& auth_message)
{
	DH dh;
	char gen[8], mod[8], pub[8], resp[8];
	char user[256], passwd[64];
	unsigned char key[8]{};

	dh.createKeys();
	int64ToBytes(dh.getValue(DH_GEN), gen);
	int64ToBytes(dh.getValue(DH_MOD), mod);
	int64ToBytes(dh.createInterKey(), pub);

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	if (!m_socket->SendExactQueue(gen, sizeof(gen))) return FALSE;
	if (!m_socket->SendExactQueue(mod, sizeof(mod))) return FALSE;
	if (!m_socket->SendExact(pub, sizeof(pub))) return FALSE;
	if (!m_socket->ReadExact(resp, sizeof(resp))) return FALSE;
	if (!m_socket->ReadExact(user, sizeof(user))) return FALSE;
	if (!m_socket->ReadExact(passwd, sizeof(passwd))) return FALSE;

	int64ToBytes(dh.createEncryptionKey(bytesToInt64(resp)), (char*)key);
	vnclog.Print(0, "After DH: g=%I64u, m=%I64u, i=%I64u, key=%I64u\n", bytesToInt64(gen), bytesToInt64(mod), bytesToInt64(pub), bytesToInt64((char*)key));
	vncDecryptBytes((unsigned char*)user, sizeof(user), key); user[255] = '\0';
	vncDecryptBytes((unsigned char*)passwd, sizeof(passwd), key); passwd[63] = '\0';

	int result = CheckUserGroupPasswordUni(user, passwd, m_client->GetClientNameAddress());
	vnclog.Print(LL_INTINFO, "CheckUserGroupPasswordUni result=%i\n", result);
	if (result == 2) { // ViewOnly?
		m_client->EnableKeyboard(false);
		m_client->EnablePointer(false);
		m_client->EnableGii(false);
	}

	if (result) {
		if (user != NULL)
			m_client->m_client_domain_username = _strdup(user);
		else
			m_client->m_client_domain_username = _strdup("<unknown>");
		return TRUE;
	}
	else {
		return FALSE;
	}
}

BOOL vncClientThread::AuthVnc(std::string& auth_message)
{
	vncPasswd::ToText plain(settings->getPasswd(), settings->getSecure());

	BOOL auth_ok = FALSE;
	{
		// Now create a 16-byte challenge
		char challenge[16]{};
		char challenge2[16]; //PGM
		char response[16];
		vncRandomBytes((BYTE*)&challenge);
		memcpy(challenge2, challenge, 16); //PGM

		{
			vnclog.Print(LL_INTINFO, "password authentication");
			if (!m_socket->SendExact(challenge, sizeof(challenge)))
			{
				vnclog.Print(LL_SOCKERR, VNCLOG("Failed to send challenge to client\n"));
				return FALSE;
			}

			// Read the response
			if (!m_socket->ReadExact(response, sizeof(response)))
			{
				vnclog.Print(LL_SOCKERR, VNCLOG("Failed to receive challenge response from client\n"));
				return FALSE;
			}
			// Encrypt the challenge bytes
			vncEncryptBytes((BYTE*)&challenge, plain);

			auth_ok = TRUE;
			// Compare them to the response
			for (int i = 0; i < sizeof(challenge); i++)
			{
				if (challenge[i] != response[i])
				{
					auth_ok = FALSE;
					break;
				}
			}
			if (!auth_ok) //PGM
			{ //PGM
				vncPasswd::ToText plain2(settings->getPasswd2(), settings->getSecure()); //PGM
				if ((strlen(plain2) > 0)) //PGM
				{ //PGM
					vnclog.Print(LL_INTINFO, "View-only password authentication"); //PGM
					m_client->EnableKeyboard(false); //PGM
					m_client->EnablePointer(false); //PGM
					m_client->EnableGii(false);
					auth_ok = TRUE; //PGM

					// Encrypt the view-only challenge bytes //PGM
					vncEncryptBytes((BYTE*)&challenge2, plain2); //PGM

					// Compare them to the response //PGM
					for (int i = 0; i < sizeof(challenge2); i++) //PGM
					{ //PGM
						if (challenge2[i] != response[i]) //PGM
						{ //PGM
							auth_ok = FALSE; //PGM
							break; //PGM
						} //PGM
					} //PGM
				} //PGM
			} //PGM
		}
	}

	if (auth_ok) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// adzm 2010-10
BOOL vncClientThread::AuthSCPrompt(std::string& auth_message)
{
	// Check if viewer accept connection
	char mytext[1024];
	getinfo(mytext);
	int size = (int)strlen(mytext);
	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	if (!m_socket->SendExactQueue((char*)&size, sizeof(int)))
		return FALSE;
	if (!m_socket->SendExact((char*)mytext, size))
		return FALSE;
	int nummer = 0;
	if (!m_socket->ReadExact((char*)&nummer, sizeof(int)))
	{
		return FALSE;
	}
	if (nummer == 0)
	{
		auth_message = "Viewer refused connection";
		return FALSE;
	}
	else {
		return TRUE;
	}
}

BOOL vncClientThread::AuthSessionSelect(std::string& auth_message)
{
	return TRUE;
	/* Session select */
	/*
	{
		//Fake Function
		CARD32 auth_result_msg = Swap32IfLE(rfbUltraVNC_SessionSelect);
		if (!m_socket->SendExact((char *)&auth_result_msg, sizeof(auth_result_msg)))
			return FALSE;
		CARD8 Items=3;
		if (!m_socket->SendExact((char *)&Items, sizeof(Items)))
			return FALSE;
		char line1[128];
		char line2[128];
		char line3[128];
		strcpy_s(line1,"line1 ");
		strcpy_s(line2,"line22 ");
		strcpy_s(line3,"line312 123 ");
		if (!m_socket->SendExact((char *)line1, 128))
			return FALSE;
		if (!m_socket->SendExact((char *)line2, 128))
			return FALSE;
		if (!m_socket->SendExact((char *)line3, 128))
			return FALSE;
		int nummer;
		if (!m_socket->ReadExact((char *)&nummer, sizeof(int)))
		{
			return FALSE;
		}
		int a=0;
	}
	*/
}

void
ClearKeyState(BYTE key)
{
	// This routine is used by the VNC client handler to clear the
	// CAPSLOCK, NUMLOCK and SCROLL-LOCK states.

	BYTE keyState[256]{};

	GetKeyboardState((LPBYTE)&keyState);

	if (keyState[key] & 1)
	{
		// Simulate the key being pressed
		keybd_uni_event(key, 0, KEYEVENTF_EXTENDEDKEY, 0);

		// Simulate it being release
		keybd_uni_event(key, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	}
}

// Modif sf@2002
// Get the local ip addresses as a human-readable string.
// If more than one, then with \n between them.
// If not available, then gets a message to that effect.
void GetIPString(char* buffer, int buflen)
{
	char namebuf[256];

	if (gethostname(namebuf, 256) != 0)
	{
		strncpy_s(buffer, buflen, "Host name unavailable", buflen);
		return;
	}
	if (settings->getIPV6()) {
		*buffer = '\0';

		LPSOCKADDR sockaddr_ip;
		struct addrinfo hint;
		struct addrinfo* serverinfo = 0;
		memset(&hint, 0, sizeof(hint));
		hint.ai_family = AF_UNSPEC;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		struct sockaddr_in6* pIpv6Addr;
		struct sockaddr_in* pIpv4Addr;
		struct sockaddr_in6 Ipv6Addr;
		struct sockaddr_in Ipv4Addr;
		memset(&Ipv6Addr, 0, sizeof(Ipv6Addr));
		memset(&Ipv4Addr, 0, sizeof(Ipv4Addr));

		//make sure the buffer is not overwritten

		if (getaddrinfo(namebuf, 0, &hint, &serverinfo) == 0)
		{
			struct addrinfo* p;
			p = serverinfo;
			for (p = serverinfo; p != NULL; p = p->ai_next) {
				switch (p->ai_family) {
				case AF_INET:
				{
					break;
				}
				case AF_INET6:
				{
					char ipstringbuffer[46];
					DWORD ipbufferlength = 46;
					ipbufferlength = 46;
					memset(ipstringbuffer, 0, 46);

					pIpv6Addr = (struct sockaddr_in6*)p->ai_addr;
					memcpy(&Ipv6Addr, pIpv6Addr, sizeof(Ipv6Addr));
					Ipv6Addr.sin6_family = AF_INET6;

					sockaddr_ip = (LPSOCKADDR)p->ai_addr;

					WSAAddressToString(sockaddr_ip, (DWORD)p->ai_addrlen, NULL, ipstringbuffer, &ipbufferlength);
					char			szText[256];
					memset(szText, 0, 256);
					strncpy_s(szText, ipstringbuffer, ipbufferlength - 4);
					strcat_s(szText, "-");
					int len = strlen(buffer);
					int len2 = strlen(szText);
					if (len + len2 < buflen)strcat_s(buffer, buflen, szText);
					break;
				}
				default:
					break;
				}
			}

		}
		freeaddrinfo(serverinfo);
	}
	else {
		HOSTENT* ph = NULL;
		ph = gethostbyname(namebuf);
		if (!ph)
		{
			strncpy_s(buffer, buflen, "IP address unavailable", buflen);
			return;
		}

		*buffer = '\0';
		char digtxt[5];
		for (int i = 0; ph->h_addr_list[i]; i++)
		{
			for (int j = 0; j < ph->h_length; j++)
			{
				sprintf_s(digtxt, "%d.", (unsigned char)ph->h_addr_list[i][j]);
				strncat_s(buffer, buflen, digtxt, (buflen - 1) - strlen(buffer));
			}
			buffer[strlen(buffer) - 1] = '\0';
			if (ph->h_addr_list[i + 1] != 0)
				strncat_s(buffer, buflen, ", ", (buflen - 1) - strlen(buffer));
		}
	}
}

// adzm 2010-08
bool vncClientThread::InitSocket()
{
	// To avoid people connecting and then halting the connection, set a timeout

	// adzm 2010-10 - Set 0 timeout if using repeater
	VBool bSocketTimeoutSet = VFalse;
	if (m_client->GetRepeaterID()) {
		bSocketTimeoutSet = m_socket->SetTimeout(0);
	}
	else {
		bSocketTimeoutSet = m_socket->SetTimeout(30000);
	}
	if (!bSocketTimeoutSet) {
		vnclog.Print(LL_INTERR, VNCLOG("failed to set socket timeout(%d)\n"), GetLastError());
	}

	// sf@2002 - DSM Plugin - Tell the client's socket where to find the DSMPlugin
	if (m_server->GetDSMPluginPointer() != NULL)
	{
		m_socket->SetDSMPluginPointer(m_server->GetDSMPluginPointer());
		vnclog.Print(LL_INTINFO, VNCLOG("DSMPlugin Pointer to socket OK\n"));

		//adzm 2010-05-12 - dsmplugin config
		//m_socket->SetDSMPluginConfig(m_server->GetDSMPluginConfig());
	}
	else
	{
		vnclog.Print(LL_INTINFO, VNCLOG("Invalid DSMPlugin Pointer\n"));
		return false;
	}

	// LOCK INITIAL SETUP
	// All clients have the m_protocol_ready flag set to FALSE initially, to prevent
	// updates and suchlike interfering with the initial protocol negotiations.

	// sf@2002 - DSMPlugin
	// Use Plugin only from this point (now BEFORE Protocol handshaking)
	if (m_server->GetDSMPluginPointer()->IsEnabled())
	{
		// sf@2007 - Current DSM code does not support multithreading
		// Data mix is causing server crash and viewer drops when more than one viewer is copnnected at a time
		// We must reject any new viewer connection BEFORE any data passes through the plugin
		// This is a dirty workaround. We ignore all Multi Viewer connection settings...
		//adzm 2009-06-20 - Fixed this to use multi-threaded version, so therefore we can handle multiple
		// clients with no issues.
		if (!m_server->GetDSMPluginPointer()->SupportsMultithreaded() && m_server->AuthClientCount() > 0)
		{
			vnclog.Print(LL_CLIENTS, VNCLOG("A connection using DSM already exist - client rejected to avoid crash \n"));
			return false;
		}

		//adzm 2009-06-20 - TODO - Not sure about this. what about pending connections via the repeater?
		if (m_server->AuthClientCount() == 0)
			m_server->GetDSMPluginPointer()->ResetPlugin();	//SEC reset if needed
		m_socket->EnableUsePlugin(true);
		m_client->m_encodemgr.EnableQueuing(false);
		// TODO: Make a more secured challenge (with time stamp)
	}
	else
		m_client->m_encodemgr.EnableQueuing(true);

	return true;
}

bool vncClientThread::TryReconnect()
{
	if (m_client == NULL) return false;
	if (fShutdownOrdered || m_client->m_Autoreconnect || !m_client->GetHost() || !m_client->GetRepeaterID()) {
		return false;
	}

	if (m_socket) {
		m_socket->Close();
		if (m_client && m_client->m_socket) {
			m_client->m_socket = NULL;
		}
		delete m_socket;
		m_socket = NULL;
	}

	// Attempt to create a new socket
	VSocket* tmpsock = new VSocket;
	if (!tmpsock) {
		return false;
	}

	m_socket = tmpsock;
	if (m_client) {
		m_client->m_socket = tmpsock;
	}

	// Connect out to the specified host on the UltraVNC Viewer listen port
	// To be really good, we should allow a display number here but
	// for now we'll just assume we're connecting to display zero
	bool result;
	if (settings->getIPV6())
		result = m_socket->CreateConnect(m_client->GetHost(), m_client->GetHostPort());
	else {
		m_socket->Create();
		result = m_socket->Connect(m_client->GetHost(), m_client->GetHostPort());
	}
	if (result) {
		if (m_client->GetRepeaterID()) {
			char finalidcode[_MAX_PATH];
			//adzm 2010-08 - this was sending uninitialized data over the wire
			ZeroMemory(finalidcode, sizeof(finalidcode));
			strncpy_s(finalidcode, m_client->GetRepeaterID(), sizeof(finalidcode) - 1);

			m_socket->Send(finalidcode, 250);

			InitSocket();

			return true;
		}
	}

	return false;
}

void
vncClientThread::run(void* arg)
{
#ifdef _Gii
#ifdef _USE_DLL
	DLL_InitializeTouchInjection = NULL;
	DLL_PInjectTouch = NULL;
	win8dllHandle = NULL;
	if (VNC_OSVersion::getInstance()->OS_WIN8 || VNC_OSVersion::getInstance()->OS_WIN10)
	{
		win8dllHandle = LoadLibrary("InjectTouch.dll");
		DLL_InitializeTouchInjection = (PInitializeTouchInjection)GetProcAddress(win8dllHandle, "DLL_InitializeTouchInjection");
		DLL_PInjectTouch = (PInjectTouch)GetProcAddress(win8dllHandle, "DLL_PInjectTouch");;
	}
#endif
	InjectTouchInputUVNC = NULL;
	InitializeTouchInjectionUVNC = NULL;
	HINSTANCE user32 = LoadLibraryA("user32.dll");
	if (user32) {
		InjectTouchInputUVNC = (PtrInjectTouchInput)(GetProcAddress(user32, "InjectTouchInput"));
		InitializeTouchInjectionUVNC = (PtrInitializeTouchInjection)(GetProcAddress(user32, "InitializeTouchInjection"));
	}

#endif
	// All this thread does is go into a socket-receive loop,
	// waiting for stuff on the given socket

	// IMPORTANT : ALWAYS call RemoveClient on the server before quitting
	// this thread.
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	vnclog.Print(LL_CLIENTS, VNCLOG("client connected : %s (%hd)\n"),
		m_client->GetClientNameName(),
		m_client->GetClientId());
	// Save the handle to the thread's original desktop
	HDESK home_desktop = GetThreadDesktop(GetCurrentThreadId());
	HDESK input_desktop = 0;

	// Initially blacklist the client so that excess connections from it get dropped
	m_server->AddAuthHostsBlacklist(m_client->GetClientNameAddress());

	// adzm 2010-08
	if (!InitSocket()) {
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}

	// GET PROTOCOL VERSION
	if (!InitVersion())
	{
		// wa@2005 - AutoReconnection attempt if required
		if (m_client->m_Autoreconnect && !fShutdownOrdered)
		{
			for (int i = 0; i < 10 * m_server->AutoReconnect_counter; i++)
			{
				Sleep(100);
				if (m_autoreconnectcounter_quit) return;
			}
			m_server->AutoReconnect_counter += 10;
			if (m_server->AutoReconnect_counter > 20) m_server->AutoReconnect_counter = 20;
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient I\n"));
			m_server->AutoReconnect(m_client->m_Autoreconnect);
			m_server->AutoReconnectPort(m_AutoReconnectPort);
			m_server->AutoReconnectAdr(m_szAutoReconnectAdr);
			m_server->AutoReconnectId(m_szAutoReconnectId);
			if (settings->getIPV6())
				postHelper::PostAddNewClient4(1111, 1111);
			else
				postHelper::PostAddNewClient(1111, 1111);
		}
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}
	m_server->AutoReconnect_counter = 0;
	vnclog.Print(LL_INTINFO, VNCLOG("negotiated version\n"));

	// AUTHENTICATE LINK
	if (!InitAuthenticate())
	{
		m_server->RemoveClient(m_client->GetClientId());
		if (!fShutdownOrdered) {
			if (m_client->m_Autoreconnect)
			{
				vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient II\n"));
				m_server->AutoReconnect(m_client->m_Autoreconnect);
				m_server->AutoReconnectPort(m_AutoReconnectPort);
				m_server->AutoReconnectAdr(m_szAutoReconnectAdr);
				m_server->AutoReconnectId(m_szAutoReconnectId);
				if (settings->getIPV6())
					postHelper::PostAddNewClient4(1111, 1111);
				else
					postHelper::PostAddNewClient(1111, 1111);
			}
		}
		return;
	}

	// Authenticated OK - remove from blacklist and remove timeout
	if (!m_client->forceBlacklist)
		m_server->RemAuthHostsBlacklist(m_client->GetClientNameAddress());
	m_client->forceBlacklist = false;
	m_socket->SetTimeout(settings->getIdleTimeout() * 1000);
	vnclog.Print(LL_INTINFO, VNCLOG("authenticated connection\n"));

	// Set Client Connect time
	m_client->SetConnectTime(GetTimeFunction());

	// INIT PIXEL FORMAT

	// Get the screen format
//	m_client->m_fullscreen = m_client->m_encodemgr.GetSize();

	// Modif sf@2002 - Scaling
	if (m_server->AreThereMultipleViewers() == false)
	{
		omni_mutex_lock l(m_client->GetUpdateLock(), 84);
		m_client->m_encodemgr.m_buffer->SetScale(settings->getDefaultScale()); // v1.1.2
	}
	else
	{
		m_client->SetScreenOffset(m_server->m_desktop->m_ScreenOffsetx, m_server->m_desktop->m_ScreenOffsety, m_server->m_desktop->nr_monitors == 1);
		m_client->SetBufferOffset(m_server->m_desktop->m_SWOffsetx, m_server->m_desktop->m_SWOffsety);
	}
	m_client->m_ScaledScreen = m_client->m_encodemgr.m_buffer->GetViewerSize();
	m_client->m_nScale = m_client->m_encodemgr.m_buffer->GetScale();

	// Get the name of this desktop
	// sf@2002 - v1.1.x - Complete the computer name with the IP address if necessary
	bool fIP = false;
	char desktopname[MAX_COMPUTERNAME_LENGTH + 3 + 256 + 32] = { 0 };
	DWORD desktopnamelen = MAX_COMPUTERNAME_LENGTH + 1 + 256 + 32;
	memset((char*)desktopname, 0, sizeof(desktopname));
	if (GetComputerName(desktopname, &desktopnamelen))
	{
		// Make the name lowercase
		for (size_t x = 0; x < strlen(desktopname); x++)
		{
			desktopname[x] = tolower(desktopname[x]);
		}
		// Check for the presence of "." in the string (then it's presumably an IP adr)
		if (strchr(desktopname, '.') != NULL) fIP = true;
	}
	else
	{
		strcpy_s(desktopname, "WinVNC");
	}

	// We add the IP address(es) to the computer name, if possible and necessary
	if (!fIP)
	{
		char szIP[256];
		GetIPString(szIP, sizeof(szIP));
		if (strlen(szIP) > 3 && szIP[0] != 'I' && szIP[0] != 'H')
		{
			strcat_s(desktopname, " ( ");
			strcat_s(desktopname, szIP);
			strcat_s(desktopname, " )");
		}
	}

	strcat_s(desktopname, " - ");
	if (settings->RunningFromExternalService()) strcat_s(desktopname, "service mode");
	else strcat_s(desktopname, "application mode");

	// Send the server format message to the client
	rfbServerInitMsg server_ini{};
	server_ini.format = m_client->m_encodemgr.m_buffer->GetLocalFormat();

	// Endian swaps
	// Modif sf@2002 - Scaling
	server_ini.framebufferWidth = Swap16IfLE(m_client->m_ScaledScreen.br.x - m_client->m_ScaledScreen.tl.x);
	server_ini.framebufferHeight = Swap16IfLE(m_client->m_ScaledScreen.br.y - m_client->m_ScaledScreen.tl.y);

	server_ini.format.redMax = Swap16IfLE(server_ini.format.redMax);
	server_ini.format.greenMax = Swap16IfLE(server_ini.format.greenMax);
	server_ini.format.blueMax = Swap16IfLE(server_ini.format.blueMax);

	CARD32 nNameLength = (CARD32)strlen(desktopname);

	server_ini.nameLength = Swap32IfLE(nNameLength);

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	if (!m_socket->SendExactQueue((char*)&server_ini, sizeof(server_ini)))
	{
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}
	if (!m_socket->SendExact(desktopname, nNameLength))
	{
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}
	vnclog.Print(LL_INTINFO, VNCLOG("sent pixel format to client\n"));

	// UNLOCK INITIAL SETUP
	// Initial negotiation is complete, so set the protocol ready flag
	m_client->EnableProtocol();

	// Add a fullscreen update to the client's update list
	// sf@2002 - Scaling
	// m_client->m_update_tracker.add_changed(m_client->m_fullscreen);
	{ // RealVNC 336
		omni_mutex_lock l(m_client->GetUpdateLock(), 91);
		m_client->m_update_tracker.add_changed(m_client->m_ScaledScreen);
	}

	// MAIN LOOP
#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText, " MAIN LOOP \n");
	OutputDebugString(szText);
#endif
	// Set the input thread to a high priority
	set_priority(omni_thread::PRIORITY_HIGH);

	m_client->cl_connected = TRUE;
	// added jeff
	BOOL need_to_disable_input = settings->getDisableLocalInputs();
	bool need_to_clear_keyboard = true;
	bool need_first_keepalive = false;
	bool need_keepalive = false;
	bool need_first_idletime = false;
	bool need_monitor_info = false;
	bool firstrun = true;
	bool need_ft_version_msg = false;
	// adzm - 2010-07 - Extended clipboard
	bool need_notify_extended_clipboard = false;
	// adzm 2010-09 - Notify streaming DSM plugin support
	bool need_notify_streaming_DSM = false;

	while (m_client->cl_connected)
	{
		rfbClientToServerMsg msg{};

		// Ensure that we're running in the correct desktop
		if (!m_client->IsFileTransBusy())
			// This desktop switch is responsible for the keyboard input
			if (desktopSelector::InputDesktopSelected() == 0)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("vncClientThread \n"));
				if (!desktopSelector::SelectDesktop(NULL, &input_desktop))
					break;
			}
		// added jeff
		// 2 May 2008 jdp paquette@atnetsend.net moved so that we're on the right desktop  when we're a service
		// Clear the CapsLock and NumLock keys
		if (m_client->m_keyboardenabled && need_to_clear_keyboard)
		{
			ClearKeyState(VK_CAPITAL);
			// *** JNW - removed because people complain it's wrong
			//ClearKeyState(VK_NUMLOCK);
			ClearKeyState(VK_SCROLL);
			need_to_clear_keyboard = false;
		}
		//
		if (need_to_disable_input)
		{
			// have to do this here if we're a service so that we're on the correct desktop
			m_client->m_encodemgr.m_buffer->m_desktop->SetDisableInput();
			need_to_disable_input = false;
		}

		// reclaim input block after local C+A+D if user currently has it blocked
		m_client->m_encodemgr.m_buffer->m_desktop->block_input(firstrun);
		firstrun = false;

		if (need_first_keepalive)
		{
			// send first keepalive to let the client know we accepted the encoding request
			m_client->SendServerStateUpdate(rfbKeepAliveInterval, settings->getkeepAliveInterval());
			m_client->SendKeepAlive(true);
			need_first_keepalive = false;
		}
		if (need_keepalive) {
			m_client->SendKeepAlive(true);
			need_keepalive = false;
		}

		if (need_first_idletime)
		{
			// send idletime to let the client know we accepted the encoding request
			m_client->SendServerStateUpdate(rfbIdleInputTimeout, settings->getIdleInputTimeout());
			need_first_idletime = false;
		}

		if (need_monitor_info)
		{
			// send idletime to let the client know we accepted the encoding request
			m_client->SendMonitorInfo();
			need_monitor_info = false;
		}

		if (m_client->m_want_update_state && m_client->m_Support_rfbSetServerInput)
		{
			m_client->m_want_update_state = false;
			m_client->SendServerStateUpdate(m_client->m_state, m_client->m_value);
#ifdef _DEBUG
			char			szText[256];
			sprintf_s(szText, "SendServerStateUpdate %i %i  \n", m_client->m_state, m_client->m_value);
			OutputDebugString(szText);
#endif
		}
		if (need_ft_version_msg)
		{
			// send a File Transfer Protocol message to client.
			m_client->SendFTProtocolMsg();
			need_ft_version_msg = false;
		}
		// adzm - 2010-07 - Extended clipboard
		if (need_notify_extended_clipboard)
		{
			m_client->NotifyExtendedClipboardSupport();
			need_notify_extended_clipboard = false;
		}
		// adzm 2010-09 - Notify streaming DSM plugin support
		if (need_notify_streaming_DSM)
		{
			m_client->NotifyPluginStreamingSupport();
			need_notify_streaming_DSM = false;
		}
		// sf@2002 - v1.1.2
		int nTO = 1; // Type offset
		// If DSM Plugin, we must read all the transformed incoming rfb messages (type included)
		// adzm 2010-09
		if (!m_socket->IsPluginStreamingIn() && m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled())
		{
			if (!m_socket->ReadExact((char*)&msg.type, sizeof(msg.type)))
			{
				m_client->cl_connected = FALSE;
				break;
			}
			nTO = 0;
		}
		else
		{
			// Try to read a message ID
			if (!m_socket->ReadExact((char*)&msg.type, sizeof(msg.type)))
			{
				m_client->cl_connected = FALSE;
				break;
			}
		}
#ifdef _DEBUG
		char			szText[256];
		sprintf_s(szText, " msg.type %i \n", msg.type);
		OutputDebugString(szText);
#endif
		// What to do is determined by the message id
		switch (msg.type)
		{
		case rfbKeepAlive:
			need_keepalive = true;
			break;

		case rfbSetPixelFormat:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbSetPixelFormatMsg - nTO))
			{
				m_client->cl_connected = FALSE;
				break;
			}

			// Swap the relevant bits.
			msg.spf.format.redMax = Swap16IfLE(msg.spf.format.redMax);
			msg.spf.format.greenMax = Swap16IfLE(msg.spf.format.greenMax);
			msg.spf.format.blueMax = Swap16IfLE(msg.spf.format.blueMax);

			// sf@2005 - Additional param for Grey Scale transformation
			m_client->m_encodemgr.EnableGreyPalette((msg.spf.format.pad1 == 1));

			// Prevent updates while the pixel format is changed
			m_client->DisableProtocol();

			// Tell the buffer object of the change
			if (!m_client->m_encodemgr.SetClientFormat(msg.spf.format))
			{
				vnclog.Print(LL_CONNERR, VNCLOG("remote pixel format invalid\n"));

				m_client->cl_connected = FALSE;
			}

			// Set the palette-changed flag, just in case...
			m_client->m_palettechanged = TRUE;

			// Re-enable updates
			m_client->EnableProtocol();

			break;

		case rfbSetEncodings:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbSetEncodingsMsg - nTO))
			{
				m_client->cl_connected = FALSE;
				break;
			}

			// RDV cache
			m_client->m_encodemgr.EnableCache(FALSE);

			// RDV XOR and client detection
			m_client->m_encodemgr.AvailableQueueEnabled(FALSE);
			m_client->m_encodemgr.AvailableZRLE(FALSE);
#ifdef _XZ
			m_client->m_encodemgr.AvailableXZ(FALSE);
#endif
			m_client->m_encodemgr.AvailableTight(FALSE);

			// sf@2002 - Tight
			m_client->m_encodemgr.SetQualityLevel(-1);
			m_client->m_encodemgr.SetFineQualityLevel(-1);
			m_client->m_encodemgr.SetCompressLevel(6);
			m_client->m_encodemgr.EnableLastRect(FALSE);

			// Tight - CURSOR HANDLING
			m_client->m_encodemgr.EnableXCursor(FALSE);
			m_client->m_encodemgr.EnableRichCursor(FALSE);
			m_client->m_use_PointerPos = FALSE;
			m_server->EnableXRichCursor(FALSE);
			m_client->m_cursor_update_pending = FALSE;
			m_client->m_cursor_update_sent = FALSE;

			// Prevent updates while the encoder is changed
			m_client->DisableProtocol();

			// Read in the preferred encodings
			msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);
			{
				int x;
				BOOL encoding_set = FALSE;
#ifdef _Gii
				BOOL gii_set = FALSE;
#endif
				// By default, don't use copyrect!
				m_client->m_update_tracker.enable_copyrect(false);

				for (x = 0; x < msg.se.nEncodings; x++)
				{
					CARD32 encoding = 0;

					// Read an encoding in
					if (!m_socket->ReadExact((char*)&encoding, sizeof(encoding)))
					{
						m_client->cl_connected = FALSE;
						break;
					}

					// Is this the CopyRect encoding (a special case)?
					if (Swap32IfLE(encoding) == rfbEncodingCopyRect)
					{
						m_client->m_update_tracker.enable_copyrect(true);
						continue;
					}

					// Is this a NewFBSize encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingNewFBSize) {
						m_client->m_use_NewSWSize = TRUE;
						continue;
					}

					if (Swap32IfLE(encoding) == rfbEncodingExtDesktopSize) {
						// only allow desktop resize when to the first client
						m_client->m_use_ExtDesktopSize = true;
						continue;
					}

					// CACHE RDV
					if (Swap32IfLE(encoding) == rfbEncodingCacheEnable)
					{
						m_client->m_encodemgr.EnableCache(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("Cache protocol extension enabled\n"));
						continue;
					}

					// XOR zlib
					if (Swap32IfLE(encoding) == rfbEncodingQueueEnable) {
						m_client->m_encodemgr.AvailableQueueEnabled(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("XOR protocol extension enabled\n"));
						continue;
					}

					// Is this a CompressLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingCompressLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingCompressLevel9))
					{
						// Client specified encoding-specific compression level
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingCompressLevel0);
						m_client->m_encodemgr.SetCompressLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("compression level requested: %d\n"), level);
						continue;
					}

					// Is this a QualityLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingQualityLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingQualityLevel9))
					{
						// Client specified image quality level used for JPEG compression
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingQualityLevel0);
						m_client->m_encodemgr.SetQualityLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("image quality level requested: %d\n"), level);
						continue;
					}

					// Is this a FineQualityLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingFineQualityLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingFineQualityLevel100))
					{
						// Client specified fine-grained image quality level used for JPEG compression
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingFineQualityLevel0);
						m_client->m_encodemgr.SetFineQualityLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("fine-grained image quality level requested: %d\n"), level);
						continue;
					}

					// Is this a Subsamp encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingSubsamp1X) &&
						(Swap32IfLE(encoding) <= rfbEncodingSubsamp16X))
					{
						// Client specified subsampling used for JPEG compression
						int subsamp = (int)(Swap32IfLE(encoding) - rfbEncodingSubsamp1X);
						m_client->m_encodemgr.SetSubsampling((subsamp_type)subsamp);
						vnclog.Print(LL_INTINFO, VNCLOG("subsampling requested: %d\n"), subsamp);
						continue;
					}

					// Is this a LastRect encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingLastRect) {
						m_client->m_encodemgr.EnableLastRect(TRUE); // We forbid Last Rect for now
						vnclog.Print(LL_INTINFO, VNCLOG("LastRect protocol extension enabled\n"));
						continue;
					}

					// Is this an XCursor encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingXCursor) {
						m_client->m_encodemgr.EnableXCursor(TRUE);
						m_server->EnableXRichCursor(TRUE);
						m_client->m_encodemgr.m_buffer->m_desktop->requestMouseShapeUpdate();
						vnclog.Print(LL_INTINFO, VNCLOG("X-style cursor shape updates enabled\n"));
						continue;
					}

					// Is this a RichCursor encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingRichCursor) {
						m_client->m_encodemgr.EnableRichCursor(TRUE);
						m_server->EnableXRichCursor(TRUE);
						m_client->m_encodemgr.m_buffer->m_desktop->requestMouseShapeUpdate();
						vnclog.Print(LL_INTINFO, VNCLOG("Full-color cursor shape updates enabled\n"));
						continue;
					}

					// Is this a PointerPos encoding request? nyama/marscha - PointerPos
					if (Swap32IfLE(encoding) == rfbEncodingPointerPos) {
						m_client->m_use_PointerPos = TRUE;
						vnclog.Print(LL_INTINFO, VNCLOG("PointerPos protocol extension enabled\n"));
						continue;
					}
					// 21 March 2008 jdp - client wants server state updates
					if (Swap32IfLE(encoding) == rfbEncodingServerState) {
						m_client->m_wants_ServerStateUpdates = true;
						settings->EnableServerStateUpdates(true);
						vnclog.Print(LL_INTINFO, VNCLOG("ServerState protocol extension enabled\n"));
						continue;
					}
					// 21 March 2008 jdp  - client wants keepalive messages
					if (Swap32IfLE(encoding) == rfbEncodingpseudoSession) {
						m_client->m_session_supported = true;
						vnclog.Print(LL_INTINFO, VNCLOG("KeepAlive protocol extension enabled\n"));
						continue;
					}

					if (Swap32IfLE(encoding) == rfbEncodingEnableKeepAlive) {
						m_client->m_wants_KeepAlive = true;
						settings->EnableKeepAlives(true);
						need_first_keepalive = true;
						vnclog.Print(LL_INTINFO, VNCLOG("KeepAlive protocol extension enabled\n"));
						continue;
					}

					if (Swap32IfLE(encoding) == rfbEncodingEnableIdleTime) {
						need_first_idletime = true;
						vnclog.Print(LL_INTINFO, VNCLOG("IdleTime protocol extension enabled\n"));
						continue;
					}

					if (Swap32IfLE(encoding) == rfbEncodingMonitorInfo) {
						need_monitor_info = true;
						vnclog.Print(LL_INTINFO, VNCLOG("IdleTime protocol extension enabled\n"));
						continue;
					}

					if (Swap32IfLE(encoding) == rfbEncodingFTProtocolVersion) {
						need_ft_version_msg = true;
						vnclog.Print(LL_INTINFO, VNCLOG("FTProtocolVersion protocol extension enabled\n"));
						continue;
					}

					// adzm - 2010-07 - Extended clipboard
					if (Swap32IfLE(encoding) == rfbEncodingExtendedClipboard) {
						need_notify_extended_clipboard = true;
						m_client->m_clipboard.settings.m_bSupportsEx = true;
						vnclog.Print(LL_INTINFO, VNCLOG("Extended clipboard protocol extension enabled\n"));
						continue;
					}

					// adzm 2010-09 - Notify streaming DSM plugin support
					if (Swap32IfLE(encoding) == rfbEncodingPluginStreaming) {
						need_notify_streaming_DSM = true;
						vnclog.Print(LL_INTINFO, VNCLOG("Streaming DSM support enabled\n"));
						continue;
					}

#ifdef _Gii
					if (Swap32IfLE(encoding) == rfbEncodingGII) {
						vnclog.Print(LL_INTINFO, VNCLOG("Gii Encoding found\n"));
						gii_set = TRUE;
						continue;
					}
#endif

					// RDV - We try to detect which type of viewer tries to connect
					if (Swap32IfLE(encoding) == rfbEncodingZRLE) {
						m_client->m_encodemgr.AvailableZRLE(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("ZRLE found \n"));
					}
#ifdef _XZ
					if (Swap32IfLE(encoding) == rfbEncodingXZ) {
						m_client->m_encodemgr.AvailableXZ(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("XZ found \n"));
					}
#endif

					if (Swap32IfLE(encoding) == rfbEncodingTight) {
						m_client->m_encodemgr.AvailableTight(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("Tight found\n"));
					}

					// Have we already found a suitable encoding?
					if (!encoding_set)
					{
						// No, so try the buffer to see if this encoding will work...
						omni_mutex_lock l(m_client->GetUpdateLock(), 85);
						if (m_client->m_encodemgr.SetEncoding(Swap32IfLE(encoding), FALSE))
							encoding_set = TRUE;
					}
				}
				// If no encoding worked then default to RAW!
				if (!encoding_set)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("defaulting to raw encoder\n"));
					omni_mutex_lock l(m_client->GetUpdateLock(), 86);
					if (!m_client->m_encodemgr.SetEncoding(Swap32IfLE(rfbEncodingRaw), FALSE))
					{
						vnclog.Print(LL_INTERR, VNCLOG("failed to select raw encoder!\n"));

						m_client->cl_connected = FALSE;
					}
				}

				// If PointerPos not supported use framebuffr drawing
				if (!m_client->m_use_PointerPos) {
					m_client->m_encodemgr.EnableXCursor(FALSE);
					m_client->m_encodemgr.EnableRichCursor(FALSE);
					m_client->m_use_PointerPos = FALSE;
					m_server->EnableXRichCursor(FALSE);
					m_client->m_cursor_update_pending = FALSE;
					m_client->m_cursor_update_sent = FALSE;
				}

				// sf@2002 - For now we disable cache protocol when more than one client are connected
				// (But the cache buffer (if exists) is kept intact (for XORZlib usage))
				if (m_server->AuthClientCount() > 1)
					m_server->DisableCacheForAllClients();
#ifdef _Gii
				// Gii encoding requested (MC Multitouch Extensions)
				if (gii_set && InjectTouchInputUVNC != NULL && InitializeTouchInjectionUVNC != NULL && m_client->m_GiiEnabled)
				{
					InitGiiVersion();
				}
#endif
			}

			// Re-enable updates
			m_client->client_settings_passed = true;
			m_client->EnableProtocol();
			break;

#ifdef _Gii
		case rfbGIIMessage:
		{
			// Read the subtype and length of the message:
			uint8_t subType = 0;
			uint16_t lengthMsg = 0;
			bool bigEndian = false;
			omni_mutex_lock ll(m_client->GetUpdateLock(), 776);
			if (!m_socket->ReadExact((char*)&subType, sizeof(subType)))
			{
				vnclog.Print(LL_INTERR, VNCLOG("GII:Extension Reading subtype failed !\n"));
				m_client->cl_connected = FALSE;
				break;
			}
			if (!m_socket->ReadExact((char*)&lengthMsg, sizeof(lengthMsg)))
			{
				vnclog.Print(LL_INTERR, VNCLOG("GII:Extension Reading length failed !\n"));
				m_client->cl_connected = FALSE;
				break;
			}
			//unmask big endian flag
			if (subType & 0x80)
			{
				bigEndian = true;
				//format is big endian, convert to multi byte data to little
				lengthMsg = Swap16IfLE(lengthMsg);
				//mask subtype
				subType &= 0x7f;
			}
			// process Gii Message
			switch (subType) {
			case _rfbGIIEventInjection:
			{
				if (!m_client->m_GiiEnabled) {
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension not supported in readonly mode\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				rfbGIIValuatorEventMsg rfbGIIValutorEvent{};
				rfbGIIValutorEvent.header.messageType = rfbGIIMessage;
				rfbGIIValutorEvent.header.subType = subType;
				rfbGIIValutorEvent.header.length = lengthMsg;
				// read rest of client's inject data
				if (!m_socket->ReadExact((char*)(&rfbGIIValutorEvent) + sz_rfbGIIMsgHeader, sizeof(rfbGIIValutorEvent) - sz_rfbGIIMsgHeader))
				{
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading client creation failed\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				if (bigEndian)
				{
					rfbGIIValutorEvent.padding = Swap16IfLE(rfbGIIValutorEvent.padding);
					rfbGIIValutorEvent.deviceOrigin = Swap32IfLE(rfbGIIValutorEvent.deviceOrigin);
					rfbGIIValutorEvent.first = Swap32IfLE(rfbGIIValutorEvent.first);
					rfbGIIValutorEvent.count = Swap32IfLE(rfbGIIValutorEvent.count);
				}
				/////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////////////////////////////////////////////////////////////
#ifdef _USE_DLL
				if (DLL_PInjectTouch)
#endif
				{
					MyTouchINfo* TI = NULL;
					TI = new MyTouchINfo[rfbGIIValutorEvent.first];
					for (unsigned int j = 0; j < rfbGIIValutorEvent.first; j++)
					{
						DWORD ValuatorFlag = 0;
						DWORD ValuatorTouch_Id = 0;
						DWORD ValuatorTouch_Position16XY = 0;
						DWORD ValuatorTouch_Area = 0;
						DWORD ValuatorTouch_Pressure = 0;
						DWORD ValuatorTime_msec = 0;
						DWORD64 ValuatorTime_usec = 0;

						if (!m_socket->ReadExact((char*)&ValuatorFlag, sizeof(DWORD))) {
							vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
							m_client->cl_connected = FALSE;
							break;
						}
						if (bigEndian) ValuatorFlag = Swap32IfLE(ValuatorFlag);

						if (!m_socket->ReadExact((char*)&ValuatorTouch_Id, sizeof(DWORD))) {
							vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
							m_client->cl_connected = FALSE;
							break;
						}
						if (bigEndian) ValuatorTouch_Id = Swap32IfLE(ValuatorTouch_Id);
						TI[j].TouchId = ValuatorTouch_Id;

						if (!m_socket->ReadExact((char*)&ValuatorTouch_Position16XY, sizeof(DWORD))) {
							vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
							m_client->cl_connected = FALSE;
							break;
						}
						if (bigEndian) ValuatorTouch_Position16XY = Swap32IfLE(ValuatorTouch_Position16XY);
						if (ValuatorFlag & LANGE_16_flag)
						{
							TI[j].X = ((ValuatorTouch_Position16XY & 0xffff0000) >> 16);
							TI[j].Y = ((ValuatorTouch_Position16XY & 0x0000ffff));
						}
						else
						{
							vnclog.Print(LL_INTERR, VNCLOG("Only 16-bit XT supported\n"));
							m_client->cl_connected = FALSE;
							break;
						}

						if (ValuatorFlag & S1_flag)
						{
							if (!m_socket->ReadExact((char*)&ValuatorTouch_Area, sizeof(DWORD))) {
								vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
								m_client->cl_connected = FALSE;
								break;
							}
							if (bigEndian) ValuatorTouch_Area = Swap32IfLE(ValuatorTouch_Area);
							TI[j].ContactWidth = ((ValuatorTouch_Area & 0xffff0000) >> 16);
							TI[j].ContactHeight = ((ValuatorTouch_Area & 0x0000ffff));
						}
						else
						{
							TI[j].ContactWidth = 4;
							TI[j].ContactHeight = 4;
						}

						if (ValuatorFlag & PR_flag)
							if (!m_socket->ReadExact((char*)&ValuatorTouch_Pressure, sizeof(DWORD))) {
								vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
								m_client->cl_connected = FALSE;
								break;
							}
						if (bigEndian) ValuatorTouch_Pressure = Swap32IfLE(ValuatorTouch_Pressure);

						if (ValuatorFlag & TI_flag)
							if (!m_socket->ReadExact((char*)&ValuatorTime_msec, sizeof(DWORD))) {
								vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
								m_client->cl_connected = FALSE;
								break;
							}
						if (bigEndian) ValuatorTime_msec = Swap32IfLE(ValuatorTime_msec);

						if (ValuatorFlag & HC_flag)
							if (!m_socket->ReadExact((char*)&ValuatorTime_usec, sizeof(DWORD64))) {
								vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading TouchINfo failed\n"));
								m_client->cl_connected = FALSE;
								break;
							}
						int endianTest = 1;
						if (bigEndian) ValuatorTime_usec = Swap64IfLE(ValuatorTime_usec);

						TI[j].pointerflag = POINTER_FLAG_NONE;

						if (TI[j].TouchId < nr_points)  // protect point_status array, else wronf value crash
						{
							if (ValuatorFlag & PF_flag)
							{
								if (point_status[TI[j].TouchId] == 1)
								{
									//move
									TI[j].pointerflag |= POINTER_FLAG_UPDATE;
									TI[j].pointerflag |= POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
								}
								else
								{
									//down
									TI[j].pointerflag |= POINTER_FLAG_DOWN;
									TI[j].pointerflag |= POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
									point_status[TI[j].TouchId] = 1;
								}
							}
							else
							{
								// UP
								TI[j].pointerflag |= POINTER_FLAG_UP;
								point_status[TI[j].TouchId] = 0;
							}
						}

						TI[j].time = 0;
					}
#ifdef _USE_DLL
					bRet = DLL_PInjectTouch(rfbGIIValutorEvent.first, TI);
#ifdef _DEBUG
					if (bRet == 0) sprintf_s(szText, "FAIL index %d %i\n", GetLastError(), rfbGIIValutorEvent.first);
					else sprintf_s(szText, "OK %i\n", rfbGIIValutorEvent.first);
					OutputDebugString(szText);
#endif
#else
					//////////////////////////////////////////////////
					// Copy src from dll
					//////////////////////////////////////////////////
					MyTouchINfo* ti_array = TI;
					BOOL value = FALSE;
					POINTER_TOUCH_INFO *contact = NULL;
					if (rfbGIIValutorEvent.first < 0 || rfbGIIValutorEvent.first >254) goto mydllend;
					contact = new POINTER_TOUCH_INFO[rfbGIIValutorEvent.first];
					if (contact == NULL)  goto mydllend;
					memset(contact, 0, sizeof(POINTER_TOUCH_INFO) * rfbGIIValutorEvent.first);

					for (unsigned int i = 0; i < rfbGIIValutorEvent.first; i++)
					{
						contact[i].pointerInfo.pointerType = PT_TOUCH; //we're sending touch input
						contact[i].pointerInfo.ptPixelLocation.x = ti_array[i].X;
						contact[i].pointerInfo.ptPixelLocation.y = ti_array[i].Y;

						contact[i].pointerInfo.dwTime = ti_array[i].time;

						contact[i].touchFlags = TOUCH_FLAG_NONE;
						contact[i].touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
						contact[i].orientation = 90;
						contact[i].pressure = 32000;

						contact[i].rcContact.top = contact[i].pointerInfo.ptPixelLocation.y - ti_array[i].ContactHeight / 2;
						contact[i].rcContact.bottom = contact[i].pointerInfo.ptPixelLocation.y + ti_array[i].ContactHeight / 2;
						contact[i].rcContact.left = contact[i].pointerInfo.ptPixelLocation.x - ti_array[i].ContactWidth / 2;
						contact[i].rcContact.right = contact[i].pointerInfo.ptPixelLocation.x + ti_array[i].ContactWidth / 2;

						contact[i].pointerInfo.pointerId = ti_array[i].TouchId;          //contact 0
						contact[i].pointerInfo.pointerFlags = ti_array[i].pointerflag;
					}
					value = InjectTouchInputUVNC(rfbGIIValutorEvent.first, contact);
#ifdef _DEBUG
					if (value == 0) sprintf_s(szText, "FAIL index %d %i\n", GetLastError(), rfbGIIValutorEvent.first);
					else sprintf_s(szText, "OK number points %i\n", rfbGIIValutorEvent.first);
					OutputDebugString(szText);
#endif
					delete[]contact;
				mydllend:
					//////////////////////////////////////////////////
#endif
					if (TI) delete[]TI;
				}
				/////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////////////////////////////////////////////////////////////
			}
			break;

			case _rfbGIIVersionMessage:
			{
				if (!m_client->m_GiiEnabled) {
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension not supported in readonly mode\n"));
					m_client->cl_connected = FALSE;
					return;
				}
				uint16_t clientVersion = 0;
				//check length
				vnclog.Print(LL_INTINFO, VNCLOG("GII: Get Subtype _rfbGIIVersionMessage \n"));
				if (lengthMsg != sizeof(clientVersion))
				{
					vnclog.Print(LL_INTERR, VNCLOG("GII:Extension invalid length: %d\n"), clientVersion);
					m_client->cl_connected = FALSE;
					break;
				}
				// read client's version info and process
				if (!m_socket->ReadExact((char*)&clientVersion, sizeof(clientVersion)))
				{
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading client version failed\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				if (bigEndian)
				{
					clientVersion = Swap16IfLE(clientVersion);
				}
				if ((clientVersion < rfbGIIMinVersion || clientVersion > rfbGIIMaxVersion))           //TBR
				{
					vnclog.Print(LL_INTERR, VNCLOG("GII: Extension client version invalid number: %d\n"), clientVersion);
					m_client->cl_connected = FALSE;
					break;
				}
				break;
			}

			case _rfbGIIDeviceCreation:
			{
				if (!m_client->m_GiiEnabled) {
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension not supported in readonly mode\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				rfbGIIClientDeviceCreationMsg rfbGIIClientDeviceCreation{};
				//prepare already known data
				omni_mutex_lock ll(m_client->GetUpdateLock(), 777);
				vnclog.Print(LL_INTINFO, VNCLOG("GII: Get Subtype _rfbGIIDeviceCreation \n"));
				rfbGIIClientDeviceCreation.header.messageType = rfbGIIMessage;
				rfbGIIClientDeviceCreation.header.subType = subType;
				rfbGIIClientDeviceCreation.header.length = lengthMsg;
				// read rest of client's creation data
				if (!m_socket->ReadExact((char*)(&rfbGIIClientDeviceCreation) + sz_rfbGIIMsgHeader, sizeof(rfbGIIClientDeviceCreation) - sz_rfbGIIMsgHeader))
				{
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading client creation failed\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				if (bigEndian)
				{
					rfbGIIClientDeviceCreation.vendorID = Swap32IfLE(rfbGIIClientDeviceCreation.vendorID);
					rfbGIIClientDeviceCreation.productID = Swap32IfLE(rfbGIIClientDeviceCreation.productID);
					rfbGIIClientDeviceCreation.eventMask = Swap32IfLE(rfbGIIClientDeviceCreation.eventMask);
					rfbGIIClientDeviceCreation.numRegisters = Swap32IfLE(rfbGIIClientDeviceCreation.numRegisters);
					rfbGIIClientDeviceCreation.numValuators = Swap32IfLE(rfbGIIClientDeviceCreation.numValuators);
					rfbGIIClientDeviceCreation.numButtons = Swap32IfLE(rfbGIIClientDeviceCreation.numButtons);                                                                                                         rfbGIIClientDeviceCreation.vendorID = Swap32IfLE(rfbGIIClientDeviceCreation.vendorID);
				}
				if (rfbGIIClientDeviceCreation.numValuators == 0 || rfbGIIClientDeviceCreation.numValuators > 10)
				{
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading device creation, invalid amount of valuators\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				else
				{
					//read all valutor
					rfbGIIValuatorMsg rfbGIIValutor{};
					for (unsigned int i = 0; i < rfbGIIClientDeviceCreation.numValuators; i++)
					{
						if (!m_socket->ReadExact((char*)&rfbGIIValutor, sizeof(rfbGIIValutor)))
						{
							vnclog.Print(LL_INTERR, VNCLOG("GIIExtension reading device creation valuators failed\n"));
							m_client->cl_connected = FALSE;
							break;
						}
						else
						{
							if (bigEndian)
							{
								rfbGIIValutor.index = Swap32IfLE(rfbGIIValutor.index);
								rfbGIIValutor.rangeMin = Swap32IfLE(rfbGIIValutor.rangeMin);
								rfbGIIValutor.rangeCenter = Swap32IfLE(rfbGIIValutor.rangeCenter);
								rfbGIIValutor.rangeMax = Swap32IfLE(rfbGIIValutor.rangeMax);
								rfbGIIValutor.SIUnit = Swap32IfLE(rfbGIIValutor.SIUnit);
								rfbGIIValutor.SIAdd = Swap32IfLE(rfbGIIValutor.SIAdd);
								rfbGIIValutor.SIMul = Swap32IfLE(rfbGIIValutor.SIMul);
								rfbGIIValutor.SIDiv = Swap32IfLE(rfbGIIValutor.SIDiv);
								rfbGIIValutor.SIShift = Swap32IfLE(rfbGIIValutor.SIShift);
							}
							//send device origin //TBR
							rfbGIIServerDeviceCreationMsg rfbGIIServerDeviceCreation{};
							// Generate the server's creation message
							rfbGIIServerDeviceCreation.header.messageType = rfbGIIMessage;
							rfbGIIServerDeviceCreation.header.subType = rfbGIIDeviceCreation;
							//rfbGIIServerDeviceCreation.header.subType =  2; //no use of little endian
							rfbGIIServerDeviceCreation.header.length = 4;
							rfbGIIServerDeviceCreation.header.length = Swap16IfLE(rfbGIIServerDeviceCreation.header.length);
							rfbGIIServerDeviceCreation.deviceOrigin = 1;       //TBR
							rfbGIIServerDeviceCreation.deviceOrigin = Swap32IfLE(rfbGIIServerDeviceCreation.deviceOrigin);
							// Send our protocol version, and get the client's protocol version

							vnclog.Print(LL_INTERR, VNCLOG("GIIExtension writing device creation\n"));
							if (!m_socket->SendExactQueue((char*)&rfbGIIServerDeviceCreation, sz_rfbGIIServerDeviceCreationMsg))
							{
								vnclog.Print(LL_INTERR, VNCLOG("GIIExtension writing device creation failed\n"));
								m_client->cl_connected = FALSE;
								break;
							}

#ifdef _USE_DLL
							if (DLL_InitializeTouchInjection) DLL_InitializeTouchInjection(rfbGIIClientDeviceCreation.numButtons);
#else
							InitializeTouchInjectionUVNC(rfbGIIClientDeviceCreation.numButtons, TOUCH_FEEDBACK_DEFAULT);
#endif
							nr_points = rfbGIIClientDeviceCreation.numButtons;
							if (nr_points > 0 && nr_points < 254) point_status = new bool[nr_points];
						}
					}
				}
				break;
			}

			case _rfbGIIDeviceDestruction:
				if (!m_client->m_GiiEnabled) {
					vnclog.Print(LL_INTERR, VNCLOG("GIIExtension not supported in readonly mode\n"));
					m_client->cl_connected = FALSE;
					break;
				}
				//not yet implemented
				vnclog.Print(LL_INTERR, VNCLOG("GIIExtension writing device creation failed\n"));
				m_client->cl_connected = FALSE;
				break;

			default:
				vnclog.Print(LL_INTERR, VNCLOG("GII: Extension invalid subtype: %d\n"), subType);
				m_client->cl_connected = FALSE;
				break;
			}
		}
		break;
#endif

		case rfbFramebufferUpdateRequest:
			if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbFramebufferUpdateRequestMsg - nTO))
			{
				m_client->cl_connected = FALSE;
				break;
			}

			if (!m_client->NotifyUpdate(msg.fur))
				m_client->cl_connected = FALSE;
			break;

		case rfbKeyEvent:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbKeyEventMsg - nTO))
			{
				if (PreConnect)
				{
					msg.ke.key = Swap32IfLE(msg.ke.key);
					if (msg.ke.down != 0)
					{
						int index = msg.ke.key - 97;
						HANDLE		hprconnectevent = NULL;
						if (index > -1 && index < 100)
						{
							PreConnectID = m_client->m_encodemgr.m_buffer->m_desktop->sesmsg[index].ID;
							hprconnectevent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraPreConnect");
							HANDLE m_hFileMa = NULL;
							m_hFileMa = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, "Global\\SessionUltraPreConnect");
							PVOID data = NULL;
							if (m_hFileMa) data = MapViewOfFile(m_hFileMa, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
							int* mydata = (int*)data;
							if (data) *mydata = PreConnectID;
							if (data) UnmapViewOfFile(data);
							if (m_hFileMa != NULL) CloseHandle(m_hFileMa);
							if (hprconnectevent) SetEvent(hprconnectevent);
							if (hprconnectevent) CloseHandle(hprconnectevent);
						}
					}
				}
				else
				{
					if (m_client->m_keyboardenabled)
					{
						msg.ke.key = Swap32IfLE(msg.ke.key);

						// Get the keymapper to do the work
						// m_client->m_keymap.DoXkeysym(msg.ke.key, msg.ke.down);
						vncKeymap::keyEvent(msg.ke.key, (0 != msg.ke.down), m_client->m_jap, m_client->m_unicode);

						m_client->m_remoteevent = TRUE;
					}
				}
			}
			m_client->TriggerUpdate();
			break;

		case rfbPointerEvent:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbPointerEventMsg - nTO))
			{
				if (PreConnect) break;
				if (m_client->m_pointerenabled)
				{
					// Convert the coords to Big Endian
					// Modif sf@2002 - Scaling
					msg.pe.x = (Swap16IfLE(msg.pe.x));
					msg.pe.y = (Swap16IfLE(msg.pe.y));
					//Error, msd.pe.x is defined as unsigned while with a negative secondary screen it's negative
					//offset need to be used later in this function
					msg.pe.x = (msg.pe.x) * m_client->m_nScale;
					msg.pe.y = (msg.pe.y) * m_client->m_nScale;

					// Work out the flags for this event
					DWORD flags = MOUSEEVENTF_ABSOLUTE;

					if (msg.pe.x != m_client->m_ptrevent.x ||
						msg.pe.y != m_client->m_ptrevent.y)
						flags |= MOUSEEVENTF_MOVE;
					if ((msg.pe.buttonMask & rfbButton1Mask) !=
						(m_client->m_ptrevent.buttonMask & rfbButton1Mask))
					{
						if (GetSystemMetrics(SM_SWAPBUTTON))
							flags |= (msg.pe.buttonMask & rfbButton1Mask)
							? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
						else
							flags |= (msg.pe.buttonMask & rfbButton1Mask)
							? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
					}
					if ((msg.pe.buttonMask & rfbButton2Mask) !=
						(m_client->m_ptrevent.buttonMask & rfbButton2Mask))
					{
						flags |= (msg.pe.buttonMask & rfbButton2Mask)
							? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
					}
					if ((msg.pe.buttonMask & rfbButton3Mask) !=
						(m_client->m_ptrevent.buttonMask & rfbButton3Mask))
					{
						if (GetSystemMetrics(SM_SWAPBUTTON))
							flags |= (msg.pe.buttonMask & rfbButton3Mask)
							? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
						else
							flags |= (msg.pe.buttonMask & rfbButton3Mask)
							? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
					}

					if (settings->getCollabo()) {
						if ((msg.pe.buttonMask & rfbButton1Mask) != (m_client->m_ptrevent.buttonMask & rfbButton1Mask) ||
							(msg.pe.buttonMask & rfbButton2Mask) != (m_client->m_ptrevent.buttonMask & rfbButton2Mask) ||
							(msg.pe.buttonMask & rfbButton1Mask) != (m_client->m_ptrevent.buttonMask & rfbButton1Mask)) {
							if (!m_client->has_mouse) {
								m_client->ask_mouse = true;
								m_server->SetHasMouse();
							}
						}

						if (!m_client->has_mouse) {
							int xx = msg.pe.x - GetSystemMetrics(SM_XVIRTUALSCREEN) + (m_client->monitor_Offsetx + m_client->m_ScreenOffsetx);
							int yy = msg.pe.y - GetSystemMetrics(SM_YVIRTUALSCREEN) + (m_client->monitor_Offsety + m_client->m_ScreenOffsety);
							if (settings->getDriver()) {
								xx = msg.pe.x + m_client->monitor_Offsetx;
								yy = msg.pe.y + m_client->monitor_Offsety;
							}
							if (m_client->simulateCursor)
								m_client->simulateCursor->moveCursor(xx, yy);
							break;
						}
					}

					// Treat buttons 4 and 5 presses as mouse wheel events
					DWORD wheel_movement = 0;
					if (m_client->m_encodemgr.IsMouseWheelTight())
					{
						if ((msg.pe.buttonMask & rfbButton4Mask) != 0 &&
							(m_client->m_ptrevent.buttonMask & rfbButton4Mask) == 0)
						{
							flags |= MOUSEEVENTF_WHEEL;
							wheel_movement = (DWORD)+120;
						}
						else if ((msg.pe.buttonMask & rfbButton5Mask) != 0 &&
							(m_client->m_ptrevent.buttonMask & rfbButton5Mask) == 0)
						{
							flags |= MOUSEEVENTF_WHEEL;
							wheel_movement = (DWORD)-120;
						}
					}
					else
					{
						// RealVNC 335 Mouse wheel support
						if (msg.pe.buttonMask & rfbWheelUpMask) {
							flags |= MOUSEEVENTF_WHEEL;
							wheel_movement = WHEEL_DELTA;
						}
						if (msg.pe.buttonMask & rfbWheelDownMask) {
							flags |= MOUSEEVENTF_WHEEL;
							wheel_movement = -WHEEL_DELTA;
						}
					}

					// Generate coordinate values
					// bug fix John Latino
					// offset for multi display
					int screenX, screenY, screenDepth;
					m_server->GetScreenInfo(screenX, screenY, screenDepth);
					// 1 , only one display, so always positive
					//primary display always have (0,0) as corner
					if (m_client->m_single_display)
					{
						unsigned long x = ((msg.pe.x + (m_client->monitor_Offsetx)) * 65535) / (screenX - 1);
						unsigned long y = ((msg.pe.y + (m_client->monitor_Offsety)) * 65535) / (screenY - 1);
						// Do the pointer event
						::mouse_event(flags, (DWORD)x, (DWORD)y, wheel_movement, m_server->SendExtraMouse() ? DWEXTRA_VNC_REMOTE : 0);
						//							vnclog.Print(LL_INTINFO, VNCLOG("########mouse_event :%i %i \n"),x,y);
					}
					else
					{//second or spanned
						if (m_client->Sendinput.isValid())
						{
							INPUT evt{};
							evt.type = INPUT_MOUSE;
							int xx = msg.pe.x - GetSystemMetrics(SM_XVIRTUALSCREEN) + (m_client->monitor_Offsetx + m_client->m_ScreenOffsetx);
							int yy = msg.pe.y - GetSystemMetrics(SM_YVIRTUALSCREEN) + (m_client->monitor_Offsety + m_client->m_ScreenOffsety);
							if (settings->getDriver()) //chris
							{
								xx = msg.pe.x + m_client->monitor_Offsetx;
								yy = msg.pe.y + m_client->monitor_Offsety;
								//vnclog.Print(LL_INTINFO, VNCLOG("MouseMove m_cursor_pos(%d, %d), new(%d, %d)\n"),
								//    xx, yy, msg.pe.x, msg.pe.y);
							}
							evt.mi.dx = (xx * 65535) / (GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1);
							evt.mi.dy = (yy * 65535) / (GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1);
							evt.mi.dwFlags = flags | MOUSEEVENTF_VIRTUALDESK;
							evt.mi.dwExtraInfo = m_server->SendExtraMouse() ? DWEXTRA_VNC_REMOTE : 0;
							evt.mi.mouseData = wheel_movement;
							evt.mi.time = 0;
							(*m_client->Sendinput)(1, &evt, sizeof(evt));
						}
						else
						{
							POINT cursorPos; GetCursorPos(&cursorPos);
							ULONG oldSpeed = 0, newSpeed = 10;
							ULONG mouseInfo[3]{};
							if (flags & MOUSEEVENTF_MOVE)
							{
								flags &= ~MOUSEEVENTF_ABSOLUTE;
								SystemParametersInfo(SPI_GETMOUSE, 0, &mouseInfo, 0);
								SystemParametersInfo(SPI_GETMOUSESPEED, 0, &oldSpeed, 0);
								ULONG idealMouseInfo[] = { 10, 0, 0 };
								SystemParametersInfo(SPI_SETMOUSESPEED, 0, &newSpeed, 0);
								SystemParametersInfo(SPI_SETMOUSE, 0, &idealMouseInfo, 0);
							}
							::mouse_event(flags, msg.pe.x - cursorPos.x, msg.pe.y - cursorPos.y, wheel_movement, m_server->SendExtraMouse() ? DWEXTRA_VNC_REMOTE : 0);
							if (flags & MOUSEEVENTF_MOVE)
							{
								SystemParametersInfo(SPI_SETMOUSE, 0, &mouseInfo, 0);
								SystemParametersInfo(SPI_SETMOUSESPEED, 0, &oldSpeed, 0);
							}
						}
					}
					// Save the old position
					m_client->m_ptrevent = msg.pe;

					// Flag that a remote event occurred
					m_client->m_remoteevent = TRUE;

					// Tell the desktop hook system to grab the screen...
					// removed, terrible performance
					// Why do we grap the screen after any inch a mouse move
					// Removing it doesn't seems to have any missing update
					m_client->TriggerUpdate();
				}
			}
			break;

		case rfbClientCutText:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbClientCutTextMsg - nTO))
			{
				// Allocate storage for the text
				// adzm - 2010-07 - Extended clipboard
				int length = Swap32IfLE(msg.cct.length);
				if (length > 104857600) // 100 MBytes max
					break;
				if (length < 0 && m_client->m_clipboard.settings.m_bSupportsEx) {
					length = abs(length);
					if (length > 104857600 || length < 0)
						break;
					ExtendedClipboardDataMessage extendedClipboardDataMessage;

					extendedClipboardDataMessage.EnsureBufferLength(length, false);

					// Read in the data
					if (!m_socket->ReadExact((char*)extendedClipboardDataMessage.GetBuffer(), length)) {
						break;
					}

					DWORD action = extendedClipboardDataMessage.GetFlags() & clipActionMask;

					// clipCaps may be combined with other actions
					if (action & clipCaps) {
						action = clipCaps;
					}

					switch (action) {
					case clipCaps:
						m_client->m_clipboard.settings.HandleCapsPacket(extendedClipboardDataMessage, true);
						break;
					case clipProvide:
						m_server->UpdateLocalClipTextEx(extendedClipboardDataMessage, m_client);
						break;
					case clipRequest:
					case clipPeek:
					{
						ClipboardData clipboardData;

						// only need an owner window when setting clipboard data -- by using NULL we can rely on fewer locks
						if (clipboardData.Load(NULL)) {
							m_client->UpdateClipTextEx(clipboardData, extendedClipboardDataMessage.GetFlags());
						}
					}
					break;
					case clipNotify:	// irrelevant coming from viewer
					default:
						// unsupported or not implemented
						break;
					}
				}
				else if (length >= 0) {
					char* winStr = NULL;
					{
						char* text = new char[length + 1];
						if (text == NULL)
							break;
						text[length] = 0;

						// Read in the text
						if (!m_socket->ReadExact(text, length)) {
							delete[] text;
							break;
						}

						int len = (int)strlen(text);
						winStr = new char[len * 2 + 1];

						int j = 0;
						for (int i = 0; i < len; i++)
						{
							if (text[i] == 10)
								winStr[j++] = 13;
							winStr[j++] = text[i];
						}
						winStr[j++] = 0;

						// Free the clip text we read
						delete[] text;
					}

					if (winStr != NULL) {
						m_client->m_clipboard.m_strLastCutText = winStr;
						// Get the server to update the local clipboard
						m_server->UpdateLocalClipText(winStr);

						// Free the transformed clip text
						delete[] winStr;
					}
				}
				else {
					break;
				}
			}
			break;

			// Modif sf@2002 - Scaling
			// Server Scaling Message received
		case rfbPalmVNCSetScaleFactor:
			if (m_server->AreThereMultipleViewers() == false)
				m_client->m_fPalmVNCScaling = true;
		case rfbSetScale: // Specific PalmVNC SetScaleFactor
			// need to be ignored if multiple viewers are running, else buffer change on the fly and one of the viewers crash.
		{
			// m_client->m_fPalmVNCScaling = false;
			// Read the rest of the message
			if (m_client->m_fPalmVNCScaling)
			{
				if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbPalmVNCSetScaleFactorMsg - nTO))
				{
					m_client->cl_connected = FALSE;
					break;
				}
			}
			else
			{
				if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbSetScaleMsg - nTO))
				{
					m_client->cl_connected = FALSE;
					break;
				}
			}
			if (m_server->AreThereMultipleViewers() == true) break;
			// Only accept reasonable scales...
			if (msg.ssc.scale < 1 || msg.ssc.scale > 9) break;
			m_client->m_nScale_viewer = msg.ssc.scale;
			m_client->m_nScale = msg.ssc.scale;
			{
				omni_mutex_lock l(m_client->GetUpdateLock(), 87);
				if (!m_client->m_encodemgr.m_buffer->SetScale(msg.ssc.scale))
				{
					m_client->cl_connected = FALSE;
					break;
				}

				m_client->fNewScale = true;
				m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.br.x = m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.br.x / msg.ssc.scale;
				m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.br.y = m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.br.y / msg.ssc.scale;
				m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.tl.x = m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.tl.x / msg.ssc.scale;
				m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.tl.y = m_client->m_encodemgr.m_buffer->m_desktop->m_cursorpos.tl.y / msg.ssc.scale;
				InvalidateRect(NULL, NULL, TRUE);
				m_client->TriggerUpdateThread();
			}
		}
		break;

		// Set Server Input
		case rfbSetServerInput:
			if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbSetServerInputMsg - nTO))
			{
				m_client->cl_connected = FALSE;
				break;
			}

			m_client->m_Support_rfbSetServerInput = true;
			if (m_client->m_keyboardenabled)
			{
				// added jeff
				vnclog.Print(LL_INTINFO, VNCLOG("rfbSetServerInput: inputs %s\n"), (msg.sim.status == 1) ? "disabled" : "enabled");
#ifdef _DEBUG
				char szText[256];
				sprintf_s(szText, "rfbSetServerInput  %i %i %i\n", msg.sim.status, m_server->GetDesktopPointer()->GetBlockInputState(), m_client->m_bClientHasBlockedInput);
				OutputDebugString(szText);
#endif
				// only allow change if this is the client that originally changed the input state
				/// fix by PGM (pgmoney)
				if (!m_server->GetDesktopPointer()->GetBlockInputState() && msg.sim.status == 1)
				{
					m_client->m_encodemgr.m_buffer->m_desktop->SetBlockInputState(true);
					m_client->m_bClientHasBlockedInput = (true);
				}

				else if (m_server->GetDesktopPointer()->GetBlockInputState() && m_client->m_bClientHasBlockedInput && msg.sim.status == 0)
				{
					m_client->m_encodemgr.m_buffer->m_desktop->SetBlockInputState(FALSE);
					m_client->m_bClientHasBlockedInput = (FALSE);
				}

				else if (!m_server->GetDesktopPointer()->GetBlockInputState() && !m_client->m_bClientHasBlockedInput && msg.sim.status == 0)
				{
					m_client->m_encodemgr.m_buffer->m_desktop->SetBlockInputState(FALSE);
					m_client->m_bClientHasBlockedInput = (FALSE);
				}
			}
			break;

		case rfbSetDesktopSize:
			if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbSetDesktopSizeMsg - nTO)) {
				m_client->cl_connected = FALSE;
				break;
			}
			{
				map< pair<int, int>, pair<int, int> > resolutionMap;
				DisplayMode flag = dmDisplay;
				for (int i = 0; i < msg.sdm.numberOfScreens; i++) {
					rfbExtDesktopScreen eds{};
					if (!m_socket->ReadExact(((char*)&eds), sz_rfbExtDesktopScreen)) {
						m_client->cl_connected = FALSE;
						break;
					}
					int id = Swap32IfLE(eds.id);
					int x = Swap16IfLE(eds.x);
					int y = Swap16IfLE(eds.y);
					int w = Swap16IfLE(eds.width);
					int h = Swap16IfLE(eds.height);
					flag = DisplayMode(Swap32IfLE(eds.flags));
#ifdef _DEBUG
					char			szText[256];
					sprintf_s(szText, "++++++++++++++++++++++++++++++ DesktopScreen %i %i %i %i \n", x, y, w, h);
					OutputDebugString(szText);
#endif
					//add some protection,
					if (w * h <= 12582912)
						resolutionMap.insert(make_pair(make_pair(x, y), make_pair(w, h)));
				}
				if (!m_server->m_virtualDisplaySupported)
					flag = dmDisplay;
				// Only the first Viewer can set the resolution
				// unless it's extended mode with a single monitor
				// This way each viewer can add his own extended display
				m_client->m_singleExtendMode = (flag == dmExtendOnly);
				if (m_server->virtualDisplay && !m_server->AreThereMultipleViewers())
					m_server->virtualDisplay->attachDisplay(flag, resolutionMap, m_client->m_singleExtendMode, m_client->m_id, m_client->displayname);
				else if (m_server->virtualDisplay && m_server->AreThereMultipleViewers() && m_client->m_singleExtendMode)
					m_server->virtualDisplay->attachDisplay(flag, resolutionMap, m_client->m_singleExtendMode, m_client->m_id, m_client->displayname);
				if (strlen(m_client->displayname) == 0)
					m_client->m_singleExtendMode = false;
			}
			break;
			// Set Single Window
		case rfbSetMonitor:
			if (m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbMonitorMsg - nTO))
			{
				m_client->m_encodemgr.m_buffer->m_desktop->SetMonitor(msg.mm.nbr);
			}
			break;
		case rfbSetSW:
			if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbSetSWMsg - nTO))
			{
				m_client->cl_connected = FALSE;
				break;
			}
			if (Swap16IfLE(msg.sw.x) < 5 && Swap16IfLE(msg.sw.y) < 5)
			{
				m_client->m_encodemgr.m_buffer->m_desktop->SetSW(1, 1);
				break;
			}
			m_client->m_encodemgr.m_buffer->m_desktop->SetSW(
				(Swap16IfLE(msg.sw.x) + m_client->monitor_Offsetx + m_client->m_ScreenOffsetx) * m_client->m_nScale,
				(Swap16IfLE(msg.sw.y) + m_client->monitor_Offsety + m_client->m_ScreenOffsety) * m_client->m_nScale);
			break;

			// Modif sf@2002 - Text Chat
		case rfbTextChat:
			m_client->m_pTextChat->ProcessTextChatMsg(nTO);
			break;

			// Modif sf@2002 - File Transfer
			// File Transfer Message
		case rfbFileTransfer:
		{
			// sf@2004 - An unlogged user can't access to FT
			bool fUserOk = true;
#ifndef SC_20
			if (settings->getFTUserImpersonation())
			{
				fUserOk = m_client->DoFTUserImpersonation();
			}
#endif // SC_20

			if (!m_client->m_keyboardenabled || !m_client->m_pointerenabled) fUserOk = false; //PGM

			omni_mutex_lock l(m_client->GetUpdateLock(), 88);

			// Read the rest of the message:
			m_client->m_fFileTransferRunning = TRUE;
			if (m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbFileTransferMsg - nTO))
			{
				switch (msg.ft.contentType)
				{
					// A new file is received from the client
						// case rfbFileHeader:
				case rfbFileTransferSessionStart:
					m_client->m_fFileSessionOpen = true;
					break;
				case rfbFileTransferSessionEnd:
					m_client->m_fFileSessionOpen = false;
					break;
				case rfbFileTransferOffer:
				{
					omni_mutex_lock ll(m_client->GetUpdateLock(), 89);
					if (!settings->getEnableFileTransfer() || !fUserOk) break;
					// bool fError = false;
					const UINT length = Swap32IfLE(msg.ft.length);
					memset(m_client->m_szFullDestName, 0, sizeof(m_client->m_szFullDestName));
					if (length > sizeof(m_client->m_szFullDestName) - 12) break; // needed for temp_prefix
					// Read in the Name of the file to create
					if (!m_socket->ReadExact(m_client->m_szFullDestName, length))
					{
						//MessageBoxSecure(NULL, "1. Abort!", "UltraVNC Server", MB_OK);
						// vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: Failed to receive FileName from Viewer. Abort!\n"));
						break;
					}

					// sf@2004 - Improving huge files size handling
					CARD32 sizeL = Swap32IfLE(msg.ft.size);
					CARD32 sizeH = 0;
					CARD32 sizeHtmp = 0;
					if (!m_socket->ReadExact((char*)&sizeHtmp, sizeof(CARD32)))
					{
						//MessageBoxSecure(NULL, "2. Abort!", "UltraVNC Server", MB_OK);
						//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: Failed to receive SizeH from Viewer. Abort!\n"));
						break;
					}
					sizeH = Swap32IfLE(sizeHtmp);

					// Parse the FileTime
					char* p = strrchr(m_client->m_szFullDestName, ',');
					if (p == NULL)
						m_client->m_szFileTime[0] = '\0';
					else
					{
						strcpy_s(m_client->m_szFileTime, p + 1);
						*p = '\0';
					}

					// make a temp file name
					strcpy_s(m_client->m_szFullDestName, make_temp_filename(m_client->m_szFullDestName).c_str());

					DWORD dwDstSize = (DWORD)0; // Dummy size, actually a return value

					// Also check the free space on destination drive
					ULARGE_INTEGER lpFreeBytesAvailable;
					ULARGE_INTEGER lpTotalBytes;
					ULARGE_INTEGER lpTotalFreeBytes;
					unsigned long dwFreeKBytes;
					char* szDestPath = new char[length + 1 + 64];
					if (szDestPath == NULL) break;
					memset(szDestPath, 0, length + 1 + 64);
					strcpy_s(szDestPath, length + 1 + 64, m_client->m_szFullDestName);
					*strrchr(szDestPath, '\\') = '\0'; // We don't handle UNCs for now

					// loadlibrary
					// needed for Windows 95 non-OSR2
					// Possible this will block File Transfer, but at least server will start
					PGETDISKFREESPACEEX pGetDiskFreeSpaceEx;
					pGetDiskFreeSpaceEx = (PGETDISKFREESPACEEX)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA");

					if (pGetDiskFreeSpaceEx)
					{
						if (!pGetDiskFreeSpaceEx((LPCTSTR)szDestPath,
							&lpFreeBytesAvailable,
							&lpTotalBytes,
							&lpTotalFreeBytes)
							)
							dwDstSize = 0xFFFFFFFF;
					}

					delete[] szDestPath;
					dwFreeKBytes = (unsigned long)(Int64ShraMod32(lpFreeBytesAvailable.QuadPart, 10));
					__int64 nnFileSize = (((__int64)sizeH) << 32) + sizeL;
					if ((__int64)dwFreeKBytes < (__int64)(nnFileSize / 1000)) dwDstSize = 0xFFFFFFFF;

					// Allocate buffer for file packets
					m_client->m_pBuff = new char[sz_rfbBlockSize + 1024];
					if (m_client->m_pBuff == NULL)
						dwDstSize = 0xFFFFFFFF;

					// Allocate buffer for DeCompression
					m_client->m_pCompBuff = new char[sz_rfbBlockSize];
					if (m_client->m_pCompBuff == NULL)
						dwDstSize = 0xFFFFFFFF;

					rfbFileTransferMsg ft = { 0 };
					ft.type = rfbFileTransfer;

					// sf@2004 - Delta Transfer
					bool fAlreadyExists = false;
					if (dwDstSize != 0xFFFFFFFF)
					{
						// sf@2004 - Directory Delta Transfer
						// If the offered file is a zipped directory, we test if it already exists here
						// and create the zip accordingly. This way we can generate the checksums for it.
						// m_client->CheckAndZipDirectoryForChecksuming(m_client->m_szFullDestName);

						if (m_client->m_hPToken)
							ImpersonateLoggedOnUser(m_client->m_hPToken); //need to set this thread's impersonation or can find mapped network or share files

						// Create Local Dest file
						m_client->m_hDestFile = CreateFile(m_client->m_szFullDestName,
							GENERIC_WRITE | GENERIC_READ,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_ALWAYS,
							FILE_FLAG_SEQUENTIAL_SCAN,
							NULL);
						fAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
						if (m_client->m_hDestFile == INVALID_HANDLE_VALUE)
							dwDstSize = 0xFFFFFFFF;
						else
							dwDstSize = 0x00;
					}
					if (fAlreadyExists && dwDstSize != 0xFFFFFFFF)
					{
						ULARGE_INTEGER n2SrcSize;
						bool bSize = m_client->MyGetFileSize(m_client->m_szFullDestName, &n2SrcSize);
						//DWORD dwFileSize = GetFileSize(m_client->m_hDestFile, NULL);
						//if (dwFileSize != 0xFFFFFFFF)
						if (bSize)
						{
							int nCSBufferSize = (4 * (int)(n2SrcSize.QuadPart / sz_rfbBlockSize)) + 1024;
							char* lpCSBuff = new char[nCSBufferSize];
							if (lpCSBuff != NULL)
							{
								int nCSBufferLen = m_client->GenerateFileChecksums(m_client->m_hDestFile,
									lpCSBuff,
									nCSBufferSize);
								if (nCSBufferLen != -1)
								{
									ft.contentType = rfbFileChecksums;
									ft.size = Swap32IfLE(nCSBufferSize);
									ft.length = Swap32IfLE(nCSBufferLen);
									//adzm 2010-09 - minimize packets. SendExact flushes the queue.
									m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
									m_socket->SendExactQueue((char*)lpCSBuff, nCSBufferLen);
									delete[] lpCSBuff;
								}
							}
						}
					}

					ft.contentType = rfbFileAcceptHeader;
					ft.size = Swap32IfLE(dwDstSize); // File Size in bytes, 0xFFFFFFFF (-1) means error
					ft.length = Swap32IfLE(strlen(m_client->m_szFullDestName));
					//adzm 2010-09 - minimize packets. SendExact flushes the queue.
					m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
					m_socket->SendExact((char*)m_client->m_szFullDestName, (const VCard)strlen(m_client->m_szFullDestName));

					if (dwDstSize == 0xFFFFFFFF)
					{
						helper::close_handle(m_client->m_hDestFile);
						if (m_client->m_pCompBuff != NULL)
						{
							delete[] m_client->m_pCompBuff;
							m_client->m_pCompBuff = NULL;
						}
						if (m_client->m_pBuff != NULL)
						{
							delete[] m_client->m_pBuff;
							m_client->m_pBuff = NULL;
						}

						//MessageBoxSecure(NULL, "3. Abort!", "UltraVNC Server", MB_OK);
						//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: Wrong Dest File size. Abort!\n"));
						m_client->FTDownloadFailureHook();
						break;
					}

					m_client->m_dwFileSize = Swap32IfLE(msg.ft.size);
					m_client->m_dwNbPackets = (DWORD)(m_client->m_dwFileSize / sz_rfbBlockSize);
					m_client->m_dwNbReceivedPackets = 0;
					m_client->m_dwNbBytesWritten = 0;
					m_client->m_dwTotalNbBytesWritten = 0;

					m_client->m_fUserAbortedFileTransfer = false;
					m_client->m_fFileDownloadError = false;
					m_client->m_fFileDownloadRunning = true;
					m_client->FTDownloadStartHook();
					m_socket->SetRecvTimeout(settings->getftTimeout() * 1000);
				}
				break;

				// The client requests a File
				case rfbFileTransferRequest:
					m_client->filetransferrequestPart1(msg, fUserOk);
					break;

					// sf@2004 - Delta Transfer
					// Destination file already exists - the viewer sends the checksums
				case rfbFileChecksums:
					m_socket->SetSendTimeout(settings->getftTimeout() * 1000);
					m_client->cl_connected = m_client->ReceiveDestinationFileChecksums(Swap32IfLE(msg.ft.size), Swap32IfLE(msg.ft.length));
					break;

					// Destination file (viewer side) is ready for reception (size > 0) or not (size = -1)
				case rfbFileHeader:
				{
					// Check if the file has been created on client side
					if (Swap32IfLE(msg.ft.size) == -1)
					{
						helper::close_handle(m_client->m_hSrcFile);
						m_client->FTUploadFailureHook();
						// MessageBoxSecure(NULL, "7. Abort!", "UltraVNC Server", MB_OK);
						//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: File not created on client side. Abort!\n"));
						break;
					}

					// Allocate buffer for file packets
					m_client->m_pBuff = new char[sz_rfbBlockSize];
					if (m_client->m_pBuff == NULL)
					{
						helper::close_handle(m_client->m_hSrcFile);
						//MessageBoxSecure(NULL, "8. Abort!", "UltraVNC Server", MB_OK);
						//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: rfbFileHeader - Unable to allocate buffer. Abort!\n"));
						m_client->FTUploadFailureHook();
						break;
					}

					// Allocate buffer for compression
					// Todo : make a global buffer with CheckBufferSize proc
					m_client->m_pCompBuff = new char[sz_rfbBlockSize + 1024]; // TODO: Improve this
					if (m_client->m_pCompBuff == NULL)
					{
						helper::close_handle(m_client->m_hSrcFile);
						if (m_client->m_pBuff != NULL) {
							delete[] m_client->m_pBuff;
							m_client->m_pBuff = NULL;
						}
						//MessageBoxSecure(NULL, "9. Abort!", "UltraVNC Server", MB_OK);
						//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: rfbFileHeader - Unable to allocate comp. buffer. Abort!\n"));
						m_client->FTUploadFailureHook();
						break;
					}

					m_client->m_fEof = false;
					m_client->m_dwNbBytesRead = 0;
					m_client->m_dwTotalNbBytesWritten = 0;
					m_client->m_nPacketCount = 0;
					m_client->m_fFileUploadError = false;
					m_client->m_fFileUploadRunning = true;
					m_client->m_fUserAbortedFileTransfer = false;

					m_client->cl_connected = m_client->SendFileChunk();
				}
				break;

				case rfbFilePacket:
					if (!settings->getEnableFileTransfer() || !fUserOk) break;
					m_client->cl_connected = m_client->ReceiveFileChunk(Swap32IfLE(msg.ft.length), Swap32IfLE(msg.ft.size));
					m_client->SendKeepAlive();
					break;

				case rfbEndOfFile:
					if (!settings->getEnableFileTransfer() || !fUserOk) break;

					if (m_client->m_fFileDownloadRunning)
					{
						m_client->FinishFileReception();
					}
					break;

					// We use this message for File Transfer rights (<= RC18)
					// The client asks for File Transfer permission
				case rfbAbortFileTransfer:

					// For now...
					if (m_client->m_fFileDownloadRunning)
					{
						m_client->m_fFileDownloadError = true;
						m_client->m_fUserAbortedFileTransfer = true;
						m_client->FinishFileReception();
					}
					else if (m_client->m_fFileUploadRunning)
					{
						m_client->m_fFileUploadError = true;
						m_client->m_fUserAbortedFileTransfer = true;
						// m_client->FinishFileSending();
					}
					else // Old method for File Transfer handshake perimssion (<= RC18)
					{
						// We reject any <= RC18 UltraVNC Viewer File Transfer
						m_client->fFTRequest = true;

						// sf@2002 - DO IT HERE FOR THE MOMENT
						// File Transfer permission requested by the client
						if (m_client->fFTRequest)
						{
							rfbFileTransferMsg ft = { 0 };
							ft.type = rfbFileTransfer;
							ft.contentType = rfbAbortFileTransfer;

							bool bOldFTProtocole = (msg.ft.contentParam == 0);
							if (bOldFTProtocole)
								ft.contentType = rfbAbortFileTransfer; // Viewer with New v2 File Transfer Protocol
							else
								ft.contentType = rfbFileTransferAccess; // Viewer with old File Transfer Protocol

							if (!bOldFTProtocole && settings->getEnableFileTransfer() && settings->getEnableRemoteInputs() && fUserOk)
								ft.size = Swap32IfLE(1);
							else
								ft.size = Swap32IfLE(-1);
							m_client->m_socket->SendExact((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
							m_client->fFTRequest = false;
						}
					}
					break;

					/* Not yet used because we want backward compatibility...
					// From RC19 versions, the UltraVNC Viewer uses this new message to request File Transfer permission
					// It also transmits its File Transfer versions
					case rfbFileTransferAccess:
						m_client->fFTRequest = true;

						// File Transfer permission requested by the client
						if (m_client->fFTRequest)
						{
							rfbFileTransferMsg ft;
							ft.type = rfbFileTransfer;
							ft.contentType = rfbFileTransferAccess;

							if (m_server->FileTransferEnabled() && m_client->m_server->RemoteInputsEnabled())
							   ft.size = Swap32IfLE(1);
							else
							   ft.size = Swap32IfLE(-1);
							m_client->m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
							m_client->fFTRequest = false;
						}

						break;
					*/

					// The client requests the content of a directory or Drives List
				case rfbDirContentRequest:
					switch (msg.ft.contentParam)
					{
						// Client requests the List of Local Drives
					case rfbRDrivesList:
					{
						TCHAR szDrivesList[256]; // Format when filled : "C:\<NULL>D:\<NULL>....Z:\<NULL><NULL>
						DWORD dwLen;
						DWORD nIndex = 0;
						int nType = 0;
						TCHAR szDrive[4];
						dwLen = GetLogicalDriveStrings(256, szDrivesList);
						// moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						if (!settings->getEnableFileTransfer() || !fUserOk) break;

						// We add Drives types to this drive list...
						while (nIndex < dwLen - 3)
						{
							strcpy_s(szDrive, szDrivesList + nIndex);
							// We replace the "\" char following the drive letter and ":"
							// with a char corresponding to the type of drive
							// We obtain something like "C:l<NULL>D:c<NULL>....Z:n\<NULL><NULL>"
							// Isn't it ugly ?
							nType = GetDriveType(szDrive);
							switch (nType)
							{
							case DRIVE_FIXED:
								szDrivesList[nIndex + 2] = 'l';
								break;
							case DRIVE_REMOVABLE:
								szDrivesList[nIndex + 2] = 'f';
								break;
							case DRIVE_CDROM:
								szDrivesList[nIndex + 2] = 'c';
								break;
							case DRIVE_REMOTE:
								szDrivesList[nIndex + 2] = 'n';
								break;
							}
							nIndex += 4;
						}

						rfbFileTransferMsg ft = { 0 };
						ft.type = rfbFileTransfer;
						ft.contentType = rfbDirPacket;
						ft.contentParam = rfbADrivesList;
						ft.length = Swap32IfLE((int)dwLen);
						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						m_socket->SendExact((char*)szDrivesList, (int)dwLen);
					}
					break;

					// Client requests the content of a directory
					case rfbRDirContent:
					{
						//omni_mutex_lock l(m_client->GetUpdateLock());

						const UINT length = Swap32IfLE(msg.ft.length);
						char szDir[MAX_PATH + 2];
						if (length > sizeof(szDir) - 1) break;

						// Read in the Name of Dir to explore
						if (!m_socket->ReadExact(szDir, length)) break;
						szDir[length] = 0;

						// moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						if (!settings->getEnableFileTransfer() || !fUserOk) break;
						// sf@2004 - Shortcuts Case
						// Todo: Cultures translation ?
						int nFolder = -1;
						char szP[MAX_PATH + 2];
						bool fShortError = false;
						if (!_strnicmp(szDir, "My Documents", 11))
							nFolder = CSIDL_PERSONAL;
						if (!_strnicmp(szDir, "Desktop", 7))
							nFolder = CSIDL_DESKTOP;
						if (!_strnicmp(szDir, "Network Favorites", 17))
							nFolder = CSIDL_NETHOOD;

						if (nFolder != -1)
							// if (SHGetSpecialFolderPath(NULL, szP, nFolder, FALSE))
							if (m_client->GetSpecialFolderPath(nFolder, szP))
							{
								if (szP[strlen(szP) - 1] != '\\') strcat_s(szP, "\\");
								strcpy_s(szDir, szP);
							}
							else {
								fShortError = true;
							}

						strcat_s(szDir, "*");

						WIN32_FIND_DATA fd;
						HANDLE ff;
						BOOL fRet = TRUE;

						rfbFileTransferMsg ft{};
						ft.type = rfbFileTransfer;
						ft.contentType = rfbDirPacket;
						ft.contentParam = rfbADirectory; // or rfbAFile...

						DWORD errmode = SetErrorMode(SEM_FAILCRITICALERRORS); // No popup please !
						ff = FindFirstFile(szDir, &fd);
						SetErrorMode(errmode);

						// Case of media not accessible
						if (ff == INVALID_HANDLE_VALUE || fShortError)
						{
							ft.length = Swap32IfLE(0);
							m_socket->SendExact((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
							break;
						}

						ft.length = Swap32IfLE(strlen(szDir) - 1);
						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						// sf@2004 - Also send back the full directory path to the viewer (necessary for Shorcuts)
						m_socket->SendExactQueue((char*)szDir, (const VCard)(strlen(szDir) - 1));

						while (fRet)
						{
							// sf@2003 - Convert file time to local time
							// We've made the choice off displaying all the files
							// off client AND server sides converted in clients local
							// time only. So we don't convert server's files times.
							/*
							FILETIME LocalFileTime;
							FileTimeToLocalFileTime(&fd.ftLastWriteTime, &LocalFileTime);
							fd.ftLastWriteTime.dwLowDateTime = LocalFileTime.dwLowDateTime;
							fd.ftLastWriteTime.dwHighDateTime = LocalFileTime.dwHighDateTime;
							*/

							if (((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && strcmp(fd.cFileName, "."))
								||
								(!strcmp(fd.cFileName, "..")))
							{
								// Serialize the interesting part of WIN32_FIND_DATA
								char szFileSpec[sizeof(WIN32_FIND_DATA)];
								int nOptLen = sizeof(WIN32_FIND_DATA) - MAX_PATH - 14 + lstrlen(fd.cFileName);
								memcpy(szFileSpec, &fd, nOptLen);

								ft.length = Swap32IfLE(nOptLen);
								//adzm 2010-09 - minimize packets. SendExact flushes the queue.
								m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
								m_socket->SendExactQueue((char*)szFileSpec, nOptLen);
							}
							else if (strcmp(fd.cFileName, "."))
							{
								// Serialize the interesting part of WIN32_FIND_DATA
								// Get rid of the trailing blanck chars. It makes a BIG
								// difference when there's a lot of files in the dir.
								char szFileSpec[sizeof(WIN32_FIND_DATA)];
								int nOptLen = sizeof(WIN32_FIND_DATA) - MAX_PATH - 14 + lstrlen(fd.cFileName);
								memcpy(szFileSpec, &fd, nOptLen);

								ft.length = Swap32IfLE(nOptLen);
								//adzm 2010-09 - minimize packets. SendExact flushes the queue.
								m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
								m_socket->SendExactQueue((char*)szFileSpec, nOptLen);
							}
							fRet = FindNextFile(ff, &fd);
						}
						FindClose(ff);

						// End of the transfer
						ft.contentParam = 0;
						ft.length = Swap32IfLE(0);
						m_socket->SendExact((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
					}
					break;
					}
					break;

					// The client sends a command
				case rfbCommand:
					switch (msg.ft.contentParam)
					{
						// Client requests the creation of a directory
					case rfbCDirCreate:
					{
						const UINT length = Swap32IfLE(msg.ft.length);
						char szDir[MAX_PATH + 1];
						if (length > sizeof(szDir) - 1) break;

						// Read in the Name of Dir to explore
						if (!m_socket->ReadExact(szDir, length))
						{
							// todo: manage error !
							break;
						}
						szDir[length] = 0;

						// moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						if (!settings->getEnableFileTransfer() || !fUserOk) break;
						// Create the Dir
						BOOL fRet = CreateDirectory(szDir, NULL);

						rfbFileTransferMsg ft{};
						ft.type = rfbFileTransfer;
						ft.contentType = rfbCommandReturn;
						ft.contentParam = rfbADirCreate;
						ft.size = fRet ? 0 : -1;
						ft.length = msg.ft.length;

						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						m_socket->SendExact((char*)szDir, (int)length);
						if (fRet)
							m_client->FTNewFolderHook(szDir);
					}
					break;

					// Client requests the deletion of a file
					case rfbCFileDelete:
					{
						UINT length = Swap32IfLE(msg.ft.length);
						char szFile[MAX_PATH + 1];
						if (length > sizeof(szFile) - 1) break;

						// Read in the Name of the File
						if (!m_socket->ReadExact(szFile, length))
						{
							// todo: manage error !
							break;
						}
						szFile[length] = 0;
						// moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						if (!settings->getEnableFileTransfer() || !fUserOk) break;

						// Delete the file
						// 13 February 2008 jdp
						bool isDir = isDirectory(szFile);
						std::string newname(szFile);
						// put the '[]' around the name if it's a folder, so that the client can display
						// the proper messages. Otherwise, the client assumes it's a file.
						if (isDir)
							newname = AddDirPrefixAndSuffix(szFile);

						length = (UINT)(newname.length() + 1);
						BOOL fRet = DeleteFileOrDirectory(szFile);

						rfbFileTransferMsg ft{};
						ft.type = rfbFileTransfer;
						ft.contentType = rfbCommandReturn;
						ft.contentParam = rfbAFileDelete;
						ft.size = fRet ? 0 : -1;
						ft.length = Swap32IfLE(length);

						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						m_socket->SendExact((char*)newname.c_str(), length);
						if (fRet)
							m_client->FTDeleteHook(szFile, isDir);
					}
					break;

					// Client requests the Renaming of a file/directory
					case rfbCFileRename:
					{
						const UINT length = Swap32IfLE(msg.ft.length);
						char szNames[(2 * MAX_PATH) + 1];
						if (length > sizeof(szNames) - 1) break;

						// Read in the Names
						if (!m_socket->ReadExact(szNames, length))
						{
							// todo: manage error !
							break;
						}
						szNames[length] = 0;
						// moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						if (!settings->getEnableFileTransfer() || !fUserOk) break;

						char* p = strrchr(szNames, '*');
						if (p == NULL) break;
						char szCurrentName[(2 * MAX_PATH) + 1];
						char szNewName[(2 * MAX_PATH) + 1];

						strcpy_s(szNewName, p + 1);
						*p = '\0';
						strcpy_s(szCurrentName, szNames);
						*p = '*';

						// Rename
						BOOL fRet = MoveFile(szCurrentName, szNewName);

						rfbFileTransferMsg ft{};
						ft.type = rfbFileTransfer;
						ft.contentType = rfbCommandReturn;
						ft.contentParam = rfbAFileRename;
						ft.size = fRet ? 0 : -1;
						ft.length = msg.ft.length;

						//adzm 2010-09 - minimize packets. SendExact flushes the queue.
						m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						m_socket->SendExact((char*)szNames, (int)length);
						if (fRet)
							m_client->FTRenameHook(szCurrentName, szNewName);
					}
					break;
					}  // End of swith
					break;
				} // End of switch
			} // End of if
			else // Fix from Jeremy C.
			{
				if (m_client->m_fFileDownloadRunning)
				{
					m_client->m_fFileDownloadError = true;
					FlushFileBuffers(m_client->m_hDestFile);
					helper::close_handle(m_client->m_hDestFile);
					m_client->FTDownloadFailureHook();
					m_client->m_fFileDownloadRunning = false;
				}
				if (m_client->m_fFileUploadRunning)
				{
					m_client->m_fFileUploadError = true;
					FlushFileBuffers(m_client->m_hSrcFile);
					helper::close_handle(m_client->m_hSrcFile);
					m_client->FTUploadFailureHook();
					m_client->m_fFileUploadRunning = false;
				}
				//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: message content reading error\n"));
			}

			m_client->m_fFileTransferRunning = FALSE;
		}
		break;

		// adzm 2010-09 - Notify streaming DSM plugin support
		case rfbNotifyPluginStreaming:
			if (sz_rfbNotifyPluginStreamingMsg > 1)
			{
				if (!m_socket->ReadExact(((char*)&msg) + nTO, sz_rfbNotifyPluginStreamingMsg - nTO))
				{
					m_client->cl_connected = FALSE;
					break;
				}
			}
			m_socket->SetPluginStreamingIn();
			break;
		default:
			// Unknown message, so fail!
			m_client->cl_connected = FALSE;
		}
#ifndef SC_20
		// sf@2005 - Cancel File Transfer User impersonation if possible
		// We do it here to ensure impersonation is cancelled
		if (settings->getFTUserImpersonation())
		{
			m_client->UndoFTUserImpersonation();
		}
#endif // SC_20
	}

	if (fShutdownOrdered) {
		m_autoreconnectcounter_quit = true;
		//needed to give autoreconnect (max 100) to quit
		Sleep(200);
	}

	if (m_client->m_fFileDownloadRunning)
	{
		m_client->m_fFileDownloadError = true;
		FlushFileBuffers(m_client->m_hDestFile);
		m_client->FTDownloadFailureHook();
		m_client->m_fFileDownloadRunning = false;
		helper::close_handle(m_client->m_hDestFile);
	}

	if (m_client->m_fFileUploadRunning)
	{
		m_client->m_fFileUploadError = true;
		FlushFileBuffers(m_client->m_hSrcFile);
		m_client->FTUploadFailureHook();
		m_client->m_fFileUploadRunning = false;
		helper::close_handle(m_client->m_hSrcFile);
	}

	// Move into the thread's original desktop
	// TAG 14
	desktopSelector::SelectHDESK(home_desktop);
	if (m_client->m_bClientHasBlockedInput)
	{
		m_client->m_encodemgr.m_buffer->m_desktop->SetBlockInputState(false);
		m_client->m_bClientHasBlockedInput = false;
	}
	if (input_desktop)
		if (!CloseDesktop(input_desktop))
			vnclog.Print(LL_INTERR, VNCLOG("failed to close desktop\n"));
	// Quit this thread. This will automatically delete the thread and the
	// associated client.
	vnclog.Print(LL_CLIENTS, VNCLOG("client disconnected : %s (%hd)\n"),
		m_client->GetClientNameName(),
		m_client->GetClientId());
	//////////////////
	// LOG it also in the event
	//////////////////
#ifndef SC_20
	typedef BOOL(*LogeventFn)(char* machine, char* user, int clientId, bool isinteractive);
	LogeventFn Logevent = 0;
	char szCurrentDir[MAX_PATH];
	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		*p = '\0';
		strcat_s(szCurrentDir, "\\logging.dll");
	}
	HMODULE hModule = LoadLibrary(szCurrentDir);
	if (hModule)
	{
		Logevent = (LogeventFn)GetProcAddress(hModule, "LOGEXIT");
		Logevent((char*)m_client->GetClientNameName(), (char*)m_client->GetClientDomainUsername(), m_client->GetClientId(), m_client->m_keyboardenabled && m_client->m_pointerenabled);
		FreeLibrary(hModule);
	}
#endif // SC_20

	// Disable the protocol to ensure that the update thread
	// is not accessing the desktop and buffer objects
	// DEADLOCK ON EXIT SLow systems... IS this realy needed
	//m_client->DisableProtocol();

	// Finally, it's safe to kill the update thread here
	if (m_client->m_updatethread) {
		m_client->m_updatethread->Kill();
		m_client->m_updatethread->join(NULL);
	}
	// Remove the client from the server
	// This may result in the desktop and buffer being destroyed
	// It also guarantees that the client will not be passed further
	// updates
	m_server->RemoveClient(m_client->GetClientId());

	if (!fShutdownOrdered) {
		if (m_client->m_Autoreconnect)
		{
			vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient II\n"));
			m_server->AutoReconnect(m_client->m_Autoreconnect);
			m_server->AutoReconnectPort(m_AutoReconnectPort);
			m_server->AutoReconnectAdr(m_szAutoReconnectAdr);
			m_server->AutoReconnectId(m_szAutoReconnectId);
			if (settings->getIPV6())
				postHelper::PostAddNewClient4(1111, 1111);
			else
				postHelper::PostAddNewClient(1111, 1111);
		}
	}
}

// The vncClient itself

// adzm - 2010-07 - Extended clipboard
vncClient::vncClient() : m_clipboard(ClipboardSettings::defaultServerCaps), Sendinput("USER32", "SendInput")
{
	vnclog.Print(LL_INTINFO, VNCLOG("vncClient() executing...\n"));

	m_hPToken = 0;

	m_socket = NULL;
	m_client_name_name = NULL;
	m_client_name_address = NULL;
	m_client_domain_username = NULL;

	// Initialise mouse fields
	m_mousemoved = FALSE;
	m_ptrevent.buttonMask = 0;
	m_ptrevent.x = 0;
	m_ptrevent.y = 0;

	// Other misc flags
	m_thread_ClientThread = NULL;
	m_palettechanged = FALSE;

	// Initialise the two update stores
	m_updatethread = NULL;
	m_update_tracker.init(this);

	m_remoteevent = FALSE;

	// adzm - 2010-07 - Extended clipboard
	//m_clipboard_text = 0;

	// IMPORTANT: Initially, client is not protocol-ready.
	m_disable_protocol = 1;

	//SINGLE WINDOW
	m_use_NewSWSize = FALSE;
	m_use_ExtDesktopSize = FALSE;
	m_requestedDesktopSizeChange = 0;
	m_lastDesktopSizeChangeError = 0;
	monitor_Offsetx = 0;
	monitor_Offsety = 0;
	m_ScreenOffsetx = 0;
	m_ScreenOffsety = 0;

	// sf@2002
	fNewScale = false;
	m_fPalmVNCScaling = false;
	fFTRequest = false;

	// Modif sf@2002 - File Transfer
	m_fFileTransferRunning = FALSE;
	m_pZipUnZip = new CZipUnZip32(); // Directory File Transfer utils

	m_hDestFile = 0;
	//m_szFullDestName = NULL;
	m_dwFileSize = 0;
	m_dwNbPackets = 0;
	m_dwNbReceivedPackets = 0;
	m_dwNbBytesWritten = 0;
	m_dwTotalNbBytesWritten = 0;
	m_fFileDownloadError = false;
	m_fUserAbortedFileTransfer = false;
	m_fFileDownloadRunning = false;
	m_hSrcFile = INVALID_HANDLE_VALUE;
	//m_szSrcFileName = NULL;
	m_fEof = false;
	m_dwNbBytesRead = 0;
	m_dwTotalNbBytesRead = 0;
	m_nPacketCount = 0;
	m_fCompressionEnabled = false;
	m_fFileUploadError = false;
	m_fFileUploadRunning = false;

	// sf@2004 - Delta Transfer
	m_lpCSBuffer = NULL;
	m_nCSOffset = 0;
	m_nCSBufferSize = 0;

	// CURSOR HANDLING
	m_cursor_update_pending = FALSE;
	m_cursor_update_sent = FALSE;
	// nyama/marscha - PointerPos
	m_use_PointerPos = FALSE;
	m_cursor_pos_changed = FALSE;
	m_cursor_pos.x = 0;
	m_cursor_pos.y = 0;

	//cachestats
	totalraw = 0;

	m_pRawCacheZipBuf = NULL;
	m_nRawCacheZipBufSize = 0;
	m_pCacheZipBuf = NULL;
	m_nCacheZipBufSize = 0;

	// sf@2005 - FTUserImpersonation
	m_fFTUserImpersonatedOk = false;
	m_lLastFTUserImpersonationTime = 0L;

	// Modif sf@2002 - Text Chat
	m_pTextChat = new TextChat(this);
	m_fUltraViewer = true;
	m_IsLoopback = false;
	m_NewSWUpdateWaiting = false;
	client_settings_passed = false;
	initialCapture_done = false;
	m_wants_ServerStateUpdates = false;
	m_bClientHasBlockedInput = false;
	m_Support_rfbSetServerInput = false;
	m_wants_KeepAlive = false;
	m_session_supported = false;
	m_fFileSessionOpen = false;
	m_pBuff = 0;
	m_pCompBuff = 0;
	m_NewSWDesktop = 0;
	NewsizeW = 0;
	NewsizeH = 0;

	// adzm 2009-07-05
	m_szRepeaterID = NULL; // as in, not using
	m_szHost = NULL;
	m_hostPort = 0;
	m_want_update_state = false;
	m_initial_update = false;
	m_nScale_viewer = 1;
	nr_incr_rgn_empty = 0;
	ThreadHandleCompressFolder = NULL;
	sendingUpdate = false;
	m_singleExtendMode = false;
	m_firstExtDesktop = true;
	m_firstExtDesktopIncremental = true;
	has_mouse = false;
	ask_mouse = false;
	simulateCursor = NULL;
	forceBlacklist = false;
	desktopUsersToken = NULL;
}

vncClient::~vncClient()
{
	cl_connected = false;
	vnclog.Print(LL_INTINFO, VNCLOG("~vncClient() executing...\n"));

	// Modif sf@2002 - Text Chat
	if (m_pTextChat)
	{
		m_pTextChat->KillDialog();
		delete(m_pTextChat);
		m_pTextChat = NULL;
	}

	// Directory File Transfer utils
	if (m_pZipUnZip) delete m_pZipUnZip;

	// We now know the thread is dead, so we can clean up
	if (m_client_name_address != NULL) {
		free(m_client_name_address);
		m_client_name_address = NULL;
	}
	
	if (m_client_domain_username != NULL) {
		free(m_client_domain_username);
		m_client_domain_username = NULL;
	}
	if (m_client_name_name != NULL) {
		free(m_client_name_name);
		m_client_name_name = NULL;
	}

	// If we have a socket then kill it
	if (m_socket != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("deleting socket\n"));

		delete m_socket;
		m_socket = NULL;
	}
	if (m_pRawCacheZipBuf != NULL)
	{
		delete[] m_pRawCacheZipBuf;
		m_pRawCacheZipBuf = NULL;
	}
	if (m_pCacheZipBuf != NULL)
	{
		delete[] m_pCacheZipBuf;
		m_pCacheZipBuf = NULL;
	}
	if (m_lpCSBuffer)
		delete[] m_lpCSBuffer;
	if (m_pBuff)
		delete[] m_pBuff;
	if (m_pCompBuff)
		delete[] m_pCompBuff;

	//thos give sometimes errors, hlogfile is already removed at this point
	//vnclog.Print(LL_INTINFO, VNCLOG("cached %d \n"),totalraw);

	if ((settings->getScExit() || (settings->RunningFromExternalServiceRdp())) && !fShutdownOrdered) // if fShutdownOrdered, hwnd may not be valid
	{
		//adzm 2009-06-20 - if we are SC, only exit if no other viewers are connected!
		// (since multiple viewers is now allowed with the new DSM plugin)
		// adzm 2009-08-02
		// adzm 2010-08 - stay alive if we have an UnauthClientCount as well, since another connection may be pending
		if ((m_server == NULL) || (m_server && m_server->AuthClientCount() == 0 && m_server->UnauthClientCount() == 0)) {
			// We want that the server exit when the viewer exit
			//adzm 2010-02-10 - Finds the appropriate VNC window for this process
			HWND hwnd = postHelper::FindWinVNCWindow(true);
			if (hwnd) SendMessage(hwnd, WM_COMMAND, ID_CLOSE, 0);
		}
	}

	// adzm 2009-07-05
	if (m_szRepeaterID) {
		free(m_szRepeaterID);
	}
	// adzm 2009-08-02
	if (m_szHost) {
		free(m_szHost);
	}
	if (m_updatethread) {
		m_updatethread->Kill();
		m_updatethread->join(NULL);
	}

	int counter = 0;
	while (m_updatethread != NULL)
	{
		char			szText[256];
		sprintf_s(szText, " m_updatethread != NULL \n");
		OutputDebugString(szText);
		Sleep(100);
		counter++;
		if (counter == 30) break;
	}
	if (ThreadHandleCompressFolder) {
		WaitForSingleObject(ThreadHandleCompressFolder, INFINITE);
		CloseHandle(ThreadHandleCompressFolder);
	}
	if (simulateCursor)
		delete simulateCursor;
	if (desktopUsersToken)
		delete desktopUsersToken;
	desktopUsersToken = NULL;
}

// Init
BOOL
vncClient::Init(vncServer* server,
	VSocket* socket,
	BOOL auth,
	BOOL shared,
	vncClientId newid)
{
	// Save the server id;
	m_server = server;

	// Save the socket
	m_socket = socket;

	// Save the name of the connecting client
	char* name = m_socket->GetPeerName(true);
	if (name != NULL)
		m_client_name_name = _strdup(name);
	else
		m_client_name_name = _strdup("<unknown>");

	char* address = m_socket->GetPeerName(false);
	if (address != NULL)
		m_client_name_address = _strdup(address);
	else
		m_client_name_address = _strdup("<unknown>");

	// Save the client id
	m_id = newid;

	// Spawn the child thread here
	m_thread_ClientThread = new vncClientThread;
	if (m_thread_ClientThread == NULL)
		return FALSE;
	return ((vncClientThread*)m_thread_ClientThread)->Init(this, m_server, m_socket, auth, shared);

	return FALSE;
}

void
vncClient::Kill(bool deleted)
{
	// Close the socket
	vnclog.Print(LL_INTERR, VNCLOG("client Kill() called"));
	if (m_pTextChat)
		m_pTextChat->KillDialog();
	if (m_socket != NULL)
		m_socket->Close();
	if (deleted)
		((vncClientThread*)m_thread_ClientThread)->m_deleted = true;
	if (m_server->virtualDisplay)
		m_server->virtualDisplay->disconnectDisplay(m_id, !m_server->AreThereMultipleViewers() && initialCapture_done);
}

// Client manipulation functions for use by the server
void
vncClient::SetBuffer(vncBuffer* buffer)
{
	// Until authenticated, the client object has no access
	// to the screen buffer. This means that there only need
	// be a buffer when there's at least one authenticated client.
	m_encodemgr.SetBuffer(buffer);
}

//helper to trigger update
bool
vncClient::NotifyUpdate(rfbFramebufferUpdateRequestMsg fur)
{
	rfb::Rect update;
	m_ScaledScreen = m_encodemgr.m_buffer->GetViewerSize();
	rfb::Region2D update_rgn;

	// Extended display with a single screen, block all updates on the other screens when all monitors
	// is selected. This minimize compression and network traffic
	monitor mon = m_encodemgr.m_buffer->m_desktop->devicenaamToPosMap[displayname];
	if (mon.Height && mon.Width && m_encodemgr.m_buffer->m_desktop->m_current_monitor == MULTI_MON_ALL) {
		m_ScaledScreen.tl.x = mon.offsetx / m_nScale;
		m_ScaledScreen.tl.y = mon.offsety / m_nScale;
		m_ScaledScreen.br.x = (mon.Width + mon.offsetx) / m_nScale;
		m_ScaledScreen.br.y = (mon.Height + mon.offsety) / m_nScale;
	}

	if (!fur.incremental) {
		if (m_use_ExtDesktopSize && m_firstExtDesktop) {
			m_NewSWUpdateWaiting = true;
		}
		update.tl.x = (m_ScaledScreen.tl.x + monitor_Offsetx) * m_nScale;
		update.tl.y = (m_ScaledScreen.tl.y + monitor_Offsety) * m_nScale;
		update.br.x = update.tl.x + (m_ScaledScreen.br.x - m_ScaledScreen.tl.x) * m_nScale;
		update.br.y = update.tl.y + (m_ScaledScreen.br.y - m_ScaledScreen.tl.y) * m_nScale;
		update_rgn = update;
		if (update_rgn.is_empty())
			return false;
		m_update_tracker.add_changed(update_rgn);
		m_encodemgr.m_buffer->m_desktop->UpdateFullScreen();
	}
	else {
		if (m_firstExtDesktopIncremental) {
			//The first full was used for the extDesktopSize, we send it now
			update.tl.x = (m_ScaledScreen.tl.x + monitor_Offsetx) * m_nScale;
			update.tl.y = (m_ScaledScreen.tl.y + monitor_Offsety) * m_nScale;
			update.br.x = update.tl.x + (m_ScaledScreen.br.x - m_ScaledScreen.tl.x) * m_nScale;
			update.br.y = update.tl.y + (m_ScaledScreen.br.y - m_ScaledScreen.tl.y) * m_nScale;
			update_rgn = update;
			if (update_rgn.is_empty())
				return false;
			m_update_tracker.add_changed(update_rgn);
			m_encodemgr.m_buffer->m_desktop->UpdateFullScreen();
			m_firstExtDesktopIncremental = false;
		}
		else {
			update.tl.x = (Swap16IfLE(fur.x) + monitor_Offsetx) * m_nScale;
			update.tl.y = (Swap16IfLE(fur.y) + monitor_Offsety) * m_nScale;
			update.br.x = update.tl.x + Swap16IfLE(fur.w) * m_nScale;
			update.br.y = update.tl.y + Swap16IfLE(fur.h) * m_nScale;
			// Verify max size, scaled changed on server while not pushed to viewer
			if (update.tl.x < (int)((m_ScaledScreen.tl.x + monitor_Offsetx) * m_nScale))
				update.tl.x = (m_ScaledScreen.tl.x + monitor_Offsetx) * m_nScale;
			if (update.tl.y < (int)((m_ScaledScreen.tl.y + monitor_Offsety) * m_nScale))
				update.tl.y = (m_ScaledScreen.tl.y + monitor_Offsety) * m_nScale;
			if (update.br.x > (int)(update.tl.x + (m_ScaledScreen.br.x - m_ScaledScreen.tl.x) * m_nScale))
				update.br.x = update.tl.x + (m_ScaledScreen.br.x - m_ScaledScreen.tl.x) * m_nScale;
			if (update.br.y > (int)(update.tl.y + (m_ScaledScreen.br.y - m_ScaledScreen.tl.y) * m_nScale))
				update.br.y = update.tl.y + (m_ScaledScreen.br.y - m_ScaledScreen.tl.y) * m_nScale;
			update_rgn = update;
			if (update_rgn.is_empty())
				return false;
		}
	}
#ifdef _DEBUG
	OutputDevMessage("Update Rect %i %i %i %i", update.tl.x, update.tl.y, update.br.x - update.tl.x, update.br.y - update.tl.y);
	OutputDevMessage("++++++ rfbFramebufferUpdateRequestMsg");
#endif
	m_incr_rgn.assign_union(update_rgn);

	// Kick the update thread (and create it if not there already)
	TriggerUpdate();
	TriggerUpdateThread();
#ifdef _DEBUG
	static DWORD sNotifyLastCopy = GetTickCount();
	DWORD now = GetTickCount();;
	OutputDevMessage("%4d", now - sNotifyLastCopy);
	sNotifyLastCopy = now;
#endif
	return TRUE;
}

void
vncClient::TriggerUpdateThread()
{
	// ALWAYS lock the client UpdateLock before calling this!
	// RealVNC 336
	// Check that this client has an update thread
	// The update thread never dissappears, and this is the only
	// thread allowed to create it, so this can be done without locking.

	if (!m_updatethread)
	{
		if (m_server->AreThereMultipleViewers())
			cl_connected = TRUE;
		m_updatethread = new vncClientUpdateThread;
		if (!m_updatethread ||
			!m_updatethread->Init(this)) {
			Kill();
		}
	}
	if (m_updatethread)
		m_updatethread->Trigger();
}

void
vncClient::UpdateMouse()
{
	RECT testrect{};
	testrect.top = m_encodemgr.m_buffer->m_desktop->m_Cliprect.tl.y + m_ScreenOffsety + monitor_Offsety;
	testrect.bottom = m_encodemgr.m_buffer->m_desktop->m_Cliprect.br.y + m_ScreenOffsety + monitor_Offsety;
	testrect.left = m_encodemgr.m_buffer->m_desktop->m_Cliprect.tl.x + m_ScreenOffsetx + monitor_Offsetx;
	testrect.right = m_encodemgr.m_buffer->m_desktop->m_Cliprect.br.x + m_ScreenOffsetx + monitor_Offsetx;

	//vnclog.Print(LL_INTINFO, VNCLOG("UpdateMouse sx %d sy %d mx %d my %d\n"), m_ScreenOffsetx, m_ScreenOffsety, monitor_Offsetx, monitor_Offsety);
	//vnclog.Print(LL_INTINFO, VNCLOG("UpdateMouse Rect %d %d %d %d\n"), testrect.left, testrect.top, testrect.right, testrect.bottom);

	POINT cursorPos;
	GetCursorPos(&cursorPos);
	if (!PtInRect(&testrect, cursorPos))
	{
		//vnclog.Print(LL_INTINFO, VNCLOG("UpdateMouse !PtInRect %d %d\n"), cursorPos.x, cursorPos.y);
		return;
	}

	if (!m_mousemoved && !m_cursor_update_sent) {
		omni_mutex_lock l(GetUpdateLock(), 93);
		m_mousemoved = TRUE;
	}
	// nyama/marscha - PointerPos
	// PointerPos code doesn take in account prim/secundary display
	// offset needed
	if (m_use_PointerPos && !m_cursor_pos_changed) {
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		cursorPos.x = cursorPos.x - (m_ScreenOffsetx + monitor_Offsetx);
		cursorPos.y = cursorPos.y - (m_ScreenOffsety + monitor_Offsety);
		//vnclog.Print(LL_INTINFO, VNCLOG("UpdateMouse m_cursor_pos(%d, %d), new(%d, %d)\n"),
		//    m_cursor_pos.x, m_cursor_pos.y, cursorPos.x, cursorPos.y);
		if (cursorPos.x != m_cursor_pos.x || cursorPos.y != m_cursor_pos.y) {
			// This movement isn't by this client, but generated locally or by other client.
			// Send it to this client.
			omni_mutex_lock l(GetUpdateLock(), 94);
			m_cursor_pos.x = cursorPos.x;
			m_cursor_pos.y = cursorPos.y;
			m_cursor_pos_changed = TRUE;
			TriggerUpdateThread();
		}
	}
}

// adzm - 2010-07 - Extended clipboard
/*
void
vncClient::UpdateClipText(const char* text)
{
	//This is already locked in the vncdesktopsynk
	//omni_mutex_lock l(GetUpdateLock());
	if (m_clipboard_text) {
		free(m_clipboard_text);
		m_clipboard_text = 0;
	}
	m_clipboard_text = _strdup(text);
	TriggerUpdateThread();
}
*/

// adzm - 2010-07 - Extended clipboard
void
vncClient::UpdateClipTextEx(ClipboardData& clipboardData, CARD32 overrideFlags)
{
	//This is already locked in the vncdesktopsynk
	//But it's just a critical section, doesn't matter how often you lock it in the same thread
	//as long as it is unlocked the same number of times.
	omni_mutex_lock l(GetUpdateLock(), 95);
	if (m_clipboard.UpdateClipTextEx(clipboardData, overrideFlags)) {
		TriggerUpdateThread();
	}
}

void
vncClient::UpdateCursorShape()
{
	//omni_mutex_lock l(GetUpdateLock(),96);
	TriggerUpdateThread();
}

void
vncClient::UpdatePalette(bool lock)
{
	if (lock) omni_mutex_lock l(GetUpdateLock(), 97);
	m_palettechanged = TRUE;
}

void
vncClient::UpdateLocalFormat(bool lock)
{
	if (lock) DisableProtocol();
	else DisableProtocol_no_mutex();
	vnclog.Print(LL_INTERR, VNCLOG("updating local pixel format\n"));
	m_encodemgr.SetServerFormat();
	if (lock) EnableProtocol();
	else EnableProtocol_no_mutex();
}

BOOL
vncClient::SetNewSWSize(long w, long h, BOOL Desktop)
{
	if (!m_use_NewSWSize) return FALSE;
	DisableProtocol_no_mutex();

	vnclog.Print(LL_INTERR, VNCLOG("updating local pixel format and buffer size\n"));
	m_encodemgr.SetServerFormat();
	m_palettechanged = TRUE;
	if (Desktop) m_encodemgr.SetEncoding(0, TRUE);//0=dummy
	m_NewSWUpdateWaiting = true;
	NewsizeW = w;
	NewsizeH = h;
	EnableProtocol_no_mutex();

	return TRUE;
}

const char*
vncClient::GetClientDomainUsername()
{
	return m_client_domain_username;
}

// Functions used to set and retrieve the client settings
const char*
vncClient::GetClientNameName()
{
	return m_client_name_name;
}

const char*
vncClient::GetClientNameAddress()
{
	return m_client_name_address;
}

// Enabling and disabling clipboard/GFX updates
void
vncClient::DisableProtocol()
{
	BOOL disable = FALSE;
	{
		omni_mutex_lock l(GetUpdateLock(), 98);
		if (m_disable_protocol == 0)
			disable = TRUE;
		m_disable_protocol++;
		if (disable && m_updatethread)
			m_updatethread->EnableUpdates(FALSE);
	}
}

void
vncClient::EnableProtocol()
{
	{
		omni_mutex_lock l(GetUpdateLock(), 99);
		if (m_disable_protocol == 0) {
			vnclog.Print(LL_INTERR, VNCLOG("protocol enabled too many times!\n"));
			m_socket->Close();
			return;
		}
		m_disable_protocol--;
		if ((m_disable_protocol == 0) && m_updatethread)
			m_updatethread->EnableUpdates(TRUE);
	}
}

void
vncClient::DisableProtocol_no_mutex()
{
	BOOL disable = FALSE;
	{
		if (m_disable_protocol == 0)
			disable = TRUE;
		m_disable_protocol++;
		if (disable && m_updatethread)
			m_updatethread->EnableUpdates(FALSE);
	}
}

void
vncClient::EnableProtocol_no_mutex()
{
	{
		if (m_disable_protocol == 0) {
			vnclog.Print(LL_INTERR, VNCLOG("protocol enabled too many times!\n"));
			m_socket->Close();
			return;
		}
		m_disable_protocol--;
		if ((m_disable_protocol == 0) && m_updatethread)
			m_updatethread->EnableUpdates(TRUE);
	}
}

// Internal methods
BOOL
vncClient::SendRFBMsg(CARD8 type, BYTE* buffer, int buflen)
{
	// Set the message type
	((rfbServerToClientMsg*)buffer)->type = type;

	// Send the message
	if (!m_socket->SendExact((char*)buffer, buflen, type))
	{
		vnclog.Print(LL_CONNERR, VNCLOG("failed to send RFB message to client\n"));

		Kill();
		return FALSE;
	}
	return TRUE;
}

//adzm 2010-09 - minimize packets. SendExact flushes the queue.
BOOL
vncClient::SendRFBMsgQueue(CARD8 type, BYTE* buffer, int buflen)
{
	// Set the message type
	((rfbServerToClientMsg*)buffer)->type = type;

	// Send the message
	if (!m_socket->SendExactQueue((char*)buffer, buflen, type))
	{
		vnclog.Print(LL_CONNERR, VNCLOG("failed to send RFB message to client\n"));

		Kill();
		return FALSE;
	}
	return TRUE;
}

BOOL
vncClient::SendUpdate(rfb::SimpleUpdateTracker& update)
{
	// If there is nothing to send then exit

	if (update.is_empty() && !m_cursor_update_pending && !m_NewSWUpdateWaiting && !m_cursor_pos_changed)
		return FALSE;

	// Get the update info from the tracker
	rfb::UpdateInfo update_info;
	update.get_update(update_info);
	update.clear();
	//Old update could be outsite the new bounding
	//We first make sure the new size is send to the client
	//The client ask a full update after screen_size change
	if (m_NewSWUpdateWaiting) {
		m_socket->ClearQueue();
		rfbFramebufferUpdateRectHeader hdr{};
		m_ScaledScreen = m_encodemgr.m_buffer->GetViewerSize();
		m_nScale = m_encodemgr.m_buffer->GetScale();
		rfbExtDesktopSizeMsg edsHdr{};
		rfbExtDesktopScreen eds{};

		if (m_use_ExtDesktopSize && m_firstExtDesktop) {
			HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
			DWORD dummy;
			char new_name[256]{};
			if (GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy) &&
				strcmp(new_name, "Default") == 0 && desktopSelector::InputDesktopSelected() != 2) {
				//Send ExtDesktopSize pseudo - rectangle to inform the client we support size changes through setDesktopSize
				hdr.encoding = Swap32IfLE(rfbEncodingExtDesktopSize);
				hdr.r.w = Swap16IfLE(m_encodemgr.m_buffer->m_desktop->m_scrinfo.framebufferWidth * m_nScale_viewer / m_nScale);
				hdr.r.h = Swap16IfLE(m_encodemgr.m_buffer->m_desktop->m_scrinfo.framebufferHeight * m_nScale_viewer / m_nScale);
				hdr.r.x = Swap16IfLE(m_server->m_virtualDisplaySupported);
				hdr.r.y = Swap16IfLE(m_lastDesktopSizeChangeError);
				edsHdr.numberOfScreens = 1;
				edsHdr.pad[0] = edsHdr.pad[1] = edsHdr.pad[2] = 0;
				for (int i = 0; i < 1; i++) {
					eds.id = Swap32IfLE(1);
					eds.x = Swap16IfLE(0);
					eds.y = Swap16IfLE(0);
					eds.width = Swap16IfLE(m_encodemgr.m_buffer->m_desktop->m_scrinfo.framebufferWidth * m_nScale_viewer / m_nScale);
					eds.height = Swap16IfLE(m_encodemgr.m_buffer->m_desktop->m_scrinfo.framebufferHeight * m_nScale_viewer / m_nScale);
					eds.flags = Swap32IfLE(0);
				}
				rfbFramebufferUpdateMsg header{};
				header.nRects = Swap16IfLE(1);
				//adzm 2010-09 - minimize packets. SendExact flushes the queue.
				SendRFBMsgQueue(rfbFramebufferUpdate, (BYTE*)&header, sz_rfbFramebufferUpdateMsg);
				m_socket->SendExact((char*)&hdr, sizeof(hdr));
				m_socket->SendExact((char*)&edsHdr, sizeof(edsHdr));
				m_socket->SendExact((char*)&eds, sizeof(eds));
				m_use_ExtDesktopSize = false;
				m_firstExtDesktop = false;
			}
		}
		else if (m_use_NewSWSize) {
			hdr.r.x = 0;
			hdr.r.y = 0;
			hdr.r.w = Swap16IfLE(NewsizeW * m_nScale_viewer / m_nScale);
			hdr.r.h = Swap16IfLE(NewsizeH * m_nScale_viewer / m_nScale);
			hdr.encoding = Swap32IfLE(rfbEncodingNewFBSize);
			rfbFramebufferUpdateMsg header{};
			header.nRects = Swap16IfLE(1);
			//adzm 2010-09 - minimize packets. SendExact flushes the queue.
			SendRFBMsgQueue(rfbFramebufferUpdate, (BYTE*)&header, sz_rfbFramebufferUpdateMsg);
			m_socket->SendExact((char*)&hdr, sizeof(hdr));
			if (m_singleExtendMode) {
				//send position of the virtual screen
				m_encodemgr.m_buffer->m_desktop->Checkmonitors();
				m_encodemgr.m_buffer->m_desktop->m_current_monitor = MULTI_MON_ALL;
				m_encodemgr.m_buffer->SetAllMonitors(true);
				monitor mon = m_encodemgr.m_buffer->m_desktop->devicenaamToPosMap[displayname];
				hdr.r.x = Swap16IfLE(mon.offsetx * m_nScale_viewer / m_nScale);
				hdr.r.y = Swap16IfLE(mon.offsety * m_nScale_viewer / m_nScale);
				hdr.r.w = Swap16IfLE(mon.Width * m_nScale_viewer / m_nScale);
				hdr.r.h = Swap16IfLE(mon.Height * m_nScale_viewer / m_nScale);
				hdr.encoding = Swap32IfLE(rfbEncodingExtViewSize);
				rfbFramebufferUpdateMsg header{};
				header.nRects = Swap16IfLE(1);
				SendRFBMsgQueue(rfbFramebufferUpdate, (BYTE*)&header, sz_rfbFramebufferUpdateMsg);
				m_socket->SendExact((char*)&hdr, sizeof(hdr));
			}
		}
		m_NewSWUpdateWaiting = false;
		return TRUE;
	}

	// Find out how many rectangles in total will be updated
	// This includes copyrects and changed rectangles split
	// up by codings such as CoRRE.
	int updates = 0;
	int numsubrects = 0;
	updates += (int)update_info.copied.size();
	if (m_encodemgr.IsCacheEnabled())
	{
		if (update_info.cached.size() > 5)
		{
			updates++;
		}
		else
		{
			updates += (int)update_info.cached.size();
			//vnclog.Print(LL_INTERR, "cached %d\n", updates);
		}
	}

	rfb::RectVector::const_iterator i;
	if (updates != 0xFFFF)
	{
		for (i = update_info.changed.begin(); i != update_info.changed.end(); i++)
		{
			// Tight specific (lastrect)
			numsubrects = m_encodemgr.GetNumCodedRects(*i);

			// Skip rest rectangles if an encoder will use LastRect extension.
			if (numsubrects == 0) {
				updates = 0xFFFF;
				break;
			}
			updates += numsubrects;
			//vnclog.Print(LL_INTERR, "changed %d\n", updates);
		}
	}

	// if no cache is supported by the other viewer
	// We need to send the cache as a normal update
	if (!m_encodemgr.IsCacheEnabled() && updates != 0xFFFF)
	{
		for (i = update_info.cached.begin(); i != update_info.cached.end(); i++)
		{
			// Tight specific (lastrect)
			numsubrects = m_encodemgr.GetNumCodedRects(*i);

			// Skip rest rectangles if an encoder will use LastRect extension.
			if (numsubrects == 0) {
				updates = 0xFFFF;
				//break;
			}
			else updates += numsubrects;
			//vnclog.Print(LL_INTERR, "cached2 %d\n", updates);
		}
	}

	//
	if (!m_encodemgr.IsXCursorSupported()) m_cursor_update_pending = false;
	// Tight specific (lastrect)
	if (updates != 0xFFFF)
	{
		// Tight - CURSOR HANDLING
		if (m_cursor_update_pending)
		{
			updates++;
		}
		// nyama/marscha - PointerPos
		if (m_cursor_pos_changed)
			updates++;
		if (updates == 0) return FALSE;
	}

	//	Sendtimer.start();

	omni_mutex_lock l(GetUpdateLock(), 100);
	// Otherwise, send <number of rectangles> header
	rfbFramebufferUpdateMsg header{};
	header.nRects = Swap16IfLE(updates);
	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	if (!SendRFBMsgQueue(rfbFramebufferUpdate, (BYTE*)&header, sz_rfbFramebufferUpdateMsg))
		return FALSE;

	// CURSOR HANDLING
	if (m_cursor_update_pending) {
		if (!SendCursorShapeUpdate())
			return FALSE;
	}
	// nyama/marscha - PointerPos
	if (m_cursor_pos_changed)
		if (!SendCursorPosUpdate())
			return FALSE;

	// Send the copyrect rectangles
	if (!update_info.copied.empty()) {
		rfb::Point to_src_delta = update_info.copy_delta.negate();
		for (i = update_info.copied.begin(); i != update_info.copied.end(); i++) {
			rfb::Point src = (*i).tl.translate(to_src_delta);
			if (!SendCopyRect(*i, src))
				return FALSE;
		}
	}

	if (m_encodemgr.IsCacheEnabled())
	{
		if (update_info.cached.size() > 5)
		{
			if (!SendCacheZip(update_info.cached))
				return FALSE;
		}
		else
		{
			if (!SendCacheRectangles(update_info.cached))
				return FALSE;
		}
	}
	else
	{
		if (!SendRectangles(update_info.cached))
			return FALSE;
	}

	if (!SendRectangles(update_info.changed))
		return FALSE;
	// Tight specific - Send LastRect marker if needed.
	if (updates == 0xFFFF)
	{
		m_encodemgr.LastRect(m_socket);
		if (!SendLastRect())
			return FALSE;
	}
	m_socket->ClearQueue();
	// vnclog.Print(LL_INTINFO, VNCLOG("Update cycle\n"));
	return TRUE;
}

// Send a set of rectangles
BOOL
vncClient::SendRectangles(const rfb::RectVector& rects)
{
	rfb::RectVector::const_iterator i;
	rfb::Rect rect;
	//int x,y;
	//int Blocksize=1920;
	//int BlocksizeX=1200;

#ifdef _XZ
	if (m_encodemgr.IsBulkRectEncoding()) {
		return m_encodemgr.EncodeBulkRects(rects, m_nScale, m_socket);
	}
#endif

	// Work through the list of rectangles, sending each one
	for (i = rects.begin(); i != rects.end(); i++) {
		rect.tl.x = (*i).tl.x;
		rect.br.x = (*i).br.x;
		rect.tl.y = (*i).tl.y;
		rect.br.y = (*i).br.y;
		/*#ifdef _DEBUG
					char			szText[256];

						sprintf_s(szText,"SendRectangles  %i %i %i %i \n",rect.tl.x,
						rect.tl.y,
						rect.br.x,
						rect.br.y);
						OutputDebugString(szText);
		#endif*/

		/*if (m_encodemgr.ultra2_encoder_in_use)
		{
			if ((rect.br.x-rect.tl.x) * (rect.br.y-rect.tl.y) > Blocksize*BlocksizeX )
			{
			for (y = rect.tl.y; y < rect.br.y; y += Blocksize)
			{
				int blockbottom = std::min(y + Blocksize, rect.br.y);
				for (x = rect.tl.x; x < rect.br.x; x += BlocksizeX)
					{
					   int blockright = std::min(x+BlocksizeX, rect.br.x);
					   rfb::Rect tilerect;
					   tilerect.tl.x=x;
					   tilerect.br.x=blockright;
					   tilerect.tl.y=y;
					   tilerect.br.y=blockbottom;
					   if (!SendRectangle(tilerect)) return FALSE;
					}
			}
			}
			else
			{
				if (!SendRectangle(rect)) return FALSE;
			}
		}
		else*/
		{
			if (!SendRectangle(*i)) return FALSE;
		}
	}
	return TRUE;
}

// Tell the encoder to send a single rectangle
BOOL
vncClient::SendRectangle(const rfb::Rect& rect)
{
	// Get the buffer to encode the rectangle
	// Modif sf@2002 - Scaling
	rfb::Rect ScaledRect;
	ScaledRect.tl.y = rect.tl.y / m_nScale;
	ScaledRect.br.y = rect.br.y / m_nScale;
	ScaledRect.tl.x = rect.tl.x / m_nScale;
	ScaledRect.br.x = rect.br.x / m_nScale;
	/*#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText,"++++++++++++++++++++++++++++++++++++++++++++++REct1 %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
	OutputDebugString(szText);
	#endif*/

	//	Totalsend+=(ScaledRect.br.x-ScaledRect.tl.x)*(ScaledRect.br.y-ScaledRect.tl.y);

	// sf@2002 - DSMPlugin
	// Some encoders (Hextile, ZRLE, Raw..) store all the data to send into
	// m_clientbuffer and return the total size from EncodeRect()
	// Some Encoders (Tight, Zlib, ZlibHex..) send data on the fly and return
	// a partial size from EncodeRect().
	// On the viewer side, the data is read piece by piece or in one shot
	// still depending on the encoding...
	// It is not compatible with DSM: we need to read/write data blocks of same
	// size on both sides in one shot
	// We create a common method to send the data
	// adzm 2010-09
	if (!m_socket->IsPluginStreamingOut() && m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled())
	{
		// Tell the SendExact() calls to write into the local NetRectBuffer memory buffer
		m_socket->SetWriteToNetRectBuffer(true);
		m_socket->SetNetRectBufOffset(0);
		// sf@2003 - we can't easely predict how many rects are going to be sent
		// (for Tight encoding for instance)
		// Then we take the worse case (screen buffer size * 1.5) for the net rect buffer size.
		// m_socket->CheckNetRectBufferSize((int)(m_encodemgr.GetClientBuffSize() * 2));
		m_socket->CheckNetRectBufferSize((int)(m_encodemgr.m_buffer->m_desktop->ScreenBuffSize() * 3 / 2));
		UINT bytes = m_encodemgr.EncodeRect(ScaledRect, m_socket);
		if (bytes == 0)
		{
			return true;
		}
		m_socket->SetWriteToNetRectBuffer(false);

		BYTE* pDataBuffer = NULL;
		UINT TheSize = 0;

		// If SendExact() was called from inside the encoder
		if (m_socket->GetNetRectBufOffset() > 0)
		{
			TheSize = m_socket->GetNetRectBufOffset();
#ifdef _DEBUG
			char			szText[256];
			DWORD error = GetLastError();
			sprintf_s(szText, " ++++++ crashtest TheSize %i \n", TheSize);
			SetLastError(0);
			OutputDebugString(szText);
#endif
			m_socket->SetNetRectBufOffset(0);
			pDataBuffer = m_socket->GetNetRectBuf();
			// Add the rest to the data buffer if it exists
			if (bytes > 0)
			{
				memcpy(pDataBuffer + TheSize, m_encodemgr.GetClientBuffer(), bytes);
			}
		}
		else // If all data was stored in m_clientbuffer
		{
			TheSize = bytes;
#ifdef _DEBUG
			char			szText[256];
			DWORD error = GetLastError();
			sprintf_s(szText, " ++++++ crashtest TheSize2 %i \n", TheSize);
			SetLastError(0);
			OutputDebugString(szText);
#endif
			bytes = 0;
			pDataBuffer = m_encodemgr.GetClientBuffer();
		}

		// Send the header
		m_socket->SendExactQueue((char*)pDataBuffer, sz_rfbFramebufferUpdateRectHeader);

		// Send the size of the following rects data buffer
		CARD32 Size = (CARD32)(TheSize + bytes - sz_rfbFramebufferUpdateRectHeader);
#ifdef _DEBUG
		char			szText[256];
		DWORD error = GetLastError();
		sprintf_s(szText, " ++++++ crashtest Size %i %i\n", Size, bytes);
		SetLastError(0);
		OutputDebugString(szText);
#endif
		Size = Swap32IfLE(Size);
		m_socket->SendExactQueue((char*)&Size, sizeof(CARD32));
		// Send the data buffer
		m_socket->SendExactQueue(((char*)pDataBuffer + sz_rfbFramebufferUpdateRectHeader),
			TheSize + bytes - sz_rfbFramebufferUpdateRectHeader
		);
	}
	else // Normal case - No DSM - Symetry is not important
	{
		UINT bytes = m_encodemgr.EncodeRect(ScaledRect, m_socket);

		// if (bytes == 0) return false; // From realvnc337. No! Causes viewer disconnections/

		// Send the encoded data
		return m_socket->SendExactQueue((char*)(m_encodemgr.GetClientBuffer()), bytes);
	}

	return true;
}

// Send a single CopyRect message
BOOL
vncClient::SendCopyRect(const rfb::Rect& dest, const rfb::Point& source)
{
	// Create the message header
	// Modif sf@2002 - Scaling
	rfbFramebufferUpdateRectHeader copyrecthdr{};
	copyrecthdr.r.x = Swap16IfLE((dest.tl.x - monitor_Offsetx) / m_nScale);
	copyrecthdr.r.y = Swap16IfLE((dest.tl.y - monitor_Offsety) / m_nScale);
	copyrecthdr.r.w = Swap16IfLE((dest.br.x - dest.tl.x) / m_nScale);
	copyrecthdr.r.h = Swap16IfLE((dest.br.y - dest.tl.y) / m_nScale);
	copyrecthdr.encoding = Swap32IfLE(rfbEncodingCopyRect);

	// Create the CopyRect-specific section
	rfbCopyRect copyrectbody{};
	copyrectbody.srcX = Swap16IfLE((source.x - monitor_Offsetx) / m_nScale);
	copyrectbody.srcY = Swap16IfLE((source.y - monitor_Offsety) / m_nScale);

	// Now send the message;
	if (!m_socket->SendExactQueue((char*)&copyrecthdr, sizeof(copyrecthdr)))
		return FALSE;
	if (!m_socket->SendExactQueue((char*)&copyrectbody, sizeof(copyrectbody)))
		return FALSE;

	return TRUE;
}

// Send the encoder-generated palette to the client
// This function only returns FALSE if the SendExact fails - any other
// error is coped with internally...
BOOL
vncClient::SendPalette()
{
	rfbSetColourMapEntriesMsg setcmap{};
	RGBQUAD* rgbquad;
	UINT ncolours = 256;

	// Reserve space for the colour data
	rgbquad = new RGBQUAD[ncolours];
	if (rgbquad == NULL)
		return TRUE;

	// Get the data
	if (!m_encodemgr.GetPalette(rgbquad, ncolours))
	{
		delete[] rgbquad;
		return TRUE;
	}

	// Compose the message
	setcmap.type = rfbSetColourMapEntries;
	setcmap.firstColour = Swap16IfLE(0);
	setcmap.nColours = Swap16IfLE(ncolours);

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	if (!m_socket->SendExactQueue((char*)&setcmap, sz_rfbSetColourMapEntriesMsg, rfbSetColourMapEntries))
	{
		delete[] rgbquad;
		return FALSE;
	}

	// Now send the actual colour data...
	for (UINT i = 0; i < ncolours; i++)
	{
		struct _PIXELDATA {
			CARD16 r, g, b;
		} pixeldata{};

		pixeldata.r = Swap16IfLE(((CARD16)rgbquad[i].rgbRed) << 8);
		pixeldata.g = Swap16IfLE(((CARD16)rgbquad[i].rgbGreen) << 8);
		pixeldata.b = Swap16IfLE(((CARD16)rgbquad[i].rgbBlue) << 8);

		//adzm 2010-09 - minimize packets. SendExact flushes the queue.
		if (!m_socket->SendExactQueue((char*)&pixeldata, sizeof(pixeldata)))
		{
			delete[] rgbquad;
			return FALSE;
		}
	}

	// Delete the rgbquad data
	delete[] rgbquad;

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	m_socket->ClearQueue();

	return TRUE;
}

void
vncClient::SetBufferOffset(int x, int y)
{
	monitor_Offsetx = x;
	monitor_Offsety = y;
	m_encodemgr.SetBufferOffset(x, y);
}

void
vncClient::SetScreenOffset(int x, int y, bool single_display)
{
	m_ScreenOffsetx = x;
	m_ScreenOffsety = y;
	m_single_display = single_display;
}

void
vncClient::InitialUpdate(bool value)
{
	m_initial_update = value;
}

// CACHE RDV
// Send a set of rectangles
BOOL
vncClient::SendCacheRectangles(const rfb::RectVector& rects)
{
	//	rfb::Rect rect;
	rfb::RectVector::const_iterator i;

	if (rects.size() == 0) return TRUE;
	vnclog.Print(LL_INTINFO, VNCLOG("******** Sending %d Cache Rects \r\n"), rects.size());

	// Work through the list of rectangles, sending each one
	for (i = rects.begin(); i != rects.end(); i++)
	{
		if (!SendCacheRect(*i))
			return FALSE;
	}

	return TRUE;
}

// Tell the encoder to send a single rectangle
BOOL
vncClient::SendCacheRect(const rfb::Rect& dest)
{
	// Create the message header
	// Modif rdv@2002 - v1.1.x - Application Resize
	// Modif sf@2002 - Scaling
	rfbFramebufferUpdateRectHeader cacherecthdr{};
	cacherecthdr.r.x = Swap16IfLE((dest.tl.x - monitor_Offsetx) / m_nScale);
	cacherecthdr.r.y = Swap16IfLE((dest.tl.y - monitor_Offsety) / m_nScale);
	cacherecthdr.r.w = Swap16IfLE((dest.br.x - dest.tl.x) / m_nScale);
	cacherecthdr.r.h = Swap16IfLE((dest.br.y - dest.tl.y) / m_nScale);
	cacherecthdr.encoding = Swap32IfLE(rfbEncodingCache);

	totalraw += (dest.br.x - dest.tl.x) * (dest.br.y - dest.tl.y) * 32 / 8; // 32-bit test
	// Create the CopyRect-specific section
	rfbCacheRect cacherectbody{};
	cacherectbody.special = Swap16IfLE(9999); //not used dummy

	// Now send the message;
	if (!m_socket->SendExactQueue((char*)&cacherecthdr, sizeof(cacherecthdr)))
		return FALSE;
	if (!m_socket->SendExactQueue((char*)&cacherectbody, sizeof(cacherectbody)))
		return FALSE;
	return TRUE;
}

BOOL
vncClient::SendCursorShapeUpdate()
{
	m_cursor_update_pending = FALSE;

	if (!m_encodemgr.SendCursorShape(m_socket)) {
		m_cursor_update_sent = FALSE;
		return m_encodemgr.SendEmptyCursorShape(m_socket);
	}

	m_cursor_update_sent = TRUE;
	return TRUE;
}

BOOL
vncClient::SendCursorPosUpdate()
{
	m_cursor_pos_changed = FALSE;

	rfbFramebufferUpdateRectHeader hdr{};
	hdr.r.x = Swap16IfLE(m_cursor_pos.x);
	hdr.r.y = Swap16IfLE(m_cursor_pos.y);
	hdr.r.w = 0;
	hdr.r.h = 0;
	hdr.encoding = Swap32IfLE(rfbEncodingPointerPos);

	if (!m_socket->SendExactQueue((char*)&hdr, sizeof(hdr)))
		return FALSE;

	return TRUE;
}

// Tight specific - Send LastRect marker indicating that there are no more rectangles to send
BOOL
vncClient::SendLastRect()
{
	// Create the message header
	rfbFramebufferUpdateRectHeader hdr{};
	hdr.r.x = 0;
	hdr.r.y = 0;
	hdr.r.w = 0;
	hdr.r.h = 0;
	hdr.encoding = Swap32IfLE(rfbEncodingLastRect);

	// Now send the message;
	if (!m_socket->SendExactQueue((char*)&hdr, sizeof(hdr)))
		return FALSE;

	return TRUE;
}

//
// sf@2002 - New cache rects transport - Uses Zlib
//
//
BOOL vncClient::SendCacheZip(const rfb::RectVector& rects)
{
	//int totalCompDataLen = 0;

	int nNbCacheRects = (int)rects.size();
	if (!nNbCacheRects) return true;
	unsigned long rawDataSize = nNbCacheRects * sz_rfbRectangle;
	unsigned long maxCompSize = (rawDataSize + (rawDataSize / 100) + 8);

	// Check RawCacheZipBuff
	// create a space big enough for the Zlib encoded cache rects list
	if (m_nRawCacheZipBufSize < rawDataSize)
	{
		if (m_pRawCacheZipBuf != NULL)
		{
			delete[] m_pRawCacheZipBuf;
			m_pRawCacheZipBuf = NULL;
		}
		m_pRawCacheZipBuf = new BYTE[rawDataSize + 1];
		if (m_pRawCacheZipBuf == NULL)
			return false;
		m_nRawCacheZipBufSize = rawDataSize;
	}

	// Copy all the cache rects coordinates into the RawCacheZip Buffer
	rfbRectangle theRect{};
	rfb::RectVector::const_iterator i;
	BYTE* p = m_pRawCacheZipBuf;
	for (i = rects.begin(); i != rects.end(); i++)
	{
		theRect.x = Swap16IfLE(((*i).tl.x - monitor_Offsetx) / m_nScale);
		theRect.y = Swap16IfLE(((*i).tl.y - monitor_Offsety) / m_nScale);
		theRect.w = Swap16IfLE(((*i).br.x - (*i).tl.x) / m_nScale);
		theRect.h = Swap16IfLE(((*i).br.y - (*i).tl.y) / m_nScale);
		memcpy(p, (BYTE*)&theRect, sz_rfbRectangle);
		p += sz_rfbRectangle;
	}

	// Create a space big enough for the Zlib encoded cache rects list
	if (m_nCacheZipBufSize < maxCompSize)
	{
		if (m_pCacheZipBuf != NULL)
		{
			delete[] m_pCacheZipBuf;
			m_pCacheZipBuf = NULL;
		}
		m_pCacheZipBuf = new BYTE[maxCompSize + 1000];
		if (m_pCacheZipBuf == NULL) return 0;
		m_nCacheZipBufSize = maxCompSize + 999;
	}

	int nRet = compress((unsigned char*)(m_pCacheZipBuf),
		(unsigned long*)&maxCompSize,
		(unsigned char*)m_pRawCacheZipBuf,
		rawDataSize
	);

	if (nRet != 0)
	{
		return false;
	}

	vnclog.Print(LL_INTINFO, VNCLOG("*** Sending CacheZip Rects=%d Size=%d (%d)\r\n"), nNbCacheRects, maxCompSize, nNbCacheRects * 14);

	// Send the Update Rect header
	rfbFramebufferUpdateRectHeader CacheRectsHeader{};
	CacheRectsHeader.r.x = Swap16IfLE(nNbCacheRects);
	CacheRectsHeader.r.y = 0;
	CacheRectsHeader.r.w = 0;
	CacheRectsHeader.r.h = 0;
	CacheRectsHeader.encoding = Swap32IfLE(rfbEncodingCacheZip);

	// Format the ZlibHeader
	rfbZlibHeader CacheZipHeader{};
	CacheZipHeader.nBytes = Swap32IfLE(maxCompSize);

	// Now send the message
	if (!m_socket->SendExactQueue((char*)&CacheRectsHeader, sizeof(CacheRectsHeader)))
		return FALSE;
	if (!m_socket->SendExactQueue((char*)&CacheZipHeader, sizeof(CacheZipHeader)))
		return FALSE;
	if (!m_socket->SendExactQueue((char*)m_pCacheZipBuf, maxCompSize))
		return FALSE;

	return TRUE;
}

//
//
//
void vncClient::EnableCache(BOOL enabled)
{
	m_encodemgr.EnableCache(enabled);
}

void vncClient::SetProtocolVersion(rfbProtocolVersionMsg* protocolMsg)
{
	if (protocolMsg != NULL) memcpy(ProtocolVersionMsg, protocolMsg, sz_rfbProtocolVersionMsg);
	else strcpy_s(ProtocolVersionMsg, "0.0.0.0");
}

void vncClient::Clear_Update_Tracker()
{
	m_update_tracker.clear();
}

void vncClient::TriggerUpdate()
{
	m_encodemgr.m_buffer->m_desktop->TriggerUpdate();
}

////////////////////////////////////////////////
// Asynchronous & Delta File Transfer functions
////////////////////////////////////////////////

//
// sf@2004 - Delta Transfer
// Create the checksums buffer of an open file
//
int vncClient::GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize)
{
	bool fEof = false;
	bool fError = false;
	DWORD dwNbBytesRead = 0;
	int nCSBufferOffset = 0;

	char* lpBuffer = new char[sz_rfbBlockSize];
	if (lpBuffer == NULL)
		return -1;

	while (!fEof)
	{
		int nRes = ReadFile(hFile, lpBuffer, sz_rfbBlockSize, &dwNbBytesRead, NULL);
		if (!nRes && dwNbBytesRead != 0)
			fError = true;

		if (nRes && dwNbBytesRead == 0)
			fEof = true;
		else
		{
			unsigned long cs = adler32(0L, Z_NULL, 0);
			cs = adler32(cs, (unsigned char*)lpBuffer, (int)dwNbBytesRead);

			memcpy(lpCSBuffer + nCSBufferOffset, &cs, 4);
			nCSBufferOffset += 4;
		}
	}

	SetFilePointer(hFile, 0L, NULL, FILE_BEGIN);
	delete[] lpBuffer;

	if (fError)
	{
		return -1;
	}

	return nCSBufferOffset;
}

//
// sf@2004 - Delta Transfer
// Destination file already exists
// The server sends the checksums of this file in one shot.
//
bool vncClient::ReceiveDestinationFileChecksums(int nSize, int nLen)
{
	if (nLen < 0 || nLen > 104857600) // 100 MBytes max
		return false;

	m_lpCSBuffer = new char[nLen + 1];
	if (m_lpCSBuffer == NULL)
	{
		return false;
	}

	memset(m_lpCSBuffer, '\0', nLen + 1);

	VBool res = m_socket->ReadExact((char*)m_lpCSBuffer, nLen);
	m_nCSBufferSize = nLen;

	return res == VTrue;
}

//
//
//
bool vncClient::ReceiveFileChunk(int nLen, int nSize)
{
	bool connected = true;

	if (!m_fFileDownloadRunning)
		return  connected;

	if (m_fFileDownloadError)
	{
		FinishFileReception();
		return connected;
	}

	if (nLen < 0)
		return false;

	if (nLen > sz_rfbBlockSize) return connected;

	bool fCompressed = true;
	BOOL fRes = true;
	bool fAlreadyHere = (nSize == 2);

	// sf@2004 - Delta Transfer - Empty packet
	if (fAlreadyHere)
	{
		DWORD dwPtr = SetFilePointer(m_hDestFile, nLen, NULL, FILE_CURRENT);
		if (dwPtr == INVALID_SET_FILE_POINTER)
			fRes = false;
	}
	else
	{
		connected = m_socket->ReadExact((char*)m_pBuff, nLen) == VTrue;
		if (connected)
		{
			if (nSize == 0) fCompressed = false;
			unsigned int nRawBytes = sz_rfbBlockSize;

			if (fCompressed)
			{
				// Decompress incoming data
				int nRet = uncompress((unsigned char*)m_pCompBuff,	// Dest
					(unsigned long*)&nRawBytes, // Dest len
					(const unsigned char*)m_pBuff,// Src
					nLen	// Src len
				);
				if (nRet != 0)
				{
					m_fFileDownloadError = true;
					FinishFileReception();
					return connected;
				}
			}

			fRes = WriteFile(m_hDestFile,
				fCompressed ? m_pCompBuff : m_pBuff,
				fCompressed ? nRawBytes : nLen,
				&m_dwNbBytesWritten,
				NULL);
		}
		else
		{
			m_fFileDownloadError = true;
			// FlushFileBuffers(m_client->m_hDestFile);
			FinishFileReception();
		}
	}

	if (!fRes)
	{
		// TODO : send an explicit error msg to the client...
		m_fFileDownloadError = true;
		FinishFileReception();
		return connected;
	}

	m_dwTotalNbBytesWritten += (fAlreadyHere ? nLen : m_dwNbBytesWritten);
	m_dwNbReceivedPackets++;

	return connected;
}

void vncClient::FinishFileReception()
{
	if (!m_fFileDownloadRunning)
		return;

	m_fFileDownloadRunning = false;
	m_socket->SetRecvTimeout(settings->getIdleTimeout() * 1000);

	// sf@2004 - Delta transfer
	SetEndOfFile(m_hDestFile);

	// if error ?
	FlushFileBuffers(m_hDestFile);

	// Set the DestFile Time Stamp
	if (strlen(m_szFileTime))
	{
		FILETIME DestFileTime;
		SYSTEMTIME FileTime{};
		FileTime.wMonth = atoi(m_szFileTime);
		FileTime.wDay = atoi(m_szFileTime + 3);
		FileTime.wYear = atoi(m_szFileTime + 6);
		FileTime.wHour = atoi(m_szFileTime + 11);
		FileTime.wMinute = atoi(m_szFileTime + 14);
		FileTime.wMilliseconds = 0;
		FileTime.wSecond = 0;
		SystemTimeToFileTime(&FileTime, &DestFileTime);
		// ToDo: hook error
		SetFileTime(m_hDestFile, &DestFileTime, &DestFileTime, &DestFileTime);
	}

	// CleanUp
	helper::close_handle(m_hDestFile);

	// sf@2004 - Delta Transfer : we can keep the existing file data :)
	// if (m_fFileDownloadError) DeleteFile(m_szFullDestName);

	// sf@2003 - Directory Transfer trick
	// If the file is an Ultra Directory Zip we unzip it here and we delete the
	// received file
	// Todo: make a better free space check above in this particular case. The free space must be at least
	// 3 times the size of the directory zip file (this zip file is ~50% of the real directory size)
	bool bWasDir = UnzipPossibleDirectory(m_szFullDestName);
	/*
	if (!m_fFileDownloadError && !strncmp(strrchr(m_szFullDestName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)))
	{
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH]; // Todo: improve this (size)
		strcpy_s(szPath, m_szFullDestName);
		// Todo: improve all this (p, p2, p3 NULL test or use a standard substring extraction function)
		char *p = strrchr(szPath, '\\') + 1;
		char *p2 = strchr(p, '-') + 1; // rfbZipDirectoryPrefix MUST have a "-" at the end...
		strcpy_s(szDirName, p2);
		char *p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat_s(szPath, szDirName);

		// Create the Directory
		// BOOL fRet = CreateDirectory(szPath, NULL);
		m_pZipUnZip->UnZipDirectory(szPath, m_szFullDestName);
		DeleteFile(m_szFullDestName);
	}
	*/

	if (m_fFileDownloadError && m_fUserAbortedFileTransfer)
	{
		SplitTransferredFileNameAndDate(m_szFullDestName, 0);
		::DeleteFile(m_szFullDestName);
		FTDownloadCancelledHook();
	}
	else
	{
		std::string realName = get_real_filename(m_szFullDestName);
		if (!m_fFileDownloadError && !bWasDir)
			m_fFileDownloadError = !replaceFile(m_szFullDestName, realName.c_str());
		if (m_fFileDownloadError)
			FTDownloadFailureHook();
		else
			FTDownloadCompleteHook();
	}
	//delete [] m_szFullDestName;

	if (m_pCompBuff != NULL)
	{
		delete[] m_pCompBuff;
		m_pCompBuff = NULL;
	}
	if (m_pBuff != NULL)
	{
		delete[] m_pBuff;
		m_pBuff = NULL;
	}
	if (m_lpCSBuffer)
	{
		delete[] m_lpCSBuffer;
		m_lpCSBuffer = NULL;
	}

	return;
}

bool vncClient::SendFileChunk()
{
	bool connected = true;
	do
	{
		connected = true;
		omni_mutex_lock l(GetUpdateLock(), 101);

		if (!m_fFileUploadRunning) return connected;
		if (m_fEof || m_fFileUploadError)
		{
			FinishFileSending();
			return connected;
		}

		int nRes = ReadFile(m_hSrcFile, m_pBuff, sz_rfbBlockSize, &m_dwNbBytesRead, NULL);
		if (!nRes && m_dwNbBytesRead != 0)
		{
			m_fFileUploadError = true;
		}

		if (nRes && m_dwNbBytesRead == 0)
		{
			m_fEof = true;
		}
		else
		{
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
						cs = adler32(cs, (unsigned char*)m_pBuff, (int)m_dwNbBytesRead);
						if (cs == nCS)
							fAlreadyThere = true;
					}
				}
			}

			if (fAlreadyThere)
			{
				// Send the FileTransferMsg with empty rfbFilePacket
				rfbFileTransferMsg ft{};
				ft.type = rfbFileTransfer;
				ft.contentType = rfbFilePacket;
				ft.size = Swap32IfLE(2); // Means "Empty packet"// Swap32IfLE(nCS);
				ft.length = Swap32IfLE(m_dwNbBytesRead);
				m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
			}
			else
			{
				// Compress the data
				// (Compressed data can be longer if it was already compressed)
				unsigned int nMaxCompSize = sz_rfbBlockSize + 1024; // TODO: Improve this...
				bool fCompressed = false;
				if (m_fCompressionEnabled)
				{
					int nRetC = compress((unsigned char*)(m_pCompBuff),
						(unsigned long*)&nMaxCompSize,
						(unsigned char*)m_pBuff,
						m_dwNbBytesRead
					);

					if (nRetC != 0)
					{
						vnclog.Print(LL_INTINFO, VNCLOG("Compress returned error in File Send :%d\n"), nRetC);
						// Todo: send data uncompressed instead
						m_fFileUploadError = true;
						FinishFileSending();
						return connected;
					}
					fCompressed = true;
				}

				// Test if we have to deal with already compressed data
				if (nMaxCompSize > m_dwNbBytesRead)
					fCompressed = false;
				// m_fCompressionEnabled = false;

				rfbFileTransferMsg ft{};

				ft.type = rfbFileTransfer;
				ft.contentType = rfbFilePacket;
				ft.size = fCompressed ? Swap32IfLE(1) : Swap32IfLE(0);
				ft.length = fCompressed ? Swap32IfLE(nMaxCompSize) : Swap32IfLE(m_dwNbBytesRead);

				//adzm 2010-09 - minimize packets. SendExact flushes the queue.
				connected = VFalse != m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
				if (connected) {
					if (fCompressed)
						connected = VFalse != m_socket->SendExact((char*)m_pCompBuff, nMaxCompSize);
					else
						connected = VFalse != m_socket->SendExact((char*)m_pBuff, m_dwNbBytesRead);
				}
			}

			m_dwTotalNbBytesRead += m_dwNbBytesRead;
			// TODO : test on nb of bytes written
		}

		/*if (connected)
		{
			// Order next asynchronous packet sending
			PostToWinVNC( FileTransferSendPacketMessage, (WPARAM)this, (LPARAM)0);
		}*/
	} while (connected);

	return connected;
}

void vncClient::FinishFileSending()
{
	omni_mutex_lock l(GetUpdateLock(), 102);

	if (!m_fFileUploadRunning)
		return;

	// restore original timeout
	m_socket->SetSendTimeout(settings->getIdleTimeout() * 1000);

	m_fFileUploadRunning = false;

	rfbFileTransferMsg ft{};

	// File Copy OK
	if (!m_fFileUploadError /*nRet == 1*/)
	{
		ft.type = rfbFileTransfer;
		ft.contentType = rfbEndOfFile;
		m_socket->SendExact((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	}
	else // Error in file copy
	{
		// TODO : send an error msg to the client...
		ft.type = rfbFileTransfer;
		ft.contentType = rfbAbortFileTransfer;
		m_socket->SendExact((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	}

	helper::close_handle(m_hSrcFile);
	if (m_pBuff != NULL)
	{
		delete[] m_pBuff;
		m_pBuff = NULL;
	}
	if (m_pCompBuff != NULL)
	{
		delete[] m_pCompBuff;
		m_pCompBuff = NULL;
	}

	// sf@2003 - Directory Transfer trick
	// If the transfered file is a Directory zip, we delete it locally, whatever the result of the transfer
	if (!strncmp(strrchr(m_szSrcFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)))
	{
		char* p = strrchr(m_szSrcFileName, ',');
		if (p != NULL) *p = '\0'; // Remove the time stamp we've added above from the file name
		DeleteFile(m_szSrcFileName);
	}
	//  jdp 8/8/2008
	if (m_fUserAbortedFileTransfer)
		FTUploadCancelledHook();
	else if (m_fFileUploadError)
		FTUploadFailureHook();
	else
		FTUploadCompleteHook();
}

bool vncClient::GetSpecialFolderPath(int nId, char* szPath)
{
	LPITEMIDLIST pidl;
	bool retval = false;

	LPMALLOC pSHMalloc;

	if (FAILED(SHGetMalloc(&pSHMalloc)))
		return false;

	if (SHGetSpecialFolderLocation(0, nId, &pidl) == NOERROR)
	{
		retval = SHGetPathFromIDList(pidl, szPath) ? true : false;

		pSHMalloc->Free(pidl);
	}

	pSHMalloc->Release();

	return retval;
}

//
// Zip a possible directory
//
int vncClient::ZipPossibleDirectory(LPSTR szSrcFileName)
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
		::GetTempPath(MAX_PATH, szWorkingDir); //PGM Use Windows Temp folder
		if (m_fFTUserImpersonatedOk)
			strcpy_s(szWorkingDir, m_szTempDir);
		if (szWorkingDir == NULL) //PGM
		{ //PGM
			if (GetModuleFileName(NULL, szWorkingDir, MAX_PATH))
			{
				char* p = strrchr(szWorkingDir, '\\');
				if (p == NULL)
					return -1;
				*(p + 1) = '\0';
			}
			else
			{
				return -1;
			}
		}//PGM

		char szPath[MAX_PATH];
		char szDirectoryName[MAX_PATH];
		strcpy_s(szPath, szSrcFileName);
		p1 = strrchr(szPath, '\\') + 1;
		strcpy_s(szDirectoryName, p1 + 2); // Skip dir prefix (2 chars)
		szDirectoryName[strlen(szDirectoryName) - 2] = '\0'; // Remove dir suffix (2 chars)
		*p1 = '\0';
		m_OrigSourceDirectoryName = std::string(szPath) + szDirectoryName;
		if ((strlen(szPath) + strlen(rfbZipDirectoryPrefix) + strlen(szDirectoryName) + 4) > (MAX_PATH - 1)) return -1;
		sprintf_s(szDirZipPath, "%s%s%s%s", szWorkingDir, rfbZipDirectoryPrefix, szDirectoryName, ".zip");
		strcat_s(szPath, szDirectoryName);
		strcpy_s(szDirectoryName, szPath);
		if (strlen(szDirectoryName) > (MAX_PATH - 4)) return -1;
		strcat_s(szDirectoryName, "\\*.*");
		bool fZip = m_pZipUnZip->ZipDirectory(szPath, szDirectoryName, szDirZipPath, true);
		if (!fZip) return -1;
		strcpy_s(szSrcFileName, 324, szDirZipPath);
		return 1;
	}
	else
		return 0;
}

int vncClient::CheckAndZipDirectoryForChecksuming(LPSTR szSrcFileName)
{
	if (!m_fFileDownloadError
		&&
		!strncmp(strrchr(szSrcFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix))
		)
	{
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH];
		char szDirectoryName[MAX_PATH * 2];
		strcpy_s(szPath, szSrcFileName);
		char* p = strrchr(szPath, '\\') + 1;
		char* p2 = strchr(p, '-') + 1;
		strcpy_s(szDirName, p2);
		char* p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat_s(szPath, szDirName);

		CreateDirectory(szPath, NULL);
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			strcpy_s(szDirectoryName, szPath);
			// p = strrchr(szPath, '\\') + 1;
			// if (p != NULL) *p = '\0'; else return -1;
			if (strlen(szDirectoryName) > (MAX_PATH - 4)) return -1;
			strcat_s(szDirectoryName, "\\*.*");
			bool fZip = m_pZipUnZip->ZipDirectory(szPath, szDirectoryName, szSrcFileName, true);
			if (!fZip) return -1;
		}
	}
	return 0;
}

//
// Unzip possible directory
// Todo: handle unzip error correctly...
//
bool vncClient::UnzipPossibleDirectory(LPSTR szFileName)
{
	//	vnclog.Print(0, _T("UnzipPossibleDirectory\n"));
	if (!m_fFileDownloadError
		&&
		!strncmp(strrchr(szFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix))
		)
	{
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH]; // Todo: improve this (size)
		strcpy_s(szPath, szFileName);
		// Todo: improve all this (p, p2, p3 NULL test or use a standard substring extraction function)
		char* p = strrchr(szPath, '\\') + 1;
		char* p2 = strchr(p, '-') + 1; // rfbZipDirectoryPrefix MUST have a "-" at the end...
		strcpy_s(szDirName, p2);
		char* p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat_s(szPath, szDirName);
		// Create the Directory
		m_pZipUnZip->UnZipDirectory(szPath, szFileName);
		DeleteFile(szFileName);
		return true;
	}
	return false;
}

//
// GetFileSize() doesn't handle files > 4GBytes...
// GetFileSizeEx() doesn't exist under Win9x...
// So let's write our own function.
//
bool vncClient::MyGetFileSize(char* szFilePath, ULARGE_INTEGER* n2FileSize)
{
	WIN32_FIND_DATA fd;
	HANDLE ff;

	DWORD errmode = SetErrorMode(SEM_FAILCRITICALERRORS); // No popup please !
	ff = FindFirstFile(szFilePath, &fd);
	SetErrorMode(errmode);

	if (ff == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	FindClose(ff);

	(*n2FileSize).LowPart = fd.nFileSizeLow;
	(*n2FileSize).HighPart = fd.nFileSizeHigh;
	(*n2FileSize).QuadPart = (((__int64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;

	return true;
}

#ifndef SC_20
bool vncClient::DoFTUserImpersonation()
{
	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - Call\n"));
	omni_mutex_lock l(GetUpdateLock(), 103);

	if (m_fFileDownloadRunning) 
		return true;
	if (m_fFileUploadRunning) 
		return true;
	if (m_fFTUserImpersonatedOk) 
		return true;
	// if we're already impersonating the user and have a session open, do nothing.
	if (m_fFileSessionOpen && m_fFTUserImpersonatedOk)
		return true;

	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - 1\n"));
	bool fUserOk = true;

	if (processHelper::IsWSLocked())
	{
		m_fFTUserImpersonatedOk = false;
		vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - WSLocked\n"));
		return false;
	}

	char username[UNLEN + 1]{};
	processHelper::CurrentUser((char*)&username, sizeof(username));
	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - currentUser = %s\n"), username);
	if (strcmp(username, "") != 0)
	{
		desktopUsersToken = new DesktopUsersToken();
		m_hPToken = desktopUsersToken->getDesktopUsersToken();

		if (!m_hPToken) {
			vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - OpenProcessToken Error\n"));
			fUserOk = false;
		}
		else {
			if (!ImpersonateLoggedOnUser(m_hPToken))
			{
				vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - ImpersonateLoggedOnUser Failed\n"));
				fUserOk = false;
			}
			vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - thread = %d\n"), GetCurrentThreadId());

			ExpandEnvironmentStringsForUser(m_hPToken, "%TEMP%", m_szTempDir, MAX_PATH);
			strcat_s(m_szTempDir, "\\");
		}
		if (!settings->RunningFromExternalService())
			fUserOk = true;
	}
	else
	{
		fUserOk = false;
	}

	if (fUserOk)
		m_lLastFTUserImpersonationTime = GetTimeFunction();
	m_fFTUserImpersonatedOk = fUserOk;
	return fUserOk;
}

void vncClient::UndoFTUserImpersonation()
{
	//vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::UNDoFTUserImpersonation - Call\n"));
	//moved to after returns, Is this lock realy needed if no revert is done ?
	//
	//omni_mutex_lock l(GetUpdateLock());

	if (!m_fFTUserImpersonatedOk) return;
	if (m_fFileDownloadRunning) return;
	if (m_fFileUploadRunning) return;
	if (m_fFileSessionOpen) return;

	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::UNDoFTUserImpersonation - 1\n"));
	DWORD lTime = GetTimeFunction();
	if (lTime - m_lLastFTUserImpersonationTime < 10000) return;
	omni_mutex_lock l(GetUpdateLock(), 104);
	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::UNDoFTUserImpersonation - Impersonationtoken exists\n"));
	RevertToSelf();
	m_fFTUserImpersonatedOk = false;
	if (desktopUsersToken)
		delete desktopUsersToken;
	desktopUsersToken = NULL;
	m_hPToken = 0;
}
#endif // SC_20

// 10 April 2008 jdp paquette@atnetsend.net
// This can crash as we can not send middle in an update...

void vncClient::Record_SendServerStateUpdate(CARD32 state, CARD32 value)
{
	m_state = state;
	m_value = value;
	m_want_update_state = true;
#ifdef _DEBUG
	char			szText[256];
	sprintf_s(szText, "Record_SendServerStateUpdate %i %i  \n", m_state, m_value);
	OutputDebugString(szText);
#endif
}

void vncClient::SendServerStateUpdate(CARD32 state, CARD32 value)
{
	if (m_wants_ServerStateUpdates && m_socket)
	{
		// send message to client
		rfbServerStateMsg rsmsg;
		memset(&rsmsg, 0, sizeof rsmsg);
		rsmsg.type = rfbServerState;
		rsmsg.state = Swap32IfLE(state);
		rsmsg.value = Swap32IfLE(value);

		m_socket->SendExact((char*)&rsmsg, sz_rfbServerStateMsg, rfbServerState);
	}
}

void vncClient::SendMonitorInfo()
{
	rfbMonitorMsg mm;
	memset(&mm, 0, sizeof mm);
	mm.type = rfbMonitorInfo;
	mm.nbr = m_encodemgr.m_buffer->m_desktop->nr_monitors;
	m_socket->SendExact((char*)&mm, sz_rfbMonitorMsg, rfbMonitorInfo);
}

void vncClient::SendKeepAlive(bool bForce)
{
	if (m_wants_KeepAlive && m_socket)
	{
		//adzm 2010-08-01
		DWORD nInterval = (DWORD)settings->getkeepAliveInterval() * 1000;
		DWORD nTicksSinceLastSent = GetTickCount() - m_socket->GetLastSentTick();

		if ((!bForce && nTicksSinceLastSent < nInterval) || nInterval == 0)
			return;

		rfbKeepAliveMsg kp;
		memset(&kp, 0, sizeof kp);
		kp.type = rfbKeepAlive;
		omni_mutex_lock l(GetUpdateLock(), 255);
		m_socket->SendExact((char*)&kp, sz_rfbKeepAliveMsg, rfbKeepAlive);
	}
}

void vncClient::SendFTProtocolMsg()
{
	rfbFileTransferMsg ft;
	memset(&ft, 0, sizeof ft);
	ft.type = rfbFileTransfer;
	ft.contentType = rfbFileTransferProtocolVersion;
	ft.contentParam = FT_PROTO_VERSION_3;
	m_socket->SendExact((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
}

// adzm - 2010-07 - Extended clipboard
void vncClient::NotifyExtendedClipboardSupport()
{
	ExtendedClipboardDataMessage extendedDataMessage;
	m_clipboard.settings.PrepareCapsPacket(extendedDataMessage);

	rfbServerCutTextMsg msg;
	memset(&msg, 0, sizeof(rfbServerCutTextMsg));
	msg.type = rfbServerCutText;
	msg.length = Swap32IfLE(-extendedDataMessage.GetDataLength());

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	m_socket->SendExactQueue((char*)&msg, sz_rfbServerCutTextMsg, rfbServerCutText);
	m_socket->SendExact((char*)(extendedDataMessage.GetData()), extendedDataMessage.GetDataLength());
}

// adzm 2010-09 - Notify streaming DSM plugin support
void vncClient::NotifyPluginStreamingSupport()
{
	rfbNotifyPluginStreamingMsg msg;
	memset(&msg, 0, sizeof(rfbNotifyPluginStreamingMsg));
	msg.type = rfbNotifyPluginStreaming;

	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	m_socket->SendExact((char*)&msg, sz_rfbNotifyPluginStreamingMsg, rfbNotifyPluginStreaming);
	m_socket->SetPluginStreamingOut();
}

DWORD WINAPI CompressFolder(LPVOID lpParam)
{
	vncClient* client = (vncClient*)lpParam;
	if (client->m_hPToken)
		ImpersonateLoggedOnUser(client->m_hPToken); //need to set this thread's impersonation or can find mapped network or share files
	int nDirZipRet = client->ZipPossibleDirectory(client->m_szSrcFileName);
	if (client->m_socket)
		return client->filetransferrequestPart2(nDirZipRet);
	return 0;
}

int  vncClient::filetransferrequestPart1(rfbClientToServerMsg msg, bool fUserOk)
{
	if (ThreadHandleCompressFolder != NULL)
		return 0;
	{
		omni_mutex_lock ll(GetUpdateLock(), 90);
		m_fCompressionEnabled = (Swap32IfLE(msg.ft.size) == 1);
		const UINT length = Swap32IfLE(msg.ft.length);
		memset(m_szSrcFileName, 0, sizeof(m_szSrcFileName));
		if (length > sizeof(m_szSrcFileName) - 2)
			return 0;
		// Read in the Name of the file to create
		if (!m_socket->ReadExact(m_szSrcFileName, length))
			return 0;
		if (!settings->getEnableFileTransfer() || !fUserOk)
			return 0;
	}
	DWORD dwTId;
	ThreadHandleCompressFolder = CreateThread(NULL, 0, CompressFolder, this, 0, &dwTId);
	return 1;
}

int  vncClient::filetransferrequestPart2(int nDirZipRet)
{
	omni_mutex_lock ll(GetUpdateLock(), 90);
	if (nDirZipRet == -1)
	{
		//MessageBoxSecure(NULL, "5. Abort!", "UltraVNC Server", MB_OK);
		//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: Failed to zip requested dir. Abort!\n"));

		//	[v1.0.2-jp1 fix] Empty directory receive problem
		rfbFileTransferMsg ft{};
		ft.type = rfbFileTransfer;
		ft.contentType = rfbFileHeader;
		ft.size = Swap32IfLE(0xffffffffu); // File Size in bytes, 0xFFFFFFFF (-1) means error
		ft.length = Swap32IfLE(strlen(m_szSrcFileName));
		//adzm 2010-09 - minimize packets. SendExact flushes the queue.
		m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
		m_socket->SendExactQueue((char*)m_szSrcFileName, (const VCard)strlen(m_szSrcFileName));
		// 2 May 2008 jdp send the highpart too, else the client will hang
		// sf@2004 - Improving huge file size handling
		// TODO: what if we're speaking the old protocol, how can we tell?
		CARD32 sizeH = Swap32IfLE(0xffffffffu);
		m_socket->SendExact((char*)&sizeH, sizeof(CARD32));

		m_fFileUploadError = true;
		m_fFileUploadRunning = false;
		FTUploadFailureHook();
		if (ThreadHandleCompressFolder)
			CloseHandle(ThreadHandleCompressFolder);
		ThreadHandleCompressFolder = NULL;
		return 0;
	}

	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::filetransferrequestPart2 - thread = %d\n"), GetCurrentThreadId());

	// Open source file
	m_hSrcFile = CreateFile(
		m_szSrcFileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
	);

	// DWORD dwSrcSize = (DWORD)0;
	ULARGE_INTEGER n2SrcSize{};
	if (m_hSrcFile == INVALID_HANDLE_VALUE)
	{
		// dwSrcSize = 0xFFFFFFFF;
		n2SrcSize.LowPart = 0xFFFFFFFF;
		n2SrcSize.HighPart = 0xFFFFFFFF;
	}
	else
	{
		// Source file size
		bool bSize = MyGetFileSize(m_szSrcFileName, &n2SrcSize);
		// dwSrcSize = GetFileSize(m_hSrcFile, NULL);
		// if (dwSrcSize == 0xFFFFFFFF)
		if (!bSize)
		{
			helper::close_handle(m_hSrcFile);
			n2SrcSize.LowPart = 0xFFFFFFFF;
			n2SrcSize.HighPart = 0xFFFFFFFF;
		}
		else
		{
			// Add the File Time Stamp to the filename
			FILETIME SrcFileModifTime;
			BOOL fRes = GetFileTime(m_hSrcFile, NULL, NULL, &SrcFileModifTime);
			if (fRes)
			{
				char szSrcFileTime[18];
				// sf@2003 - Convert file time to local time
				// We've made the choice off displaying all the files
				// off client AND server sides converted in clients local
				// time only. So we don't convert server's files times.
				/*
				FILETIME LocalFileTime;
				FileTimeToLocalFileTime(&SrcFileModifTime, &LocalFileTime);
				*/
				SYSTEMTIME FileTime;
				FileTimeToSystemTime(&SrcFileModifTime/*&LocalFileTime*/, &FileTime);
				wsprintf(szSrcFileTime, "%2.2d/%2.2d/%4.4d %2.2d:%2.2d",
					FileTime.wMonth,
					FileTime.wDay,
					FileTime.wYear,
					FileTime.wHour,
					FileTime.wMinute
				);
				strcat_s(m_szSrcFileName, ",");
				strcat_s(m_szSrcFileName, szSrcFileTime);
			}
		}
	}

	// sf@2004 - Delta Transfer
	if (m_lpCSBuffer != NULL)
	{
		delete[] m_lpCSBuffer;
		m_lpCSBuffer = NULL;
	}
	m_nCSOffset = 0;
	m_nCSBufferSize = 0;

	// Send the FileTransferMsg with rfbFileHeader
	rfbFileTransferMsg ft = { 0 };

	ft.type = rfbFileTransfer;
	ft.contentType = rfbFileHeader;
	ft.size = Swap32IfLE(n2SrcSize.LowPart); // File Size in bytes, 0xFFFFFFFF (-1) means error
	ft.length = Swap32IfLE(strlen(m_szSrcFileName));
	//adzm 2010-09 - minimize packets. SendExact flushes the queue.
	m_socket->SendExactQueue((char*)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	m_socket->SendExactQueue((char*)m_szSrcFileName, (const VCard)strlen(m_szSrcFileName));

	// sf@2004 - Improving huge file size handling
	CARD32 sizeH = Swap32IfLE(n2SrcSize.HighPart);
	m_socket->SendExact((char*)&sizeH, sizeof(CARD32));

	// delete [] szSrcFileName;
	if (n2SrcSize.LowPart == 0xFFFFFFFF && n2SrcSize.HighPart == 0xFFFFFFFF)
	{
		//MessageBoxSecure(NULL, "6. Abort!", "UltraVNC Server", MB_OK);
		//vnclog.Print(LL_INTINFO, VNCLOG("*** File Transfer: Wrong Src File size. Abort!\n"));
		FTUploadFailureHook();
		if (ThreadHandleCompressFolder)
			CloseHandle(ThreadHandleCompressFolder);
		ThreadHandleCompressFolder = NULL;
		return 0;
	}

	FTUploadStartHook();
	if (ThreadHandleCompressFolder)
		CloseHandle(ThreadHandleCompressFolder);
	ThreadHandleCompressFolder = NULL;
	return 0;
}

extern HINSTANCE	hAppInstance;
void vncClient::SetHasMouse(bool has_mouse)
{
	this->has_mouse = has_mouse;
	if (has_mouse) {
		if (simulateCursor)
			delete simulateCursor;
		simulateCursor = NULL;
	}
	else {
		simulateCursor = new SimulateCursor(hAppInstance);
	}
}
