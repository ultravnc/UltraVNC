
QT = core gui #Fix Error 'QMainWindow' file not found and "Project ERROR: Unknown module(s) in QT: Core" don't use "Core" use "core" respect case

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    \addon\ms-logon\authadm\authadmin.cpp \
    \addon\ms-logon\authSSP\authSSP.cpp \
    \addon\ms-logon\authSSP\EventLogging.cpp \
    \addon\ms-logon\authSSP\GenClientServerContext.cpp \
    \addon\ms-logon\authSSP\vncAccessControl.cpp \
    \addon\ms-logon\authSSP\vncSecurityEditor.cpp \
    \addon\ms-logon\authSSP\vncSSP.cpp \
    \addon\ms-logon\ldapauth\ldapAuth.cpp \
    \addon\ms-logon\ldapauth9x\ldapAuth9x.cpp \
    \addon\ms-logon\ldapauthNT4\ldapAuthnt4.cpp \
    \addon\ms-logon\logging\logging.cpp \
    \addon\ms-logon\MSLogonACL\MSLogonACL.cpp \
    \addon\ms-logon\MSLogonACL\vncExportACL.cpp \
    \addon\ms-logon\MSLogonACL\vncImportACL.cpp \
    \addon\ms-logon\testauth\ntlogon.cpp \
    \addon\ms-logon\workgrpdomnt4\workgrpdomnt4.cpp \
    \avilog\avilog\AVIGenerator.cpp \
    \avilog\avilog\stdafx.cpp \
    \common\Clipboard.cpp \
    \common\d3des.c \
    \common\Hyperlinks.cpp \
    \common\mn_wordlist.c \
    \common\mnemonic.c \
    \common\UltraVncZ.cpp \
    \common\win32_helpers.cpp \
    \DSMPlugin\DSMPlugin.cpp \
    \DSMPlugin\MSRC4Plugin\crypto.cpp \
    \DSMPlugin\MSRC4Plugin\EnvReg.cpp \
    \DSMPlugin\MSRC4Plugin\logging.cpp \
    \DSMPlugin\MSRC4Plugin\main.cpp \
    \DSMPlugin\MSRC4Plugin\MSRC4Plugin.cpp \
    \DSMPlugin\MSRC4Plugin\myDebug.cpp \
    \DSMPlugin\MSRC4Plugin\registry.cpp \
    \DSMPlugin\MSRC4Plugin\StdAfx.cpp \
    \DSMPlugin\MSRC4Plugin\utils.cpp \
    \DSMPlugin\TestPlugin\StdAfx.cpp \
    \DSMPlugin\TestPlugin\TestPlugin.cpp \
    \lzo\minilzo.c \
    \omnithread\nt.cpp \
    \omnithread\threaddata.cpp \
    \rdr\FdInStream.cxx \
    \rdr\FdOutStream.cxx \
    \rdr\InStream.cxx \
    \rdr\NullOutStream.cxx \
    \rdr\xzInStream.cxx \
    \rdr\xzOutStream.cxx \
    \rdr\ZlibInStream.cxx \
    \rdr\ZlibOutStream.cxx \
    \rdr\ZstdInStream.cxx \
    \rdr\ZstdOutStream.cxx \
    \repeater\gui.cpp \
    \repeater\lists_functions.cpp \
    \repeater\mode12_listener.cpp \
    \repeater\mode2_listener_server.cpp \
    \repeater\repeater.cpp \
    \repeater\socket_functions.cpp \
    \repeater\webgui\settings.c \
    \repeater\webgui\webclib.c \
    \repeater\webgui\webfs.c \
    \repeater\webgui\webio.c \
    \repeater\webgui\webobjs.c \
    \repeater\webgui\websys.c \
    \repeater\webgui\webtest.c \
    \repeater\webgui\webutils.c \
    \repeater\webgui\wsfcode.c \
    \repeater\webgui\wsfdata.c \
    \rfb\dh.cpp \
    \rfb\vncauth.c \
    \rfb\xzywtemplate.c \
    \rfb\zywrletemplate.c \
    \setcad\setcad\setcad.cpp \
    \setcad\setcad\stdafx.cpp \
    \setpasswd\setpasswd\inifile.cpp \
    \setpasswd\setpasswd\setpasswd.cpp \
    \setpasswd\setpasswd\stdafx.cpp \
    \udt4\app\appclient.cpp \
    \udt4\app\appserver.cpp \
    \udt4\app\recvfile.cpp \
    \udt4\app\sendfile.cpp \
    \udt4\app\test.cpp \
    \udt4\src\api.cpp \
    \udt4\src\buffer.cpp \
    \udt4\src\cache.cpp \
    \udt4\src\ccc.cpp \
    \udt4\src\channel.cpp \
    \udt4\src\common.cpp \
    \udt4\src\core.cpp \
    \udt4\src\epoll.cpp \
    \udt4\src\list.cpp \
    \udt4\src\md5.cpp \
    \udt4\src\packet.cpp \
    \udt4\src\queue.cpp \
    \udt4\src\window.cpp \
    \UdtCloudlib\pch.cpp \
    \UdtCloudlib\proxy\Cloudthread.cpp \
    \UdtCloudlib\proxy\proxy.cpp \
    \UdtCloudlib\proxy\ringbuffer.cpp \
    \UltraViewer\CardList.cpp \
    \UltraViewer\DlgPasswordBox.cpp \
    \UltraViewer\IPC.cpp \
    \UltraViewer\PollThread.cpp \
    \UltraViewer\Rijndael.cpp \
    \UltraViewer\SettingsManager.cpp \
    \UltraViewer\SizeChangedThread.cpp \
    \UltraViewer\Tabsheet.cpp \
    \UltraViewer\TabsheetList.cpp \
    \UltraViewer\TopBarPanel.cpp \
    \UltraViewer\uAddCard.cpp \
    \UltraViewer\UdpPoll.cpp \
    \UltraViewer\uFrmCard.cpp \
    \UltraViewer\UltraVNC_Manager.cpp \
    \UltraViewer\UltraVNCManager.cpp \
    \UltraViewer\uMainForm.cpp \
    \UltraViewer\uProposal.cpp \
    \uvnckeyboardhelper\uvnckeyboardhelper\stdafx.cpp \
    \uvnckeyboardhelper\uvnckeyboardhelper\uvnckeyboardhelper.cpp \
    \uvnc_settings\uvnc_settings\capture.cpp \
    \uvnc_settings\uvnc_settings\connections.cpp \
    \uvnc_settings\uvnc_settings\createsfx2.cpp \
    \uvnc_settings\uvnc_settings\createsfx3.cpp \
    \uvnc_settings\uvnc_settings\dsmplugin.cpp \
    \uvnc_settings\uvnc_settings\filetransfer.cpp \
    \uvnc_settings\uvnc_settings\firewall.cpp \
    \uvnc_settings\uvnc_settings\inifile.cpp \
    \uvnc_settings\uvnc_settings\log.cpp \
    \uvnc_settings\uvnc_settings\misc.cpp \
    \uvnc_settings\uvnc_settings\network.cpp \
    \uvnc_settings\uvnc_settings\security.cpp \
    \uvnc_settings\uvnc_settings\service.cpp \
    \uvnc_settings\uvnc_settings\stdafx.cpp \
    \uvnc_settings\uvnc_settings\upnp.cpp \
    \uvnc_settings\uvnc_settings\uvnc_settings.cpp \
    \uvnc_settings\uvnc_settings\videodrivercheck.cpp \
    \uvnc_settings\uvnc_settings\vncOSVersion.cpp \
    \uvnc_settings\uvnc_settings\vncsetauth.cpp \
    \vncviewer\AboutBox.cpp \
    \vncviewer\AccelKeys.cpp \
    \vncviewer\AuthDialog.cpp \
    \vncviewer\buildtime.cpp \
    \vncviewer\ClientConnection.cpp \
    \vncviewer\ClientConnectionCacheRect.cpp \
    \vncviewer\ClientConnectionClipboard.cpp \
    \vncviewer\ClientConnectionCopyRect.cpp \
    \vncviewer\ClientConnectionCoRRE.cpp \
    \vncviewer\ClientConnectionCursor.cpp \
    \vncviewer\ClientConnectionFile.cpp \
    \vncviewer\ClientConnectionFullScreen.cpp \
    \vncviewer\ClientConnectionHextile.cpp \
    \vncviewer\ClientConnectionRaw.cpp \
    \vncviewer\ClientConnectionRRE.cpp \
    \vncviewer\ClientConnectionRSAAES.cpp \
    \vncviewer\ClientConnectionTight.cpp \
    \vncviewer\ClientConnectionTLS.cpp \
    \vncviewer\ClientConnectionUltra.cpp \
    \vncviewer\ClientConnectionUltra2.cpp \
    \vncviewer\ClientConnectionZlib.cpp \
    \vncviewer\ClientConnectionZlibHex.cpp \
    \vncviewer\Daemon.cpp \
    \vncviewer\display.cpp \
    \vncviewer\Exception.cpp \
    \vncviewer\FileTransfer.cpp \
    \vncviewer\FullScreenTitleBar.cpp \
    \vncviewer\InfoBox.cpp \
    \vncviewer\KeyMap.cpp \
    \vncviewer\KeyMapjap.cpp \
    \vncviewer\Log.cpp \
    \vncviewer\LowLevelHook.cpp \
    \vncviewer\MessBox.cpp \
    \vncviewer\MRU.cpp \
    \vncviewer\SessionDialog.cpp \
    \vncviewer\sessiondialogLoadSave.cpp \
    \vncviewer\SessionDialogTabs.cpp \
    \vncviewer\Snapshot.cpp \
    \vncviewer\stdhdrs.cpp \
    \vncviewer\TextChat.cpp \
    \vncviewer\UltraVNCHerperFunctions.cpp \
    \vncviewer\vncauth.c \
    \vncviewer\VNCOptions.cpp \
    \vncviewer\vnctouch.cpp \
    \vncviewer\vncviewer.cpp \
    \vncviewer\vncviewerQt.cpp \
    \vncviewer\VNCviewerApp.cpp \
    \vncviewer\VNCviewerApp32.cpp \
    \vncviewer\xz.cpp \
    \vncviewer\zrle.cpp \
    \vncviewer\directx\directxviewer.cpp \
    \winvnc\createpassword\createpassword.cpp \
    \winvnc\loadmemory\loadDllFromMemory.cpp \
    \winvnc\loadmemory\MemoryModule.c \
    \winvnc\vnchooks\SharedData.cpp \
    \winvnc\vnchooks\VNCHooks.cpp \
    \winvnc\winvnc\benchmark.cpp \
    \winvnc\winvnc\buildtime.cpp \
    \winvnc\winvnc\cadthread.cpp \
    \winvnc\winvnc\CloudDialog.cpp \
    \winvnc\winvnc\CpuUsage.cpp \
    \winvnc\winvnc\credentials.cpp \
    \winvnc\winvnc\DeskdupEngine.cpp \
    \winvnc\winvnc\Dtwinver.cpp \
    \winvnc\winvnc\getinfo.cpp \
    \winvnc\winvnc\HelperFunctions.cpp \
    \winvnc\winvnc\HideDesktop.cpp \
    \winvnc\winvnc\inifile.cpp \
    \winvnc\winvnc\initipp.cpp \
    \winvnc\winvnc\IPC.cpp \
    \winvnc\winvnc\LayeredWindows.cpp \
    \winvnc\winvnc\MouseSimulator.cpp \
    \winvnc\winvnc\PropertiesDialog.cpp \
    \winvnc\winvnc\rfbRegion_win32.cpp \
    \winvnc\winvnc\rfbRegion_X11.cxx \
    \winvnc\winvnc\rfbUpdateTracker.cpp \
    \winvnc\winvnc\RulesListView.cpp \
    \winvnc\winvnc\ScreenCapture.cpp \
    \winvnc\winvnc\ScSelect.cpp \
    \winvnc\winvnc\service.cpp \
    \winvnc\winvnc\SettingsManager.cpp \
    \winvnc\winvnc\stdhdrs.cpp \
    \winvnc\winvnc\tableinitcmtemplate.cpp \
    \winvnc\winvnc\tableinittctemplate.cpp \
    \winvnc\winvnc\tabletranstemplate.cpp \
    \winvnc\winvnc\TextChat.cpp \
    \winvnc\winvnc\Timer.cpp \
    \winvnc\winvnc\translate.cpp \
    \winvnc\winvnc\UdpEchoServer.cpp \
    \winvnc\winvnc\UltraVNCService.cpp \
    \winvnc\winvnc\uvncUiAccess.cpp \
    \winvnc\winvnc\videodriver.cpp \
    \winvnc\winvnc\videodrivercheck.cpp \
    \winvnc\winvnc\VirtualDisplay.cpp \
    \winvnc\winvnc\vistahook.cpp \
    \winvnc\winvnc\vncabout.cpp \
    \winvnc\winvnc\vncacceptdialog.cpp \
    \winvnc\winvnc\vncauth.c \
    \winvnc\winvnc\vncbuffer.cpp \
    \winvnc\winvnc\vncclient.cpp \
    \winvnc\winvnc\vncconndialog.cpp \
    \winvnc\winvnc\vncdesktop.cpp \
    \winvnc\winvnc\vncdesktopsink.cpp \
    \winvnc\winvnc\vncDesktopSW.cpp \
    \winvnc\winvnc\vncdesktopthread.cpp \
    \winvnc\winvnc\vncencodecorre.cpp \
    \winvnc\winvnc\vncencodehext.cpp \
    \winvnc\winvnc\vncencoder.cpp \
    \winvnc\winvnc\vncencoderCursor.cpp \
    \winvnc\winvnc\vncencoderre.cpp \
    \winvnc\winvnc\vncEncodeTight.cpp \
    \winvnc\winvnc\vncEncodeUltra.cpp \
    \winvnc\winvnc\vncEncodeUltra2.cpp \
    \winvnc\winvnc\vncEncodeXZ.cpp \
    \winvnc\winvnc\vncEncodeZlib.cpp \
    \winvnc\winvnc\vncEncodeZlibHex.cpp \
    \winvnc\winvnc\vncencodezrle.cpp \
    \winvnc\winvnc\vnchttpconnect.cpp \
    \winvnc\winvnc\vncinsthandler.cpp \
    \winvnc\winvnc\vnckeymap.cpp \
    \winvnc\winvnc\vncListDlg.cpp \
    \winvnc\winvnc\vnclog.cpp \
    \winvnc\winvnc\vnclogon.cpp \
    \winvnc\winvnc\vncmenu.cpp \
    \winvnc\winvnc\vncMultiMonitor.cpp \
    \winvnc\winvnc\vncntlm.cpp \
    \winvnc\winvnc\vncOSVersion.cpp \
    \winvnc\winvnc\vncserver.cpp \
    \winvnc\winvnc\vncsetauth.cpp \
    \winvnc\winvnc\vncsockconnect.cpp \
    \winvnc\winvnc\vnctimedmsgbox.cpp \
    \winvnc\winvnc\vsocket.cpp \
    \winvnc\winvnc\winvnc.cpp \
    \zipunzip_src\unzip\api.c \
    \zipunzip_src\unzip\apihelp.c \
    \zipunzip_src\unzip\crc32.c \
    \zipunzip_src\unzip\crctab.c \
    \zipunzip_src\unzip\crypt.c \
    \zipunzip_src\unzip\envargs.c \
    \zipunzip_src\unzip\explode.c \
    \zipunzip_src\unzip\extract.c \
    \zipunzip_src\unzip\fileio.c \
    \zipunzip_src\unzip\funzip.c \
    \zipunzip_src\unzip\gbloffs.c \
    \zipunzip_src\unzip\globals.c \
    \zipunzip_src\unzip\inflate.c \
    \zipunzip_src\unzip\list.c \
    \zipunzip_src\unzip\match.c \
    \zipunzip_src\unzip\process.c \
    \zipunzip_src\unzip\timezone.c \
    \zipunzip_src\unzip\ttyio.c \
    \zipunzip_src\unzip\unreduce.c \
    \zipunzip_src\unzip\unshrink.c \
    \zipunzip_src\unzip\unzip.c \
    \zipunzip_src\unzip\unzipstb.c \
    \zipunzip_src\unzip\zipinfo.c \
    \zipunzip_src\unzip\win32\crc_i386.c \
    \zipunzip_src\unzip\win32\nt.c \
    \zipunzip_src\unzip\win32\win32.c \
    \zipunzip_src\unzip\windll\uzexampl.c \
    \zipunzip_src\unzip\windll\windll.c \
    \zipunzip_src\zip20\api.c \
    \zipunzip_src\zip20\crc32.c \
    \zipunzip_src\zip20\crctab.c \
    \zipunzip_src\zip20\crypt.c \
    \zipunzip_src\zip20\deflate.c \
    \zipunzip_src\zip20\fileio.c \
    \zipunzip_src\zip20\globals.c \
    \zipunzip_src\zip20\timezone.c \
    \zipunzip_src\zip20\trees.c \
    \zipunzip_src\zip20\ttyio.c \
    \zipunzip_src\zip20\util.c \
    \zipunzip_src\zip20\zip.c \
    \zipunzip_src\zip20\zipcloak.c \
    \zipunzip_src\zip20\zipfile.c \
    \zipunzip_src\zip20\zipnote.c \
    \zipunzip_src\zip20\zipsplit.c \
    \zipunzip_src\zip20\zipup.c \
    \zipunzip_src\zip20\win32\crc_i386.c \
    \zipunzip_src\zip20\win32\nt.c \
    \zipunzip_src\zip20\win32\win32.c \
    \zipunzip_src\zip20\win32\win32zip.c \
    \zipunzip_src\zip20\windll\example.c \
    \zipunzip_src\zip20\windll\windll.c \
    \ZipUnZip32\ZipUnzip32.cpp \

