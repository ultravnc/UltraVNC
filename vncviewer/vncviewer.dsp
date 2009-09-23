# Microsoft Developer Studio Project File - Name="vncviewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vncviewer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vncviewer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vncviewer.mak" CFG="vncviewer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vncviewer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vncviewer - x64 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vncviewer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vncviewer - x64 Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "omnithread" /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /FR /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /Gf /Gy /I "omnithread" /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_MBCS" /D "_CRT_SECURE_NO_WARNINGS" /FR"Debug/" /Fp"Debug/vncviewer.pch" /YX"stdhdrs.h" /Fo"Debug/" /Fd"Debug/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\.." /i ".." /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Debug\vncviewer.bsc"
# ADD BSC32 /nologo /o ".\Debug\vncviewer.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /map /debug /machine:IX86 /pdbtype:sept /libpath:"./omnithread/Debug"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /map /debug /machine:IX86 /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"./omnithread/Debug"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Setting build time...
PreLink_Cmds=cl /nologo /MT /FoRelease\ /FdRelease\ /c buildtime.cpp
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP BASE Intermediate_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP Intermediate_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "omnithread" /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /FR /Fo".\Debug/" /Fd".\Debug/" /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "omnithread" /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /FR /Fo".\Debug/" /Fd".\Debug/" /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Debug\vncviewer.bsc"
# ADD BSC32 /nologo /o ".\Debug\vncviewer.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /pdb:".\Debug\vncviewer.pdb" /map:".\Debug\vncviewer.map" /debug /machine:IX86 /out:".\Debug\vncviewer.exe" /pdbtype:sept /libpath:"./omnithread/Debug"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /pdb:".\Debug\vncviewer.pdb" /map:".\Debug\vncviewer.map" /debug /machine:IX86 /out:".\Debug\vncviewer.exe" /pdbtype:sept /libpath:"./omnithread/Debug"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Setting build time...
PreLink_Cmds=cl /nologo /MTd /FoDebug\ /FdDebug\ /c buildtime.cpp
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Ox /Ot /I "omnithread" /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /c
# ADD CPP /MT /W3 /GX /I "omnithread" /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /c
# SUBTRACT CPP /O<none>
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Release\vncviewer.bsc"
# ADD BSC32 /nologo /o ".\Release\vncviewer.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /incremental:yes /machine:IX86 /pdbtype:sept /libpath:"omnithread/Release"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib Advapi32.lib /nologo /subsystem:windows /incremental:yes /machine:IX86 /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"omnithread/Release"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Setting build time...
PreLink_Cmds=cl \nologo \MT \FoRelease\ \FdRelease\ \c buildtime.cpp
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP BASE Intermediate_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP Intermediate_Dir "$(PlatformName)\$(ConfigurationName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Ox /Ot /I "omnithread" /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /Fo".\Release/" /c
# ADD CPP /nologo /MT /W3 /GX /Ox /Ot /I "omnithread" /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /Fo".\Release/" /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Release\vncviewer.bsc"
# ADD BSC32 /nologo /o ".\Release\vncviewer.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /incremental:yes /pdb:".\Release\vncviewer.pdb" /machine:IX86 /out:".\Release\vncviewer.exe" /pdbtype:sept /libpath:"omnithread/Release"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /subsystem:windows /incremental:yes /pdb:".\Release\vncviewer.pdb" /machine:IX86 /out:".\Release\vncviewer.exe" /pdbtype:sept /libpath:"omnithread/Release"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Setting build time...
PreLink_Cmds=cl \nologo \MT \FoRelease\ \FdRelease\ \c buildtime.cpp
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "vncviewer - Win32 Debug"
# Name "vncviewer - x64 Debug"
# Name "vncviewer - Win32 Release"
# Name "vncviewer - x64 Release"
# Begin Group "Resources"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=res\B46.ico
# End Source File
# Begin Source File

SOURCE=res\stat\back.bmp
# End Source File
# Begin Source File

SOURCE=res\background2.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\background2.bmp
# End Source File
# Begin Source File

