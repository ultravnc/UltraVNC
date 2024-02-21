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


#define MAJOR_INT 1
#define MINOR_INT 2
#define BUILD_INT 4
#define SPECIAL_INT 0

#define BUILD_NUMBER "1.2.4.0"



#ifdef _WITH_REGISTRY
#define PLUGIN_NAME "UltraVNC MS RC4 Encryption Plugin"
#define PLUGIN_FILE "MSRC4Plugin.dsm"
#else
#define PLUGIN_NAME "UltraVNC MS RC4 Encryption Plugin"
#define PLUGIN_FILE "MSRC4Plugin140.dsm"
#endif
