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

//  Copyright (C) 2000 Const Kaplinsky. All Rights Reserved.
#include "stdhdrs.h"
#include "vncencoder.h"
#include "vncbuffer.h"
#include "vncdesktop.h"

//
// New code implementing cursor shape updates.
//
BOOL
vncEncoder::IsXCursorSupported()
{
	return m_use_xcursor || m_use_richcursor;
}

// Cache for accessibility cursor size (avoid reading registry on every cursor update)
static DWORD s_cachedCursorBaseSize = 0;
static DWORD s_lastCursorSizeCheck = 0;

static DWORD GetAccessibilityCursorSize()
{
	// Cache cursor size for 5 seconds to avoid registry reads on every cursor update
	DWORD now = GetTickCount();
	if (s_cachedCursorBaseSize != 0 && (now - s_lastCursorSizeCheck) < 5000) {
		return s_cachedCursorBaseSize;
	}
	
	DWORD cursorBaseSize = 32;  // Default cursor size
	HKEY hKey;
	
	// Try primary location: Control Panel\Cursors\CursorBaseSize
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Cursors", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		DWORD dataSize = sizeof(DWORD);
		RegQueryValueEx(hKey, "CursorBaseSize", NULL, NULL, (LPBYTE)&cursorBaseSize, &dataSize);
		RegCloseKey(hKey);
	}
	
	// Also check Software\Microsoft\Accessibility for CursorSize multiplier (Windows 10/11)
	if (cursorBaseSize == 32) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Accessibility", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			DWORD cursorSizeMultiplier = 1;
			DWORD dataSize = sizeof(DWORD);
			if (RegQueryValueEx(hKey, "CursorSize", NULL, NULL, (LPBYTE)&cursorSizeMultiplier, &dataSize) == ERROR_SUCCESS) {
				// CursorSize is a multiplier (1-15), convert to pixel size
				if (cursorSizeMultiplier > 1) {
					cursorBaseSize = 32 + (cursorSizeMultiplier - 1) * 8;
				}
			}
			RegCloseKey(hKey);
		}
	}
	
	s_cachedCursorBaseSize = cursorBaseSize;
	s_lastCursorSizeCheck = now;
	return cursorBaseSize;
}

BOOL
vncEncoder::SendEmptyCursorShape(VSocket *outConn)
{
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(0);
	hdr.r.y = Swap16IfLE(0);
	hdr.r.w = Swap16IfLE(0);
	hdr.r.h = Swap16IfLE(0);
	if (m_use_xcursor) {
		hdr.encoding = Swap32IfLE(rfbEncodingXCursor);
	} else {
		hdr.encoding = Swap32IfLE(rfbEncodingRichCursor);
	}

	return outConn->SendExactQueue((char *)&hdr, sizeof(hdr));
}