SOURCE=res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\bitmap4.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\bitmap5.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\bitmap8.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\bitmap9.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\both.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\closeup.bmp
# End Source File
# Begin Source File

SOURCE=res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=res\cursorsel.cur
# End Source File
# Begin Source File

SOURCE=res\dir.ico
# End Source File
# Begin Source File

SOURCE=res\drive.ico
# End Source File
# Begin Source File

SOURCE=res\file.ico
# End Source File
# Begin Source File

SOURCE=res\stat\flash.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\front.bmp
# End Source File
# Begin Source File

SOURCE=res\hoch.bmp
# End Source File
# Begin Source File

SOURCE=res\idr_tray.ico
# End Source File
# Begin Source File

SOURCE=res\idr_tray_disabled.ico
# End Source File
# Begin Source File

SOURCE=res\stat\mainicon.ico
# End Source File
# Begin Source File

SOURCE=res\stat\maximizeup.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\minimizeup.bmp
# End Source File
# Begin Source File

SOURCE=res\nocursor.cur
# End Source File
# Begin Source File

SOURCE=res\stat\none.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\pindown.bmp
# End Source File
# Begin Source File

SOURCE=res\stat\pinup.bmp
# End Source File
# Begin Source File

SOURCE=res\quer.bmp
# End Source File
# Begin Source File

SOURCE=res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE="res\ultra-logo.bmp"
# End Source File
# Begin Source File

SOURCE=res\vnc.bmp
# End Source File
# Begin Source File

SOURCE=res\vnc32.BMP
# End Source File
# Begin Source File

SOURCE=res\vnc64.BMP
# End Source File
# Begin Source File

SOURCE=res\vncviewer.ico
# End Source File
# End Group
# Begin Source File

SOURCE=AboutBox.cpp
DEP_CPP_ABOUT=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_ABOUT=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=AboutBox.h
# End Source File
# Begin Source File

SOURCE=AccelKeys.cpp
DEP_CPP_ACCEL=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_ACCEL=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=AccelKeys.h
# End Source File
# Begin Source File

SOURCE=AuthDialog.cpp
DEP_CPP_AUTHD=\
	"..\common\win32_helpers.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\AuthDialog.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_AUTHD=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=AuthDialog.h
# End Source File
# Begin Source File

SOURCE=buildtime.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnection.cpp
DEP_CPP_CLIEN=\
	"..\common\win32_helpers.h"\
	"..\DSMPlugin\DSMPlugin.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\Exception.h"\
	"..\rdr\FdInStream.h"\
	"..\rdr\InStream.h"\
	"..\rdr\types.h"\
	"..\rdr\ZlibInStream.h"\
	"..\rfb\dh.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AboutBox.h"\
	".\AccelKeys.h"\
	".\AuthDialog.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\LowLevelHook.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\SessionDialog.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\vncauth.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIEN=\
	"..\DSMPlugin\omnithreadce.h"\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnection.h
# End Source File
# Begin Source File

SOURCE=ClientConnectionCacheRect.cpp
DEP_CPP_CLIENT=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENT=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionClipboard.cpp
DEP_CPP_CLIENTC=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTC=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionCopyRect.cpp
DEP_CPP_CLIENTCO=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCO=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionCoRRE.cpp
DEP_CPP_CLIENTCON=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCON=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionCursor.cpp
DEP_CPP_CLIENTCONN=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONN=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionFile.cpp
DEP_CPP_CLIENTCONNE=\
	"..\DSMPlugin\DSMPlugin.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\SessionDialog.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\vncauth.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNE=\
	"..\DSMPlugin\omnithreadce.h"\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionFullScreen.cpp
DEP_CPP_CLIENTCONNEC=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNEC=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionHextile.cpp
DEP_CPP_CLIENTCONNECT=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECT=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionRaw.cpp
DEP_CPP_CLIENTCONNECTI=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECTI=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionRRE.cpp
DEP_CPP_CLIENTCONNECTIO=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECTIO=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionTight.cpp
DEP_CPP_CLIENTCONNECTION=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECTION=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionUltra.cpp
DEP_CPP_CLIENTCONNECTIONU=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\lzo\lzoconf.h"\
	"..\lzo\lzodefs.h"\
	"..\lzo\minilzo.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECTIONU=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionZlib.cpp
