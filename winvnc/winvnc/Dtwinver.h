// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


/*
Module : Dtwinver.h
Purpose: Declaration of a comprehensive class to perform OS version detection
Created: PJN / 11-05-1996

Copyright (c) 1997 - 2024 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise)
when your product is released in binary form. You are allowed to modify the source code in any way you want
except you cannot modify the copyright details at the top of each module. If you want to distribute source
code with your application, then you are only allowed to distribute versions released by the author. This is
to maintain a single distribution point for the source code.

*/


////////////////////////////////// Initial setup //////////////////////////////

#ifndef __DTWINVER_H__
#define __DTWINVER_H__

//Preprocessor values we need to control the various code paths
#if defined(UNDER_CE)
  #define COSVERSION_CE 1
#endif
#if defined(_DOS)
  #define COSVERSION_DOS 1
#else
  #define COSVERSION_WIN 1
  #if defined(_WIN64)
    #define COSVERSION_WIN64 1
  #elif defined(_WIN32)
    #define COSVERSION_WIN32 1
  #else
    #define COSVERSION_WIN16 1
  #endif
#endif

#if defined(COSVERSION_DOS) || defined(COSVERSION_WIN16)
  #define COSVERSION_WIN16_OR_DOS 1
#endif


////////////////////////////////// Includes ///////////////////////////////////
#include <winsock2.h>
#include <windows.h> 
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
#include <tchar.h>
#else
#include <ctype.h>
#include <stdlib.h>
#include <shellapi.h>
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
#include <string.h> //NOLINT(modernize-deprecated-headers)
#include <stdio.h> //NOLINT(modernize-deprecated-headers)
#include <stdarg.h> //NOLINT(modernize-deprecated-headers)


////////////////////////////////// Defines ////////////////////////////////////

#define COSVERSION_SUITE_SMALLBUSINESS                         0x00000001 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_ENTERPRISE                            0x00000002 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER             0x00000004 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER              0x00000008 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_TERMINAL                              0x00000010 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_DATACENTER                            0x00000020 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_PERSONAL                              0x00000040 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_WEBEDITION                            0x00000080 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_EMBEDDED                              0x00000100 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL              0x00000200 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_UNIPROCESSOR_FREE                     0x00000400 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_UNIPROCESSOR_CHECKED                  0x00000800 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_MULTIPROCESSOR_FREE                   0x00001000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_MULTIPROCESSOR_CHECKED                0x00002000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_MEDIACENTER                           0x00004000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_TABLETPC                              0x00008000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_STARTER_EDITION                       0x00010000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_R2_EDITION                            0x00020000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_COMPUTE_SERVER                        0x00040000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_STORAGE_SERVER                        0x00080000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_SECURITY_APPLIANCE                    0x00100000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_BACKOFFICE                            0x00200000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_ULTIMATE                              0x00400000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_N                                     0x00800000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_HOME_BASIC                            0x01000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_HOME_PREMIUM                          0x02000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_HYPERV_TOOLS                          0x04000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_BUSINESS                              0x08000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_HOME_SERVER                           0x10000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_SERVER_CORE                           0x20000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MANAGEMENT  0x40000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING   0x80000000 //NOLINT(modernize-macro-to-enum)

