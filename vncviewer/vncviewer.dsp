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
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /I "omnithread" /I ".." /Zi /W3 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /Gm /Fp".\Debug/vncviewer.pch" /Fo".\Debug/" /Fd".\Debug/" /FR /GZ /c /GX 
# ADD CPP /nologo /MTd /I "omnithread" /I ".." /Zi /W3 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /Gm /Fp".\Debug/vncviewer.pch" /Fo".\Debug/" /Fd".\Debug/" /FR /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\vncviewer.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\vncviewer.tlb" /win32 
# ADD BASE RSC /l 1033 /d "_DEBUG" 
# ADD RSC /l 1033 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Debug\vncviewer.bsc" 
# ADD BSC32 /nologo /o ".\Debug\vncviewer.bsc" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Debug\vncviewer.exe" /incremental:yes /libpath:"./omnithread/Debug" /debug /pdb:".\Debug\vncviewer.pdb" /pdbtype:sept /map:".\Debug\vncviewer.map" /subsystem:windows /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Debug\vncviewer.exe" /incremental:yes /libpath:"./omnithread/Debug" /debug /pdb:".\Debug\vncviewer.pdb" /pdbtype:sept /map:".\Debug\vncviewer.map" /subsystem:windows /machine:ix86 
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Setting build time...
PreLink_Cmds=cl \nologo \MTd \FoDebug\ \FdDebug\ \c buildtime.cpp
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
# ADD BASE CPP /nologo /MTd /I "omnithread" /I ".." /Zi /W3 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /Gm /Fp".\Debug/vncviewer.pch" /Fo".\Debug/" /Fd".\Debug/" /FR /GZ /c /GX 
# ADD CPP /nologo /MTd /I "omnithread" /I ".." /Zi /W3 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /Gm /Fp".\Debug/vncviewer.pch" /Fo".\Debug/" /Fd".\Debug/" /FR /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\vncviewer.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\vncviewer.tlb" /win32 
# ADD BASE RSC /l 1033 /d "_DEBUG" 
# ADD RSC /l 1033 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Debug\vncviewer.bsc" 
# ADD BSC32 /nologo /o ".\Debug\vncviewer.bsc" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Debug\vncviewer.exe" /incremental:yes /libpath:"./omnithread/Debug" /debug /pdb:".\Debug\vncviewer.pdb" /pdbtype:sept /map:".\Debug\vncviewer.map" /subsystem:windows 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Debug\vncviewer.exe" /incremental:yes /libpath:"./omnithread/Debug" /debug /pdb:".\Debug\vncviewer.pdb" /pdbtype:sept /map:".\Debug\vncviewer.map" /subsystem:windows 
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Setting build time...
PreLink_Cmds=cl \nologo \MTd \FoDebug\ \FdDebug\ \c buildtime.cpp
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
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /I "omnithread" /I ".." /W3 /Ox /Ot /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /Fp".\Release/vncviewer.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD CPP /nologo /MT /I "omnithread" /I ".." /W3 /Ox /Ot /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /Fp".\Release/vncviewer.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\vncviewer.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\vncviewer.tlb" /win32 
# ADD BASE RSC /l 1033 /d "NDEBUG" 
# ADD RSC /l 1033 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Release\vncviewer.bsc" 
# ADD BSC32 /nologo /o ".\Release\vncviewer.bsc" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Release\vncviewer.exe" /incremental:yes /libpath:"omnithread/Release" /pdb:".\Release\vncviewer.pdb" /pdbtype:sept /subsystem:windows /machine:ix86 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Release\vncviewer.exe" /incremental:yes /libpath:"omnithread/Release" /pdb:".\Release\vncviewer.pdb" /pdbtype:sept /subsystem:windows /machine:ix86 
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
# ADD BASE CPP /nologo /MT /I "omnithread" /I ".." /W3 /Ox /Ot /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /Fp".\Release/vncviewer.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD CPP /nologo /MT /I "omnithread" /I ".." /W3 /Ox /Ot /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NT__" /D "_WINSTATIC" /D "__WIN32__" /D "_CRT_SECURE_NO_WARNINGS" /D "_X64" /Fp".\Release/vncviewer.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\vncviewer.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\vncviewer.tlb" /win32 
# ADD BASE RSC /l 1033 /d "NDEBUG" 
# ADD RSC /l 1033 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o ".\Release\vncviewer.bsc" 
# ADD BSC32 /nologo /o ".\Release\vncviewer.bsc" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Release\vncviewer.exe" /incremental:yes /libpath:"omnithread/Release" /pdb:".\Release\vncviewer.pdb" /pdbtype:sept /subsystem:windows 
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib omnithread.lib wsock32.lib /nologo /out:".\Release\vncviewer.exe" /incremental:yes /libpath:"omnithread/Release" /pdb:".\Release\vncviewer.pdb" /pdbtype:sept /subsystem:windows 
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

SOURCE=res\ultra-logo.bmp
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnection.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionClipboard.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionCopyRect.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionCoRRE.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionCursor.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionFile.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionFullScreen.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionHextile.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionRaw.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionRRE.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionTight.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionUltra.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionZlib.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=ClientConnectionZlibHex.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

SOURCE=Flasher.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=Flasher.h
# End Source File
# Begin Source File

SOURCE=FullScreenTitleBar.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

SOURCE=.\LinkLabel.cpp
# End Source File
# Begin Source File

SOURCE=.\LinkLabel.h
# End Source File
# Begin Source File

SOURCE=Log.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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
# End Source File
# Begin Source File

SOURCE=..\lzo\minilzo.c

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=MRU.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /Yc"stdhdrs.h" /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /Yc"stdhdrs.h" /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD RSC /l 1033 /i "res" 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD RSC /l 1033 /i "res" 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD RSC /l 1033 /i "res" 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD RSC /l 1033 /i "res" 
!ENDIF

# End Source File
# Begin Source File

SOURCE=VNCviewerApp.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

SOURCE=..\ZipUnZip32\ZipUnzip32.cpp

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
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

!IF  "$(CFG)" == "vncviewer - Win32 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Debug"

# ADD CPP /nologo /GZ /GX 
!ELSEIF  "$(CFG)" == "vncviewer - Win32 Release"

# ADD CPP /nologo /GX 
!ELSEIF  "$(CFG)" == "vncviewer - x64 Release"

# ADD CPP /nologo /GX 
!ENDIF

# End Source File
# End Target
# End Project

