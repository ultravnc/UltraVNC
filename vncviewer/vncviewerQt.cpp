/*
 * mainwindows Qt Source vncviewerQt.cpp from vncviewer.cpp
*/

#include "vncviewerQt.h"
#include "vncviewerQt.ui"

VNCViewerQt::VNCViewerQt(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::VNCViewerQt)
{
    ui->setupUi(this);
}

VNCViewerQt::~VNCViewerQt()
{
    delete ui;
}



/*
*
* Original code NotQt below
*
*/

// #include "stdhdrs.h"
// #include "vncviewer.h"
// #include "Exception.h"
// #include "display.h"
// /*#ifdef _INTERNALLIB
// #pragma comment(lib, "zlibstat.lib")
// #pragma comment(lib, "zip32.lib")
// #pragma comment(lib, "unz32lib.lib")
// #pragma comment(lib, "libjpeg-turbo-win-static.lib")
// #pragma comment(lib, "libzstd_static.lib")
// #endif*/

// #include "omnithread/omnithread.h"
// #include "VNCviewerApp32.h"
// #include "shellscalingapi.h"
// #include "UltraVNCHelperFunctions.h"

// // All logging is done via the log object
// Log vnclog;
// char sz_A1[64];
// char sz_A2[64];
// char sz_A3[64];
// char sz_A4[64];
// char sz_A5[64];
// char sz_B1[64];
// char sz_B2[64];
// char sz_B3[64];
// char sz_C1[64];
// char sz_C2[64];
// char sz_C3[64];
// char sz_D1[64];
// char sz_D2[64];
// char sz_D3[64];
// char sz_D4[64];
// char sz_D5[64];
// char sz_D6[64];
// char sz_D7[64];
// char sz_D8[64];
// char sz_D9[64];
// char sz_D10[64];
// char sz_D11[64];
// char sz_D12[64];
// char sz_D13[64];
// char sz_D14[64];
// char sz_D15[64];
// char sz_D16[64];
// char sz_D17[64];
// char sz_D18[64];
// char sz_D19[64];
// char sz_D20[64];
// char sz_D21[64];
// char sz_D22[64];
// char sz_D23[64];
// char sz_D24[64];
// char sz_D25[64];
// char sz_D26[64];
// char sz_D27[64];
// char sz_D28[64];
// char sz_E1[64];
// char sz_E2[64];
// char sz_F1[64];
// char sz_F3[64];
// char sz_F4[64];
// char sz_F5[128];
// char sz_F6[64];
// char sz_F7[128];
// char sz_F8[128];
// char sz_F10[64];
// char sz_F11[64];
// char sz_G1[64];
// char sz_G2[64];
// char sz_G3[64];
// char sz_H1[64];
// char sz_H2[64];
// char sz_H3[128];
// char sz_H4[64];
// char sz_H5[64];
// char sz_H6[64];
// char sz_H7[64];
// char sz_H8[64];
// char sz_H9[64];
// char sz_H10[64];
// char sz_H11[64];
// char sz_H12[64];
// char sz_H13[64];
// char sz_H14[64];
// char sz_H15[64];
// char sz_H16[64];
// char sz_H17[64];
// char sz_H18[64];
// char sz_H19[64];
// char sz_H20[64];
// char sz_H21[64];
// char sz_H22[64];
// char sz_H23[64];
// char sz_H24[64];
// char sz_H25[64];
// char sz_H26[64];
// char sz_H27[64];
// char sz_H28[64];
// char sz_H29[64];
// char sz_H30[64];
// char sz_H31[64];
// char sz_H32[64];
// char sz_H33[64];
// char sz_H34[64];
// char sz_H35[64];
// char sz_H36[64];
// char sz_H37[64];
// char sz_H38[128];
// char sz_H39[64];
// char sz_H40[64];
// char sz_H41[64];
// char sz_H42[64];
// char sz_H43[128];
// char sz_H44[64];
// char sz_H45[64];
// char sz_H46[128];
// char sz_H47[64];
// char sz_H48[64];
// char sz_H49[64];
// char sz_H50[64];
// char sz_H51[64];
// char sz_H52[64];
// char sz_H53[64];
// char sz_H54[64];
// char sz_H55[64];
// char sz_H56[64];
// char sz_H57[64];

