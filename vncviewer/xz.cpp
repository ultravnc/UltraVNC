//
// Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
//
#ifdef _XZ
#include <rdr/xzInStream.h>
#include <rdr/FdInStream.h>
#include <rdr/Exception.h>
#include "stdhdrs.h"
#include "vncviewer.h"
// #include "ClientConnection.h"

// Instantiate the decoding function for 8, 16 and 32 BPP

//#define FAVOUR_FILL_RECT

#define xzDecode ClientConnection::xzDecode

#define ENDIAN_LITTLE 0
#define ENDIAN_BIG 1
#define ENDIAN_NO 2
#define BPP 8
#define XZYW_ENDIAN ENDIAN_NO
#define IMAGE_RECT(x,y,w,h,data)                \
    SETUP_COLOR_SHORTCUTS;                      \
    SETPIXELS(m_netbuf,8,x,y,w,h)
#define FILL_RECT(x,y,w,h,pix)                          \
    SETUP_COLOR_SHORTCUTS;                              \
    COLORREF color = COLOR_FROM_PIXEL8_ADDRESS(&pix);   \
    FillSolidRect_ultra(x,y,w,h, m_myFormat.bitsPerPixel,(BYTE*)&pix)

#include <rfb/xzDecode.h>
#undef BPP
#undef XZYW_ENDIAN
#undef IMAGE_RECT
#undef FILL_RECT

#define BPP 16
#define XZYW_ENDIAN ENDIAN_LITTLE
#define IMAGE_RECT(x,y,w,h,data)                \
    SETUP_COLOR_SHORTCUTS;                      \
    SETPIXELS(m_netbuf,16,x,y,w,h)
#define FILL_RECT(x,y,w,h,pix)                          \
    SETUP_COLOR_SHORTCUTS;                              \
    COLORREF color = COLOR_FROM_PIXEL16_ADDRESS(&pix);  \
    FillSolidRect_ultra(x,y,w,h, m_myFormat.bitsPerPixel,(BYTE*)&pix)

#include <rfb/xzDecode.h>
#undef BPP
#undef XZYW_ENDIAN
#define BPP 15
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzDecode.h>
#undef BPP
#undef XZYW_ENDIAN
#undef IMAGE_RECT
#undef FILL_RECT

#define IMAGE_RECT(x,y,w,h,data)                \
    SETUP_COLOR_SHORTCUTS;                      \
    SETPIXELS(m_netbuf,32,x,y,w,h)
#define FILL_RECT(x,y,w,h,pix)                          \
    SETUP_COLOR_SHORTCUTS;                              \
    COLORREF color = COLOR_FROM_PIXEL32_ADDRESS(&pix);  \
    FillSolidRect_ultra(x,y,w,h, m_myFormat.bitsPerPixel,(BYTE*)&pix)


#define BPP 32
#define XZYW_ENDIAN ENDIAN_LITTLE
#include <rfb/xzDecode.h>
#define CPIXEL 24A
#include <rfb/xzDecode.h>
#undef CPIXEL
#define CPIXEL 24B
#include <rfb/xzDecode.h>
#undef CPIXEL
#undef BPP
#undef XZYW_ENDIAN
#undef IMAGE_RECT
#undef FILL_RECT

#undef xzDecode

void ClientConnection::xzDecode(int x, int y, int w, int h)
{
  try {
    CheckBufferSize(rfbXZTileWidth * rfbXZTileHeight * 4);
    omni_mutex_lock l(m_bitmapdcMutex);

	//int length = fis->readU32();

	if( xzyw ){
	  if( !m_opts.m_enableJpegCompression ){
		  xzyw_level = 1;
	  }else if( m_opts.m_jpegQualityLevel < 3 ){
		  xzyw_level = 3;
	  }else if( m_opts.m_jpegQualityLevel < 6 ){
		  xzyw_level = 2;
	  }else{
		  xzyw_level = 1;
	  }
	}else{
	  xzyw_level = 0;
	}

    switch (m_myFormat.bitsPerPixel) {

    case 8:
      xzDecode8NE(x,y,w,h,fis,xzis,(rdr::U8*)m_netbuf);
      break;

    case 16:
      if( m_myFormat.greenMax > 0x1F ){
        xzDecode16LE(x,y,w,h,fis,xzis,(rdr::U16*)m_netbuf);
	  }else{
        xzDecode15LE(x,y,w,h,fis,xzis,(rdr::U16*)m_netbuf);
	  }
      break;

    case 32:
      bool fitsInLS3Bytes
        = ((m_myFormat.redMax   << m_myFormat.redShift)   < (1<<24) &&
           (m_myFormat.greenMax << m_myFormat.greenShift) < (1<<24) &&
           (m_myFormat.blueMax  << m_myFormat.blueShift)  < (1<<24));

      bool fitsInMS3Bytes = (m_myFormat.redShift   > 7  &&
                             m_myFormat.greenShift > 7  &&
                             m_myFormat.blueShift  > 7);

      if ((fitsInLS3Bytes && !m_myFormat.bigEndian) ||
          (fitsInMS3Bytes && m_myFormat.bigEndian))
      {
        xzDecode24ALE(x,y,w,h,fis,xzis,(rdr::U32*)m_netbuf);
      }
      else if ((fitsInLS3Bytes && m_myFormat.bigEndian) ||
               (fitsInMS3Bytes && !m_myFormat.bigEndian))
      {
        xzDecode24BLE(x,y,w,h,fis,xzis,(rdr::U32*)m_netbuf);
      }
      else
      {
        xzDecode32LE(x,y,w,h,fis,xzis,(rdr::U32*)m_netbuf);
      }
      break;
    }

    //xzis->reset();

  } catch (rdr::Exception& e) {
    fprintf(stderr,"XZ decoder exception: %s\n",e.str());
    throw;
  }
}
#endif
