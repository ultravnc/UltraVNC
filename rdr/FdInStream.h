/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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


//
// FdInStream streams from a file descriptor.
//

#ifndef __RDR_FDINSTREAM_H__
#define __RDR_FDINSTREAM_H__

#pragma once

#include "InStream.h"

namespace rdr {

  class FdInStream : public InStream {

  public:

    FdInStream(int fd, int timeout=0, int bufSize=0);
    FdInStream(int fd, void (*blockCallback)(void*), void* blockCallbackArg=0,
		  int bufSize=0);
    virtual ~FdInStream();

    int getFd() { return fd; }
    int pos();
    void readBytes(void* data, int length);
    int bytesInBuf() { return (int)(end - ptr); }

    void startTiming();
    void stopTiming();
    unsigned int kbitsPerSecond();
    unsigned int timeWaited() { return timeWaitedIn100us; }

	void SetDSMMode(bool fDSMMode){m_fDSMMode = fDSMMode;}; // sf@2002 
	void SetReadFromMemoryBuffer(int nReadSize, char* pMemBuffer); // sf@2002 
	bool GetReadFromMemoryBuffer(void) {return m_fReadFromNetRectBuf;}; // sf@2002 
	__int64 GetBytesRead() {return m_nBytesRead;};
	int Check_if_buffer_has_data();

  protected:
    int overrun(int itemSize, int nItems);

  private:
    int checkReadable(int fd, int timeout);
    int readWithTimeoutOrCallback(void* buf, int len);

    int fd;
    int timeout;
    void (*blockCallback)(void*);
    void* blockCallbackArg;

    bool timing;
    unsigned int timeWaitedIn100us;
    unsigned int timedKbits;

    int bufSize;
    int offset;
    U8* start;

	// sf@2002 - DSMPlugin hack
	bool m_fDSMMode;
	char* m_pNetRectBuf;
	bool m_fReadFromNetRectBuf; 
	int m_nNetRectBufOffset;
	int m_nReadSize;
	__int64 m_nBytesRead;
    const U8* m_pNetRectSavePtr;
    const U8* m_pNetRectSaveEnd;
    U8* m_pNetRectSaveStart;
  };

} // end of namespace rdr

#endif
