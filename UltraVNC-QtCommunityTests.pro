QT = Core

CONFIG += c++17 cmdline

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
		\addon\ms-logon\ldapauth\ldapAuth.h \
		\addon\ms-logon\ldapauth\resource.h \
		\addon\ms-logon\ldapauth9x\ldapAuth9x.cpp \
		\addon\ms-logon\ldapauthNT4\ldapAuthnt4.cpp \
		\addon\ms-logon\logging\logging.cpp \
		\addon\ms-logon\MSLogonACL\MSLogonACL.cpp \
		\addon\ms-logon\MSLogonACL\vncExportACL.cpp \
		\addon\ms-logon\MSLogonACL\vncImportACL.cpp \
		\addon\ms-logon\testauth\ntlogon.cpp \
		\addon\ms-logon\workgrpdomnt4\\workgrpdomnt4.cpp \
		\avilog\avilog\AVIGenerator.cpp \
		\avilog\avilog\stdafx.cpp \
		\common\d3des.c \
		\common\mn_wordlist.c \
		\common\mnemonic.c \
		\common\Clipboard.cpp \
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
		# To Do repeater to ZipUnZip32 and thiers subfolders

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    \addon\versioninfo.h \
	\addon\ms-logon\authadm\authadmin.h \
	\addon\ms-logon\authadm\resource.h \
	\addon\ms-logon\authSSP\Auth_Seq.h \
	\addon\ms-logon\authSSP\authSSP.h \
	\addon\ms-logon\authSSP\EventLogging.h \
	\addon\ms-logon\authSSP\GenClientServerContext.h \
	\addon\ms-logon\authSSP\resource.h \
	\addon\ms-logon\authSSP\vncAccessControl.h \
	\addon\ms-logon\authSSP\vncSecurityEditor.h \
	\addon\ms-logon\authSSP\vncSecurityEditorProps.h \
	\addon\ms-logon\authSSP\vncSSP.h \
	\addon\ms-logon\ldapauth\ldapAuth.cpp \
	\addon\ms-logon\ldapauth9x\ldapAuth9x.h \
	\addon\ms-logon\ldapauth9x\resource.h \
	\addon\ms-logon\ldapauthNT4\ldapAuthnt4.h \
	\addon\ms-logon\ldapauthNT4\resource.h \
	\addon\ms-logon\logging\logging.h \
	\addon\ms-logon\logging\resource.h \
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
	# To Do repeater to ZipUnZip32 and thiers subfolders