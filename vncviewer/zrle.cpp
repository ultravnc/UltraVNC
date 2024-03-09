//  /////////////////////////////////////////////////////////////////////////////
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


#include <rdr/ZlibInStream.h>
#include <rdr/FdInStream.h>
#include <rdr/Exception.h>
#include "stdhdrs.h"
#include "vncviewer.h"
// #include "ClientConnection.h"

// Instantiate the decoding function for 8, 16 and 32 BPP

#define zrleDecode ClientConnection::zrleDecode

#define ENDIAN_LITTLE 0
#define ENDIAN_BIG 1
#define ENDIAN_NO 2
#define BPP 8
#define ZYWRLE_ENDIAN ENDIAN_NO
#define IMAGE_RECT(x,y,w,h,data)                \
    SETPIXELS(m_netbuf,8,x,y,w,h)

#include <rfb/zrleDecode.h>
#undef BPP
#undef ZYWRLE_ENDIAN
#undef IMAGE_RECT

#define BPP 16
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#define IMAGE_RECT(x,y,w,h,data)                \
    SETPIXELS(m_netbuf,16,x,y,w,h)

#include <rfb/zrleDecode.h>
#undef BPP
#undef ZYWRLE_ENDIAN
#define BPP 15
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleDecode.h>
#undef BPP
#undef ZYWRLE_ENDIAN
#undef IMAGE_RECT

#define IMAGE_RECT(x,y,w,h,data)                \
    SETPIXELS(m_netbuf,32,x,y,w,h)


#define BPP 32
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <rfb/zrleDecode.h>
#define CPIXEL 24A
#include <rfb/zrleDecode.h>
#undef CPIXEL
#define CPIXEL 24B
#include <rfb/zrleDecode.h>
#undef CPIXEL
#undef BPP
#undef ZYWRLE_ENDIAN
#undef IMAGE_RECT

#undef zrleDecode

void ClientConnection::zrleDecode(int x, int y, int w, int h, bool use_zstd)
{
  try {
    CheckBufferSize(rfbZRLETileWidth * rfbZRLETileHeight * 4);
    //omni_mutex_lock l(m_bitmapdcMutex);

	if( zywrle ){
	  if( !m_opts->m_enableJpegCompression ){
		  zywrle_level = 1;
	  }else if( m_opts->m_jpegQualityLevel < 3 ){
		  zywrle_level = 3;
	  }else if( m_opts->m_jpegQualityLevel < 6 ){
		  zywrle_level = 2;
	  }else{
		  zywrle_level = 1;
	  }
	}else{
	  zywrle_level = 0;
	}
	if (!use_zstd) {
		switch (m_myFormat.bitsPerPixel) {

		case 8:
			zrleDecode8NE(x, y, w, h, fis, zis, (rdr::U8*)m_netbuf);
			break;

		case 16:
			if (m_myFormat.greenMax > 0x1F) {
				zrleDecode16LE(x, y, w, h, fis, zis, (rdr::U16*)m_netbuf);
			}
			else {
				zrleDecode15LE(x, y, w, h, fis, zis, (rdr::U16*)m_netbuf);
			}
			break;

		case 32:
			bool fitsInLS3Bytes
				= (((CARD32)m_myFormat.redMax << m_myFormat.redShift) < (1 << 24) &&
				((CARD32)m_myFormat.greenMax << m_myFormat.greenShift) < (1 << 24) &&
					((CARD32)m_myFormat.blueMax << m_myFormat.blueShift) < (1 << 24));

			bool fitsInMS3Bytes = (m_myFormat.redShift > 7 &&
				m_myFormat.greenShift > 7 &&
				m_myFormat.blueShift > 7);

			if ((fitsInLS3Bytes && !m_myFormat.bigEndian) ||
				(fitsInMS3Bytes && m_myFormat.bigEndian))
			{
				zrleDecode24ALE(x, y, w, h, fis, zis, (rdr::U32*)m_netbuf);
			}
			else if ((fitsInLS3Bytes && m_myFormat.bigEndian) ||
				(fitsInMS3Bytes && !m_myFormat.bigEndian))
			{
				zrleDecode24BLE(x, y, w, h, fis, zis, (rdr::U32*)m_netbuf);
			}
			else
			{
				zrleDecode32LE(x, y, w, h, fis, zis, (rdr::U32*)m_netbuf);
			}
			break;
		}
	}
	else {
		switch (m_myFormat.bitsPerPixel) {

		case 8:
			zrleDecode8NE(x, y, w, h, fis, zstdis, (rdr::U8*)m_netbuf);
			break;

		case 16:
			if (m_myFormat.greenMax > 0x1F) {
				zrleDecode16LE(x, y, w, h, fis, zstdis, (rdr::U16*)m_netbuf);
			}
			else {
				zrleDecode15LE(x, y, w, h, fis, zstdis, (rdr::U16*)m_netbuf);
			}
			break;

		case 32:
			bool fitsInLS3Bytes
				= (((CARD32)m_myFormat.redMax << m_myFormat.redShift) < (1 << 24) &&
				((CARD32)m_myFormat.greenMax << m_myFormat.greenShift) < (1 << 24) &&
					((CARD32)m_myFormat.blueMax << m_myFormat.blueShift) < (1 << 24));

			bool fitsInMS3Bytes = (m_myFormat.redShift > 7 &&
				m_myFormat.greenShift > 7 &&
				m_myFormat.blueShift > 7);

			if ((fitsInLS3Bytes && !m_myFormat.bigEndian) ||
				(fitsInMS3Bytes && m_myFormat.bigEndian))
			{
				zrleDecode24ALE(x, y, w, h, fis, zstdis, (rdr::U32*)m_netbuf);
			}
			else if ((fitsInLS3Bytes && m_myFormat.bigEndian) ||
				(fitsInMS3Bytes && !m_myFormat.bigEndian))
			{
				zrleDecode24BLE(x, y, w, h, fis, zstdis, (rdr::U32*)m_netbuf);
			}
			else
			{
				zrleDecode32LE(x, y, w, h, fis, zstdis, (rdr::U32*)m_netbuf);
			}
			break;
		}
	}

  } catch (rdr::Exception& e) {
    fprintf(stderr,"ZRLE decoder exception: %s\n",e.str());
    throw;
  }
}