// char sz_H58[64];
// char sz_H59[64];
// char sz_H60[64];
// char sz_H61[64];
// char sz_H62[128];
// char sz_H63[64];
// char sz_H64[64];
// char sz_H65[64];
// char sz_H66[64];
// char sz_H67[64];
// char sz_H68[128];
// char sz_H69[64];
// char sz_H70[64];
// char sz_H71[64];
// char sz_H72[128];
// char sz_H73[64];

// char sz_I1[64];
// char sz_I2[64];
// char sz_I3[64];
// char sz_J1[128];
// char sz_J2[64];
// char sz_K1[64];
// char sz_K2[64];
// char sz_K3[128];
// char sz_K4[64];
// char sz_K5[64];
// char sz_K6[64];
// char sz_K7[64];
// char sz_L1[64];
// char sz_L2[64];
// char sz_L3[64];
// char sz_L4[64];
// char sz_L5[64];
// char sz_L6[64];
// char sz_L7[64];
// char sz_L8[64];
// char sz_L9[64];
// char sz_L10[64];
// char sz_L11[64];
// char sz_L12[64];
// char sz_L13[64];
// char sz_L14[64];
// char sz_L15[64];
// char sz_L16[64];
// char sz_L17[64];
// char sz_L18[64];
// char sz_L19[64];
// char sz_L20[64];
// char sz_L21[64];
// char sz_L22[64];
// char sz_L23[64];
// char sz_L24[64];
// char sz_L25[64];
// char sz_L26[64];
// char sz_L27[64];
// char sz_L28[64];
// char sz_L29[64];
// char sz_L30[64];
// char sz_L31[64];
// char sz_L32[64];
// char sz_L33[64];
// char sz_L34[64];
// char sz_L35[64];
// char sz_L36[64];
// char sz_L37[64];
// char sz_L38[64];
// char sz_L39[64];
// char sz_L40[64];
// char sz_L41[64];
// char sz_L42[64];
// char sz_L43[64];
// char sz_L44[64];
// char sz_L45[64];
// char sz_L46[64];
// char sz_L47[64];
// char sz_L48[64];
// char sz_L49[64];
// char sz_L50[64];
// char sz_L51[128];
// char sz_L52[64];
// char sz_L53[64];
// char sz_L54[64];
// char sz_L55[64];
// char sz_L56[64];
// char sz_L57[64];
// char sz_L58[64];
// char sz_L59[64];
// char sz_L60[64];
// char sz_L61[64];
// char sz_L62[64];
// char sz_L63[64];
// char sz_L64[64];
// char sz_L65[64];
// char sz_L66[64];
// char sz_L67[64];
// char sz_L68[64];
// char sz_L69[64];
// char sz_L70[64];
// char sz_L71[64];
// char sz_L72[64];
// char sz_L73[64];
// char sz_L74[64];
// char sz_L75[64];
// char sz_L76[64];
// char sz_L77[128];
// char sz_L78[64];
// char sz_L79[64];
// char sz_L80[64];
// char sz_L81[128];
// char sz_L82[64];
// char sz_L83[64];
// char sz_L84[64];
// char sz_L85[64];
// char sz_L86[64];
// char sz_L87[64];
// char sz_L88[64];
// char sz_L89[64];
// char sz_L90[64];
// char sz_L91[64];
// char sz_L92[64];
// char sz_L93[64];
// char sz_L94[64];

// // 14 April 2008 jdp
// char sz_H94[64];
// char sz_H95[64];
// char sz_H96[64];
// char sz_H97[64];
// char sz_H98[64];
// char sz_H99[64];
// char sz_H100[64];
// char sz_H101[64];
// char sz_H102[128];
// // File/dir Rename messages
// char sz_M1[64];
// char sz_M2[64];
// char sz_M3[64];
// char sz_M4[64];
// char sz_M5[64];
// char sz_M6[64];
// char sz_M7[64];
// char sz_M8[64];


// bool command_line=true;
// bool g_passwordfailed=true;
// bool g_ConnectionLossAlreadyReported = false;


// // Accelerator Keys
// AccelKeys TheAccelKeys;
// HINSTANCE m_hInstResDLL;


// typedef void (CALLBACK* LPFNSETDLLDIRECTORY)(LPCTSTR);
// static LPFNSETDLLDIRECTORY MySetDllDirectory = NULL;

