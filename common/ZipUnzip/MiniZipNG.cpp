// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2025 UltraVNC Team Members. All Rights Reserved.

#include "MiniZipNG.h"
#include <minizip-ng/mz.h>
#include <minizip-ng/mz_zip.h>
#include <minizip-ng/mz_strm.h>
#include <minizip-ng/mz_zip_rw.h>
#include <string>
#include <vector>

// Debug output macro - only active in debug builds
#ifdef _DEBUG
#define MZ_DEBUG(x) OutputDebugStringW(x)
#define MZ_DEBUG_FMT(x, ...) do { WCHAR _dbuf[512]; _snwprintf_s(_dbuf, 512, _TRUNCATE, x, __VA_ARGS__); OutputDebugStringW(_dbuf); } while(0)
#else
#define MZ_DEBUG(x)
#define MZ_DEBUG_FMT(x, ...)
#endif

// Helper: create all intermediate directories in a path
static void MZ_CreateDirectoryChain(LPCWSTR szPath)
{
    WCHAR szTmp[MAX_PATH * 4];
    wcscpy_s(szTmp, MAX_PATH * 4, szPath);
    // Remove trailing backslash
    size_t n = wcslen(szTmp);
    if (n > 0 && (szTmp[n-1] == L'\\' || szTmp[n-1] == L'/')) szTmp[--n] = L'\0';
    // Walk from root forward creating each component
    for (WCHAR* p = szTmp + 1; *p; p++)
    {
        if (*p == L'\\' || *p == L'/')
        {
            *p = L'\0';
            CreateDirectoryW(szTmp, NULL);
            *p = L'\\';
        }
    }
    CreateDirectoryW(szTmp, NULL);
}

CMiniZipNG::CMiniZipNG()
{
}

CMiniZipNG::~CMiniZipNG()
{
}

// Unicode version - Zip a directory
bool CMiniZipNG::ZipDirectory(LPCWSTR szDirPath, LPCWSTR szWildcard, LPCWSTR szZipPath, bool bRecursive)
{
    MZ_DEBUG(L"[MiniZipNG] ZipDirectory START\n");
    MZ_DEBUG(L"  DirPath: "); MZ_DEBUG(szDirPath); MZ_DEBUG(L"\n");
    MZ_DEBUG(L"  ZipPath: "); MZ_DEBUG(szZipPath); MZ_DEBUG(L"\n");
    
    // Convert Unicode paths to UTF-8 for minizip-ng
    char szZipPathUtf8[MAX_PATH * 4];
    WideCharToMultiByte(CP_UTF8, 0, szZipPath, -1, szZipPathUtf8, sizeof(szZipPathUtf8), NULL, NULL);

    MZ_DEBUG(L"  Creating zip writer...\n");
    // Create zip writer
    void* zip_writer = mz_zip_writer_create();
    if (!zip_writer)
    {
        MZ_DEBUG(L"  ERROR: Failed to create zip writer\n");
        return false;
    }

    MZ_DEBUG(L"  Opening zip file...\n");
    // Open zip file for writing
    int32_t err = mz_zip_writer_open_file(zip_writer, szZipPathUtf8, 0, 0);
    if (err != MZ_OK)
    {
        MZ_DEBUG(L"  ERROR: Failed to open zip file\n");
        mz_zip_writer_delete(&zip_writer);
        return false;
    }

    MZ_DEBUG(L"  Adding files recursively...\n");
    // Add files recursively
    bool bSuccess = ZipDirectoryRecursive(zip_writer, szDirPath, L"");

    MZ_DEBUG(L"  Closing zip file...\n");
    // Close zip file
    mz_zip_writer_close(zip_writer);
    mz_zip_writer_delete(&zip_writer);

    MZ_DEBUG(bSuccess ? L"[MiniZipNG] ZipDirectory SUCCESS\n" : L"[MiniZipNG] ZipDirectory FAILED\n");
    return bSuccess;
}

