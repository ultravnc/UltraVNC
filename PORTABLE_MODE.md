# UltraVNC Configuration Modes

## Overview

UltraVNC supports three deployment modes:

1. **Hybrid Mode (Default)** - User-specific configs with shared fallback
2. **Portable Mode** - Config stored in application folder
3. **Service Mode** - Always uses shared system config

## Configuration Priority

### Application (winvnc.exe)
1. **Command line**: `-config "path"` (highest priority)
2. **Portable mode**: If `ultravnc.portable` exists → use install folder
3. **User config**: `%LOCALAPPDATA%\UltraVNC\ultravnc.ini` (if exists)
4. **Shared config**: `C:\ProgramData\UltraVNC\ultravnc.ini` (fallback)

### Service (uvnc_service)
1. **Portable mode**: If `ultravnc.portable` exists → use install folder
2. **Shared config**: `C:\ProgramData\UltraVNC\ultravnc.ini` (default)

## How to Enable Portable Mode

Create an empty file named `ultravnc.portable` in the UltraVNC application folder.

### Windows Command Line
```cmd
cd "C:\Path\To\UltraVNC"
type nul > ultravnc.portable
```

### PowerShell
```powershell
cd "C:\Path\To\UltraVNC"
New-Item -Path "ultravnc.portable" -ItemType File
```

### Manual Method
1. Navigate to the UltraVNC folder
2. Right-click → New → Text Document
3. Rename to `ultravnc.portable` (remove .txt extension)

## Configuration File Locations

### Hybrid Mode (Default)
**User-specific config** (when user creates personal settings):
- **Location**: `%LOCALAPPDATA%\UltraVNC\ultravnc.ini`
  - Example: `C:\Users\John\AppData\Local\UltraVNC\ultravnc.ini`
- **Shared**: No (each user has their own)
- **Permissions**: No admin rights required
- **Editable**: Always (users control their own settings)

**Shared config** (fallback when no user config exists):
- **Location**: `C:\ProgramData\UltraVNC\ultravnc.ini`
- **Shared**: Yes (all users see same settings)
- **Permissions**: Requires administrator rights to modify
- **Editable**: Only for administrators
- **Migration**: Automatically migrates from install folder on first run

### Portable Mode
- **Location**: `[Application Folder]\ultravnc.ini`
- **Shared**: Yes (if service runs from same folder)
- **Permissions**: No admin rights required (if folder is writable)
- **Migration**: None (uses install folder directly)

## Use Cases

### Hybrid Mode (Default - Recommended for)
- ✅ **Multi-user systems**: Each user can have personalized settings
- ✅ **Enterprise deployments**: Shared config for default settings
- ✅ **Mixed environments**: Service uses shared, users can customize
- ✅ **Non-admin users**: Can create personal configs without admin rights
- ✅ **Flexible management**: Admins control defaults, users customize

**How users get personal configs:**
Users can copy the shared config to their LocalAppData and customize:
```cmd
mkdir "%LOCALAPPDATA%\UltraVNC"
copy "C:\ProgramData\UltraVNC\ultravnc.ini" "%LOCALAPPDATA%\UltraVNC\ultravnc.ini"
```

### Portable Mode (Recommended for)
- ✅ USB drive installations
- ✅ Temporary/guest systems
- ✅ No admin rights available
- ✅ Self-contained deployments
- ✅ Testing and development
- ✅ Multiple parallel instances

## Creating User-Specific Configurations

To create a personal configuration (user-specific settings):

```cmd
REM Create user config folder
mkdir "%LOCALAPPDATA%\UltraVNC"

REM Copy shared config as template (optional)
copy "C:\ProgramData\UltraVNC\ultravnc.ini" "%LOCALAPPDATA%\UltraVNC\ultravnc.ini"

REM Or create new empty config
echo. > "%LOCALAPPDATA%\UltraVNC\ultravnc.ini"
```

Once created, the application will use the user-specific config and allow full editing without admin rights.

## Switching Modes

### From Hybrid to Portable
1. Create `ultravnc.portable` file
2. Copy config to application folder if needed
3. Restart UltraVNC

### From Portable to Hybrid
1. Delete `ultravnc.portable` file
2. Restart UltraVNC
3. Will use LocalAppData (user) or ProgramData (shared)

### From Shared to User-Specific
1. Copy shared config to `%LOCALAPPDATA%\UltraVNC\ultravnc.ini`
2. Restart UltraVNC
3. Modify settings (now saved to user config)

### From User-Specific to Shared
1. Delete `%LOCALAPPDATA%\UltraVNC\ultravnc.ini`
2. Restart UltraVNC
3. Will use shared ProgramData config

## Service Considerations

When running UltraVNC as a Windows Service:

- **Hybrid/Standard Mode**: Service always uses `C:\ProgramData\UltraVNC\ultravnc.ini`
- **Portable Mode**: Service uses `[Install Folder]\ultravnc.ini`