// static BOOL read_reg_string(HKEY key, char* sub_key, char* val_name, LPBYTE data, LPDWORD data_len) {
//     HKEY hkey;
//     BOOL ret = FALSE;
//     int retv;

//     if(ERROR_SUCCESS == RegOpenKeyEx(key,
//                                       sub_key,
//                                       0,  KEY_QUERY_VALUE, &hkey)) {
//         if(ERROR_SUCCESS == (retv=RegQueryValueEx(hkey, val_name, 0, NULL, data, data_len)))
//             ret = TRUE;
//         else
//             vnclog.Print(3,"Could not read reg key '%s' subkey '%s' value: '%s'\nError: %u\n",
//                          ((key == HKEY_LOCAL_MACHINE) ? "HKLM" : (key == HKEY_CURRENT_USER) ? "HKCU" : "???"),
//                          sub_key, val_name, (UINT)GetLastError());
//         RegCloseKey(key);
//     }
//     else
//         vnclog.Print(3,"Could not open reg subkey: %s\nError: %u\n", sub_key, (UINT)GetLastError());

//     return ret;
// }

// //#define CRASHRPT
// #ifdef CRASHRPT
// #ifndef _X64
// #include "C:/DATA/crash/crashrpt/include/crashrpt.h"
// #pragma comment(lib, "C:/DATA/crash/crashrpt/lib/CrashRpt1403")
// #else
// #include "C:/DATA/crash/crashrpt/include/crashrpt.h"
// #pragma comment(lib, "C:/DATA/crash/crashrpt/lib/x64/CrashRpt1403")
// #endif
// #endif

// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR szCmdLine, int iCmdShow)
// {
//     HMODULE hUser32 = LoadLibrary(_T("user32.dll"));
//     HMODULE shcoreDLL = LoadLibrary("SHCORE.DLL");
//     //Min  Vista
//     typedef BOOL(WINAPI *SetProcessDPIAwareFunc)();
//     SetProcessDPIAwareFunc setDPIAwareF = NULL;
//     //Min Windows 8.1
//     typedef HRESULT(WINAPI *SetProcessDpiAwarenessFunc) (PROCESS_DPI_AWARENESS);
//     SetProcessDpiAwarenessFunc setDPIpiAwarenessF = NULL;
//     //Min Windows 10, version 1703
//     typedef HRESULT(WINAPI *SetProcessDpiAwarenessContextFunc) (DPI_AWARENESS_CONTEXT);
//     SetProcessDpiAwarenessContextFunc SetProcessDpiAwarenessContextF = NULL;
//     if (hUser32) {
//         setDPIAwareF = (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
//         SetProcessDpiAwarenessContextF = (SetProcessDpiAwarenessContextFunc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
//     }
//     if (shcoreDLL) {
//         setDPIpiAwarenessF =  (SetProcessDpiAwarenessFunc)GetProcAddress(shcoreDLL, "SetProcessDpiAwareness");
//     }

//     HRESULT hr = S_FALSE;
//     if (SetProcessDpiAwarenessContextF)
//         hr = SetProcessDpiAwarenessContextF(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
//     if (hr != S_OK && setDPIpiAwarenessF)
//         hr = setDPIpiAwarenessF(PROCESS_PER_MONITOR_DPI_AWARE);
//     if (hr != S_OK && (setDPIAwareF))
//         setDPIAwareF();

//     if (hUser32)
//         FreeLibrary(hUser32);
//     if (shcoreDLL)
//         FreeLibrary(shcoreDLL);

// #ifdef CRASHRPT
//     CR_INSTALL_INFO info;
//     memset(&info, 0, sizeof(CR_INSTALL_INFO));
//     info.cb = sizeof(CR_INSTALL_INFO);
//     info.pszAppName = _T("UVNC");
//     info.pszAppVersion = _T("1.4.4.0-dev");
//     info.pszEmailSubject = _T("UltraVNC Viewer 1.4.4.0-dev Error Report");
//     info.pszEmailTo = _T("uvnc@skynet.be");
//     info.uPriorities[CR_SMAPI] = 1; // Third try send report over Simple MAPI
//     // Install all available exception handlers
//     info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;
//     // Restart the app on crash
//     info.dwFlags |= CR_INST_APP_RESTART;
//     info.dwFlags |= CR_INST_SEND_QUEUED_REPORTS;
//     info.dwFlags |= CR_INST_AUTO_THREAD_HANDLERS;
//     info.pszRestartCmdLine = _T("/restart");
//     // Define the Privacy Policy URL

