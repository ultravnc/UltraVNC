/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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


#include <stdio.h>
#include <windows.h>

#if !defined( _RingBuffer )
#define _RingBuffer

class RingBuffer
{
public:
	 RingBuffer();
     void RingBufferSet( int sizeBytes );
     ~RingBuffer();
     int Read( char* dataPtr, int numBytes);
     int Write( char *dataPtr, int numBytes );
     bool Empty( void );
	 void NewSize(unsigned int size);
     int GetSize( );
     int GetWriteAvail( );
     int GetReadAvail( );
	 HANDLE inputevent;
private:
     char * _data;
     int _size;
     int _readPtr;
     int _writePtr;
     int _writeBytesAvail;
	 CRITICAL_SECTION CriticalSection;
};

#endif