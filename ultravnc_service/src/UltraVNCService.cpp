// UltraVNCService.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "UltraVNCService.h"
#include "UltraVNCServiceClass.h"



#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "wtsapi32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HINSTANCE g_hInstance;

void yesUVNCMessageBox(HWND m_hWnd, const char* body, const char* szHeader, int icon);
int StartUVNCMessageBox(HWND m_hWnd, const char* body, const char* szHeader, int icon);

int APIENTRY WinMain(HINSTANCE hInstance,      // Handle to the current instance of the application
    HINSTANCE hPrevInstance,  // Handle to the previous instance (always NULL in modern Windows)
    LPSTR lpCmdLine,          // Command-line arguments as a single string
    int nCmdShow              // Flags that specify how the window should be shown
)
{
    g_hInstance = hInstance;
    // Make a writable copy of the command-line string
    char cmdLineBuffer[MAX_PATH];
    strncpy_s(cmdLineBuffer, lpCmdLine, MAX_PATH - 1);
    cmdLineBuffer[MAX_PATH - 1] = '\0'; // Ensure null-termination

    // Convert the command-line string to lowercase
    for (size_t i = 0; i < strlen(cmdLineBuffer); i++) {
        cmdLineBuffer[i] = tolower(cmdLineBuffer[i]);
    }

    // Parse command-line arguments
    BOOL argfound = FALSE;
    for (size_t i = 0; i < strlen(cmdLineBuffer); i++) {
        if (cmdLineBuffer[i] <= ' ')
            continue; // Skip whitespace

        argfound = TRUE;
        if (strncmp(&cmdLineBuffer[i], "-install", strlen("-install")) == 0) {
            UltraVNCService::install_service();
            return 0; // Exit after handling the argument
        }
        if (strncmp(&cmdLineBuffer[i], "-uninstall", strlen("-uninstall")) == 0) {
            UltraVNCService::uninstall_service();
            return 0; // Exit after handling the argument
        }
        if (strncmp(&cmdLineBuffer[i], "-service", strlen("-service")) == 0) {
            // Set shutdown parameters: medium priority, no retry prompt on failure  
            SetProcessShutdownParameters(0x100, SHUTDOWN_NORETRY);
            UltraVNCService::start_service(lpCmdLine);
            return 0; // Exit after handling the argument
        }
        if (strncmp(&cmdLineBuffer[i], "-stop", strlen("-stop")) == 0) {
            char command[MAX_PATH + 32]; // 29 January 2008 jdp
            _snprintf_s(command, sizeof command, "net stop \"%s\"", UltraVNCService::service_name);
            WinExec(command, SW_HIDE);
            return 0;
        }

        if (strncmp(&cmdLineBuffer[i], "-start", strlen("-start")) == 0) {
            char command[MAX_PATH + 32]; // 29 January 2008 jdp
            _snprintf_s(command, sizeof command, "net start \"%s\"", UltraVNCService::service_name);
            WinExec(command, SW_HIDE);
            return 0;
        }
    }

    StartUVNCMessageBox(NULL, "The service can not be run as an application directly.\nHowever, you can manage it using the following commands.",
        "UltraVNC", MB_ICONERROR);
    return 0;
}
