// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"

void ClientConnection::ReadCursorShape(rfbFramebufferUpdateRectHeader *pfburh) {

	const int cursorWidth = pfburh->r.w;
	const int cursorHeight = pfburh->r.h;
	const int cursorPixels = cursorWidth * cursorHeight;
	
	vnclog.Print(6, _T("Receiving cursor shape update, cursor %dx%d\n"),
				 cursorWidth, cursorHeight);

	const int bytesPerRow = (cursorWidth + 7) / 8;
	const int bytesMaskData = bytesPerRow * cursorHeight;
	const int bytesSourceData = cursorPixels * (m_myFormat.bitsPerPixel / 8);
	CheckBufferSize(bytesMaskData);

	SoftCursorFree();

	if (cursorPixels == 0)
		return;

	if (pfburh->r.x > m_si.framebufferWidth || pfburh->r.y > m_si.framebufferHeight ||
		pfburh->r.x < 0 || pfburh->r.y < 0)
		return;


	// Ignore cursor shape updates if requested by user
	if (m_opts->m_ignoreShapeUpdates) {
		int bytesToSkip = (pfburh->encoding == rfbEncodingXCursor) ?
			(6 + 2 * bytesMaskData) : (bytesSourceData + bytesMaskData);
		CheckBufferSize(bytesToSkip);
		ReadExact(m_netbuf, bytesToSkip);
		return;
	}

	// Read cursor pixel data.

	rcSource = new COLORREF[cursorPixels];

	if (pfburh->encoding == rfbEncodingXCursor) {
		CARD8 xcolors[6];
		ReadExact((char *)xcolors, 6);
		COLORREF rcolors[2];
		rcolors[1] = PALETTERGB(xcolors[0], xcolors[1], xcolors[2]);
		rcolors[0] = PALETTERGB(xcolors[3], xcolors[4], xcolors[5]);

		ReadExact(m_netbuf, bytesMaskData);
		const int fullBytes = cursorWidth / 8;
		const int remainBits = cursorWidth % 8;
		int i = 0;
		for (int y = 0; y < cursorHeight; y++) {
			char *rowPtr = m_netbuf + y * bytesPerRow;
			for (int x = 0; x < fullBytes; x++) {
				int b = rowPtr[x];
				for (int n = 7; n >= 0; n--)
					rcSource[i++] = rcolors[(b >> n) & 1];
			}
			if (remainBits > 0) {
				int b = rowPtr[fullBytes];
				for (int n = 7; n >= 8 - remainBits; n--)
					rcSource[i++] = rcolors[(b >> n) & 1];
			}
		}
	} else {
		// rfb.EncodingRichCursor
		CheckBufferSize(bytesSourceData);
		ReadExact(m_netbuf, bytesSourceData);
		SETUP_COLOR_SHORTCUTS;
		char *p = m_netbuf;
		for (int i = 0; i < cursorPixels; i++) {
			switch (m_myFormat.bitsPerPixel) {
			case 8:
				rcSource[i] = COLOR_FROM_PIXEL8_ADDRESS(p);
				p++;
				break;
			case 16:
				rcSource[i] = *(CARD16*)p;
				p += 2;
				break;
			case 32:
				rcSource[i] = *(CARD32*)p;
				p += 4;
				break;
			}
		}
	}

	// Read and decode mask data.

	ReadExact(m_netbuf, bytesMaskData);

	rcMask = new bool[cursorPixels];

	{
		const int fullBytes = cursorWidth / 8;
		const int remainBits = cursorWidth % 8;
		int i = 0;
		for (int y = 0; y < cursorHeight; y++) {
			char *rowPtr = m_netbuf + y * bytesPerRow;
			for (int x = 0; x < fullBytes; x++) {
				int b = rowPtr[x];
				for (int n = 7; n >= 0; n--)
					rcMask[i++] = ((b >> n) & 1) != 0;
			}
			if (remainBits > 0) {
				int b = rowPtr[fullBytes];
				for (int n = 7; n >= 8 - remainBits; n--)
					rcMask[i++] = ((b >> n) & 1) != 0;
			}
		}
	}

	// Set remaining data associated with cursor.

	omni_mutex_lock l(m_bitmapdcMutex);

	rcWidth = cursorWidth;
	rcHeight = cursorHeight;
	rcHotX = (pfburh->r.x < rcWidth) ? pfburh->r.x : rcWidth - 1;
	rcHotY = (pfburh->r.y < rcHeight) ? pfburh->r.y : rcHeight - 1;

	if (m_SavedAreaBIB) delete[] m_SavedAreaBIB;
	m_SavedAreaBIB = new BYTE[cursorPixels * (m_myFormat.bitsPerPixel / 8)];

	SoftCursorSaveArea();
	SoftCursorDraw();

	rcCursorHidden = false;
	rcLockSet = false;
	prevCursorSet = true;
}

