# librats Build Information

## Project Files

### Primary (Included in Git)
- **`rats_native.vcxproj`** - Native Visual Studio project with Win32 and x64 support
  - ✅ Used by `winvnc.sln` and `vncviewer.sln`
  - ✅ No CMake required
  - ✅ Committed to Git

### Generated (Gitignored)
- **`win32/`** - CMake-generated files (optional, not required for building)
  - ❌ Not committed to Git (in .gitignore)
  - Only needed if you want to use CMake
  - Can be regenerated with `regenerate_vs_projects.bat`

## Build Instructions

### Normal Build (No CMake)

Just open and build:
```
1. Open winvnc\winvnc.sln or vncviewer\vncviewer.sln
2. Select Win32 or x64 platform
3. Build
```

### If You Need CMake (Advanced)

Only if you need to regenerate CMake files:
```batch
cd librats
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 17 2022" -T v142 win32
```

## Output Directories

- **Win32 builds**: `librats\Win32\Debug\` or `Release\`
- **x64 builds**: `librats\x64\Debug\` or `Release\`

## Key Files

- `src\version.h` - Version information (static, committed to Git)
- `src\version.h.in` - CMake template (only used if regenerating with CMake)
- `rats_native.vcxproj` - Primary project file
- `CMakeLists.txt` - CMake configuration (optional)

## Why win32/ is Gitignored

The `win32/` folder contains CMake-generated files that are specific to each developer's environment:
- Binary paths
- Visual Studio version paths  
- Intermediate build files
- Cache files

These should always be regenerated locally, not shared via Git.
