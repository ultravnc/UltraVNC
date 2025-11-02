# Building librats for Windows (Visual Studio)

## Multi-Architecture Support

The librats project supports building for both **Win32 (x86)** and **x64** architectures from the same Visual Studio solution using a **native Visual Studio project** (`rats_native.vcxproj`).

## Quick Start (Native Project - RECOMMENDED)

### Using Native Visual Studio Project

The project includes a native `rats_native.vcxproj` that supports both Win32 and x64 without CMake:

1. **Open** your solution (`winvnc.sln` or `vncviewer.sln`)
2. Select platform: **Win32** or **x64**
3. **Build → Rebuild Solution**

✅ That's it! No CMake required.

## Alternative: Using CMake (Optional)

1. **Install CMake** (if not already installed):
   - Download from: https://cmake.org/download/
   - Or use Visual Studio's built-in CMake support

2. **Regenerate project files**:
   ```batch
   cd librats
   regenerate_vs_projects.bat
   ```

3. **Open in Visual Studio**:
   - Open `winvnc\winvnc.sln` or `vncviewer\vncviewer.sln`
   - Select platform: **Win32** or **x64**
   - Build as normal

### Option 2: Manual CMake Configuration

```batch
cd librats
mkdir win32
cd win32
cmake .. -G "Visual Studio 17 2022" -T v142 -DRATS_BUILD_TESTS=OFF -DRATS_BUILD_EXAMPLES=ON
```

**Note:** `-T v142` specifies the Visual Studio 2019 (v142) Platform Toolset for compatibility with UltraVNC.

## Switching Between Architectures

In Visual Studio:
1. Use the **Platform** dropdown to select:
   - **Win32** for 32-bit builds
   - **x64** for 64-bit builds

2. Clean the solution if switching architectures for the first time

The build system automatically uses separate intermediate directories:
- `win32\Debug\` or `win32\Release\` for Win32 builds  
- `win32\x64\Debug\` or `win32\x64\Release\` for x64 builds

## Troubleshooting

### Error: "module machine type 'x64' conflicts with target machine type 'x86'"

This means there are leftover object files from a different architecture. Fix:

1. **In Visual Studio**:
   - Right-click solution → **Clean Solution**
   - Switch platform
   - **Rebuild Solution**

2. **Or manually delete**:
   ```batch
   cd librats\win32
   rmdir /s /q Debug Release x64 Win32
   ```

### CMake not found

If `cmake` command is not recognized:
- Install CMake: https://cmake.org/download/
- Add to PATH during installation
- Or use Visual Studio's built-in CMake (File → Open → CMake)

## Build Configurations

- **Debug**: Full debugging information, no optimizations
- **Release**: Optimized build for production

## Integration with UltraVNC

The librats library is automatically included when building:
- `winvnc\winvnc.sln` (VNC Server)
- `vncviewer\vncviewer.sln` (VNC Viewer)

Both solutions reference the CMake-generated `librats\win32\rats.vcxproj`.

## Notes

- The `win32` folder contains CMake-generated files (gitignored)
- Always regenerate after pulling changes to `CMakeLists.txt`
- Both architectures can coexist without conflicts
