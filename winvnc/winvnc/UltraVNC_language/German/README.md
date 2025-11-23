# German Language Resources for UltraVNC Server

This folder contains the German language resource files for the UltraVNC Server application.

## Files Included

### Resource Files (Ready for Translation)
- `winvnc.rc` - Main German resource script with dialogs, menus, and string tables
- `UltraVNC_Server_language_German.vcxproj` - Visual Studio project file for building the German DLL

### Visual Resources (Referenced from Original Location)
All bitmap and icon files are referenced from their original location:
- **Icons**: Referenced from `..\..\*.ico`
- **Bitmaps**: Referenced from `..\..\*.bmp`

## Translation Process

1. **Edit String Tables**: Open `winvnc.rc` and translate all STRINGTABLE entries
2. **Translate Dialogs**: Translate all dialog text and menu items
3. **Keep Resource IDs**: Do not modify any resource IDs - only change the text content
4. **Keep File Paths**: Do not modify the `..\..\` file paths - they reference the original visual resources
5. **Maintain Layout**: Ensure translated text fits within dialog dimensions
6. **Test Build**: Build the project to verify all resources compile correctly

## String Table IDs

The server uses a localization system with LoadString() calls. All translatable strings are in STRINGTABLE blocks with IDs like:
- `ID_FAILED_INIT` - "Failed to initialise the socket system"
- `ID_ANOTHER_INST` - "Another instance of UltraVNC Server is already running"
- `ID_WARNING` - "WARNING"
- And many more...

## Building the German DLL

Build the project to create the German language DLL:
- Project file: `UltraVNC_Server_language_German.vcxproj`
- Output DLL: `winvnclang_German.dll`
- Resource reference: `winvnc.rc`

## Usage

When built, the resulting DLL can be loaded by UltraVNC Server to provide German interface:
```cpp
HMODULE hGermanDLL = LoadLibrary("winvnclang_German.dll");
Load_Localization(hGermanDLL);
```

The server's `Localization.h` file contains all the LoadString() calls that load text from the DLL.

## Notes

- Only translatable text resources are in this folder
- Visual resources (bmp/ico) remain at original `..\..\` location  
- Resource paths automatically point to the correct original files
- Ready for German translation of all UI elements while maintaining original visual assets
- The main server code uses `sz_ID_*` variables loaded via LoadString() from the DLL