DEP_CPP_CLIENTCONNECTIONZ=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECTIONZ=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ClientConnectionZlibHex.cpp
DEP_CPP_CLIENTCONNECTIONZL=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\lzo\lzoconf.h"\
	"..\lzo\lzodefs.h"\
	"..\lzo\minilzo.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_CLIENTCONNECTIONZL=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=COPYING.txt
# End Source File
# Begin Source File

SOURCE=d3des.c
DEP_CPP_D3DES=\
	".\d3des.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=d3des.h
# End Source File
# Begin Source File

SOURCE=Daemon.cpp
DEP_CPP_DAEMO=\
	"..\common\win32_helpers.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AboutBox.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Daemon.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_DAEMO=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Daemon.h
# End Source File
# Begin Source File

SOURCE=..\rfb\dh.cpp
DEP_CPP_DH_CP=\
	"..\rfb\dh.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\rfb\dh.h
# End Source File
# Begin Source File

SOURCE=..\DSMPlugin\DSMPlugin.cpp
DEP_CPP_DSMPL=\
	"..\DSMPlugin\DSMPlugin.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	
NODEP_CPP_DSMPL=\
	"..\DSMPlugin\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\DSMPlugin\DSMPlugin.h
# End Source File
# Begin Source File

SOURCE=Exception.cpp
DEP_CPP_EXCEP=\
	"..\rfb\rfbproto.h"\
	".\Exception.h"\
	".\messbox.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Exception.h
# End Source File
# Begin Source File

SOURCE=FileTransfer.cpp
DEP_CPP_FILET=\
	"..\common\win32_helpers.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_FILET=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=FileTransfer.h
# End Source File
# Begin Source File

SOURCE=FullScreenTitleBar.cpp
DEP_CPP_FULLS=\
	"..\common\win32_helpers.h"\
	"..\rfb\rfbproto.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\Log.h"\
	".\multimon.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=FullScreenTitleBar.h
# End Source File
# Begin Source File

SOURCE=FullScreenTitleBarConst.h
# End Source File
# Begin Source File

SOURCE=History.txt
# End Source File
# Begin Source File

SOURCE=KeyMap.cpp
DEP_CPP_KEYMA=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_KEYMA=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=KeyMap.h
# End Source File
# Begin Source File

SOURCE=.\KeyMapjap.cpp
DEP_CPP_KEYMAP=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_KEYMAP=\
	".\omnithreadce.h"\
	
# End Source File
# Begin Source File

SOURCE=.\KeyMapjap.h
# End Source File
# Begin Source File

SOURCE=keysymdef.h
# End Source File
# Begin Source File

SOURCE=.\keysymdefjap.h
# End Source File
# Begin Source File

SOURCE=LICENCE.txt
# End Source File
# Begin Source File

SOURCE=Log.cpp
DEP_CPP_LOG_C=\
	"..\rfb\rfbproto.h"\
	".\Log.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=Log.h
# End Source File
# Begin Source File

SOURCE=LowLevelHook.cpp
DEP_CPP_LOWLE=\
	"..\common\win32_helpers.h"\
	".\LowLevelHook.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=LowLevelHook.h
# End Source File
# Begin Source File

SOURCE=.\MessBox2.cpp
DEP_CPP_MESSB=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_MESSB=\
	".\omnithreadce.h"\
	
# End Source File
# Begin Source File

SOURCE=..\lzo\minilzo.c
DEP_CPP_MINIL=\
	"..\lzo\lzoconf.h"\
	"..\lzo\lzodefs.h"\
	"..\lzo\minilzo.h"\
	
NODEP_CPP_MINIL=\
	"..\lzo\lzo\lzo1x.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=MRU.cpp
DEP_CPP_MRU_C=\
	".\MRU.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=MRU.h
