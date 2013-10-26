//
// Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
// USA.

//
// zrleDecode.h - zrle decoding function.
//
// Before including this file, you must define a number of CPP macros.
//
// BPP should be 8, 16 or 32 depending on the bits per pixel.
// FILL_RECT
// IMAGE_RECT

#include <rdr/ZlibInStream.h>
#include <rdr/InStream.h>
#include <assert.h>

using namespace rdr;

/* __RFB_CONCAT2 concatenates its two arguments.  __RFB_CONCAT2E does the same
   but also expands its arguments if they are macros */

#ifndef __RFB_CONCAT2E
#define __RFB_CONCAT2(a,b) a##b
#define __RFB_CONCAT2E(a,b) __RFB_CONCAT2(a,b)
#endif

#ifndef __RFB_CONCAT3E
#define __RFB_CONCAT3(a,b,c) a##b##c
#define __RFB_CONCAT3E(a,b,c) __RFB_CONCAT3(a,b,c)
#endif

#undef END_FIX
#if ZYWRLE_ENDIAN == ENDIAN_LITTLE
#  define END_FIX LE
#elif ZYWRLE_ENDIAN == ENDIAN_BIG
#  define END_FIX BE
#else
#  define END_FIX NE
#endif

#ifdef CPIXEL
#define PIXEL_T __RFB_CONCAT2E(rdr::U,BPP)
#define READ_PIXEL __RFB_CONCAT2E(readOpaque,CPIXEL)
#define ZRLE_DECODE_BPP __RFB_CONCAT3E(zrleDecode,CPIXEL,END_FIX)
#define BPPOUT BPP
#elif BPP==15
#define PIXEL_T __RFB_CONCAT2E(rdr::U,16)
#define READ_PIXEL __RFB_CONCAT2E(readOpaque,16)
#define ZRLE_DECODE_BPP __RFB_CONCAT3E(zrleDecode,BPP,END_FIX)
#define BPPOUT 16
#else
#define PIXEL_T __RFB_CONCAT2E(rdr::U,BPP)
#define READ_PIXEL __RFB_CONCAT2E(readOpaque,BPP)
#define ZRLE_DECODE_BPP __RFB_CONCAT3E(zrleDecode,BPP,END_FIX)
#define BPPOUT BPP
#endif

#if BPP!=8
#define ZYWRLE_DECODE
#include <rfb/zywrletemplate.c>
#endif

