#define AppName "UltraVNC"
#define AppID "Ultravnc2"


[Setup]
AppName=UltraVNC
AppVerName=UltraVNC 1.0.9.6
AppVersion=1.0.9.6
VersionInfoVersion=1.0.9.6
AppPublisher=uvnc bvba
AppCopyright=UltraVnc Team
AppPublisherURL={cm:PublisherURL}
AppSupportURL={cm:SupportURL}
AppUpdatesURL={cm:UpdatesURL}
DefaultDirName={pf}\{cm:MyAppName}
DefaultGroupName={cm:MyAppName}
WindowVisible=false
DisableStartupPrompt=true
DisableReadyPage=false
ChangesAssociations=true
MinVersion=0,5.0.2195
PrivilegesRequired=admin
AppID={#AppID}
UninstallRestartComputer=true
DirExistsWarning=no
OutputDir=setupfile
OutputBaseFilename=UltraVNC_1.0.9.6_update_simple
BackColorDirection=lefttoright
UserInfoPage=false
ShowLanguageDialog=yes
LanguageDetectionMethod=uilanguage
AllowUNCPath=false
WindowShowCaption=false
WindowStartMaximized=false
WindowResizable=false
Compression=lzma/Ultra
AlwaysRestart=false
VersionInfoDescription={#AppName} Setup
WizardImageBackColor=clWhite
WizardImageStretch=false
SetupIconFile=UltraVNC.ico
WizardImageFile=UltraVNC-splash.bmp
WizardSmallImageFile=UltraVNC-logo.bmp
InfoAfterFile=Readme.txt
InfoBeforeFile=Whatsnew.txt
LicenseFile=Licence.txt
InternalCompressLevel=Ultra
SolidCompression=true
SignTool=signing
VersionInfoCompany=uvnc bvba
VersionInfoCopyright=UltraVnc Team
VersionInfoProductName=UltraVnc 
VersionInfoProductVersion=1.0.9.6
UninstallDisplayName=UltraVnc

[Languages]
Name: en; MessagesFile: compiler:Default.isl
Name: de; MessagesFile: compiler:Languages\german.isl
Name: fr; MessagesFile: compiler:Languages\french.isl

[CustomMessages]
en.MyAppName={#AppName}
en.MyAppVerName={#AppName} %1
en.firewall=Configuring Windows firewall...
en.SupportURL=http://forum.ultravnc.info
en.UpdatesURL=http://www.uvnc.com
en.PublisherURL=http://www.uvnc.com

de.MyAppName={#AppName}
de.MyAppVerName={#AppName} %1
de.firewall=Die Windows Firewall wird konfiguriert...
de.SupportURL=http://forum.ultravnc.info
de.UpdatesURL=http://www.uvnc.com
de.PublisherURL=http://www.uvnc.com

fr.MyAppName={#AppName}
fr.MyAppVerName={#AppName} %1
fr.firewall=Configuration du Pare Feu Windows...
fr.SupportURL=http://forum.ultravnc.info
fr.UpdatesURL=http://www.uvnc.com
fr.PublisherURL=http://www.uvnc.com

en.FullInstall=Full installation
de.FullInstall=Vollständige Installation
fr.FullInstall=Installation complète

en.CustomInstall=Custom installation
de.CustomInstall=Benutzerdefinierte Installation
fr.CustomInstall=Installation personnalisée

en.ServerOnly=UltraVNC Server Only
de.ServerOnly=Nur UltraVNC Server
fr.ServerOnly=Seulement UltraVNC Server
en.ServerOnlyS=UltraVNC Server Only   "silent"
de.ServerOnlyS=Nur UltraVNC Server  "silent"
fr.ServerOnlyS=Seulement UltraVNC Server  "mode silencieux"

en.ViewerOnly=UltraVNC Viewer Only
de.ViewerOnly=Nur UltraVNC Viewer
fr.ViewerOnly=Seulement UltraVNC Viewer

en.InstallService=&Register %1 as a system service
de.InstallService=%1 als System-Dienst &registrieren
fr.InstallService=&Installation et enregistrement d'%1 comme service système

en.ServerConfig=Server configuration:
de.ServerConfig=Server Konfiguration:
fr.ServerConfig=Configuration serveur:

en.StartService=&Start or restart %1 service
de.StartService=%1 Dienst (erneut) &starten
fr.StartService=&Démarrer ou redémarrer %1 le service

en.CreateDesktopIcons=Create %1 &desktop icons
de.CreateDesktopIcons=%1 &Desktop-Symbole anlegen
fr.CreateDesktopIcons=Créer les icônes d'%1 sur le &bureau

en.Starting=Starting %1 service...
de.Starting=%1 Dienst starten...
fr.Starting=Démarrage du service %1...

en.Stopping=Stopping %1 service...
de.Stopping=%1 Dienst stoppen...
fr.Stopping=Arrêt du service %1...

en.Removing=Removing %1 service...
de.Removing=%1 Dienst entfernen...
fr.Removing=Suppression du service %1...

en.Registering=Registering %1 service...
de.Registering=%1 Dienst registrieren...
fr.Registering=Enregistrement service %1...

en.Passwd=Check set initial password...
de.Passwd=Check set initial password...
fr.Passwd=Vérification du mot de passe initial...

[Types]

[Components]


[Tasks]


[Files]
; component independent files
Source: isxdl.dll; Flags: dontcopy
Source: UltraVNC.ico; Flags: dontcopy
Source: WizModernSmallImage-IS.bmp; Flags: dontcopy

Source: check_install.exe; DestDir: {app}; BeforeInstall: StopVNC_S; Flags: deleteafterinstall; 
Source: Whatsnew.txt; DestDir: {app}
Source: Licence.txt; DestDir: {app}
Source: Readme.txt; DestDir: {app}
; server files
; winvnc.exe needs to be first here because it triggers stopping WinVNC service/app.
Source: winvnc.exe; DestDir: {app}; Flags: ignoreversion replacesameversion onlyifdestfileexists 32bit; Check: Can_cont(); MinVersion: ,5.1.2600; 
Source: w2k\winvnc.exe; DestDir: {app}; Flags: ignoreversion replacesameversion onlyifdestfileexists 32bit; Check: Can_cont(); MinVersion: ,5.0.2195; OnlyBelowVersion: 0,5.1.2600; 
Source: vncviewer.exe; DestDir: {app}; Flags: ignoreversion replacesameversion onlyifdestfileexists 32bit; MinVersion: ,5.1.2600; 
Source: w2k\vncviewer.exe; DestDir: {app}; Flags: ignoreversion replacesameversion onlyifdestfileexists 32bit; MinVersion: ,5.0.2195; OnlyBelowVersion: 0,5.1.2600; 




[Icons]


[Registry]


[Run]

Filename: net; Parameters: start uvnc_service; Flags: runhidden; StatusMsg: {cm:Starting,UltraVNC}

[UninstallRun]
Filename: net; Parameters: stop uvnc_service; Flags: runhidden; RunOnceId: StopVncService;   StatusMsg: {cm:Stopping, UltraVNC}


[_ISTool]
UseAbsolutePaths=true
[Code]
var
  SelectedTasks: String;
  ConCont: Integer;
  ISset: Boolean;

function isxdl_SetOption(Option, Value: PChar): Integer;
external 'isxdl_SetOption@files:isxdl.dll stdcall';

function GetModuleHandle(lpModuleName: LongInt): LongInt;
external 'GetModuleHandleA@kernel32.dll stdcall';
function ExtractIcon(hInst: LongInt; lpszExeFileName: PChar; nIconIndex: LongInt): LongInt;
external 'ExtractIconA@shell32.dll stdcall';
function DrawIconEx(hdc: LongInt; xLeft, yTop: Integer; hIcon: LongInt; cxWidth, cyWidth: Integer; istepIfAniCur: LongInt; hbrFlickerFreeDraw, diFlags: LongInt): LongInt;
external 'DrawIconEx@user32.dll stdcall';
function DestroyIcon(hIcon: LongInt): LongInt;
external 'DestroyIcon@user32.dll stdcall';


const
  DI_NORMAL = 3;

function InitializeSetup(): Boolean;
begin
  Result := True;
  ISset := False;
end;


function Can_cont(): Boolean;
begin
if ConCont <> 5 then
Result := false;
if ConCont = 5  then
  Result := true;
end;

procedure StopVNC();
begin
  if UsingWinNT() = True then
  ExtractTemporaryFile('check_install.exe');
  if Exec(ExpandConstant('{tmp}\check_install.exe'), '', '', SW_HIDE, ewWaitUntilTerminated, ConCont) then
  begin
    Log('Checking system status');
  end
  else begin
    Log('Checking system status');
  end;
end;

procedure StopVNC_S();
begin
  if UsingWinNT() = True then
  ExtractTemporaryFile('check_install.exe');
  if Exec(ExpandConstant('{tmp}\check_install.exe'), 'silent', '', SW_HIDE, ewWaitUntilTerminated, ConCont) then
  begin
    Log('Checking system status');
  end
  else begin
    Log('Checking system status');
  end;
end;

function IsTaskChecked(Taskname: String): Boolean;
begin
  Log('SelectedTasks='+SelectedTasks);
  if CompareStr(SelectedTasks, '?') <> 0 then
    Result := (Pos(Taskname, SelectedTasks) > 0)
  else
  begin
    // default if not set through inf file
    Result := false;
    case Taskname of
    'desktopicon':
      Result := true;
    'associate':
      Result := true;
  end;
  end;
end;

[Dirs]

[InnoIDE_Settings]
LogFileOverwrite=false

