#include "UltraVNCMessageBox.h"

char str50275[128];
char str50276[128];
char str50277[128];
char str50278[128];
char str50279[128];
char str50280[128];
char str50281[128];
char str50282[128];
char str50283[128];
char str50284[128];
char str50285[128];
char str50286[128];
char str50287[128];
char str50288[128];

void loadStrings(HINSTANCE m_hInstResDLL)
{
    LoadString(m_hInstResDLL, IDS_STRING50275, str50275, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50276, str50276, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50277, str50277, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50278, str50278, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50279, str50279, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50280, str50280, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50281, str50281, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50282, str50282, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50283, str50283, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50284, str50284, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50285, str50285, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50286, str50286, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50287, str50287, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50288, str50288, 128 - 1);
}

bool yesnoBox(HINSTANCE hInst, HWND m_hWnd, char* szHeader, char* body, char* okStr, char* cancelStr, char* checkbox, BOOL &bCheckboxChecked)
{
    wchar_t w_header[128];
    wchar_t w_body[1024];
    wchar_t w_checkbox[1024];
    wchar_t w_okStr[512];
    wchar_t w_cancelStr[512];
    size_t outSize;
    mbstowcs_s(&outSize, w_header, szHeader, strlen(szHeader) + 1);
    mbstowcs_s(&outSize, w_body, body, strlen(body) + 1);
    if (strlen(checkbox) > 0)
        mbstowcs_s(&outSize, w_checkbox, checkbox, strlen(checkbox) + 1);
    mbstowcs_s(&outSize, w_okStr, okStr, strlen(okStr) + 1);
    mbstowcs_s(&outSize, w_cancelStr, cancelStr, strlen(cancelStr) + 1);

    HRESULT hr;
    TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
    int nClickedBtn;
    LPCWSTR szTitle = L"UltraVNC Viewer";
    /*,
        szHeader = L"The server does not support encryption, despite the viewer's request for it.",
        szBodyText = L"Do you want to continue?",
        szCheckboxText = L"Don't ask anymore, always allow insecure connection.";*/
    TASKDIALOG_BUTTON aCustomButtons[] = {
        { 1000, w_okStr},
        { 1001, w_cancelStr}
    };
    tdc.cbSize = sizeof(tdc);
    tdc.hwndParent = m_hWnd;
    tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
    tdc.pButtons = aCustomButtons;
    tdc.cButtons = _countof(aCustomButtons);
    tdc.pszWindowTitle = szTitle;
    tdc.nDefaultButton = 1001;

    tdc.hInstance = hInst;
    tdc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);// TD_INFORMATION_ICON;
    tdc.pszMainInstruction = w_header;
    tdc.pszContent = w_body;
    if (strlen(checkbox) > 0)
        tdc.pszVerificationText = w_checkbox;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, &bCheckboxChecked);

    if (SUCCEEDED(hr) && 1000 == nClickedBtn)
        return true;
    return false;
}