**Important**: 
- Services cannot access user-specific configs in LocalAppData
- Service and user application may use different configs in hybrid mode
- For consistent behavior, use shared config or portable mode

⚠️ **Note**: Portable mode with service requires the install folder to be writable by the service account.

## Behavior Examples

### Scenario 1: Fresh Installation (No existing config)
**Mode**: Hybrid (Default)
1. Application starts
2. No portable marker, no user config, no shared config
3. Creates: `C:\ProgramData\UltraVNC\ultravnc.ini`
4. Non-admin users: Read-only access to shared config
5. Service: Uses same shared config

**Config Used**:
- Application: `C:\ProgramData\UltraVNC\ultravnc.ini` (read-only for non-admins)
- Service: `C:\ProgramData\UltraVNC\ultravnc.ini`

### Scenario 2: User Creates Personal Config
**Mode**: Hybrid with user customization
1. User copies shared config to `%LOCALAPPDATA%\UltraVNC\`
2. Application now uses user config
3. User can modify settings without admin rights
4. Service continues using shared config

**Config Used**:
- Application: `C:\Users\{Username}\AppData\Local\UltraVNC\ultravnc.ini` (editable)
- Service: `C:\ProgramData\UltraVNC\ultravnc.ini`

### Scenario 3: Portable Mode
**Mode**: Portable (self-contained)
1. Create `ultravnc.portable` file in install folder
2. Application uses `[Install Folder]\ultravnc.ini`
3. Service uses `[Install Folder]\ultravnc.ini`
4. Both use same config (fully synchronized)

**Config Used**:
- Application: `C:\Program Files\UltraVNC\ultravnc.ini` (if folder writable)
- Service: `C:\Program Files\UltraVNC\ultravnc.ini`

### Scenario 4: Multi-User System
**Mode**: Hybrid (each user customized)
1. Admin sets defaults in `C:\ProgramData\UltraVNC\ultravnc.ini`
2. User A creates: `C:\Users\UserA\AppData\Local\UltraVNC\ultravnc.ini`
3. User B creates: `C:\Users\UserB\AppData\Local\UltraVNC\ultravnc.ini`
4. User C uses shared config (no personal config)
5. Service uses shared defaults

**Config Used**:
- Application (User A): `C:\Users\UserA\AppData\Local\UltraVNC\ultravnc.ini` (editable)
- Application (User B): `C:\Users\UserB\AppData\Local\UltraVNC\ultravnc.ini` (editable)
- Application (User C): `C:\ProgramData\UltraVNC\ultravnc.ini` (read-only)
- Service: `C:\ProgramData\UltraVNC\ultravnc.ini`

### Scenario 5: USB Portable Installation
**Mode**: Portable on removable drive
1. Copy UltraVNC to `D:\PortableApps\UltraVNC\`
2. Create `D:\PortableApps\UltraVNC\ultravnc.portable`
3. All settings stored in `D:\PortableApps\UltraVNC\ultravnc.ini`
4. No installation required, works on any PC

**Config Used**:
- Application: `D:\PortableApps\UltraVNC\ultravnc.ini` (portable)
- Service: Not typically used in portable mode

## Example: Creating Portable Package

```cmd
REM Create portable package
xcopy /E /I "C:\Program Files\UltraVNC" "D:\PortableApps\UltraVNC"
cd "D:\PortableApps\UltraVNC"
echo. > ultravnc.portable
```

Now the `D:\PortableApps\UltraVNC` folder is fully self-contained and portable!

## Troubleshooting

**Config not found in portable mode?**
- Verify `ultravnc.portable` exists in the same folder as `winvnc.exe`
- Check the log file for "Portable mode detected" message

**Cannot save settings in portable mode?**
- Ensure the application folder has write permissions
- Run as administrator if the folder is in a protected location

**Service not using portable config?**
- Verify `ultravnc.portable` exists in the service executable folder
- Check service logs for portable mode detection message

**Settings not saving for non-admin user?**
- User is likely using shared config (read-only)
- Create personal config: `mkdir "%LOCALAPPDATA%\UltraVNC" && copy "C:\ProgramData\UltraVNC\ultravnc.ini" "%LOCALAPPDATA%\UltraVNC\"`

## Quick Reference Table

| Mode | Marker File | Application Config | Service Config | Editable By |
|------|-------------|-------------------|----------------|-------------|
| **Hybrid (default)** | None | LocalAppData → ProgramData | ProgramData | User (LocalAppData)<br>Admin (ProgramData) |
| **Portable** | `ultravnc.portable` | Install Folder | Install Folder | Anyone (if folder writable) |
| **Command line** | Any | Custom path | ProgramData | Depends on path permissions |

### Config File Paths by Mode

```
Hybrid Mode:
  User:    C:\Users\{Username}\AppData\Local\UltraVNC\ultravnc.ini
  Shared:  C:\ProgramData\UltraVNC\ultravnc.ini
  
Portable Mode:
  Both:    {InstallFolder}\ultravnc.ini
  
Command Line:
  Both:    {SpecifiedPath}
```
