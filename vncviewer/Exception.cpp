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

Exception::Exception(const char *info,int error_nr) : m_error_nr(-1)
{
	assert(info != NULL);
	m_info = new char[strlen(info)+1];
	strcpy_s(m_info, strlen(info)+1, info);
    if (error_nr)
	m_error_nr=error_nr;
}

Exception::~Exception()
{
	delete [] m_info;
}

// ---------------------------------------


QuietException::QuietException(const char *info,int error_nr) : Exception(info,error_nr)
{

}

QuietException::~QuietException()
{

}

void QuietException::Report()
{
#ifdef _MSC_VER
	_RPT1(_CRT_WARN, "Warning : %s\n", m_info);
#endif
}

// ---------------------------------------

WarningException::WarningException(const char *info,int error_nr, bool close) : Exception(info,error_nr)
{
	m_close = close;
}

WarningException::~WarningException()
{

}

void WarningException::Report()
{
#ifdef _MSC_VER
	_RPT1(_CRT_WARN, "Warning : %s\n", m_info);
#endif
	ShowMessageBox2(m_info,m_error_nr);
}

// ---------------------------------------

ErrorException::ErrorException(const char *info,int error_nr) : Exception(info,error_nr)
{

}

ErrorException::~ErrorException()
{

}

void ErrorException::Report()
{
#ifdef _MSC_VER
	_RPT1(_CRT_WARN, "Warning : %s\n", m_info);
#endif
	ShowMessageBox2(m_info,m_error_nr);
}
