import argparse
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
MSBUILD = pathlib.Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe")


def source(path, revision):
    if revision:
        return subprocess.check_output(["git", "show", f"{revision}:{path}"], cwd=ROOT).decode("utf-8-sig").replace("\r\n", "\n")
    return (ROOT / path).read_text(encoding="utf-8-sig")


def function(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


HARNESS = r'''
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <tchar.h>
#include "Exception.h"

int checks = 0, failures = 0, reports = 0, messages = 0, attempts = 0;
int lastCounter = -1, sleeps = 0, closedSocket = -1;
void require(bool ok, const char* name) {
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL: %s\n", name); }
}
Exception::Exception(const wchar_t* info, int error) : m_error_nr(error), m_info(_wcsdup(info)) {}
Exception::~Exception() { std::free(m_info); }
QuietException::QuietException(const wchar_t* info, int error) : Exception(info, error) {}
QuietException::~QuietException() {}
void QuietException::Report() {}
WarningException::WarningException(const wchar_t* info, int error, bool close) : Exception(info, error), m_close(close) {}
WarningException::~WarningException() {}
void WarningException::Report() { ++reports; }

#define WINAPI
using DWORD = unsigned long;
using TCHAR = wchar_t;
using LPVOID = void*;
constexpr int WM_CLOSE = 16, INVALID_SOCKET = -1;
constexpr DWORD INFINITE = 0xffffffff;
constexpr int IDC_STATUS = 1;
void SetDlgItemTextW(int, int, const wchar_t*) {}
void UpdateWindow(int) {}
void ResetEvent(int) {}
struct CredentialManager {
    static void DeleteVncPassword(const wchar_t*, int) {}
    static void DeleteMsLogonPassword(const wchar_t*, int) {}
};
bool forcedexit = false;
struct Logger { template<typename... T> void Print(T...) {} } vnclog;
void PostMessage(int, int message, int counter, int) {
    require(message == WM_CLOSE, "reconnect posts WM_CLOSE");
    ++messages; lastCounter = counter;
}
void Sleep(DWORD) { ++sleeps; }
void closesocket(int socket) { closedSocket = socket; }
void WaitForSingleObject(void*, DWORD) {}
void CloseHandle(void*) {}

enum Outcome { Success, NetworkEOF, CancelPrompt, RefusePrompt };
struct ClientConnection {
    STATE_FIELDS
    int m_autoReconnect = 1, reconnectcounter = 3, m_nServerScale = 1, m_hwndMain = 1, m_sock = 42;
    bool m_bKillThread = true, m_running = false, m_pendingFormatChange = false;
    bool m_bReachedAuth = false, m_Is_Listening = false, m_usedSavedVncPassword = false;
    int m_reconnectcounter = 3, m_hwndStatus = 0, m_port = 5900, KillEvent = 0, failuresLeft = 1;
    wchar_t m_host[16] = L"localhost";
    char m_clearPasswd[8] = {}, m_encPasswd[8] = {}, m_encPasswdMs[8] = {};
    void* ThreadSocketTimeout = nullptr;
    Outcome outcome = Success;
    void DoConnection(bool = false) {
        ++attempts;
        if (failuresLeft-- <= 0) return;
        if (outcome == NetworkEOF) throw QuietException(L"network EOF");
        if (outcome == CancelPrompt) throw CancelException(L"user cancelled");
        if (outcome == RefusePrompt) {
            Pressed_Cancel = true;
            throw WarningException(L"security warning declined");
        }
    }
    void SendServerScale(int) {}
    void Internal_SendAppropriateFramebufferUpdateRequest() {}
    void Reconnect();
    void RunInitial();
    static DWORD WINAPI SocketTimeout(LPVOID);
    void StopSocketTimeout();
};

FUNCTIONS

using BYTE = unsigned char;
struct Certificate { DWORD length = 20; bool valid = true; };
using PCCERT_CONTEXT = Certificate*;
constexpr int CERT_HASH_PROP_ID = 3;
bool CertGetCertificateContextProperty(PCCERT_CONTEXT cert, int, BYTE* bytes, DWORD* size) {
    if (!cert->valid || cert->length > *size) return false;
    *size = cert->length;
    for (DWORD i = 0; i < *size; ++i) bytes[i] = static_cast<BYTE>(i);
    return true;
}

THUMBPRINT

void reset() {
    reports = messages = attempts = sleeps = 0;
    lastCounter = closedSocket = -1;
}
void reconnectChecks(bool thread) {
    auto run = [thread](ClientConnection& cc) {
        if (thread) ReconnectThreadProc(&cc); else cc.Reconnect();
    };
    reset();
    ClientConnection closed, live;
    closed.forcedexit = true;
    forcedexit = true;
    run(live);
    require(attempts == 1 && live.m_running, "another session closing does not abort reconnect");
    require(messages == 0, "successful reconnect keeps session open");
    forcedexit = false;
    reset();
    ClientConnection network;
    network.outcome = NetworkEOF;
    run(network);
    require(attempts == 1 && messages == 1 && lastCounter == 2, "quiet network EOF consumes one retry, not all retries");
    require(reports == 0, "quiet EOF remains quiet");
    reset();
    ClientConnection cancelled;
    cancelled.outcome = CancelPrompt;
    run(cancelled);
    require(messages == 1 && lastCounter == 0, "cancelled auth closes without retry");
    reset();
    ClientConnection refused;
    refused.outcome = RefusePrompt;
    run(refused);
    require(messages == 1 && lastCounter == 0, "declined security warning closes disconnected session");
    reset();
    ClientConnection stopping;
    stopping.forcedexit = true;
    run(stopping);
    require(attempts == 0 && messages == 0, "closing session does not start reconnect");
    reset();
    ClientConnection exhausted;
    exhausted.reconnectcounter = 1;
    exhausted.outcome = NetworkEOF;
    run(exhausted);
    require(messages == 1 && lastCounter == 0, "last retry closes session");
}
int main() {
    reconnectChecks(true);
    reconnectChecks(false);
    reset();
    ClientConnection initial;
    initial.outcome = NetworkEOF;
    bool failed = false;
    try { initial.RunInitial(); } catch (Exception&) { failed = true; }
    require(!failed && attempts == 2, "initial quiet network failure retries successfully");
    reset();
    ClientConnection cancelInitial;
    cancelInitial.outcome = CancelPrompt;
    bool cancelled = false;
    try { cancelInitial.RunInitial(); } catch (CancelException&) { cancelled = true; }
    require(cancelled && attempts == 1, "initial auth cancellation never retries");
    Certificate cert;
    struct Buffer { TCHAR text[64]; unsigned guard; } buffer = {};
    buffer.guard = 0x1234abcd;
    require(GetCertificateThumbprint(&cert, buffer.text, _countof(buffer.text)), "SHA1 thumbprint formats successfully");
    require(_tcslen(buffer.text) == 59 && buffer.guard == 0x1234abcd, "Unicode thumbprint length and canary");
    require(_tcscmp(buffer.text, _T("00-01-02-03-04-05-06-07-08-09-0a-0b-0c-0d-0e-0f-10-11-12-13")) == 0, "thumbprint matches expected string");
    EXTRA_CHECKS
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--revision")
    parser.add_argument("--msbuild", type=pathlib.Path, default=MSBUILD)
    args = parser.parse_args()
    cpp = source("vncviewer/ClientConnection.cpp", args.revision)
    tls = source("vncviewer/ClientConnectionTLS.cpp", args.revision)
    header = source("vncviewer/ClientConnection.h", args.revision)
    exceptions = source("vncviewer/Exception.h", args.revision)
    if "class CancelException" not in exceptions:
        exceptions += '\nclass CancelException : public QuietException { public: explicit CancelException(const wchar_t* s) : QuietException(s) {} };\n'
    fields = []
    for name in ("Pressed_Cancel", "forcedexit", "havetobekilled"):
        fields.append(next((line.strip() for line in header.splitlines() if f"std::atomic_bool {name}" in line), f"bool {name} = false;"))
    functions = [function(cpp, "DWORD WINAPI ReconnectThreadProc("), function(cpp, "void ClientConnection::Reconnect()")]
    run = function(cpp, "void ClientConnection::Run()")
    functions.append("void ClientConnection::RunInitial() {\n" + run[run.index("reconnectcounter = m_reconnectcounter;"):run.index("GTGBS_CreateDisplay();")] + "\n}")
    extra = ""
    if not args.revision:
        lines = cpp.splitlines()
        watchers = [i for i, line in enumerate(lines) if "ThreadSocketTimeout = CreateThread(" in line]
        assert len(watchers) == 6
        assert all(lines[i - 1].strip() == "havetobekilled = true;" and "SocketTimeout, this," in lines[i] for i in watchers)
        functions += [function(cpp, "DWORD WINAPI ClientConnection::SocketTimeout("), function(cpp, "void ClientConnection::StopSocketTimeout()")]
        extra = r'''
    require(!GetCertificateThumbprint(&cert, buffer.text, 59), "short output rejected before secure CRT call");
    cert.length = 32;
    require(!GetCertificateThumbprint(&cert, buffer.text, 64), "oversized hash rejected");
    cert.length = 0;
    require(!GetCertificateThumbprint(&cert, buffer.text, 64), "empty hash rejected");
    reset();
    ClientConnection first, second;
    first.havetobekilled = first.forcedexit = true;
    second.havetobekilled = true;
    ClientConnection::SocketTimeout(&first);
    require(first.m_sock == INVALID_SOCKET && second.m_sock == 42 && second.havetobekilled, "socket cancellation is per connection");
    first.ThreadSocketTimeout = reinterpret_cast<void*>(1);
    first.StopSocketTimeout();
    require(!first.havetobekilled && !first.ThreadSocketTimeout, "watcher joined and cleared");
'''
    harness = HARNESS.replace("STATE_FIELDS", "\n".join(fields)).replace("FUNCTIONS", "\n\n".join(functions)).replace("THUMBPRINT", function(tls, "static bool GetCertificateThumbprint(")).replace("EXTRA_CHECKS", extra)
    project = '''<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
<ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Release|x64"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>
<PropertyGroup Label="Globals"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
<PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v143</PlatformToolset><CharacterSet>Unicode</CharacterSet></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
<PropertyGroup><VcpkgEnabled>false</VcpkgEnabled><OutDir>$(ProjectDir)out\\</OutDir><IntDir>$(ProjectDir)obj\\</IntDir></PropertyGroup>
<ItemDefinitionGroup><ClCompile><LanguageStandard>stdcpp17</LanguageStandard><ExceptionHandling>Sync</ExceptionHandling><PreprocessorDefinitions>UNICODE;_UNICODE;_CRT_SECURE_NO_WARNINGS</PreprocessorDefinitions></ClCompile></ItemDefinitionGroup>
<ItemGroup><ClCompile Include="reconnect.cpp" /></ItemGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" /></Project>'''
    with tempfile.TemporaryDirectory(prefix="uvnc-reconnect-") as directory:
        folder = pathlib.Path(directory)
        (folder / "Exception.h").write_text(exceptions, encoding="utf-8")
        (folder / "reconnect.cpp").write_text(harness, encoding="utf-8")
        (folder / "reconnect.vcxproj").write_text(project, encoding="utf-8")
        subprocess.run([str(args.msbuild), str(folder / "reconnect.vcxproj"), "-p:Configuration=Release", "-p:Platform=x64", "-verbosity:minimal", "-nologo"], check=True)
        subprocess.run([str(folder / "out/reconnect.exe")], check=True)


if __name__ == "__main__":
    main()
