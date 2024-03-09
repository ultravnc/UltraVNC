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


void Add_viewer_list(mystruct *Viewerstruct);
ULONG  Find_server_list(mystruct *Serverstruct);
ULONG Find_viewer_list(mystruct *Viewerstruct);
ULONG Find_viewer_list_id(mystruct *Viewerstruct);
void Remove_viewer_list(ULONG code);
void Remove_server_list(ULONG code);
void Add_server_list(mystruct *Serverstruct);
ULONG Find_server_list_id(mystruct *Serverstruct);
void Clean_server_List();
void Clean_viewer_List();
void Start_cleaupthread();
void Stop_cleaupthread();
