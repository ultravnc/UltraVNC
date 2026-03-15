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
#include "Exception.h"
#include "MEssBox.h"

Exception::Exception(const wchar_t *info, int error_nr) : m_error_nr(-1)
{
	assert(info != NULL);
	size_t len = wcslen(info) + 1;
	m_info = new wchar_t[len];
	wcscpy_s(m_info, len, info);
	if (error_nr)
		m_error_nr = error_nr;
}

Exception::~Exception()
{
	delete[] m_info;
}

// ---------------------------------------


QuietException::QuietException(const wchar_t *info, int error_nr) : Exception(info, error_nr)
{

}

QuietException::~QuietException()
{

}

void QuietException::Report()
{
#ifdef _MSC_VER
	_RPT1(_CRT_WARN, "Warning : %S\n", GetInfo());
#endif
}

// ---------------------------------------

WarningException::WarningException(const wchar_t *info, int error_nr, bool close) : Exception(info, error_nr)
{
	m_close = close;
}

WarningException::~WarningException()
{

}

void WarningException::Report()
{
#ifdef _MSC_VER
	_RPT1(_CRT_WARN, "Warning : %S\n", GetInfo());
#endif
	ShowMessageBox2(GetInfo(), m_error_nr);
}

// ---------------------------------------

ErrorException::ErrorException(const wchar_t *info, int error_nr) : Exception(info, error_nr)
{

}

ErrorException::~ErrorException()
{

}

void ErrorException::Report()
{
#ifdef _MSC_VER
	_RPT1(_CRT_WARN, "Warning : %S\n", GetInfo());
#endif
	ShowMessageBox2(GetInfo(), m_error_nr);
}
