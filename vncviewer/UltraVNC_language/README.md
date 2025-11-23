# UltraVNC Multi-Language Resource Setup

This project supports multiple language translations for the UltraVNC Viewer application through resource-only DLLs.

## Available Languages

- **French** (`french/`) - Ready for French translation
- **German** (`german/`) - Ready for German translation  
- **Spanish** (`spanish/`) - Ready for Spanish translation

## Project Structure

```
UltraVNC_language/
├── french/
│   ├── vncviewer.rc      # French resource script
│   ├── resource.h        # Resource IDs
│   ├── version.rc2       # Version info
│   └── README.md         # French documentation
├── german/
│   ├── vncviewer.rc      # German resource script
│   ├── resource.h        # Resource IDs
│   ├── version.rc2       # Version info
│   └── README.md         # German documentation
├── spanish/
│   ├── vncviewer.rc      # Spanish resource script
│   ├── resource.h        # Resource IDs
│   ├── version.rc2       # Version info
│   └── README.md         # Spanish documentation
├── UltraVNC_language.vcxproj    # Main project file
└── UltraVNC_language.cpp        # DLL entry point
```

## Building Different Language DLLs

### To build a specific language DLL:

1. **Update the project file** to reference the desired language:
   ```xml
   <!-- For French -->
   <ResourceCompile Include="french\vncviewer.rc" />
   
   <!-- For German -->
   <ResourceCompile Include="german\vncviewer.rc" />
   
   <!-- For Spanish -->
   <ResourceCompile Include="spanish\vncviewer.rc" />
   ```

2. **Build the project** in Visual Studio
3. **Output**: `UltraVNC_language.dll` with the selected language resources

### Alternative: Create separate project configurations

You can create separate build configurations (Debug_French, Release_German, etc.) each with different resource references.

## Translation Workflow

1. **Select a language folder** (french, german, or spanish)
2. **Open `vncviewer.rc`** in that folder
3. **Translate all text content**:
   - Dialog titles and control text
   - Menu items and submenus
   - String table entries
   - Message box text
4. **Keep resource IDs unchanged** - only modify text content
5. **Test the build** to ensure all resources compile correctly

## Resource Sharing

All language folders share the same visual resources:
- Icons, bitmaps, and cursors are referenced from `..\res\`
- Only text content is duplicated for translation
- Visual consistency maintained across all languages

## Deployment

Each language DLL can be distributed separately:
- `UltraVNC_language.dll` (French version)
- `UltraVNC_language.dll` (German version)
- `UltraVNC_language.dll` (Spanish version)

The main UltraVNC Viewer can load the appropriate language DLL based on user preference or system locale.

## Adding New Languages

To add a new language:

1. **Create a new folder** (e.g., `italian/`)
2. **Copy resource files** from `french/` folder
3. **Update the README** with the new language information
4. **Translate the text content** in the new `vncviewer.rc`
5. **Update project reference** to build the new language DLL

## Notes

- All language configurations follow Microsoft resource-only DLL specifications
- `/NOENTRY` linker option is properly configured
- Unicode character set support for proper international text
- Resource IDs remain consistent across all language versions
