# German Language Resources for UltraVNC

This folder contains the German language resource files for the UltraVNC Viewer application.

## Files Included

### Resource Files (Ready for Translation)
- `vncviewer.rc` - Main German resource script with paths to original resources
- `resource.h` - Resource ID definitions  
- `version.rc2` - Version information

### Visual Resources (Referenced from Original Location)
All bitmap, icon, and cursor files are referenced from their original location:
- **Icons**: Referenced from `..\res\*.ico`
- **Bitmaps**: Referenced from `..\res\*.bmp` and `..\res\stat\*.bmp`
- **Cursors**: Referenced from `..\res\*.cur`

## Translation Process

1. **Edit Dialog Templates**: Open `vncviewer.rc` and translate all dialog text, menus, and string table entries to German
2. **Keep Resource IDs**: Do not modify any resource IDs - only change the text content
3. **Keep File Paths**: Do not modify the `..\res\` file paths - they reference the original visual resources
4. **Maintain Layout**: Ensure translated text fits within dialog dimensions
5. **Test Build**: Build the project to verify all resources compile correctly

## Resource Path Configuration

All visual resources are correctly referenced using relative paths:
```rc
IDC_FINGER              CURSOR                  "..\\res\\Finger.cur"
IDB_COLLAPS             BITMAP                  "..\\res\\up.bmp"
IDR_TRAY                ICON                    "..\\res\\idr_tray.ico"
IDB_STAT_BACK           BITMAP                  "..\\res\\stat/back.bmp"
```

## Building the German DLL

To build the German language DLL:
1. Update the project file to reference `german\vncviewer.rc`
2. Build the UltraVNC_language project
3. Output: `UltraVNC_language.dll` (containing German text with original visual resources)

## Usage

When built, the resulting DLL can be loaded by UltraVNC Viewer to provide German interface:
```cpp
HMODULE hGermanDLL = LoadLibrary(L"UltraVNC_language.dll");
```

## Notes

- Only translatable text resources are in this folder
- Visual resources (bmp/ico/cur) remain at original `..\res\` location
- Resource paths automatically point to the correct original files
- Ready for German translation of all UI elements while maintaining original visual assets