//     // Install crash reporting
//     int nResult = crInstall(&info);
//     if (nResult != 0)
//     {
//         // Something goes wrong. Get error message.
//         TCHAR szErrorMsg[512] = _T("");
//         crGetLastErrorMsg(szErrorMsg, 512);
//         _tprintf_s(_T("%s\n"), szErrorMsg);
//         return 1;
//     }
// #endif

//     setbuf(stderr, 0);
//     bool console = false;
//     m_hInstResDLL = NULL;

//     // [v1.0.2-jp1 fix]
//     //m_hInstResDLL = LoadLibrary("lang.dll");
//     HMODULE hmod;
//     HKEY hkey;
//     if((hmod=GetModuleHandle("kernel32.dll")))
//     {
//         MySetDllDirectory = (LPFNSETDLLDIRECTORY)GetProcAddress(hmod, "SetDllDirectoryA");
//         if(MySetDllDirectory)  MySetDllDirectory("");
//         else
//         {
//             OSVERSIONINFO osinfo;
//             osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
//             GetVersionEx(&osinfo);
//             if((osinfo.dwMajorVersion == 5 && osinfo.dwMinorVersion == 0 && strcmp(osinfo.szCSDVersion, "Service Pack 3") >= 0) ||
//                 (osinfo.dwMajorVersion == 5 &&  osinfo.dwMinorVersion == 1 && strcmp(osinfo.szCSDVersion, "") >= 0))
//             {
//                 DWORD regval = 1;
//                 DWORD reglen = sizeof(DWORD);

//                 vnclog.Print(3,"Using Win2k (SP3+) / WinXP (No SP).. Checking SafeDllSearch\n");
//                 read_reg_string(HKEY_LOCAL_MACHINE,
//                                 "System\\CurrentControlSet\\Control\\Session Manager",
//                                 "SafeDllSearchMode",
//                                 (LPBYTE)&regval,
//                                 &reglen);

//                 if(regval != 0) {
//                     vnclog.Print(3,"Trying to set SafeDllSearchMode to 0\n");
//                     regval = 0;
//                     if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
//                                      "System\\CurrentControlSet\\Control\\Session Manager",
//                                      0,  KEY_SET_VALUE, &hkey) == ERROR_SUCCESS) {
//                         if(RegSetValueEx(hkey,
//                                           "SafeDllSearchMode",
//                                           0,
//                                           REG_DWORD,
//                                           (LPBYTE) &regval,
//                                           sizeof(DWORD)) != ERROR_SUCCESS)
//                             vnclog.Print(3,"Error writing SafeDllSearchMode. Error: %u\n",(UINT)GetLastError());
//                         RegCloseKey(hkey);
//                     }
//                     else
//                         vnclog.Print(3,"Error opening Session Manager key for writing. Error: %u\n",(UINT)GetLastError());
//                 }
//                 else
//                     vnclog.Print(3,"SafeDllSearchMode is set to 0\n");


//             }


//         }
//     }

//     //limit the vnclang.dll searchpath to avoid
//     char szCurrentDir[MAX_PATH]="";
//     char szCurrentDir_vnclangdll[MAX_PATH]="";
//     if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
//     {
//         char* p = strrchr(szCurrentDir, '\\');
//         *p = '\0';
//     }
//     strcpy_s (szCurrentDir_vnclangdll,szCurrentDir);
//     strcat_s (szCurrentDir_vnclangdll,"\\");
//     strcat_s (szCurrentDir_vnclangdll,"vnclang.dll");
//     m_hInstResDLL = LoadLibrary(szCurrentDir_vnclangdll);


