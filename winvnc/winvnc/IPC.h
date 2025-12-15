// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#pragma once

struct mystruct
{
	short counter;
	short locked;
	RECT rect1[2000];
	RECT rect2[2000];
	short type[2000];
	short test;
};

// Class for Inter Process Communication using Memory Mapped Files
class CIPC
{
public:
	CIPC();
	virtual ~CIPC();

	/**unsigned char * CreateBitmap();
	unsigned char * CreateBitmap(int size);
	void CloseBitmap();*/
	bool CreateIPCMMF(void);
	bool OpenIPCMMF(void);
	void CloseIPCMMF(void);
	mystruct*  listall();

	bool CreateIPCMMFBitmap(int size);
	bool OpenIPCMMFBitmap(void);
	void CloseIPCMMFBitmap(void);

	bool IsOpen(void) const {return (m_hFileMap != NULL);}

protected:
	HANDLE m_hFileMap;
	LPVOID m_FileView;

	HANDLE m_hFileMapBitmap;
	LPVOID m_FileViewBitmap;

	mystruct list;
	mystruct *plist;
	unsigned char *pBitmap;
};