# End Source File
# Begin Source File

SOURCE=res\resource.h
# End Source File
# Begin Source File

SOURCE=rfb.h
# End Source File
# Begin Source File

SOURCE=SessionDialog.cpp
DEP_CPP_SESSI=\
	"..\common\win32_helpers.h"\
	"..\DSMPlugin\DSMPlugin.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\SessionDialog.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_SESSI=\
	"..\DSMPlugin\omnithreadce.h"\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=SessionDialog.h
# End Source File
# Begin Source File

SOURCE=stdhdrs.cpp
DEP_CPP_STDHD=\
	"..\rfb\rfbproto.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /Yc"stdhdrs.h" /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /Yc"stdhdrs.h" /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=stdhdrs.h
# End Source File
# Begin Source File

SOURCE=TextChat.cpp
DEP_CPP_TEXTC=\
	"..\common\win32_helpers.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\Exception.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_TEXTC=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=TextChat.h
# End Source File
# Begin Source File

SOURCE=ToDo.txt
# End Source File
# Begin Source File

SOURCE=vncauth.c
DEP_CPP_VNCAU=\
	"..\rfb\rfbproto.h"\
	".\d3des.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\vncauth.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=vncauth.h
# End Source File
# Begin Source File

SOURCE=VNCOptions.cpp
DEP_CPP_VNCOP=\
	"..\common\win32_helpers.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_VNCOP=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=VNCOptions.h
# End Source File
# Begin Source File

SOURCE=vncviewer.cpp
DEP_CPP_VNCVI=\
	"..\..\..\..\crash\crashrpt\include\crashrpt.h"\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Daemon.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	".\VNCviewerApp32.h"\
	
NODEP_CPP_VNCVI=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=vncviewer.h
# End Source File
# Begin Source File

SOURCE=res\vncviewer.rc
# ADD BASE RSC /l 0x409 /i "res"
# ADD RSC /l 0x409 /i "res"
# End Source File
# Begin Source File

SOURCE=VNCviewerApp.cpp
DEP_CPP_VNCVIE=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_VNCVIE=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=VNCviewerApp.h
# End Source File
# Begin Source File

SOURCE=VNCviewerApp32.cpp
DEP_CPP_VNCVIEW=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\types.h"\
	"..\rfb\rfbproto.h"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\Daemon.h"\
	".\Exception.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	".\VNCviewerApp32.h"\
	
NODEP_CPP_VNCVIEW=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=VNCviewerApp32.h
# End Source File
# Begin Source File

SOURCE=..\common\win32_helpers.cpp
DEP_CPP_WIN32=\
	"..\common\win32_helpers.h"\
	
# End Source File
# Begin Source File

SOURCE=..\common\win32_helpers.h
# End Source File
# Begin Source File

SOURCE=..\ZipUnZip32\ZipUnzip32.cpp
DEP_CPP_ZIPUN=\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zipunzip_src\unzip\globals.h"\
	"..\zipunzip_src\unzip\unzip.h"\
	"..\zipunzip_src\unzip\unzpriv.h"\
	"..\zipunzip_src\unzip\win32\rsxntwin.h"\
	"..\zipunzip_src\unzip\win32\w32cfg.h"\
	"..\zipunzip_src\unzip\windll\structs.h"\
	"..\zipunzip_src\zip20\api.h"\
	"..\zipunzip_src\zip20\tailor.h"\
	"..\zipunzip_src\zip20\win32\osdep.h"\
	"..\zipunzip_src\zip20\zip.h"\
	"..\zipunzip_src\zip20\ziperr.h"\
	