//     if (m_hInstResDLL==NULL)
//     {
//         m_hInstResDLL = hInstance;
//     }
//     if (strcmp(szCmdLine,"")==0) command_line=false;
//     LoadString(m_hInstResDLL, IDS_A1, sz_A1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_A2, sz_A2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_A3, sz_A3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_A4, sz_A4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_A5, sz_A5, 64 -1);
//     LoadString(m_hInstResDLL, IDS_B1, sz_B1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_B2, sz_B2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_B3, sz_B3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_C1, sz_C1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_C2, sz_C2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_C3, sz_C3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D1, sz_D1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D2, sz_D2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D3, sz_D3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D4, sz_D4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D5, sz_D5, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D6, sz_D6, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D7, sz_D7, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D8, sz_D8, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D9, sz_D9, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D10, sz_D10, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D11, sz_D11, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D12, sz_D12, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D13, sz_D13, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D14, sz_D14, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D15, sz_D15, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D16, sz_D16, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D17, sz_D17, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D18, sz_D18, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D19, sz_D19, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D20, sz_D20, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D21, sz_D21, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D22, sz_D22, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D23, sz_D23, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D24, sz_D24, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D25, sz_D25, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D26, sz_D26, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D27, sz_D27, 64 -1);
//     LoadString(m_hInstResDLL, IDS_D28, sz_D28, 64 -1);
//     LoadString(m_hInstResDLL, IDS_E1, sz_E1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_E2, sz_E2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_F1, sz_F1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_F3, sz_F3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_F4, sz_F4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_F5, sz_F5, 128 -1);
//     LoadString(m_hInstResDLL, IDS_F6, sz_F6, 64 -1);
//     LoadString(m_hInstResDLL, IDS_F7, sz_F7, 128 -1);
//     LoadString(m_hInstResDLL, IDS_F8, sz_F8, 128 -1);
//     LoadString(m_hInstResDLL, IDS_F10, sz_F10, 64 -1);
//     LoadString(m_hInstResDLL, IDS_F11, sz_F11, 64 -1);
//     LoadString(m_hInstResDLL, IDS_G1, sz_G1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_G1, sz_G2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_G1, sz_G3, 64 -1);

//     LoadString(m_hInstResDLL, IDS_H1, sz_H1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H2, sz_H2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H3, sz_H3, 128 -1);
//     LoadString(m_hInstResDLL, IDS_H4, sz_H4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H5, sz_H5, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H6, sz_H6, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H7, sz_H7, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H8, sz_H8, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H9, sz_H9, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H10, sz_H10, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H11, sz_H11, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H12, sz_H12, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H13, sz_H13, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H14, sz_H14, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H15, sz_H15, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H16, sz_H16, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H17, sz_H17, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H18, sz_H18, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H19, sz_H19, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H20, sz_H20, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H21, sz_H21, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H22, sz_H22, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H23, sz_H23, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H24, sz_H24, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H25, sz_H25, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H26, sz_H26, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H27, sz_H27, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H28, sz_H28, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H29, sz_H29, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H30, sz_H30, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H31, sz_H31, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H32, sz_H32, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H33, sz_H33, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H34, sz_H34, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H35, sz_H35, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H36, sz_H36, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H37, sz_H37, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H38, sz_H38, 128 -1);
//     LoadString(m_hInstResDLL, IDS_H39, sz_H39, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H40, sz_H40, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H41, sz_H41, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H42, sz_H42, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H43, sz_H43, 128 -1);
//     LoadString(m_hInstResDLL, IDS_H44, sz_H44, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H45, sz_H45, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H46, sz_H46, 128 -1);
//     LoadString(m_hInstResDLL, IDS_H47, sz_H47, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H48, sz_H48, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H49, sz_H49, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H50, sz_H50, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H51, sz_H51, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H52, sz_H52, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H53, sz_H53, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H54, sz_H54, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H55, sz_H55, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H56, sz_H56, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H57, sz_H57, 64 -1);

//     LoadString(m_hInstResDLL, IDS_H58, sz_H58, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H59, sz_H59, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H60, sz_H60, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H61, sz_H61, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H62, sz_H62, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H63, sz_H63, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H64, sz_H64, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H65, sz_H65, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H66, sz_H66, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H67, sz_H67, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H68, sz_H68, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H69, sz_H69, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H70, sz_H70, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H71, sz_H71, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H72, sz_H72, 128 -1);
//     LoadString(m_hInstResDLL, IDS_H73, sz_H73, 64 -1);


//     LoadString(m_hInstResDLL, IDS_I1, sz_I1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_I2, sz_I2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_I3, sz_I3, 64 -1);