#define COSVERSION_SUITE2_ESSENTIAL_BUSINESS_SERVER_SECURITY   0x00000001 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_CLUSTER_SERVER                       0x00000002 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_SMALLBUSINESS_PREMIUM                0x00000004 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_STORAGE_EXPRESS_SERVER               0x00000008 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_STORAGE_WORKGROUP_SERVER             0x00000010 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_STANDARD                             0x00000020 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_E                                    0x00000040 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_PROFESSIONAL                         0x00000080 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_FOUNDATION                           0x00000100 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_MULTIPOINT                           0x00000200 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_HYPERV_SERVER                        0x00000400 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_HOME_SERVER_PREMIUM                  0x00000800 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_STORAGE_SERVER_ESSENTIALS            0x00001000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_PRERELEASE                           0x00002000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_EVALUATION                           0x00004000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_PREMIUM                              0x00008000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_MULTIPOINT_SERVER_PREMIUM            0x00010000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_THINPC                               0x00020000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_AUTOMOTIVE                           0x00040000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_CHINA                                0x00080000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_SINGLE_LANGUAGE                      0x00100000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_WIN32S                               0x00200000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_WINDOWS812012R2UPDATE                0x00400000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_CORECONNECTED                        0x00800000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_EDUCATION                            0x01000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_INDUSTRY                             0x02000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_CORE                                 0x04000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_STUDENT                              0x08000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_MOBILE                               0x10000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_IOTUAP                               0x20000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_LTSB                                 0x40000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE2_NANO_SERVER                          0x80000000 //NOLINT(modernize-macro-to-enum)

#define COSVERSION_SUITE3_CLOUD_STORAGE_SERVER                 0x00000001 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_ARM64_SERVER                         0x00000002 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_PPI_PRO                              0x00000004 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_CONNECTED_CAR                        0x00000008 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_HANDHELD                             0x00000010 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_CLOUD_HOST_INFRASTRUCTURE_SERVER     0x00000020 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_HOLOGRAPHIC                          0x00000040 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_SUBSCRIPTION                         0x00000080 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_UTILITY_VM                           0x00000100 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_PRO_WORKSTATIONS                     0x00000200 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_AZURE                                0x00000400 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_IOTCOMMERCIAL                        0x00000800 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_ENTERPRISEG                          0x00001000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_S                                    0x00002000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_SERVERRDSH                           0x00004000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_HUBOS                                0x00008000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_COMMUNICATIONS_SERVER                0x00010000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_RESTRICTED                           0x00020000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_ONECOREUPDATEOS                      0x00040000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_ANDROMEDA                            0x00080000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_XBOX_SYSTEMOS                        0x00100000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_IOTOS                                0x00200000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_IOTEDGEOS                            0x00400000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_IOTENTERPRISE                        0x00800000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_LITE                                 0x01000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_XBOX_NATIVEOS                        0x02000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_XBOX_GAMEOS                          0x04000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_XBOX_ERAOS                           0x08000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_XBOX_DURANGOHOSTOS                   0x10000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_XBOX_SCARLETTHOSTOS                  0x20000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_AZURESTACKHCI_SERVER_CORE            0x40000000 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE3_DATACENTER_SERVER_AZURE_EDITION      0x80000000 //NOLINT(modernize-macro-to-enum)

#define COSVERSION_SUITE4_DATACENTER_SERVER_CORE_AZURE_EDITION 0x00000001 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_AZURE_SERVER_CLOUDHOST               0x00000002 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_AZURE_SERVER_CLOUDMOS                0x00000004 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_WINDOWS365                           0x00000008 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_XBOX_KEYSTONE                        0x00000010 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_WNC                                  0x00000020 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_VALIDATION                           0x00000040 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_SERVER_FOR_SB_SOLUTIONS              0x00000080 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_SERVER_SOLUTIONS                     0x00000100 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_UNLICENSED                           0x00000200 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_AZURE_SERVER_AGENTBRIDGE             0x00000400 //NOLINT(modernize-macro-to-enum)
#define COSVERSION_SUITE4_AZURE_SERVER_NANOHOST                0x00000800 //NOLINT(modernize-macro-to-enum)

#ifndef _Success_
#define _Success_(expr)
#endif //#ifndef _Success_

#ifndef _In_
#define _In_
#endif //#ifndef _In_

#ifndef _Inout_
#define _Inout_
#endif //#ifndef _Inout_

#ifndef _Out_
#define _Out_
#endif //#ifndef _Out_

#ifndef _In_z_
#define _In_z_
#endif //#ifndef _In_z_


////////////////////////////////// Classes ////////////////////////////////////