// marscha PointerPos
void ClientConnection::ReadCursorPos(rfbFramebufferUpdateRectHeader *pfburh)
{
	int x = (int)pfburh->r.x / this->m_opts->m_nServerScale;
	if (x >= m_si.framebufferWidth)
		x = m_si.framebufferWidth - 1;
	int y = (int)pfburh->r.y / this->m_opts->m_nServerScale;
	if (y >= m_si.framebufferHeight)
		y = m_si.framebufferHeight - 1;
	//vnclog.Print(2, _T("reading cursor pos (%d, %d)\n"), x, y);
	SoftCursorMove(x, y);
}

//
// SoftCursorLockArea(). This method should be used to prevent
// collisions between simultaneous framebuffer update operations and
// cursor drawing operations caused by movements of pointing device.
// The parameters denote a rectangle where mouse cursor should not
// be drawn. Every next call to this function expands locked area so
// previous locks remain active.
//

void ClientConnection::SoftCursorLockArea(int x, int y, int w, int h) {

	omni_mutex_lock l(m_bitmapdcMutex);//m_cursorMutex);

	if (!prevCursorSet)
		return;

	if (!rcLockSet) {
		rcLockX = x;
		rcLockY = y;
		rcLockWidth = w;
		rcLockHeight = h;
		rcLockSet = true;
	} else {
		int newX = (x < rcLockX) ? x : rcLockX;
		int newY = (y < rcLockY) ? y : rcLockY;
		rcLockWidth = (x + w > rcLockX + rcLockWidth) ?
			(x + w - newX) : (rcLockX + rcLockWidth - newX);
		rcLockHeight = (y + h > rcLockY + rcLockHeight) ?
			(y + h - newY) : (rcLockY + rcLockHeight - newY);
		rcLockX = newX;
		rcLockY = newY;
	}

	if (!rcCursorHidden && SoftCursorInLockedArea()) {
		SoftCursorRestoreArea();
		rcCursorHidden = true;
	}
}

//
// SoftCursorUnlockScreen(). This function discards all locks
// performed since previous SoftCursorUnlockScreen() call.
//

void ClientConnection::SoftCursorUnlockScreen() {

	omni_mutex_lock l(m_bitmapdcMutex);//m_cursorMutex);

	if (!prevCursorSet)
		return;

	if (rcCursorHidden) {
		SoftCursorSaveArea();
		SoftCursorDraw();
		rcCursorHidden = false;
	}
	rcLockSet = false;
}

//
// SoftCursorMove(). Moves soft cursor in particular location. This
// function respects locking of screen areas so when the cursor is
// moved in the locked area, it becomes invisible until
// SoftCursorUnlockScreen() method is called.
//

void ClientConnection::SoftCursorMove(int x, int y) {

	omni_mutex_lock l(m_bitmapdcMutex);//m_cursorMutex);

	if (prevCursorSet && !rcCursorHidden) {
		SoftCursorRestoreArea();
		rcCursorHidden = true;
	}

	rcCursorX = x;
	rcCursorY = y;

	if (prevCursorSet && !(rcLockSet && SoftCursorInLockedArea())) {
		SoftCursorSaveArea();
		SoftCursorDraw();
		rcCursorHidden = false;
	}
}

 //
 // Free all data associated with cursor.
 //