BOOL
vncEncoder::SendCursorShape(VSocket *outConn, vncDesktop *desktop)
{
	// Make sure the function is used correctly
	if (!m_use_xcursor && !m_use_richcursor)
		return FALSE;

	// Check mouse cursor handle
	HCURSOR hcursor = desktop->GetCursor();
	if (hcursor == NULL) {
		vnclog.Print(LL_INTINFO, VNCLOG("cursor handle is NULL.\n"));
		return FALSE;
	}

	// Get cursor info
	ICONINFO IconInfo;
	if (!GetIconInfo(hcursor, &IconInfo)) {
		vnclog.Print(LL_INTINFO, VNCLOG("GetIconInfo() failed.\n"));
		return FALSE;
	}
	BOOL isColorCursor = FALSE;
	BITMAP bmColor = {0};
	if (IconInfo.hbmColor != NULL) {
		isColorCursor = TRUE;
		// Get color bitmap dimensions before deleting - needed for enlarged accessibility cursors
		GetObject(IconInfo.hbmColor, sizeof(BITMAP), (LPVOID)&bmColor);
		DeleteObject(IconInfo.hbmColor);
	}
	if (IconInfo.hbmMask == NULL) {
		vnclog.Print(LL_INTINFO, VNCLOG("cursor bitmap handle is NULL.\n"));
		return FALSE;
	}

	// Check bitmap info for the cursor
	BITMAP bmMask;
	if (!GetObject(IconInfo.hbmMask, sizeof(BITMAP), (LPVOID)&bmMask)) {
		vnclog.Print(LL_INTINFO, VNCLOG("GetObject() for bitmap failed.\n"));
		DeleteObject(IconInfo.hbmMask);
		return FALSE;
	}
	if (bmMask.bmPlanes != 1 || bmMask.bmBitsPixel != 1) {
		vnclog.Print(LL_INTINFO, VNCLOG("incorrect data in cursor bitmap.\n"));
		DeleteObject(IconInfo.hbmMask);
		return FALSE;
	}

	// Compute cursor dimensions
	// For color cursors (including enlarged accessibility cursors), use color bitmap dimensions
	// For monochrome cursors, mask height is doubled (AND mask + XOR mask)
	int width, height;
	if (isColorCursor && bmColor.bmWidth > 0 && bmColor.bmHeight > 0) {
		width = bmColor.bmWidth;
		height = bmColor.bmHeight;
	} else {
		width = bmMask.bmWidth;
		height = (isColorCursor) ? bmMask.bmHeight : bmMask.bmHeight/2;
	}

	// Check for Windows accessibility cursor scaling (cached to avoid registry reads)
	int xHotspot = IconInfo.xHotspot;
	int yHotspot = IconInfo.yHotspot;
	DWORD cursorBaseSize = GetAccessibilityCursorSize();
	
	// If cursor base size is larger than default, scale the cursor
	if (cursorBaseSize > 32 && width == 32 && height == 32) {
		width = cursorBaseSize;
		height = cursorBaseSize;
		// Scale hotspot proportionally
		xHotspot = (IconInfo.xHotspot * cursorBaseSize) / 32;
		yHotspot = (IconInfo.yHotspot * cursorBaseSize) / 32;
	}

	// Get monochrome bitmap data for cursor
	// NOTE: they say we should use GetDIBits() instead of GetBitmapBits().
	BYTE *mbits = new BYTE[bmMask.bmWidthBytes * bmMask.bmHeight];
	if (mbits == NULL)
    {
		DeleteObject(IconInfo.hbmMask);
		return FALSE;
    }

	BOOL success = GetBitmapBits(IconInfo.hbmMask,
								 bmMask.bmWidthBytes * bmMask.bmHeight, mbits);
	DeleteObject(IconInfo.hbmMask);

	if (!success) {
		vnclog.Print(LL_INTINFO, VNCLOG("GetBitmapBits() failed.\n"));
		delete[] mbits;
		return FALSE;
	}

	// For enlarged accessibility cursors, the color bitmap may be larger than the mask bitmap.
	// In this case, we need to generate a new mask that matches the color cursor dimensions.
	BYTE *workMbits = mbits;
	int workMaskWidthBytes = bmMask.bmWidthBytes;
	BOOL needsGeneratedMask = isColorCursor && (width != bmMask.bmWidth || height != bmMask.bmHeight);

	if (needsGeneratedMask) {
		// Allocate a new mask buffer for the enlarged cursor dimensions
		int newMaskWidthBytes = (width + 7) / 8;
		workMbits = new BYTE[newMaskWidthBytes * height];
		if (workMbits == NULL) {
			delete[] mbits;
			return FALSE;
		}
		memset(workMbits, 0, newMaskWidthBytes * height);  // Start with all transparent
		workMaskWidthBytes = newMaskWidthBytes;
	}

	// Call appropriate routine to send cursor shape update
	if (!isColorCursor && m_use_xcursor) {
		FixCursorMask(workMbits, NULL, width, bmMask.bmHeight, workMaskWidthBytes);
		success = SendXCursorShape(outConn, workMbits,
								   xHotspot, yHotspot,
								   width, height);
	}
	else if (m_use_richcursor) {
		int cbits_size = width * height * 4;
		BYTE *cbits = new BYTE[cbits_size];
		if (cbits == NULL) {
			if (needsGeneratedMask) delete[] workMbits;
			delete[] mbits;
			return FALSE;
		}
		if (!desktop->GetRichCursorData(cbits, hcursor, width, height)) {
			vnclog.Print(LL_INTINFO, VNCLOG("vncDesktop::GetRichCursorData() failed.\n"));
			if (needsGeneratedMask) delete[] workMbits;
			delete[] mbits;
			delete[] cbits;
			return FALSE;
		}
		if (needsGeneratedMask) {
			// For enlarged accessibility cursors, generate mask from color data
			// Mask bit 1 = visible pixel, bit 0 = transparent
			const int bytes_pixel = m_localformat.bitsPerPixel / 8;
			int bytes_row = width * bytes_pixel;
			while (bytes_row % sizeof(DWORD))
				bytes_row++;
			const int packed_width_bytes = workMaskWidthBytes;
			
			for (int y = 0; y < height; y++) {
				BYTE *srcRow = cbits + y * bytes_row;
				BYTE *dstRow = workMbits + y * packed_width_bytes;
				BYTE bitmask = 0x80;
				int dstByteIdx = 0;
				
				for (int x = 0; x < width; x++) {
					// Check if pixel has any non-zero color component (visible)
					BYTE *pixel = srcRow + x * bytes_pixel;
					BOOL hasColor = FALSE;
					for (int b = 0; b < bytes_pixel; b++) {
						if (pixel[b] != 0) {
							hasColor = TRUE;
							break;
						}
					}
					if (hasColor) {
						dstRow[dstByteIdx] |= bitmask;
					}
					if ((bitmask >>= 1) == 0) {
						bitmask = 0x80;
						dstByteIdx++;
					}
				}
			}
		} else {
			FixCursorMask(workMbits, cbits, width, height, workMaskWidthBytes);
		}
		success = SendRichCursorShape(outConn, workMbits, cbits,
									  xHotspot, yHotspot,
									  width, height);
		delete[] cbits;
	}
	else {
		success = FALSE;	// FIXME: We could convert RichCursor -> XCursor.
	}

	// Cleanup
	if (needsGeneratedMask) delete[] workMbits;
	delete[] mbits;

	return success;
}

