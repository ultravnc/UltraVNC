#pragma once

class Snapshot 
{
public:
	Snapshot();
	virtual ~Snapshot();
	int DoDialog(TCHAR folder[MAX_PATH], TCHAR prefix[56]);
	static BOOL CALLBACK DlgProc(  HWND hwndDlg,  UINT uMsg, WPARAM wParam, LPARAM lParam );
	TCHAR m_folder[MAX_PATH];
	TCHAR m_prefix[56];
	void SaveJpeg(HBITMAP membit,TCHAR folder[MAX_PATH], TCHAR prefix[56]);
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	TCHAR *getFolder(){return m_folder;}
	TCHAR *getPrefix(){return m_prefix;}
};