//     LoadString(m_hInstResDLL, IDS_J1, sz_J1, 128 -1);
//     LoadString(m_hInstResDLL, IDS_J2, sz_J2, 64 -1);

//     LoadString(m_hInstResDLL, IDS_K1, sz_K1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_K2, sz_K2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_K3, sz_K3, sizeof(sz_K3)/sizeof(sz_K3[0]));
//     LoadString(m_hInstResDLL, IDS_K4, sz_K4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_K5, sz_K5, 64 -1);
//     LoadString(m_hInstResDLL, IDS_K6, sz_K6, 64 -1);

//     LoadString(m_hInstResDLL, IDS_L1, sz_L1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L2, sz_L2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L3, sz_L3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L4, sz_L4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L5, sz_L5, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L6, sz_L6, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L7, sz_L7, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L8, sz_L8, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L9, sz_L9, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L10, sz_L10, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L11, sz_L11, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L12, sz_L12, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L13, sz_L13, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L14, sz_L14, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L15, sz_L15, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L16, sz_L16, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L17, sz_L17, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L18, sz_L18, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L19, sz_L19, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L20, sz_L20, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L21, sz_L21, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L22, sz_L22, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L23, sz_L23, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L24, sz_L24, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L25, sz_L25, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L26, sz_L26, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L27, sz_L27, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L28, sz_L28, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L29, sz_L29, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L30, sz_L30, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L31, sz_L31, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L32, sz_L32, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L33, sz_L33, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L34, sz_L34, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L35, sz_L35, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L36, sz_L36, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L37, sz_L37, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L38, sz_L38, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L39, sz_L39, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L40, sz_L40, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L41, sz_L41, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L42, sz_L42, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L43, sz_L43, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L44, sz_L44, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L45, sz_L45, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L46, sz_L46, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L47, sz_L47, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L48, sz_L48, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L49, sz_L49, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L50, sz_L50, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L51, sz_L51, 128 -1);
//     LoadString(m_hInstResDLL, IDS_L52, sz_L52, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L53, sz_L53, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L54, sz_L54, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L55, sz_L55, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L56, sz_L56, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L57, sz_L57, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L58, sz_L58, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L59, sz_L59, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L60, sz_L60, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L61, sz_L61, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L62, sz_L62, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L63, sz_L63, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L64, sz_L64, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L65, sz_L65, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L66, sz_L66, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L67, sz_L67, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L68, sz_L68, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L69, sz_L69, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L70, sz_L70, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L71, sz_L71, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L72, sz_L72, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L73, sz_L73, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L74, sz_L74, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L75, sz_L75, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L76, sz_L76, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L77, sz_L77, 128 -1);
//     LoadString(m_hInstResDLL, IDS_L78, sz_L78, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L79, sz_L79, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L80, sz_L80, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L81, sz_L81, 128 -1);
//     LoadString(m_hInstResDLL, IDS_L82, sz_L82, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L83, sz_L83, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L84, sz_L84, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L85, sz_L85, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L86, sz_L86, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L87, sz_L87, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L88, sz_L88, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L89, sz_L89, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L90, sz_L90, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L91, sz_L91, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L92, sz_L92, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L93, sz_L93, 64 -1);
//     LoadString(m_hInstResDLL, IDS_L94, sz_L94, 64 -1);

//     LoadString(m_hInstResDLL, IDS_M1, sz_M1, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M2, sz_M2, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M3, sz_M3, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M4, sz_M4, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M5, sz_M5, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M6, sz_M6, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M7, sz_M7, 64 -1);
//     LoadString(m_hInstResDLL, IDS_M8, sz_M8, 64 -1);

//     // 14 April 2008 jdp
//     LoadString(m_hInstResDLL, IDS_H94, sz_H94, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H95, sz_H95, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H96, sz_H96, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H97, sz_H97, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H98, sz_H98, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H99, sz_H99, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H100, sz_H100, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H101, sz_H101, 64 -1);
//     LoadString(m_hInstResDLL, IDS_H102, sz_H102, 128 -1);
//     //	RegisterLinkLabel(m_hInstResDLL);
//     loadStrings(m_hInstResDLL);

//     /////////////////////////////////////////////////////////////

//     // The state of the application as a whole is contained in the one app object
//     VNCviewerApp32 app(hInstance, szCmdLine);