BOOL
vncEncoder::SendXCursorShape(VSocket *outConn, BYTE *mask,
							 int xhot, int yhot, int width, int height)
{
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(xhot);
	hdr.r.y = Swap16IfLE(yhot);
	hdr.r.w = Swap16IfLE(width);
	hdr.r.h = Swap16IfLE(height);
	hdr.encoding = Swap32IfLE(rfbEncodingXCursor);

	BYTE colors[6] = { 0, 0, 0, 0xFF, 0xFF, 0xFF };
	int maskRowSize = (width + 7) / 8;
	int maskSize = maskRowSize * height;

	if ( !outConn->SendExactQueue((char *)&hdr, sizeof(hdr)) ||
		 !outConn->SendExactQueue((char *)colors, 6) ||
		 !outConn->SendExactQueue((char *)&mask[maskSize], maskSize) ||
		 !outConn->SendExactQueue((char *)mask, maskSize) ) {
		return FALSE;
	}
	return TRUE;
}

BOOL
vncEncoder::SendRichCursorShape(VSocket *outConn, BYTE *mbits, BYTE *cbits,
								int xhot, int yhot, int width, int height)
{
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(xhot);
	hdr.r.y = Swap16IfLE(yhot);
	hdr.r.w = Swap16IfLE(width);
	hdr.r.h = Swap16IfLE(height);
	hdr.encoding = Swap32IfLE(rfbEncodingRichCursor);

	// Cet cursor image in local pixel format
	int srcbuf_rowsize = width * (m_localformat.bitsPerPixel / 8);
	while (srcbuf_rowsize % sizeof(DWORD))
		srcbuf_rowsize++;	// Actually, this should never happen

	// Translate image to client pixel format
	int dstbuf_size = width * height * (m_remoteformat.bitsPerPixel / 8);
	BYTE *dstbuf = new BYTE[dstbuf_size];
	Translate(cbits, dstbuf, width, height, srcbuf_rowsize);

	// Send the data
	int mask_rowsize = (width + 7) / 8;
	int mask_size = mask_rowsize * height;
	if ( !outConn->SendExactQueue((char *)&hdr, sizeof(hdr)) ||
		 !outConn->SendExactQueue((char *)dstbuf, dstbuf_size) ||
		 !outConn->SendExactQueue((char *)mbits, mask_size) ) {
		delete[] dstbuf;
		return FALSE;
	}
	delete[] dstbuf;
	return TRUE;
}

void
vncEncoder::FixCursorMask(BYTE *mbits, BYTE *cbits,
						  int width, int height, int width_bytes)
{
	int packed_width_bytes = (width + 7) / 8;

	// Pack and invert bitmap data (mbits)
	int x, y;
	for (y = 0; y < height; y++)
		for (x = 0; x < packed_width_bytes; x++)
			mbits[y * packed_width_bytes + x] = ~mbits[y * width_bytes + x];

	// Replace "inverted background" bits with black color to ensure
	// cross-platform interoperability. Not beautiful but necessary code.
	if (cbits == NULL) {
		BYTE m, c;
		height /= 2;
		for (y = 0; y < height; y++) {
			for (x = 0; x < packed_width_bytes; x++) {
				m = mbits[y * packed_width_bytes + x];
				c = mbits[(height + y) * packed_width_bytes + x];
				mbits[y * packed_width_bytes + x] |= ~(m | c);
				mbits[(height + y) * packed_width_bytes + x] |= ~(m | c);
			}
		}
	} else {
		int bytes_pixel = m_localformat.bitsPerPixel / 8;
		int bytes_row = width * bytes_pixel;
		while (bytes_row % sizeof(DWORD))
			bytes_row++;	// Actually, this should never happen

		BYTE bitmask;
		int b1, b2;
		for (y = 0; y < height; y++) {
			bitmask = 0x80;
			for (x = 0; x < width; x++) {
				if ((mbits[y * packed_width_bytes + x / 8] & bitmask) == 0) {
					for (b1 = 0; b1 < bytes_pixel; b1++) {
						if (cbits[y * bytes_row + x * bytes_pixel + b1] != 0) {
							mbits[y * packed_width_bytes + x / 8] ^= bitmask;
							for (b2 = b1; b2 < bytes_pixel; b2++)
								cbits[y * bytes_row + x * bytes_pixel + b2] = 0x00;
							break;
						}
					}
				}
				if ((bitmask >>= 1) == 0)
					bitmask = 0x80;
			}
		}
	}
}

// Translate a rectangle (using arbitrary m_bytesPerRow value,
// always translating from the beginning of the source pixel array)
// NOTE: overloaded function!
inline void
vncEncoder::Translate(BYTE *source, BYTE *dest, int w, int h, int bytesPerRow)
{
	// Call the translation function
	(*m_transfunc) (m_transtable, &m_localformat, &m_transformat,
					(char *)source, (char *)dest, bytesPerRow, w, h);
}