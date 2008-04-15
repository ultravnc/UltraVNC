//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


#include "stdhdrs.h"
#include "Exception.h"
#include "messbox.h"

Exception::Exception(const char *info,int error_nr) : m_error_nr(-1)
{
	assert(info != NULL);
	m_info = new char[strlen(info)+1];
	strcpy(m_info, info);
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
	//MessageBox(NULL, m_info, " Ultr@VNC Info", MB_OK| MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
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
	//MessageBox(NULL, m_info, " Ultr@VNC Info", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
}
