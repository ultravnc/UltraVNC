// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2025 UltraVNC Team Members. All Rights Reserved.

#ifndef _MINIZIPNG_H
#define _MINIZIPNG_H

#include <windows.h>

// Unicode-aware zip/unzip wrapper using minizip-ng
// Compatible interface with ZipUnZip32 but supports Unicode paths
class CMiniZipNG
{
public:
    CMiniZipNG();
    ~CMiniZipNG();

    // Zip a directory (Unicode paths supported)
    // szDirPath: Directory to zip (e.g., "C:\folder")
    // szWildcard: Wildcard pattern (e.g., "C:\folder\*.*")
    // szZipPath: Output zip file path
    // bRecursive: Recurse into subdirectories
    // Returns: true on success
    bool ZipDirectory(LPCWSTR szDirPath, LPCWSTR szWildcard, LPCWSTR szZipPath, bool bRecursive);

    // Unzip a directory (Unicode paths supported)
    // szExtractPath: Where to extract (e.g., "C:\dest")
    // szZipPath: Zip file to extract
    // Returns: true on success
    bool UnZipDirectory(LPCWSTR szExtractPath, LPCWSTR szZipPath);

    // Legacy ANSI interface for compatibility
    bool ZipDirectory(LPCSTR szDirPath, LPCSTR szWildcard, LPCSTR szZipPath, bool bRecursive);
    bool UnZipDirectory(LPCSTR szExtractPath, LPCSTR szZipPath);

private:
    bool ZipDirectoryRecursive(void* zip_writer, LPCWSTR szBasePath, LPCWSTR szRelPath);
    bool AddFileToZip(void* zip_writer, LPCWSTR szFilePath, LPCWSTR szZipPath);
};

#endif // _MINIZIPNG_H