NODEP_CPP_ZIPUN=\
	"..\zipunzip_src\unzip\acorn\riscos.h"\
	"..\zipunzip_src\unzip\amiga\amiga.h"\
	"..\zipunzip_src\unzip\aosvs\aosvs.h"\
	"..\zipunzip_src\unzip\atheos\athcfg.h"\
	"..\zipunzip_src\unzip\beos\beocfg.h"\
	"..\zipunzip_src\unzip\flexos\flxcfg.h"\
	"..\zipunzip_src\unzip\maccfg.h"\
	"..\zipunzip_src\unzip\msdos\doscfg.h"\
	"..\zipunzip_src\unzip\novell\nlmcfg.h"\
	"..\zipunzip_src\unzip\os2\os2cfg.h"\
	"..\zipunzip_src\unzip\os2\os2data.h"\
	"..\zipunzip_src\unzip\qdos\izqdos.h"\
	"..\zipunzip_src\unzip\tandem.h"\
	"..\zipunzip_src\unzip\theos\thscfg.h"\
	"..\zipunzip_src\unzip\unix\unxcfg.h"\
	"..\zipunzip_src\unzip\vmmvs.h"\
	"..\zipunzip_src\unzip\wince\punzip.h"\
	"..\zipunzip_src\unzip\wince\wcecfg.h"\
	"..\zipunzip_src\unzip\zlib.h"\
	"..\zipunzip_src\zip20\acorn\osdep.h"\
	"..\zipunzip_src\zip20\amiga\osdep.h"\
	"..\zipunzip_src\zip20\aosvs\osdep.h"\
	"..\zipunzip_src\zip20\atari\osdep.h"\
	"..\zipunzip_src\zip20\atheos\osdep.h"\
	"..\zipunzip_src\zip20\beos\osdep.h"\
	"..\zipunzip_src\zip20\cmsmvs.h"\
	"..\zipunzip_src\zip20\human68k\osdep.h"\
	"..\zipunzip_src\zip20\macos\osdep.h"\
	"..\zipunzip_src\zip20\msdos\osdep.h"\
	"..\zipunzip_src\zip20\os2\osdep.h"\
	"..\zipunzip_src\zip20\qdos\osdep.h"\
	"..\zipunzip_src\zip20\tandem.h"\
	"..\zipunzip_src\zip20\tanzip.h"\
	"..\zipunzip_src\zip20\theos\osdep.h"\
	"..\zipunzip_src\zip20\tops20\osdep.h"\
	"..\zipunzip_src\zip20\unix\osdep.h"\
	"..\zipunzip_src\zip20\vms\osdep.h"\
	"..\zipunzip_src\zip20\zlib.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\ZipUnZip32\ZipUnZip32.h
# End Source File
# Begin Source File

SOURCE=zrle.cpp
DEP_CPP_ZRLE_=\
	"..\libjpeg\jconfig.h"\
	"..\libjpeg\jerror.h"\
	"..\libjpeg\jmorecfg.h"\
	"..\libjpeg\jpegint.h"\
	"..\libjpeg\jpeglib.h"\
	"..\rdr\Exception.h"\
	"..\rdr\FdInStream.h"\
	"..\rdr\InStream.h"\
	"..\rdr\types.h"\
	"..\rdr\ZlibInStream.h"\
	"..\rfb\rfbproto.h"\
	"..\rfb\zrledecode.h"\
	"..\rfb\zywrletemplate.c"\
	"..\ZipUnZip32\ZipUnZip32.h"\
	"..\zlib\zconf.h"\
	"..\zlib\zlib.h"\
	".\AccelKeys.h"\
	".\ClientConnection.h"\
	".\FileTransfer.h"\
	".\FullScreenTitleBar.h"\
	".\FullScreenTitleBarConst.h"\
	".\KeyMap.h"\
	".\KeyMapjap.h"\
	".\keysym.h"\
	".\keysymdef.h"\
	".\keysymdefjap.h"\
	".\Log.h"\
	".\MRU.h"\
	".\multimon.h"\
	".\omnithread\nt.h"\
	".\omnithread\omnithread.h"\
	".\rfb.h"\
	".\stdhdrs.h"\
	".\TextChat.h"\
	".\VNCOptions.h"\
	".\vncviewer.h"\
	".\VNCviewerApp.h"\
	
NODEP_CPP_ZRLE_=\
	".\omnithreadce.h"\
	

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GX /GZ

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX

!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX

!ENDIF 

# End Source File
# End Target
# End Project