void ZRLE_DECODE_BPP (int x, int y, int w, int h, rdr::InStream* is,
                      rdr::ZlibInStream* zis, PIXEL_T* buf)
{
  int length = is->readU32();
  zis->setUnderlying(is, length);

  for (int ty = y; ty < y+h; ty += rfbZRLETileHeight) {
    int th = rfbZRLETileHeight;
    if (th > y+h-ty) th = y+h-ty;
    for (int tx = x; tx < x+w; tx += rfbZRLETileWidth) {
      int tw = rfbZRLETileWidth;
      if (tw > x+w-tx) tw = x+w-tx;

#if BPP!=8
top:
#endif
      int mode = zis->readU8();
      BOOL rle = mode & 128;
      int palSize = mode & 127;
      PIXEL_T palette[128];

      //        fprintf(stderr,"rle %d palSize %d\n",rle,palSize);

      for (int i = 0; i < palSize; i++) {
        palette[i] = zis->READ_PIXEL();
      }

      if (palSize == 1) {
        PIXEL_T* ptr = buf;
        for (int i = 0; i < tw*th; i++) {
			*ptr++ = palette[0];
		}
		goto draw;
      }

      if (!rle) {
        if (palSize == 0) {

          // raw

#if BPP!=8
          if( (zywrle_level>0)&& !(zywrle_level & 0x80) ){
			zywrle_level |= 0x80;
			goto top;
		  }else
#endif
#ifdef CPIXEL
          for (PIXEL_T* ptr = buf; ptr < buf+tw*th; ptr++) {
            *ptr = zis->READ_PIXEL();
          }
#else
          zis->readBytes(buf, tw * th * (BPPOUT / 8));
#endif

        } else {

          // packed pixels
          int bppp = ((palSize > 16) ? 8 :
                      ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));

          PIXEL_T* ptr = buf;

          for (int i = 0; i < th; i++) {
            PIXEL_T* eol = ptr + tw;
            U8 byte = 0;
            U8 nbits = 0;

            while (ptr < eol) {
              if (nbits == 0) {
                byte = zis->readU8();
                nbits = 8;
              }
              nbits -= bppp;
              U8 index = (byte >> nbits) & ((1 << bppp) - 1) & 127;
              *ptr++ = palette[index];
            }
          }
        }

#ifdef FAVOUR_FILL_RECT
       //fprintf(stderr,"copying data to screen %dx%d at %d,%d\n",tw,th,tx,ty);
        IMAGE_RECT(tx,ty,tw,th,buf);
#endif

      } else {

        if (palSize == 0) {

          // plain RLE

          PIXEL_T* ptr = buf;
          PIXEL_T* end = ptr + th * tw;	    
          while (ptr < end) {
            PIXEL_T pix = zis->READ_PIXEL();
            int len = 1;
            int b;
            do {
              b = zis->readU8();
              len += b;
            } while (b == 255);

            assert(len <= end - ptr);

#ifdef FAVOUR_FILL_RECT
            int i = ptr - buf;
            ptr += len;

            int runX = i % tw;
            int runY = i / tw;

            if (runX + len > tw) {
              if (runX != 0) {
                FILL_RECT(tx+runX, ty+runY, tw-runX, 1, pix);
                len -= tw-runX;
                runX = 0;
                runY++;
              }

              if (len > tw) {
                FILL_RECT(tx, ty+runY, tw, len/tw, pix);
                runY += len / tw;
                len = len % tw;
              }
            }

            if (len != 0) {
              FILL_RECT(tx+runX, ty+runY, len, 1, pix);
            }
#else
            while (len-- > 0) *ptr++ = pix;
#endif

          }
        } else {

          // palette RLE

          PIXEL_T* ptr = buf;
          PIXEL_T* end = ptr + th * tw;
          while (ptr < end) {
            int index = zis->readU8();
            int len = 1;
            if (index & 128) {
              int b;
              do {
                b = zis->readU8();
                len += b;
              } while (b == 255);

              assert(len <= end - ptr);
            }

            index &= 127;

            PIXEL_T pix = palette[index];

#ifdef FAVOUR_FILL_RECT
            int i = ptr - buf;
            ptr += len;

            int runX = i % tw;
            int runY = i / tw;

            if (runX + len > tw) {
              if (runX != 0) {
                FILL_RECT(tx+runX, ty+runY, tw-runX, 1, pix);
                len -= tw-runX;
                runX = 0;
                runY++;
              }

              if (len > tw) {
                FILL_RECT(tx, ty+runY, tw, len/tw, pix);
                runY += len / tw;
                len = len % tw;
              }
            }

            if (len != 0) {
              FILL_RECT(tx+runX, ty+runY, len, 1, pix);
            }
#else
            while (len-- > 0) *ptr++ = pix;
#endif
          }
        }
      }

#ifndef FAVOUR_FILL_RECT
      //fprintf(stderr,"copying data to screen %dx%d at %d,%d\n",tw,th,tx,ty);
draw:
	  omni_mutex_lock l(m_bitmapdcMutex);
#if BPP!=8
      if( zywrle_level & 0x80 ){
	    zywrle_level &= 0x7F;
		ZYWRLE_SYNTHESIZE( buf, buf, tw, th, tw, zywrle_level, zywrleBuf );
	  }
#endif
      IMAGE_RECT(tx,ty,tw,th,buf);
	   if (initialupdate_counter < 4) if (!directx_used)InvalidateRect(m_hwndcn, NULL, FALSE);
#endif
    }
  }

  zis->reset();
}

#undef ZRLE_DECODE_BPP
#undef READ_PIXEL
#undef PIXEL_T
#undef BPPOUT