HEADERS += \
    \addon\versioninfo.h \
    \addon\eventMessageLogger\messages.h \
    \addon\ms-logon\authadm\authadmin.h \
    \addon\ms-logon\authadm\resource.h \
    \addon\ms-logon\authSSP\Auth_Seq.h \
    \addon\ms-logon\authSSP\authSSP.h \
    \addon\ms-logon\authSSP\buildtime.h \
    \addon\ms-logon\authSSP\EventLogging.h \
    \addon\ms-logon\authSSP\GenClientServerContext.h \
    \addon\ms-logon\authSSP\resource.h \
    \addon\ms-logon\authSSP\vncAccessControl.h \
    \addon\ms-logon\authSSP\vncSecurityEditor.h \
    \addon\ms-logon\authSSP\vncSecurityEditorProps.h \
    \addon\ms-logon\authSSP\vncSSP.h \
    \addon\ms-logon\ldapauth\ldapAuth.h \
    \addon\ms-logon\ldapauth\resource.h \
    \addon\ms-logon\ldapauth9x\ldapAuth9x.h \
    \addon\ms-logon\ldapauth9x\resource.h \
    \addon\ms-logon\ldapauthNT4\ldapAuthnt4.h \
    \addon\ms-logon\ldapauthNT4\resource.h \
    \addon\ms-logon\logging\logging.h \
    \addon\ms-logon\logging\resource.h \
    \addon\ms-logon\MSLogonACL\buildtime.h \
    \addon\ms-logon\MSLogonACL\MSLogonACL.h \
    \addon\ms-logon\MSLogonACL\resource.h \
    \addon\ms-logon\MSLogonACL\vncExportACL.h \
    \addon\ms-logon\MSLogonACL\vncImportACL.h \
    \addon\ms-logon\testauth\resource.h \
    \addon\ms-logon\workgrpdomnt4\resource.h \
    \addon\ms-logon\workgrpdomnt4\workgrpdomnt4.h \
    \avilog\avilog\AVIGenerator.h \
    \avilog\avilog\stdafx.h \
    \common\Clipboard.h \
    \common\d3des.h \
    \common\Hyperlinks.h \
    \common\mnemonic.h \
    \common\rfb.h \
    \common\ScopeGuard.h \
    \common\stdhdrs.h \
    \common\UltraVncZ.h \
    \common\VersionHelpers.h \
    \common\win32_helpers.h \
    \DSMPlugin\DSMPlugin.h \
    \DSMPlugin\MSRC4Plugin\crypto.h \
    \DSMPlugin\MSRC4Plugin\EnvReg.h \
    \DSMPlugin\MSRC4Plugin\logging.h \
    \DSMPlugin\MSRC4Plugin\main.h \
    \DSMPlugin\MSRC4Plugin\MSRC4Plugin.h \
    \DSMPlugin\MSRC4Plugin\myDebug.h \
    \DSMPlugin\MSRC4Plugin\registry.h \
    \DSMPlugin\MSRC4Plugin\resource.h \
    \DSMPlugin\MSRC4Plugin\StdAfx.h \
    \DSMPlugin\MSRC4Plugin\utils.h \
    \DSMPlugin\MSRC4Plugin\version.h \
    \DSMPlugin\TestPlugin\resource.h \
    \DSMPlugin\TestPlugin\StdAfx.h \
    \DSMPlugin\TestPlugin\TestPlugin.h \
    \lzo\lzoconf.h \
    \lzo\lzodefs.h \
    \lzo\minilzo.h \
    \omnithread\nt.h \
    \omnithread\omnithread.h \
    \rdr\Exception.h \
    \rdr\FdInStream.h \
    \rdr\FdOutStream.h \
    \rdr\FixedMemOutStream.h \
    \rdr\InStream.h \
    \rdr\MemInStream.h \
    \rdr\MemOutStream.h \
    \rdr\NullOutStream.h \
    \rdr\OutStream.h \
    \rdr\types.h \
    \rdr\xzInStream.h \
    \rdr\xzOutStream.h \
    \rdr\ZlibInStream.h \
    \rdr\ZlibOutStream.h \
    \rdr\ZstdInStream.h \
    \rdr\ZstdOutStream.h \
    \repeater\list_functions.h \
    \repeater\repeater.h \
    \repeater\resource.h \
    \repeater\resources.h \
    \repeater\webgui\linuxdefs.h \
    \repeater\webgui\webfs.h \
    \repeater\webgui\webgui.h \
    \repeater\webgui\webio.h \
    \repeater\webgui\websys.h \
    \repeater\webgui\windowsdefs.h \
    \repeater\webgui\wsfdata.h \
    \rfb\dh.h \
    \rfb\gii.h \
    \rfb\rfbproto.h \
    \rfb\vncauth.h \
    \rfb\xzDecode.h \
    \rfb\xzEncode.h \
    \rfb\zrleDecode.h \
    \rfb\zrleEncode.h \
    \setcad\setcad\resource.h \
    \setcad\setcad\stdafx.h \
    \setcad\setcad\targetver.h \
    \setpasswd\setpasswd\inifile.h \
    \setpasswd\setpasswd\resource.h \
    \setpasswd\setpasswd\stdafx.h \
    \setpasswd\setpasswd\targetver.h \
    \udt4\app\cc.h \
    \udt4\app\test_util.h \
    \udt4\src\api.h \
    \udt4\src\buffer.h \
    \udt4\src\cache.h \
    \udt4\src\ccc.h \
    \udt4\src\channel.h \
    \udt4\src\common.h \
    \udt4\src\core.h \
    \udt4\src\epoll.h \
    \udt4\src\list.h \
    \udt4\src\md5.h \
    \udt4\src\packet.h \
    \udt4\src\queue.h \
    \udt4\src\udt.h \
    \udt4\src\window.h \
    \UdtCloudlib\framework.h \
    \UdtCloudlib\pch.h \
    \UdtCloudlib\proxy\Cloudthread.h \
    \UdtCloudlib\proxy\proxy.h \
    \UdtCloudlib\proxy\ringbuffer.h \
    \UltraViewer\CardList.h \
    \UltraViewer\CardSetting.h \
    \UltraViewer\DlgPasswordBox.h \
    \UltraViewer\IPC.h \
    \UltraViewer\PollThread.h \
    \UltraViewer\Rijndael.h \
    \UltraViewer\SettingsManager.h \
    \UltraViewer\SizeChangedThread.h \
    \UltraViewer\Tabsheet.h \
    \UltraViewer\TabsheetList.h \
    \UltraViewer\TopBarPanel.h \
    \UltraViewer\uAddCard.h \
    \UltraViewer\UdpPoll.h \
    \UltraViewer\uFrmCard.h \
    \UltraViewer\UltraVNC_ManagerPCH1.h \
    \UltraViewer\UltraVNCManagerPCH1.h \
    \UltraViewer\uMainForm.h \
    \UltraViewer\Unit1.h \
    \UltraViewer\uProposal.h \
    \uvnckeyboardhelper\uvnckeyboardhelper\Resource.h \
    \uvnckeyboardhelper\uvnckeyboardhelper\stdafx.h \
    \uvnckeyboardhelper\uvnckeyboardhelper\targetver.h \
    \uvnckeyboardhelper\uvnckeyboardhelper\uvnckeyboardhelper.h \
    \uvnc_settings\uvnc_settings\dsmplugin.h \
    \uvnc_settings\uvnc_settings\firewall.h \
    \uvnc_settings\uvnc_settings\inifile.h \
    \uvnc_settings\uvnc_settings\log.h \
    \uvnc_settings\uvnc_settings\resource.h \
    \uvnc_settings\uvnc_settings\stdafx.h \
    \uvnc_settings\uvnc_settings\upnp.h \
    \uvnc_settings\uvnc_settings\uvnc_settings.h \
    \uvnc_settings\uvnc_settings\vncOSVersion.h \
    \uvnc_settings\uvnc_settings\vncsetauth.h \
    \vncviewer\AboutBox.h \
    \vncviewer\AccelKeys.h \
    \vncviewer\AuthDialog.h \
    \vncviewer\ClientConnection.h \
    \vncviewer\Daemon.h \
    \vncviewer\display.h \
    \vncviewer\Exception.h \
    \vncviewer\FileTransfer.h \
    \vncviewer\FpsCounter.h \
    \vncviewer\FullScreenTitleBar.h \
    \vncviewer\FullScreenTitleBarConst.h \
    \vncviewer\KeyMap.h \
    \vncviewer\KeyMapjap.h \
    \vncviewer\keysym.h \
    \vncviewer\keysymdef.h \
    \vncviewer\keysymdefjap.h \
    \vncviewer\Log.h \
    \vncviewer\LowLevelHook.h \
    \vncviewer\MEssBox.h \
    \vncviewer\MRU.h \
    \vncviewer\multimon.h \
    \vncviewer\rfb.h \
    \vncviewer\SessionDialog.h \
    \vncviewer\Snapshot.h \
    \vncviewer\stdhdrs.h \
    \vncviewer\TextChat.h \
    \vncviewer\UltraVNCHelperFunctions.h \
    \vncviewer\vncauth.h \
    \vncviewer\VNCOptions.h \
    \vncviewer\vnctouch.h \
    \vncviewer\vncviewer.h \
    \vncviewer\vncviewerQt.h \
    \vncviewer\VNCviewerApp.h \
    \vncviewer\VNCviewerApp32.h \
    \vncviewer\directx\directxviewer.h \
    \vncviewer\res\resource.h \
    \winvnc\loadmemory\loadDllFromMemory.h \
    \winvnc\loadmemory\MemoryModule.h \
    \winvnc\vnchooks\resource.h \
    \winvnc\vnchooks\SharedData.h \
    \winvnc\vnchooks\VNCHooks.h \
    \winvnc\winvnc\cadthread.h \
    \winvnc\winvnc\CloudDialog.h \
    \winvnc\winvnc\CpuUsage.h \
    \winvnc\winvnc\credentials.h \
    \winvnc\winvnc\DeskdupEngine.h \
    \winvnc\winvnc\Dtwinver.h \
    \winvnc\winvnc\HelperFunctions.h \
    \winvnc\winvnc\HideDesktop.h \
    \winvnc\winvnc\inifile.h \
    \winvnc\winvnc\IPC.h \
    \winvnc\winvnc\keysymdef.h \
    \winvnc\winvnc\LayeredWindows.h \
    \winvnc\winvnc\Localization.h \
    \winvnc\winvnc\minmax.h \
    \winvnc\winvnc\MouseSimulator.h \
    \winvnc\winvnc\PropertiesDialog.h \
    \winvnc\winvnc\resource.h \
    \winvnc\winvnc\rfb.h \
    \winvnc\winvnc\rfbMisc.h \
    \winvnc\winvnc\rfbRect.h \
    \winvnc\winvnc\rfbRegion.h \
    \winvnc\winvnc\rfbRegion_win32.h \
    \winvnc\winvnc\rfbRegion_X11.h \
    \winvnc\winvnc\rfbUpdateTracker.h \
    \winvnc\winvnc\RulesListView.h \
    \winvnc\winvnc\ScreenCapture.h \
    \winvnc\winvnc\ScSelect.h \
    \winvnc\winvnc\SettingsManager.h \
    \winvnc\winvnc\stdhdrs.h \
    \winvnc\winvnc\TextChat.h \
    \winvnc\winvnc\Timer.h \
    \winvnc\winvnc\translate.h \
    \winvnc\winvnc\UdpEchoServer.h \
    \winvnc\winvnc\UltraVNCService.h \
    \winvnc\winvnc\uvncUiAccess.h \
    \winvnc\winvnc\videodriver.h \
    \winvnc\winvnc\VirtualDisplay.h \
    \winvnc\winvnc\vncabout.h \
    \winvnc\winvnc\vncacceptdialog.h \
    \winvnc\winvnc\vncauth.h \
    \winvnc\winvnc\vncbuffer.h \
    \winvnc\winvnc\vncclient.h \
    \winvnc\winvnc\vncconndialog.h \
    \winvnc\winvnc\vncdesktop.h \
    \winvnc\winvnc\vncdesktopthread.h \
    \winvnc\winvnc\vncencodecorre.h \
    \winvnc\winvnc\vncencodehext.h \
    \winvnc\winvnc\vncencodemgr.h \
    \winvnc\winvnc\vncencoder.h \
    \winvnc\winvnc\vncencoderre.h \
    \winvnc\winvnc\vncEncodeTight.h \
    \winvnc\winvnc\vncEncodeUltra.h \
    \winvnc\winvnc\vncEncodeUltra2.h \
    \winvnc\winvnc\vncEncodeXZ.h \
    \winvnc\winvnc\vncEncodeZlib.h \
    \winvnc\winvnc\vncEncodeZlibHex.h \
    \winvnc\winvnc\vncencodezrle.h \
    \winvnc\winvnc\vnchttpconnect.h \
    \winvnc\winvnc\vncinsthandler.h \
    \winvnc\winvnc\vnckeymap.h \
    \winvnc\winvnc\vncListDlg.h \
    \winvnc\winvnc\vnclog.h \
    \winvnc\winvnc\vnclogon.h \
    \winvnc\winvnc\vncmemcpy.h \
    \winvnc\winvnc\vncmenu.h \
    \winvnc\winvnc\vncOSVersion.h \
    \winvnc\winvnc\vncpasswd.h \
    \winvnc\winvnc\vncserver.h \
    \winvnc\winvnc\vncsetauth.h \
    \winvnc\winvnc\vncsockconnect.h \
    \winvnc\winvnc\vnctimedmsgbox.h \
    \winvnc\winvnc\vsocket.h \
    \winvnc\winvnc\vtypes.h \
    \winvnc\winvnc\winvnc.h \
    \zipunzip_src\unzip\consts.h \
    \zipunzip_src\unzip\crypt.h \
    \zipunzip_src\unzip\ebcdic.h \
    \zipunzip_src\unzip\globals.h \
    \zipunzip_src\unzip\inflate.h \
    \zipunzip_src\unzip\tables.h \
    \zipunzip_src\unzip\timezone.h \
    \zipunzip_src\unzip\ttyio.h \
    \zipunzip_src\unzip\unzip.h \
    \zipunzip_src\unzip\unzpriv.h \
    \zipunzip_src\unzip\unzvers.h \
    \zipunzip_src\unzip\zip.h \
    \zipunzip_src\unzip\win32\nt.h \
    \zipunzip_src\unzip\win32\rsxntwin.h \
    \zipunzip_src\unzip\win32\w32cfg.h \
    \zipunzip_src\unzip\windll\decs.h \
    \zipunzip_src\unzip\windll\structs.h \
    \zipunzip_src\unzip\windll\uzexampl.h \
    \zipunzip_src\unzip\windll\windll.h \
    \zipunzip_src\zip20\api.h \
    \zipunzip_src\zip20\crypt.h \
    \zipunzip_src\zip20\ebcdic.h \
    \zipunzip_src\zip20\revision.h \
    \zipunzip_src\zip20\tailor.h \
    \zipunzip_src\zip20\timezone.h \
    \zipunzip_src\zip20\ttyio.h \
    \zipunzip_src\zip20\zip.h \
    \zipunzip_src\zip20\ziperr.h \
    \zipunzip_src\zip20\win32\nt.h \
    \zipunzip_src\zip20\win32\osdep.h \
    \zipunzip_src\zip20\win32\rsxntwin.h \
    \zipunzip_src\zip20\win32\win32zip.h \
    \zipunzip_src\zip20\win32\zipup.h \
    \zipunzip_src\zip20\windll\example.h \
    \zipunzip_src\zip20\windll\resource.h \
    \zipunzip_src\zip20\windll\structs.h \
    \zipunzip_src\zip20\windll\windll.h \
    \zipunzip_src\zip20\windll\zipver.h \
    \ZipUnZip32\ZipUnZip32.h \