class COSVersion
{
public:
//Enums
  enum OS_PLATFORM
  {
    UnknownOSPlatform = 0,
    Dos = 1,
    Windows3x = 2,
    Windows9x = 3,
    WindowsNT = 4,
    WindowsCE = 5,
  };

  enum OS_TYPE
  {
    UnknownOSType = 0,
    Workstation = 1,
    Server = 2,
    DomainController = 3,
  };

  enum PROCESSOR_TYPE
  {
    UNKNOWN_PROCESSOR = 0,
    X86_PROCESSOR = 1,
    MIPS_PROCESSOR = 2,
    ALPHA_PROCESSOR = 3,
    PPC_PROCESSOR = 4,
    IA64_PROCESSOR = 5,
    AMD64_PROCESSOR = 6,
    ALPHA64_PROCESSOR = 7,
    MSIL_PROCESSOR = 8,
    ARM_PROCESSOR = 9,
    SHX_PROCESSOR = 10,
    IA32_ON_WIN64_PROCESSOR = 11,
    NEUTRAL_PROCESSOR = 12,
    ARM64_PROCESSOR = 13,
    ARM32_ON_WIN64_PROCESSOR = 14,
    IA32_ON_ARM64_PROCESSOR = 15
  };

//Defines
  struct OS_VERSION_INFO
  {
    //What version of OS is being emulated
    DWORD dwEmulatedMajorVersion;
    DWORD dwEmulatedMinorVersion;
    DWORD dwEmulatedBuildNumber;
    OS_PLATFORM EmulatedPlatform;
  #if !defined(COSVERSION_CE)
    PROCESSOR_TYPE EmulatedProcessorType; //The emulated processor type
  #endif
  #if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  #if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    TCHAR szEmulatedCSDVersion[128]; //NOLINT(modernize-avoid-c-arrays)
  #endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  #else
    char szEmulatedCSDVersion[128]; //NOLINT(modernize-avoid-c-arrays)
  #endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    WORD wEmulatedServicePackMajor;
    WORD wEmulatedServicePackMinor;

    //What version of OS is really running
    DWORD dwUnderlyingMajorVersion;
    DWORD dwUnderlyingMinorVersion;
    DWORD dwUnderlyingBuildNumber;
    OS_PLATFORM UnderlyingPlatform;
  #if !defined(COSVERSION_CE)
    PROCESSOR_TYPE UnderlyingProcessorType; //The underlying processor type
  #endif //#if !defined(COSVERSION_CE)
  #if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
  #if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    TCHAR szUnderlyingCSDVersion[128]; //NOLINT(modernize-avoid-c-arrays)
  #endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  #else
    char szUnderlyingCSDVersion[128]; //NOLINT(modernize-avoid-c-arrays)
  #endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    WORD wUnderlyingServicePackMajor;
    WORD wUnderlyingServicePackMinor;
    DWORD dwSuiteMask; //Bitmask of various OS suites
    DWORD dwSuiteMask2; //Second bitmask of various OS suites
    DWORD dwSuiteMask3; //Third bitmask of various OS suites
    DWORD dwSuiteMask4; //Fourth bitmask of various OS suites
    OS_TYPE OSType; //The basic OS type
    DWORD dwUBR; //"Updated Build Revision" value. Only applicable for Windows 10 / Server 2016+
    DWORD dwProductType; //The value as returned from GetProductInfo
    BOOL bSemiAnnual; //Is this version of Windows from a Semi-annual server release
    ULONGLONG ullUAPInfo; //The first value returned from RtlGetDeviceFamilyInfoEnum
    DWORD ulDeviceFamily; //The second value returned from RtlGetDeviceFamilyInfoEnum
    DWORD ulDeviceForm; //The third value returned from RtlGetDeviceFamilyInfoEnum

