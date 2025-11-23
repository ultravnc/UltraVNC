# French Language Resources for UltraVNC

This folder contains the French language resource files for the UltraVNC Viewer application.

## Files Included

### Resource Files (Ready for Translation)
- `vncviewer.rc` - Main French resource script with updated paths to original resources
- `resource.h` - Resource ID definitions  
- `version.rc2` - Version information

### Visual Resources (Referenced from Original Location)
All bitmap, icon, and cursor files are referenced from their original location:
- **Icons**: Referenced from `..\res\*.ico`
- **Bitmaps**: Referenced from `..\res\*.bmp` and `..\res\stat\*.bmp`
- **Cursors**: Referenced from `..\res\*.cur`

## Translation Process

1. **Edit Dialog Templates**: Open `vncviewer.rc` and translate all dialog text, menus, and string table entries
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

## Building the French DLL

The main UltraVNC_language project now references these French resources:
- Project file: `UltraVNC_language.vcxproj`
- Resource reference: `french\vncviewer.rc`
- Output: `UltraVNC_language.dll` (containing French text with original visual resources)

## Usage

When built, the resulting DLL can be loaded by UltraVNC Viewer to provide French interface:
```cpp
HMODULE hFrenchDLL = LoadLibrary(L"UltraVNC_language.dll");
```

## Notes

- Only translatable text resources are in this folder
- Visual resources (bmp/ico/cur) remain at original `..\res\` location
- Resource paths automatically point to the correct original files
- Ready for French translation of all UI elements while maintaining original visual assets