void ClientConnection::SoftCursorFree() {

	omni_mutex_lock l(m_bitmapdcMutex);//m_cursorMutex);

	if (prevCursorSet) {
		if (!rcCursorHidden)
			SoftCursorRestoreArea();
		if (m_SavedAreaBIB) delete[] m_SavedAreaBIB;
		m_SavedAreaBIB=NULL;
		if ( rcSource) delete[] rcSource;
		rcSource=NULL;
		if (rcMask) delete[] rcMask;
		rcMask=NULL;
		prevCursorSet = false;
	}
}

//////////////////////////////////////////////////////////////////
//
// Low-level methods implementing software cursor functionality.
//

//
// Check if cursor is within locked part of screen.
//

bool ClientConnection::SoftCursorInLockedArea() {

    return (rcLockX < rcCursorX - rcHotX + rcWidth &&
			rcLockY < rcCursorY - rcHotY + rcHeight &&
			rcLockX + rcLockWidth > rcCursorX - rcHotX &&
			rcLockY + rcLockHeight > rcCursorY - rcHotY);
}

//
// Save screen data in memory buffer.
//

void ClientConnection::SoftCursorSaveArea() {

	RECT r;
	SoftCursorToScreen(&r, NULL);
	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	omni_mutex_lock l(m_bitmapdcMutex);
	if (m_DIBbits && m_SavedAreaBIB) Copyto0buffer(w, h, x, y,m_myFormat.bitsPerPixel/8,(BYTE*)m_DIBbits,m_SavedAreaBIB,m_si.framebufferWidth,m_si.framebufferHeight);
}

//
// Restore screen data saved in memory buffer.
//

void ClientConnection::SoftCursorRestoreArea() {

	RECT r;
	SoftCursorToScreen(&r, NULL);
	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	omni_mutex_lock l(m_bitmapdcMutex);
	if (m_DIBbits && m_SavedAreaBIB) Copyfrom0buffer(w, h, x, y,m_myFormat.bitsPerPixel/8,m_SavedAreaBIB,(BYTE*)m_DIBbits,m_si.framebufferWidth,m_si.framebufferHeight);

	if (!m_opts->m_Directx)InvalidateScreenRect(&r);
}

//
// Draw cursor.
//

void ClientConnection::SoftCursorDraw() {

	omni_mutex_lock l(m_bitmapdcMutex);
	
	if (!m_DIBbits)
		return;

	const int bytesPerPixel = m_myFormat.bitsPerPixel / 8;
	const int baseX = rcCursorX - rcHotX;
	const int baseY = rcCursorY - rcHotY;
	const int fbWidth = m_si.framebufferWidth;
	const int fbHeight = m_si.framebufferHeight;

	// Pre-calculate row stride with 4-byte alignment
	int bytesPerOutputRow = fbWidth * bytesPerPixel;
	if (bytesPerOutputRow % 4)
		bytesPerOutputRow += 4 - bytesPerOutputRow % 4;

	BYTE* destBase = (BYTE*)m_DIBbits;

	for (int y = 0; y < rcHeight; y++) {
		const int y0 = baseY + y;
		if (y0 >= 0 && y0 < fbHeight) {
			const int rowOffset = y * rcWidth;
			BYTE* destRow = destBase + (bytesPerOutputRow * y0);
			for (int x = 0; x < rcWidth; x++) {
				const int x0 = baseX + x;
				if (x0 >= 0 && x0 < fbWidth) {
					const int offset = rowOffset + x;
					if (rcMask[offset]) {
						// Inline pixel copy - avoids per-pixel function call overhead
						memcpy(destRow + (x0 * bytesPerPixel), &rcSource[offset], bytesPerPixel);
					}
				}
			}
		}
	}

	RECT r;
	SoftCursorToScreen(&r, NULL);
	if (!m_opts->m_Directx) InvalidateScreenRect(&r);
}