  #if defined(COSVERSION_CE)
    TCHAR szOEMInfo[256];
    TCHAR szPlatformType[256];
  #endif //#if defined(COSVERSION_CE)
  };
  typedef OS_VERSION_INFO* POS_VERSION_INFO; //NOLINT(modernize-use-using)
  typedef FAR OS_VERSION_INFO* LPOS_VERSION_INFO; //NOLINT(modernize-use-using)
  typedef const FAR OS_VERSION_INFO* LPCOS_VERSION_INFO; //NOLINT(modernize-use-using)

//Constructors / Destructors
  COSVersion();
  ~COSVersion();

//Methods:
  _Success_(return != FALSE) BOOL GetVersion(_Inout_ LPOS_VERSION_INFO lpVersionInformation);

//Please note that the return values for the following group of functions 
//are mutually exclusive for example if you are running on 
//95 OSR2 then IsWindows95 will return FALSE etc.
  _Success_(return != FALSE) BOOL IsWindows30(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows31(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows311(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsForWorkgroups(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsCE(_In_ LPCOS_VERSION_INFO const lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows95(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows95SP1(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows95B(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows95C(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows98(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows98SP1(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows98SE(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsME(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsNT31(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsNT35(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsNT351(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsNT4(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows2000(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsXPOrWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsVistaOrWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows7OrWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows8OrWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows8Point1OrWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10OrWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2019(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

//Returns the various flavors of the "os" that is installed. Note that these
//functions are not completely mutually exclusive
  _Success_(return != FALSE) BOOL IsWin32sInstalled(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsNTPreWin2k(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsNTWorkstation(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsNTStandAloneServer(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsNTPDC(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsNTBDC(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsNTEnterpriseServer(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsNTDatacenterServer(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWindows2000Professional(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows2000Server(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows2000AdvancedServer(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows2000DatacenterServer(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows2000DomainController(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWindowsXP(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsXPPersonal(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsXPProfessional(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsVista(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows7(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows8(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows8Point1(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows8Point1Update(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1507(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1511(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1607(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1703(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1709(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1803(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1809(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1903(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version1909(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version2004(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version20H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version21H1(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version21H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10Version22H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows10ActiveDevelopmentBranch(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows11(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows11Version21H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows11Version22H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows11Version23H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindows11ActiveDevelopmentBranch(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2003(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2003R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2003R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2003R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2003R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2003R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2003R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2008R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2012(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2012R2Update(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2012R2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2016(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion1709(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion1803(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion1809(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion1903(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion1909(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion2004(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion20H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2019(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2019(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2019(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2019(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2019(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2019ActiveDevelopmentBranch(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWindowsServer2022(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWebWindowsServer2022(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2022(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2022(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2022(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2022(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2022ActiveDevelopmentBranch(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServerVersion23H2(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsWindowsServer2025(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWebWindowsServer2025(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2025(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2025(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2025(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2025(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsHomeBasic(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHomeBasicPremium(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsBusiness(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsProfessional(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEnterprise(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsUltimate(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsPersonal(_In_ LPCOS_VERSION_INFO lpVersionInformation);

  _Success_(return != FALSE) BOOL IsWebWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsStandardWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEnterpriseWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDatacenterWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsDomainControllerWindowsServer2008(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);

  _Success_(return != FALSE) BOOL IsTerminalServices(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsSmallBusiness(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsSmallBusinessServerPremium(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsRestricted(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL Is64Bit(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsEmbedded(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsTerminalServicesInRemoteAdminMode(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsMediaCenter(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsTabletPC(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsStarterEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsComputeClusterServerEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHomeServerEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHomeServerPremiumEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsSecurityAppliance(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsBackOffice(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsNEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHyperVTools(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHyperVServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsServerCore(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsMultiPointServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsServerFoundation(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsUniprocessorFree(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsUniprocessorChecked(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsMultiprocessorFree(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsMultiprocessorChecked(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEssentialBusinessServerManagement(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEssentialBusinessServerMessaging(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEssentialBusinessServerSecurity(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsClusterServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEnterpriseStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsExpressStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsStandardStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWorkgroupStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEssentialsStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsPreRelease(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEvaluation(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWindowsRT(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsWindowsCENET(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsPremium(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWindowsEmbeddedCompact(_In_ LPCOS_VERSION_INFO lpVersionInformation, _In_ BOOL bCheckUnderlying);
  _Success_(return != FALSE) BOOL IsMultipointServerPremiumEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsThinPC(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAutomotive(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsChina(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsSingleLanguage(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWindows8Point1Or2012R2Update(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsCoreConnected(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEducation(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIndustry(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsCore(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsStudent(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsMobile(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsCloudHostInfrastructureServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsLTSB(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsNanoServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsCloudStorageServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsARM64Server(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsPPIPro(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsConnectedCar(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHandheld(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHolographic(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsSubscription(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsUtilityVM(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsProWorkstations(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAzure(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsSEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsEnterpriseG(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsServerRDSH(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsHubOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsCommunicationsServer(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsOneCoreUpdateOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAndromeda(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTCore(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTCommercial(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTEdgeOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTEnterprise(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTEnterpriseSK(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTEnterpriseK(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsIoTEnterpriseEvaluation(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWNC(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsValidation(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAzureServerAgentBridge(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAzureServerNanoHost(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWindows10X(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxSystemOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxNativeOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxGamesOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxEraOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxDurangoHostOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxScarlettHostOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsXBoxKeystone(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAzureStackHCIServerCore(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsDatacenterServerAzureEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsDatacenterServerCoreAzureEdition(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAzureServerCloudhost(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsAzureServerCloudMOS(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsWindows365(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsUnlicensed(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsServerForSBSolutions(_In_ LPCOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) BOOL IsServerSolutions(_In_ LPCOS_VERSION_INFO lpVersionInformation);

protected:
//Defines / typedefs
#if defined(COSVERSION_WIN16_OR_DOS)
  #define CPEX_DEST_STDCALL 0x00000000L
  #define HKEY32 DWORD
  #define HKEY_LOCAL_MACHINE (( HKEY32 ) 0x80000002 )
  #define TCHAR char
  #define WCHAR unsigned short
  #define _T
  #define _tcsicmp strcmpi
  #define _tcscpy strcpy
  #define _tcslen strlen
  #define _istdigit isdigit
  #define _ttoi atoi
  #define _tcsupr strupr
  #define _tcsstr strstr
  #define _stscanf sscanf
  #define _tcsstr strstr
  #define LPCTSTR LPCSTR
  #define ERROR_CALL_NOT_IMPLEMENTED 120
  #define REG_DWORD ( 4 )
  #define REG_MULTI_SZ ( 7 )
  #define VER_PLATFORM_WIN32s 0
  #define VER_PLATFORM_WIN32_WINDOWS 1
  #define VER_PLATFORM_WIN32_NT 2
  #define VER_PLATFORM_WIN32_CE 3
  #define LPTSTR LPSTR

  typedef struct OSVERSIONINFO
  {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    char szCSDVersion[128];
  } OSVERSIONINFO, *POSVERSIONINFO, FAR *LPOSVERSIONINFO;

  typedef struct _SYSTEM_INFO
  {
    union
    {
      DWORD dwOemId;
      struct
      {
        WORD wProcessorArchitecture;
        WORD wReserved;
      };
    };
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
  } SYSTEM_INFO, *LPSYSTEM_INFO;

  #define PROCESSOR_ARCHITECTURE_INTEL 0
  #define PROCESSOR_ARCHITECTURE_MIPS 1
  #define PROCESSOR_ARCHITECTURE_ALPHA 2
  #define PROCESSOR_ARCHITECTURE_PPC 3
  #define PROCESSOR_ARCHITECTURE_SHX 4
  #define PROCESSOR_ARCHITECTURE_ARM 5
  #define PROCESSOR_ARCHITECTURE_IA64 6
  #define PROCESSOR_ARCHITECTURE_ALPHA64 7
  #define PROCESSOR_ARCHITECTURE_MSIL  8
  #define PROCESSOR_ARCHITECTURE_AMD64 9
  #define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 10
  #define PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF

//Methods
  static BOOL WFWLoaded();
  LONG RegQueryValueEx(HKEY32 hKey, LPSTR lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE  lpbData, LPDWORD lpcbData);

//Function Prototypes
  typedef DWORD (FAR PASCAL *lpfnLoadLibraryEx32W)(LPCSTR, DWORD, DWORD);
  typedef BOOL (FAR PASCAL *lpfnFreeLibrary32W)(DWORD);
  typedef DWORD (FAR PASCAL *lpfnGetProcAddress32W)(DWORD, LPCSTR);
  typedef DWORD (FAR __cdecl *lpfnCallProcEx32W)(DWORD, DWORD, DWORD, DWORD, ...);
  typedef WORD (FAR PASCAL *lpfnWNetGetCaps)(WORD);
  typedef BOOL (FAR PASCAL *lpfnGetProductInfo)(DWORD, DWORD, DWORD, DWORD, PDWORD);

//Member variables
  lpfnLoadLibraryEx32W m_lpfnLoadLibraryEx32W;
  lpfnFreeLibrary32W m_lpfnFreeLibrary32W;
  lpfnGetProcAddress32W m_lpfnGetProcAddress32W;
  lpfnCallProcEx32W m_lpfnCallProcEx32W;
  DWORD m_hAdvApi32;
  DWORD m_hKernel32;
  DWORD m_lpfnRegQueryValueExA;
  DWORD m_lpfnGetVersionExA;
  DWORD m_lpfnGetVersion;
  DWORD m_lpfnGetSystemInfo;
  DWORD m_lpfnGetNativeSystemInfo;
  DWORD m_lpfnGetProductInfo;
#endif //#if defined(COSVERSION_WIN16_OR_DOS)

#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
//Function Prototypes
  typedef BOOL (WINAPI *lpfnIsWow64Process)(HANDLE, PBOOL); //NOLINT(modernize-use-using)
  typedef void (WINAPI *lpfnGetNativeSystemInfo)(LPSYSTEM_INFO); //NOLINT(modernize-use-using)
  typedef BOOL (WINAPI *lpfnGetProductInfo)(DWORD, DWORD, DWORD, DWORD, DWORD*); //NOLINT(modernize-use-using)
#endif

//Defines / Macros
  typedef struct OSVERSIONINFOEX //NOLINT(modernize-use-using)
  {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    TCHAR szCSDVersion[128]; //NOLINT(modernize-avoid-c-arrays)
    WORD wServicePackMajor;
    WORD wServicePackMinor;
    WORD wSuiteMask;
    BYTE wProductType;
    BYTE wReserved;
  } OSVERSIONINFOEX, *POSVERSIONINFOEX, *LPOSVERSIONINFOEX; //NOLINT(modernize-use-using)

  typedef struct _OSVERSIONINFOEXW //NOLINT(modernize-use-using)
  {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WCHAR szCSDVersion[128]; //NOLINT(modernize-avoid-c-arrays)
    WORD wServicePackMajor;
    WORD wServicePackMinor;
    WORD wSuiteMask;
    BYTE wProductType;
    BYTE wReserved;
  } RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW; //NOLINT(modernize-use-using)

#if defined(_WIN32) || defined(_WINDOWS)
//Function Prototypes
  typedef BOOL (WINAPI *lpfnGetVersionEx)(LPOSVERSIONINFO); //NOLINT(modernize-use-using)
  typedef DWORD (WINAPI *lpfnGetVersion)(); //NOLINT(modernize-use-using)
  typedef LONG (WINAPI *lpfnRtlGetVersion)(PRTL_OSVERSIONINFOEXW); //NOLINT(modernize-use-using)
  typedef VOID (WINAPI *lpfnRtlGetDeviceFamilyInfo)(ULONGLONG*, DWORD*, DWORD*); //NOLINT(modernize-use-using)
#endif //#if defined(_WIN32) || defined(_WINDOWS)

//Methods
#if defined(COSVERSION_DOS)
  BOOL GetInfoBySpawingWriteVer(COSVersion::LPOS_VERSION_INFO lpVersionInformation);
  void GetWinInfo();
#else
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  void GetNTSP6aDetailsFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation, _In_ BOOL bUpdateEmulatedAlso);
  void GetXPSP1aDetailsFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation, _In_ BOOL bUpdateEmulatedAlso);
  void GetUBRFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetSemiAnnualFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  OS_TYPE GetNTOSTypeFromRegistry();
  OS_TYPE GetNTOSTypeFromRegistry(_In_ HKEY hKeyProductOptions);
  void GetNTOSTypeFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation, _In_ BOOL bOnlyUpdateDCDetails);
  void GetBingEditionIDFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetProductSuiteDetailsFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetTerminalServicesRemoteAdminModeDetailsFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetMediaCenterDetails(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetProductInfo(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetTabletPCDetails(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetStarterEditionDetails(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetR2Details(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void GetNTHALDetailsFromRegistry(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif //#if defined(COSVERSION_DOS)

#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  static DWORD GetNTCurrentBuildFromRegistry(_In_ HKEY hKeyCurrentVersion);
  _Success_(return != FALSE) static BOOL GetNTCurrentBuildFromRegistry(_Out_ DWORD& dwCurrentBuild);
  _Success_(return != FALSE) static BOOL GetNTRTLVersion(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) static BOOL GetDeviceFamilyInfo(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  _Success_(return != FALSE) static BOOL GetWindows8Point1Or2012R2Update(_Inout_ LPOS_VERSION_INFO lpVersionInformation);
  void _GetVersion(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber, _In_z_ const TCHAR* szCSDVersion, _In_ WORD wServicePackMajor,
                   _In_ WORD wServicePackMinor, _In_ DWORD dwPlatformId, _In_ BOOL bOnlyUpdateDCDetails, _Inout_ LPOS_VERSION_INFO lpVersionInformation);
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)   

  _Success_(return != FALSE) static BOOL IsWindows95SP1(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  _Success_(return != FALSE) static BOOL IsWindows95B(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  _Success_(return != FALSE) static BOOL IsWindows95C(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  _Success_(return != FALSE) static BOOL IsWindows98(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  _Success_(return != FALSE) static BOOL IsWindows98SP1(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  _Success_(return != FALSE) static BOOL IsWindows98SE(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  _Success_(return != FALSE) static BOOL IsWindowsME(_In_ DWORD dwMajorVersion, _In_ DWORD dwMinorVersion, _In_ DWORD dwBuildNumber);
  static WORD GetNTServicePackFromCSDString(_In_z_ LPCTSTR pszCSDVersion);
  static PROCESSOR_TYPE MapProcessorArchitecture(_In_ WORD wProcessorArchitecture);
  static void MapWin32SuiteMask(_In_ WORD wSuiteMask, _Inout_ LPOS_VERSION_INFO lpVersionInformation);
  static OS_TYPE MapWin32ProductType(_In_ WORD wProductType);
  static OS_PLATFORM MapWin32PlatformId(_In_ DWORD dwPlatformId);
  _Success_(return != FALSE) BOOL GetProcessorType(_Inout_ LPOS_VERSION_INFO lpVersionInformation);

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  _Success_(return != FALSE) BOOL GetVersion(_Out_ DWORD& dwVersion);
  _Success_(return != FALSE) BOOL GetVersionEx(_Inout_ LPOSVERSIONINFO lpVersionInfo);
  _Success_(return != FALSE) BOOL GetVersionEx(_Inout_ LPOSVERSIONINFOEX lpVersionInfo);
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
};

#endif //__DTWINVER_H__