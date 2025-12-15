// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTHSSP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTHSSP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef AUTHSSP_EXPORTS
#define AUTHSSP_API __declspec(dllexport)
#else
#define AUTHSSP_API __declspec(dllimport)
#endif

#include <windows.h>
#include <aclui.h>
#include <aclapi.h>
#include <stdio.h>

struct vncSecurityInfo : ISecurityInformation
{
	long  m_cRefs;
	const wchar_t* const m_pszObjectName;
	const wchar_t* const m_pszPageTitle;
	
	vncSecurityInfo(const wchar_t* pszObjectName,
		const wchar_t* pszPageTitle = 0 )
		: m_cRefs(0),
		m_pszObjectName(pszObjectName),
		m_pszPageTitle(pszPageTitle) {}

	STDMETHODIMP QueryInterface( REFIID iid, void** ppv );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP GetObjectInformation( SI_OBJECT_INFO* poi );
	STDMETHODIMP GetSecurity(SECURITY_INFORMATION ri, PSECURITY_DESCRIPTOR * ppsd, BOOL bDefault);
	STDMETHODIMP SetSecurity(SECURITY_INFORMATION ri, void* psd);
	STDMETHODIMP PropertySheetPageCallback(HWND hwnd, UINT msg, SI_PAGE_TYPE pt);
	STDMETHODIMP GetAccessRights(const GUID*,
								 DWORD dwFlags,
								 SI_ACCESS** ppAccess,
								 ULONG* pcAccesses,
								 ULONG* piDefaultAccess);
	STDMETHODIMP MapGeneric(const GUID*, UCHAR* pAceFlags, ACCESS_MASK* pMask);
	STDMETHODIMP GetInheritTypes(SI_INHERIT_TYPE** ppInheritTypes, ULONG* pcInheritTypes);
};

AUTHSSP_API void vncEditSecurity(HWND hwnd, HINSTANCE hInstance);

bool CheckAclUI();
extern TCHAR *AddToModuleDir(TCHAR *filename, int length);