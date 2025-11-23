# UltraVNC Server Language Resources

This folder contains the language-specific resource files for UltraVNC Server (winvnc), enabling multi-language support through separate resource DLLs.

## Structure

```
UltraVNC_language/
├── UltraVNC_Server_language.sln   # Visual Studio solution for all language projects
├── French/
│   ├── UltraVNC_Server_language_french.vcxproj
│   ├── winvnc.rc
│   └── README.md
├── Spanish/
│   ├── UltraVNC_Server_language_spanish.vcxproj
│   ├── winvnc.rc
│   └── README.md
└── German/
    ├── UltraVNC_Server_language_german.vcxproj
    ├── winvnc.rc
    └── README.md
```

## Building Language DLLs

### Using Visual Studio
1. Open `UltraVNC_Server_language.sln` in Visual Studio
2. Select the desired configuration (Debug/Release) and platform (x86/x64)
3. Build Solution (Ctrl+Shift+B)

This will create three language DLLs:
- `winvnclang_french.dll`
- `winvnclang_spanish.dll`
- `winvnclang_german.dll`

### Individual Projects
Each language can be built separately by opening its respective `.vcxproj` file.

## How It Works

The UltraVNC Server uses a localization system defined in `Localization.h`:

1. **String Variables**: All translatable strings are stored in `char sz_ID_*` variables
2. **LoadString Calls**: The `Load_Localization()` function loads all strings from the DLL
3. **Resource DLLs**: Each language has its own resource-only DLL containing STRINGTABLE entries

### Example Usage

```cpp
// Load the French language DLL
HMODULE hFrenchDLL = LoadLibrary("winvnclang_french.dll");
if (hFrenchDLL) {
    Load_Localization(hFrenchDLL);
    // Now all sz_ID_* variables contain French text
}
```

## Translation Guide

To translate to a new language:

1. **Copy a base folder**: Duplicate one of the existing language folders
2. **Rename the project**: Update the `.vcxproj` filename and internal settings
3. **Translate strings**: Edit the `winvnc.rc` file and translate all STRINGTABLE entries
4. **Translate dialogs**: Translate all dialog text, menu items, and control labels
5. **Add to solution**: Include the new project in `UltraVNC_Server_language.sln`
6. **Build and test**: Compile the DLL and test it with UltraVNC Server

## String Table IDs

All translatable strings use resource IDs defined in `resource.h`. Examples:

- `ID_FAILED_INIT` - Socket initialization errors
- `ID_ANOTHER_INST` - Multiple instance warnings
- `ID_WARNING` - General warning captions
- `ID_SERV_SUCCESS_INST` - Service installation messages
- And 100+ more string IDs

## Visual Resources

All bitmap (.bmp) and icon (.ico) files are referenced from the parent directory using relative paths (`..\..\`). This means:
- Visual resources don't need to be duplicated per language
- Only text resources need translation
- All languages share the same visual assets

## Configuration Notes

- **NoEntryPoint**: Projects use NoEntryPoint as they are resource-only DLLs
- **Resource Include Directories**: Set to `..\..\` to find parent resource files
- **Target Name**: Each language outputs a uniquely named DLL
- **Platform Support**: Both Win32 (x86) and x64 platforms are supported

## Current Languages

- **French** - Complete resource file ready for translation
- **Spanish** - Complete resource file ready for translation
- **German** - Complete resource file ready for translation

## Related Files

- `winvnc\winvnc\Localization.h` - String variable declarations and LoadString calls
- `winvnc\winvnc\resource.h` - Resource ID definitions
- `winvnc\winvnc\winvnc.rc` - Original English resource file (template)
- `winvnc\winvnc\winvnc.cpp` - Main application that loads the language DLL

## Version Information

Built for Visual Studio 2019 (v142 toolset) with Windows SDK 10.0.
Compatible with Windows XP and later.