FORMS += \
    \vncviewer\vncviewerQt.ui \

RESOURCES += \
    \addon\eventMessageLogger\messages.rc \
    \addon\ms-logon\authadm\authadmin.qrc \
    \addon\ms-logon\authadm\authadmin.rc \
    \addon\ms-logon\authSSP\authSSP.rc \
    \addon\ms-logon\ldapauth\ldapAuth.rc \
    \addon\ms-logon\ldapauth9x\ldapAuth9x.rc \
    \addon\ms-logon\ldapauthNT4\ldapAuthnt4.rc \
    \addon\ms-logon\logging\logging.rc \
    \addon\ms-logon\MSLogonACL\MSLogonACL.rc \
    \addon\ms-logon\testauth\testauth.rc \
    \addon\ms-logon\workgrpdomnt4\workgrpdomnt4.rc \
    \DSMPlugin\MSRC4Plugin\crypto.rc \
    \DSMPlugin\MSRC4Plugin\MSRC4Plugin.rc \
    \DSMPlugin\MSRC4Plugin\version.rc \
    \DSMPlugin\TestPlugin\TestPlugin.rc \
    \JavaViewer\mk.bat \
    \JavaViewer\run.bat \
    \JavaViewer\runapplet.bat \
    \omnithread\build-bcc32.bat \
    \repeater\resources.rc \
    \repeater\webgui\buildfs.bat \
    \setcad\setcad\setcad.rc \
    \setpasswd\setpasswd\setpasswd.rc \
    \uvnckeyboardhelper\uvnckeyboardhelper\uvnckeyboardhelper.rc \
    \uvnc_settings\uvnc_settings\uvnc_settings.rc \
    \vncviewer\res\build-bcc32.bat \
    \vncviewer\res\vncviewer.rc \
    \winvnc\vnchooks\vnchooks.rc \
    \winvnc\winvnc\winvnc.rc \
    \zipunzip_src\unzip\windll\windll.rc \
    \zipunzip_src\zip20\windll\windll.rc \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