//     console = app.m_options.m_logToConsole;

//     // Start a new connection if specified on command line,
//     // or if not in listening mode
//     MSG msg;
//     while(g_passwordfailed==true)
//     {
//         g_passwordfailed=false;
//         if (app.m_options.m_connectionSpecified && !app.m_options.m_listening) {
//             app.NewConnection(false,app.m_options.m_host_options, app.m_options.m_port);
//         } else if (!app.m_options.m_listening) {
//             // This one will also read from config file if specified
//             app.NewConnection(false);
//         }

//         try
//         {
//             while ( GetMessage(&msg, NULL, 0,0) )
//             {
//                 if (!TheAccelKeys.TranslateAccelKeys(&msg))
//                 {
//                     TranslateMessage(&msg);
//                     DispatchMessage(&msg);
//                 }
//             }
//         }
//         catch (Exception &e)
//         {
//             if (!g_ConnectionLossAlreadyReported)
//                 e.Report();
//         }
//     }
//     // Clean up winsock
//     WSACleanup();
//     vnclog.Print(3, _T("Exiting\n"));
// #ifdef CRASHRPT
//     crUninstall();
// #endif
//     if (console) Sleep(2000);
//     return msg.wParam;
// }


// // Move the given window to the centre of the screen
// // and bring it to the top.
// void CentreWindow(HWND hwnd)
// {
//     RECT winrect, workrect;

//     // Find how large the desktop work area is
//     SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
//     //RECT workrect;
//     /*tempdisplayclass tdc;
//     tdc.Init();
//     workrect.left=tdc.monarray[selected_screen].wl;
//     workrect.right=tdc.monarray[selected_screen].wr;
//     workrect.top=tdc.monarray[selected_screen].wt;
//     workrect.bottom=tdc.monarray[selected_screen].wb;*/

//     int workwidth = workrect.right -  workrect.left;
//     int workheight = workrect.bottom - workrect.top;

//     // And how big the window is
//     GetWindowRect(hwnd, &winrect);
//     int winwidth = winrect.right - winrect.left;
//     int winheight = winrect.bottom - winrect.top;
//     // Make sure it's not bigger than the work area
//     winwidth = min(winwidth, workwidth);
//     winheight = min(winheight, workheight);

//     // Now centre it
//     SetWindowPos(hwnd,
//                  HWND_TOP,
//                  workrect.left + (workwidth-winwidth) / 2,
//                  workrect.top + (workheight-winheight) / 2,
//                  winwidth, winheight,
//                  SWP_SHOWWINDOW);
//     SetForegroundWindow(hwnd);
// }


// // sf@2002 - TightVNC method - RealVNC method
// // Convert "host:display" or "host::port" or "host:port" if port > 100, into host and port
// // Returns true if valid format, false if not.
// // Takes initial string, addresses of results and size of host buffer in wchars.
// // If the display info passed in is longer than the size of the host buffer, it
// // is assumed to be invalid, so false is returned.
// bool ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *pport)
// {
//     if (hostlen < (int)_tcslen(display))
//         return false;

//     int tmp_port;
//     TCHAR *colonpos = _tcschr(display, L':');
//     if (colonpos == NULL)
//     {
//         // No colon -- use default port number
//         tmp_port = RFB_PORT_OFFSET;
//         _tcsncpy_s(phost, MAX_HOST_NAME_LEN, display, MAX_HOST_NAME_LEN);
//     }
//     else
//     {
//         _tcsncpy_s(phost, MAX_HOST_NAME_LEN, display, colonpos - display);
//         phost[colonpos - display] = L'\0';
//         if (colonpos[1] == L':') {
//             // Two colons -- interpret as a port number
//             if (_stscanf_s(colonpos + 2, TEXT("%d"), &tmp_port) != 1)
//                 return false;
//         }
//         else
//         {
//             // One colon -- interpret as a display number or port number
//             if (_stscanf_s(colonpos + 1, TEXT("%d"), &tmp_port) != 1)
//                 return false;

//             // RealVNC method - If port < 100 interpret as display number else as Port number
//             if (tmp_port < 100)
//                 tmp_port += RFB_PORT_OFFSET;
//         }
//     }
//     *pport = tmp_port;
//     return true;
// }