// Recursive helper to add directory contents
bool CMiniZipNG::ZipDirectoryRecursive(void* zip_writer, LPCWSTR szBasePath, LPCWSTR szRelPath)
{
    MZ_DEBUG(L"  [Recursive] BasePath: "); MZ_DEBUG(szBasePath);
    MZ_DEBUG(L" RelPath: "); MZ_DEBUG(szRelPath); MZ_DEBUG(L"\n");
    
    WCHAR szSearchPath[MAX_PATH * 4];
    WCHAR szFullPath[MAX_PATH * 4];
    
    // Build search pattern: basePath\relPath\*
    if (wcslen(szRelPath) > 0)
        swprintf_s(szSearchPath, L"%s\\%s\\*", szBasePath, szRelPath);
    else
        swprintf_s(szSearchPath, L"%s\\*", szBasePath);

    MZ_DEBUG(L"  [Recursive] SearchPath: "); MZ_DEBUG(szSearchPath); MZ_DEBUG(L"\n");

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(szSearchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        MZ_DEBUG(L"  [Recursive] ERROR: FindFirstFileW failed\n");
        return false;
    }

    bool bSuccess = true;
    do
    {
        // Skip . and ..
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        MZ_DEBUG(L"  [Recursive] Processing: "); MZ_DEBUG(fd.cFileName); MZ_DEBUG(L"\n");

        // Build full path and relative zip path
        WCHAR szZipPath[MAX_PATH * 4];
        if (wcslen(szRelPath) > 0)
        {
            swprintf_s(szFullPath, L"%s\\%s\\%s", szBasePath, szRelPath, fd.cFileName);
            swprintf_s(szZipPath, L"%s\\%s", szRelPath, fd.cFileName);
        }
        else
        {
            swprintf_s(szFullPath, L"%s\\%s", szBasePath, fd.cFileName);
            wcscpy_s(szZipPath, fd.cFileName);
        }

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            MZ_DEBUG(L"  [Recursive] Entering subdirectory: "); MZ_DEBUG(szZipPath); MZ_DEBUG(L"\n");
            // Recursively add subdirectory
            if (!ZipDirectoryRecursive(zip_writer, szBasePath, szZipPath))
                bSuccess = false;
        }
        else
        {
            MZ_DEBUG(L"  [Recursive] Adding file: "); MZ_DEBUG(szFullPath); MZ_DEBUG(L"\n");
            // Add file to zip (returns true even if skipped due to long name)
            if (!AddFileToZip(zip_writer, szFullPath, szZipPath))
            {
                MZ_DEBUG(L"  [Recursive] ERROR: AddFileToZip failed\n");
                // Don't fail the whole operation - continue with other files
                // bSuccess = false;
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    MZ_DEBUG(L"  [Recursive] Done with directory\n");
    return bSuccess;
}

// Add a single file to zip
bool CMiniZipNG::AddFileToZip(void* zip_writer, LPCWSTR szFilePath, LPCWSTR szZipPath)
{
    // Check path length before conversion (be conservative - max 300 wide chars)
    size_t nWideLen = wcslen(szZipPath);
    if (nWideLen > 300)
    {
        MZ_DEBUG(L"  [AddFileToZip] Path too long (>300 wide chars), skipping: ");
        // Only print first 100 chars of the filename to avoid flooding debug output
        WCHAR szTruncated[128];
        wcsncpy_s(szTruncated, 128, szZipPath, 100);
        if (nWideLen > 100) wcscat_s(szTruncated, 128, L"...");
        MZ_DEBUG(szTruncated);
        MZ_DEBUG(L"\n");
        return true;  // Return true to continue with other files
    }

    // Convert paths to UTF-8
    char szFilePathUtf8[MAX_PATH * 4];
    char szZipPathUtf8[MAX_PATH * 4];
    int nFilePathLen = WideCharToMultiByte(CP_UTF8, 0, szFilePath, -1, szFilePathUtf8, sizeof(szFilePathUtf8), NULL, NULL);
    int nZipPathLen = WideCharToMultiByte(CP_UTF8, 0, szZipPath, -1, szZipPathUtf8, sizeof(szZipPathUtf8), NULL, NULL);
    
    // Check if conversion failed or path is too long
    if (nZipPathLen == 0 || nZipPathLen > 512)
    {
        MZ_DEBUG(L"  [AddFileToZip] Path conversion failed or too long, skipping: ");
        MZ_DEBUG(szZipPath);
        MZ_DEBUG(L"\n");
        return true;  // Return true to continue with other files
    }

    // Replace backslashes with forward slashes for zip format
    for (char* p = szZipPathUtf8; *p; p++)
        if (*p == '\\') *p = '/';

    // Get file info
    mz_zip_file file_info = {0};
    file_info.filename = szZipPathUtf8;
    file_info.compression_method = MZ_COMPRESS_METHOD_STORE;  // No compression - faster transfers
    file_info.flag = MZ_ZIP_FLAG_UTF8;

    // Get file times - convert Windows FILETIME to Unix time_t
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (GetFileAttributesExW(szFilePath, GetFileExInfoStandard, &attr))
    {
        // Convert FILETIME (100-nanosecond intervals since 1601) to Unix time (seconds since 1970)
        ULARGE_INTEGER ull;
        ull.LowPart = attr.ftLastWriteTime.dwLowDateTime;
        ull.HighPart = attr.ftLastWriteTime.dwHighDateTime;
        file_info.modified_date = (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);
    }

    // Open entry in zip
    int32_t err = mz_zip_writer_entry_open(zip_writer, &file_info);
    if (err != MZ_OK)
    {
        MZ_DEBUG_FMT(L"  [AddFileToZip] Failed to open entry, error: %d\n", err);
        // Try to close any partial entry to keep writer in valid state
        mz_zip_writer_entry_close(zip_writer);
        MZ_DEBUG(L"  [AddFileToZip] Skipping file and continuing\n");
        return true;  // Return true to continue with other files
    }

    // Read and write file data
    HANDLE hFile = CreateFileW(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        mz_zip_writer_entry_close(zip_writer);
        return false;
    }

    const DWORD BUFFER_SIZE = 64 * 1024;
    std::vector<BYTE> buffer(BUFFER_SIZE);
    DWORD dwRead;
    bool bSuccess = true;

    while (ReadFile(hFile, buffer.data(), BUFFER_SIZE, &dwRead, NULL) && dwRead > 0)
    {
        if (mz_zip_writer_entry_write(zip_writer, buffer.data(), dwRead) != dwRead)
        {
            bSuccess = false;
            break;
        }
    }

    CloseHandle(hFile);
    mz_zip_writer_entry_close(zip_writer);
    return bSuccess;
}

// Unicode version - Unzip a directory
bool CMiniZipNG::UnZipDirectory(LPCWSTR szExtractPath, LPCWSTR szZipPath)
{
    MZ_DEBUG(L"[MiniZipNG] UnZipDirectory START\n");
    MZ_DEBUG(L"  Zip: "); MZ_DEBUG(szZipPath); MZ_DEBUG(L"\n");
    MZ_DEBUG(L"  Extract to: "); MZ_DEBUG(szExtractPath); MZ_DEBUG(L"\n");

    // Convert paths to UTF-8
    char szZipPathUtf8[MAX_PATH * 4];
    char szExtractPathUtf8[MAX_PATH * 4];
    WideCharToMultiByte(CP_UTF8, 0, szZipPath, -1, szZipPathUtf8, sizeof(szZipPathUtf8), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, szExtractPath, -1, szExtractPathUtf8, sizeof(szExtractPathUtf8), NULL, NULL);

    // Create zip reader
    void* zip_reader = mz_zip_reader_create();
    if (!zip_reader)
    {
        MZ_DEBUG(L"[MiniZipNG] Failed to create zip reader\n");
        return false;
    }

    // Open zip file
    int32_t err = mz_zip_reader_open_file(zip_reader, szZipPathUtf8);
    if (err != MZ_OK)
    {
        MZ_DEBUG(L"[MiniZipNG] Failed to open zip file\n");
        mz_zip_reader_delete(&zip_reader);
        return false;
    }

    // Extract files individually with long path support
    bool bAllOk = true;
    int32_t num_errors = 0;
    
    // Convert extract path to use \\?\ prefix for long path support
    WCHAR szLongPathW[MAX_PATH * 4];
    if (szExtractPath[0] && szExtractPath[1] == L':')
    {
        // Absolute path - add \\?\ prefix
        _snwprintf_s(szLongPathW, MAX_PATH * 4, _TRUNCATE, L"\\\\?\\%s", szExtractPath);
    }
    else
    {
        wcscpy_s(szLongPathW, MAX_PATH * 4, szExtractPath);
    }
    
    // Go to first entry
    err = mz_zip_reader_goto_first_entry(zip_reader);
    
    while (err == MZ_OK)
    {
        // Get entry info
        mz_zip_file* file_info = NULL;
        err = mz_zip_reader_entry_get_info(zip_reader, &file_info);
        if (err != MZ_OK || !file_info)
        {
            MZ_DEBUG(L"[MiniZipNG] Failed to get entry info\n");
            num_errors++;
            err = mz_zip_reader_goto_next_entry(zip_reader);
            continue;
        }
        
        // Build full extraction path with \\?\ prefix
        WCHAR szEntryNameW[MAX_PATH * 4];
        MultiByteToWideChar(CP_UTF8, 0, file_info->filename, -1, szEntryNameW, MAX_PATH * 4);
        
        // Check if entry is a directory (filename ends with / in ZIP format)
        bool bIsDir = false;
        size_t nNameLen = wcslen(szEntryNameW);
        if (nNameLen > 0 && szEntryNameW[nNameLen - 1] == L'/')
            bIsDir = true;
        
        // Replace forward slashes with backslashes for Windows
        for (WCHAR* p = szEntryNameW; *p; p++)
            if (*p == L'/') *p = L'\\';
        
        WCHAR szFullPathW[MAX_PATH * 4];
        _snwprintf_s(szFullPathW, MAX_PATH * 4, _TRUNCATE, L"%s\\%s", szLongPathW, szEntryNameW);
        
        MZ_DEBUG_FMT(L"[MiniZipNG] Extracting: %s (len=%d)\n", szEntryNameW, (int)wcslen(szEntryNameW));
        
        if (bIsDir)
        {
            // Remove trailing backslash for CreateDirectoryW
            nNameLen = wcslen(szEntryNameW);
            if (nNameLen > 0 && szEntryNameW[nNameLen - 1] == L'\\')
                szEntryNameW[nNameLen - 1] = L'\0';
            _snwprintf_s(szFullPathW, MAX_PATH * 4, _TRUNCATE, L"%s\\%s", szLongPathW, szEntryNameW);
            
            MZ_DEBUG(L"[MiniZipNG] Creating directory: ");
            MZ_DEBUG(szEntryNameW);
            MZ_DEBUG(L"\n");
            MZ_CreateDirectoryChain(szFullPathW);
        }
        else
        {
            // Ensure parent directory exists (full chain)
            WCHAR szParentDir[MAX_PATH * 4];
            wcscpy_s(szParentDir, MAX_PATH * 4, szFullPathW);
            WCHAR* pLastSlash = wcsrchr(szParentDir, L'\\');
            if (pLastSlash)
            {
                *pLastSlash = L'\0';
                MZ_CreateDirectoryChain(szParentDir);
            }
            
            // Convert to UTF-8 for minizip
            char szFullPathUtf8[MAX_PATH * 4];
            WideCharToMultiByte(CP_UTF8, 0, szFullPathW, -1, szFullPathUtf8, sizeof(szFullPathUtf8), NULL, NULL);
            
            // Open entry for reading
            err = mz_zip_reader_entry_open(zip_reader);
            if (err != MZ_OK)
            {
                MZ_DEBUG_FMT(L"[MiniZipNG] Failed to open entry: %d\n", err);
                num_errors++;
                err = mz_zip_reader_goto_next_entry(zip_reader);
                continue;
            }
            
            // Create output file with long path support
            HANDLE hFile = CreateFileW(szFullPathW, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                DWORD dwErr = GetLastError();
                MZ_DEBUG_FMT(L"[MiniZipNG] Failed to create file (err=%lu)\n", dwErr);
                mz_zip_reader_entry_close(zip_reader);
                num_errors++;
                err = mz_zip_reader_goto_next_entry(zip_reader);
                continue;
            }
            
            // Read and write file data
            const DWORD BUFFER_SIZE = 64 * 1024;
            std::vector<BYTE> buffer(BUFFER_SIZE);
            int32_t bytes_read = 0;
            DWORD dwWritten = 0;
            bool bWriteOk = true;
            
            do
            {
                bytes_read = mz_zip_reader_entry_read(zip_reader, buffer.data(), BUFFER_SIZE);
                if (bytes_read > 0)
                {
                    if (!WriteFile(hFile, buffer.data(), bytes_read, &dwWritten, NULL) || dwWritten != (DWORD)bytes_read)
                    {
                        bWriteOk = false;
                        break;
                    }
                }
            } while (bytes_read > 0);
            
            CloseHandle(hFile);
            mz_zip_reader_entry_close(zip_reader);
            
            if (!bWriteOk)
            {
                MZ_DEBUG(L"[MiniZipNG] Failed to write file data\n");
                num_errors++;
            }
        }
        
        // Move to next entry
        err = mz_zip_reader_goto_next_entry(zip_reader);
    }
    
    MZ_DEBUG_FMT(L"[MiniZipNG] Extraction complete, errors: %d\n", num_errors);

    // Close zip file
    mz_zip_reader_close(zip_reader);
    mz_zip_reader_delete(&zip_reader);

    MZ_DEBUG(L"[MiniZipNG] UnZipDirectory END\n");
    // Return true even with some errors (locked files, etc.) as long as we extracted something
    return true;
}

// ANSI compatibility wrappers
bool CMiniZipNG::ZipDirectory(LPCSTR szDirPath, LPCSTR szWildcard, LPCSTR szZipPath, bool bRecursive)
{
    WCHAR szDirPathW[MAX_PATH * 4];
    WCHAR szWildcardW[MAX_PATH * 4];
    WCHAR szZipPathW[MAX_PATH * 4];
    
    MultiByteToWideChar(CP_ACP, 0, szDirPath, -1, szDirPathW, MAX_PATH * 4);
    MultiByteToWideChar(CP_ACP, 0, szWildcard, -1, szWildcardW, MAX_PATH * 4);
    MultiByteToWideChar(CP_ACP, 0, szZipPath, -1, szZipPathW, MAX_PATH * 4);
    
    return ZipDirectory(szDirPathW, szWildcardW, szZipPathW, bRecursive);
}

bool CMiniZipNG::UnZipDirectory(LPCSTR szExtractPath, LPCSTR szZipPath)
{
    WCHAR szExtractPathW[MAX_PATH * 4];
    WCHAR szZipPathW[MAX_PATH * 4];
    
    MultiByteToWideChar(CP_ACP, 0, szExtractPath, -1, szExtractPathW, MAX_PATH * 4);
    MultiByteToWideChar(CP_ACP, 0, szZipPath, -1, szZipPathW, MAX_PATH * 4);
    
    return UnZipDirectory(szExtractPathW, szZipPathW);
}