//
// Calculate position, size and offset for the part of cursor
// located inside framebuffer bounds.
//

void ClientConnection::SoftCursorToScreen(RECT *screenArea, POINT *cursorOffset) {

	int cx = 0, cy = 0;

	int x = rcCursorX - rcHotX;
	int y = rcCursorY - rcHotY;
	int w = rcWidth;
	int h = rcHeight;

	if (x < 0) {
		cx = -x;
		w -= cx;
		x = 0;
	} else if (x + w > m_si.framebufferWidth) {
		w = m_si.framebufferWidth - x;
	}
	if (y < 0) {
		cy = -y;
		h -= cy;
		y = 0;
	} else if (y + h > m_si.framebufferHeight) {
		h = m_si.framebufferHeight - y;
	}

	if (w < 0) {
		cx = 0; x = 0; w = 0;
	}
	if (h < 0) {
		cy = 0; y = 0; h = 0;
	}

	if (screenArea != NULL) {
		SetRect(screenArea, x, y, x + w, y + h);
	}
	if (cursorOffset != NULL) {
		cursorOffset->x = cx;
		cursorOffset->y = cy;
	}
}

void ClientConnection::InvalidateScreenRect(const RECT *pRect) {
	RECT rect;

	// If we're scaling, we transform the coordinates of the rectangle
	// received into the corresponding window coords, and invalidate
	// *that* region.

	if (m_opts->m_scaling) {
		// First, we adjust coords to avoid rounding down when scaling.
		int n = m_opts->m_scale_num;
		int d = m_opts->m_scale_den;
		int left   = (pRect->left / d) * d;
		int top    = (pRect->top  / d) * d;
		int right  = (pRect->right  + d - 1) / d * d; // round up
		int bottom = (pRect->bottom + d - 1) / d * d; // round up

		// Then we scale the rectangle, which should now give whole numbers.
		rect.left   = (left   * n / d) - m_hScrollPos;
		rect.top    = (top    * n / d) - m_vScrollPos;
		rect.right  = (right  * n / d) - m_hScrollPos;
		rect.bottom = (bottom * n / d) - m_vScrollPos;
	} else {
		rect.left   = pRect->left   - m_hScrollPos;
		rect.top    = pRect->top    - m_vScrollPos;
		rect.right  = pRect->right  - m_hScrollPos;
		rect.bottom = pRect->bottom - m_vScrollPos;
	}
	InvalidateRect(m_hwndcn, &rect, FALSE);
}

void ClientConnection::InvalidateRegion(const RECT *pRect,HRGN *prgn) {
	RECT rect;

	// If we're scaling, we transform the coordinates of the rectangle
	// received into the corresponding window coords, and invalidate
	// *that* region.

	if (m_opts->m_scaling) {
		// First, we adjust coords to avoid rounding down when scaling.
		int n = m_opts->m_scale_num;
		int d = m_opts->m_scale_den;
		int left   = (pRect->left / d) * d;
		int top    = (pRect->top  / d) * d;
		int right  = (pRect->right  + d - 1) / d * d; // round up
		int bottom = (pRect->bottom + d - 1) / d * d; // round up

		// Then we scale the rectangle, which should now give whole numbers.
		rect.left   = (left   * n / d) - m_hScrollPos;
		rect.top    = (top    * n / d) - m_vScrollPos;
		rect.right  = (right  * n / d) - m_hScrollPos;
		rect.bottom = (bottom * n / d) - m_vScrollPos;
	} else {
		rect.left   = pRect->left   - m_hScrollPos;
		rect.top    = pRect->top    - m_vScrollPos;
		rect.right  = pRect->right  - m_hScrollPos;
		rect.bottom = pRect->bottom - m_vScrollPos;
	}
	HRGN tempregion = CreateRectRgnIndirect(&rect);
	CombineRgn(*prgn,*prgn,tempregion,RGN_OR);
	DeleteObject(tempregion);
}

