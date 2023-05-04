#ifdef SC_20
#include "ScSelect.h"
#include "resource.h"
#pragma comment (lib, "comctl32")

namespace ScSelect {
	LONG old_pref = 99;
	bool g_dis_uac = false;
	wchar_t		TextTop[150] = L"";
	wchar_t		TextMiddle[150] = L"";
	wchar_t		TextBottom[150] = L"";
	wchar_t		TextRTop[150] = L"";
	wchar_t		TextRBottom[150] = L"";
	wchar_t		TextButton[150] = L"";
	wchar_t		TextCButton[150] = L"";
	wchar_t		TextRMiddle[150] = L"";
	wchar_t		TextTitle[150] = L"";
	wchar_t		Webpage[150] = L"";
	bool		bWebpage = false;
	wchar_t		Balloon1Title[150] = L"";
	wchar_t		Balloon2Title[150] = L"";
	wchar_t		Balloon1A[150] = L"";
	wchar_t		Balloon1B[150] = L"";
	wchar_t		Balloon1C[150] = L"";
	wchar_t		Balloon2A[150] = L"";
	wchar_t		Balloon2B[150] = L"";
	wchar_t		Balloon2C[150] = L"";
	wchar_t		TextENTERCODE[150] = L"";
	char		idStr[15] = "";
	int			 wdd = 0;
	int			htd = 0;
	static HWND hList = NULL;
	LVCOLUMN	LvCol;
	LVITEMW		LvItem;
	char		g_var[25][150]{};
	char		defaultCommandLine[1024] = "";
	int			iSelect = 0;
	int			flag = 0;
	HWND		hEdit = NULL;
	int			m_mytimerid = 0;
	wchar_t		g_var_20[25][50]{};
	bool		entercode = false;
	bool		g_wallpaper_enabled = false;
	char		sfxname[100] = "";
	bool bExpanded = true;
	int			cx =0, cy = 0;

	///////////////////////////////////////////////////////////////
	HBRUSH CreateGradientBrush(COLORREF top, COLORREF bottom, LPNMCUSTOMDRAW item)
	{
		HBRUSH Brush = NULL;
		HDC hdcmem = CreateCompatibleDC(item->hdc);
		HBITMAP hbitmap = CreateCompatibleBitmap(item->hdc, item->rc.right - item->rc.left, item->rc.bottom - item->rc.top);
		SelectObject(hdcmem, hbitmap);

		int r1 = GetRValue(top), r2 = GetRValue(bottom), g1 = GetGValue(top), g2 = GetGValue(bottom), b1 = GetBValue(top), b2 = GetBValue(bottom);
		for (int i = 0; i < item->rc.bottom - item->rc.top; i++)
		{
			RECT temp;
			int r, g, b;
			r = int(r1 + double(i * (r2 - r1) / item->rc.bottom - item->rc.top));
			g = int(g1 + double(i * (g2 - g1) / item->rc.bottom - item->rc.top));
			b = int(b1 + double(i * (b2 - b1) / item->rc.bottom - item->rc.top));
			Brush = CreateSolidBrush(RGB(r, g, b));
			temp.left = 0;
			temp.top = i;
			temp.right = item->rc.right - item->rc.left;
			temp.bottom = i + 1;
			FillRect(hdcmem, &temp, Brush);
			DeleteObject(Brush);
		}
		HBRUSH pattern = CreatePatternBrush(hbitmap);

		DeleteDC(hdcmem);
		DeleteObject(Brush);
		DeleteObject(hbitmap);

		return pattern;
	}


	char *InitSC(HINSTANCE hInstance, PSTR szCmdLine)
	{
		if (strcmp(szCmdLine, "-dis_auc") == NULL) {
			Disbale_UAC_for_admin();
			return 0;
		}
		if (strstr(szCmdLine, "-en_auc ") != NULL) {
			char temp[10];
			strcpy_s(temp, szCmdLine + 8);
			int i = atoi(temp);
			Restore_UAC_for_admin(i);
			return 0;
		}

		if (strstr(szCmdLine, "-dsmpluginhelper ") != NULL)
			return szCmdLine;

		if (strstr(szCmdLine, "-dsmplugininstance ") != NULL)
			return szCmdLine;

		INITCOMMONCONTROLSEX InitCtrls;
		InitCtrls.dwICC = ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES | ICC_BAR_CLASSES |
			ICC_COOL_CLASSES | ICC_TAB_CLASSES | ICC_WIN95_CLASSES |
			ICC_PROGRESS_CLASS | ICC_PAGESCROLLER_CLASS;
		InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
		BOOL bRet = InitCommonControlsEx(&InitCtrls);
		DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDC_TITLEWINDOW), NULL, (DLGPROC)DialogProc, 0);

		if (strlen(szCmdLine) == 0)
			szCmdLine = defaultCommandLine;

		for (unsigned int i = 0; i < strlen(szCmdLine); i++)
			szCmdLine[i] = tolower(szCmdLine[i]);

		

		

		strcat_s(defaultCommandLine, " -run");
		return szCmdLine;
	}


	char* strreplace(char* s, const char* s1, const char* s2) {
		char* p = strstr(s, s1);
		if (p != NULL) {
			size_t len1 = strlen(s1);
			size_t len2 = strlen(s2);
			if (len1 != len2)
				memmove(p + len2, p + len1, strlen(p + len1) + 1);
			memcpy(p, s2, len2);
		}
		return s;
	}

	void connectRemote(HWND hWnd, HBRUSH defaultbrush, HBRUSH hotbrush, HBRUSH selectbrush)
	{
		char Text[255] = { 0 };
		char temp[255] = { 0 };
		char temp1[255] = { 0 };
		int iSlected = 0;
		int j = 0;
		iSlected = SendMessageW(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
		if (iSlected == -1) {
			MessageBox(hWnd, "No Items in ListView", "Error", MB_OK | MB_ICONINFORMATION);
			return;
		}

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iSubItem = 0;
		LvItem.pszText = (LPWSTR)Text;
		LvItem.cchTextMax = 256;
		LvItem.iItem = iSlected;
		SendMessageW(hList, LVM_GETITEMTEXT, iSlected, (LPARAM)&LvItem);

		sprintf_s(temp1, Text);
		sprintf_s(temp1, g_var[iSlected]);

		char* temp2;
		temp2 = _strupr(temp1);
		char* result = strstr(temp2, "[ENTERCODE]");
		if (result != NULL) {
			SendDlgItemMessage(hWnd, IDC_IDCODE, WM_GETTEXT, 150, (LONG)idStr);
			if (strlen(idStr) == 0) {
				ExpandBox(hWnd, result != NULL);
				return;
			}
			strreplace(temp2, "[ENTERCODE]", idStr);
			strcpy_s(defaultCommandLine, temp2);
		}
		else
			strcpy_s(defaultCommandLine, temp1);

		DeleteObject(defaultbrush);
		DeleteObject(selectbrush);
		DeleteObject(hotbrush);
		EndDialog(hWnd, 0);
	}

	BOOL CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		static HBRUSH defaultbrush = NULL;
		static HBRUSH hotbrush = NULL;
		static HBRUSH selectbrush = NULL;
		switch (Message) {

		case WM_NOTIFY:
		{
			LPNMHDR some_item = (LPNMHDR)lParam;

			if ((some_item->idFrom == IDC_CONNECT || some_item->idFrom == IDC_CLOSE || some_item->idFrom == IDC_HELPWEB)&& some_item->code == NM_CUSTOMDRAW)
			{
				LPNMCUSTOMDRAW item = (LPNMCUSTOMDRAW)some_item;

				if (item->uItemState & CDIS_SELECTED)
				{
					//Select our color when the button is selected
					if (selectbrush == NULL)
						selectbrush = CreateGradientBrush(RGB(220, 220, 220), RGB(220, 220, 220), item);

					//Create pen for button border
					HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

					//Select our brush into hDC
					HGDIOBJ old_pen = SelectObject(item->hdc, pen);
					HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

					//If you want rounded button, then use this, otherwise use FillRect().
					RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 5, 5);

					//Clean up
					SelectObject(item->hdc, old_pen);
					SelectObject(item->hdc, old_brush);
					DeleteObject(pen);

					//Now, I don't want to do anything else myself (draw text) so I use this value for return:
					return CDRF_DODEFAULT;
					//Let's say I wanted to draw text and stuff, then I would have to do it before return with
					//DrawText() or other function and return CDRF_SKIPDEFAULT
				}
				else
				{
					if (item->uItemState & CDIS_HOT) //Our mouse is over the button
					{
						//Select our color when the mouse hovers our button
						if (hotbrush == NULL)
							hotbrush = CreateGradientBrush(RGB(220, 220, 220), RGB(255, 255, 255), item);

						HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

						HGDIOBJ old_pen = SelectObject(item->hdc, pen);
						HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

						RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 5, 5);

						SelectObject(item->hdc, old_pen);
						SelectObject(item->hdc, old_brush);
						DeleteObject(pen);

						return CDRF_DODEFAULT;
					}

					//Select our color when our button is doing nothing
					if (defaultbrush == NULL)
						defaultbrush = CreateGradientBrush(RGB(255, 255, 255), RGB(220, 220, 220), item);

					HPEN pen = CreatePen(PS_INSIDEFRAME, 0, RGB(0, 0, 0));

					HGDIOBJ old_pen = SelectObject(item->hdc, pen);
					HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

					RoundRect(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom, 5, 5);

					SelectObject(item->hdc, old_pen);
					SelectObject(item->hdc, old_brush);
					DeleteObject(pen);

					return CDRF_DODEFAULT;
				}
				return CDRF_DODEFAULT;
			}


			switch (LOWORD(wParam)) {
			case IDC_LIST:
				if (((LPNMHDR)lParam)->code == NM_DBLCLK) {

					connectRemote(hWnd, defaultbrush, hotbrush, selectbrush);
				}
				if (((LPNMHDR)lParam)->code == NM_CLICK) {
					iSelect = SendMessageW(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
					if (iSelect == -1) {
						MessageBox(hWnd, "No Vnc server selected", "Error", MB_OK | MB_ICONINFORMATION);
						break;
					}
					char temp1[255] = { 0 };
					char* temp2;
					sprintf_s(temp1, g_var[iSelect]);
					temp2 = _strupr(temp1);
					char *result = strstr(temp2, "[ENTERCODE]");
					ExpandBox(hWnd, result != NULL);
					flag = 1;
				}

				if (((LPNMHDR)lParam)->code == LVN_BEGINLABELEDIT)
					hEdit = ListView_GetEditControl(hList);

				if (((LPNMHDR)lParam)->code == LVN_ENDLABELEDIT) {
					int iIndex;
					char text[255] = "";
					iIndex = SendMessageW(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
					LvItem.iSubItem = 0;
					LvItem.pszText = (LPWSTR)text;
					GetWindowText(hEdit, text, sizeof(text));
					SendMessageW(hList, LVM_SETITEMTEXT, (WPARAM)iIndex, (LPARAM)&LvItem);
				}
				break;
			}
		}
		break;
		case WM_ERASEBKGND:
			return   DoSDKEraseBkGnd((HDC)wParam, RGB(254, 254, 254));
		case WM_CTLCOLORSTATIC:
		//case WM_CTLCOLOREDIT:
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (DWORD)GetStockObject(NULL_BRUSH);
		case WM_CLOSE:
			EndDialog(hWnd, 0); // kill dialog
			PostMessage(hWnd, WM_QUIT, 0, 0);
			exit(0);
		case WM_PAINT:
			InvalidateRect(hWnd, NULL, true);
			return 0;
		case WM_TIMER:
			EndDialog(hWnd, 0);
			KillTimer(NULL, m_mytimerid);
			break;
		case WM_INITDIALOG: {
			SetFocus(hWnd);
			hList = GetDlgItem(hWnd, IDC_LIST); // get the ID of the ListView
			SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,   LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FLATSB | LVS_REPORT); // Set style
			memset(&LvCol, 0, sizeof(LvCol)); // Reset Coluom
			LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; // Type of mask
			LvCol.cx = 0x200;
			char selectHelpdask[38] = "Select Helpdesk to connect to .......";// width between each coloum
			LvCol.pszText = selectHelpdask;                     // First Header
			SendMessageW(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol); // Insert/Show the coloum
			memset(&LvItem, 0, sizeof(LvItem)); // Reset Item Struct
			LvItem.mask = LVIF_TEXT;   // Text Style
			LvItem.cchTextMax = 256; // Max size of test
			LvItem.iItem = 0;          // choose item  
			LvItem.iSubItem = 0;       // Put in first coluom
			wchar_t item0[7] = L"Item 0";
			LvItem.pszText = item0; // Text to display (can be from a char variable) (Items)
			FILE* fid;
			bool done = false;
			char configfile[1024];
			int iItem;
			if (GetModuleFileName(NULL, configfile, 1024)) {
				char* p = strrchr(configfile, '\\');
				if (p == NULL) return 0;
				*p = '\0';
				strcat_s(configfile, "\\helpdesk.txt");
			}

			int i = 0;
			bool direct = false;
			if ((fid = fopen(configfile, "r")) != NULL) {
				HBITMAP hImage = (HBITMAP)LoadImage(NULL, "logo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				HWND logo = GetDlgItem(hWnd, IDC_LOGO);
				if (hImage) SendMessageW(logo, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)(HANDLE)hImage);
				char myline[150];
				bWebpage = false;
				EnableWindow(GetDlgItem(hWnd, IDC_HELPWEB), false);
				ShowWindow(GetDlgItem(hWnd, IDC_HELPWEB), SW_HIDE);
				while (fgets(myline, sizeof(myline), fid)) {
					int j = 0;
					if (strncmp(myline, "[END]", strlen("[END]")) == 0)
						break;
					if (strncmp(myline, "[BEGIN HOSTLIST]", strlen("[BEGIN HOSTLIST]")) == 0)
						continue;
					if (strncmp(myline, "[HOST]", strlen("[HOST]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(g_var_20[i], myline);
						iItem = SendMessageW(hList, LVM_GETITEMCOUNT, 0, 0);
						LvItem.iItem = iItem;            // choose item  
						LvItem.iSubItem = 0;         // Put in first coluom
						LvItem.pszText = g_var_20[i];
						SendMessageW(hList, LVM_INSERTITEMW, 0, (LPARAM)&LvItem);
						fgets(myline, sizeof(myline), fid);
						strcpy_s(g_var[i], "");
						strcpy_s(g_var[i], myline);
						i++;
					}

					if (strncmp(myline, "[TEXTTOP]", strlen("[TEXTTOP]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextTop, myline);
						SendDlgItemMessageW(hWnd, IDC_TEXTTOP, WM_SETTEXT, 0, (LONG)TextTop);
					}

					if (strncmp(myline, "[TEXTMIDDLE]", strlen("[TEXTMIDDLE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextMiddle, myline);
						SendDlgItemMessageW(hWnd, IDC_TEXTMIDDLE, WM_SETTEXT, 0, (LONG)TextMiddle);
					}

					if (strncmp(myline, "[TEXTBOTTOM]", strlen("[TEXTBOTTOM]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextBottom, myline);
						SendDlgItemMessageW(hWnd, IDC_TEXTBOTTOM, WM_SETTEXT, 0, (LONG)TextBottom);
					}

					if (strncmp(myline, "[TEXTRTOP]", strlen("[TEXTRTOP]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextRTop, myline);
						SendDlgItemMessageW(hWnd, IDC_TEXTRICHTTOP, WM_SETTEXT, 0, (LONG)TextRTop);
					}

					if (strncmp(myline, "[TEXTRBOTTOM]", strlen("[TEXTRBOTTOM]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextRBottom, myline);
						SendDlgItemMessageW(hWnd, IDC_TEXTRIGHTBOTTOM, WM_SETTEXT, 0, (LONG)TextRBottom);
					}

					if (strncmp(myline, "[TEXTRMIDDLE]", strlen("[TEXTRMIDDLE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextRMiddle, myline);
						SendDlgItemMessageW(hWnd, IDC_TEXTRIGHTMIDDLE, WM_SETTEXT, 0, (LONG)TextRMiddle);
					}

					if (strncmp(myline, "[TEXTBUTTON]", strlen("[TEXTBUTTON]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextButton, myline);
						ShowWindow(GetDlgItem(hWnd, IDC_HELPWEB), SW_SHOW);
						EnableWindow(GetDlgItem(hWnd, IDC_HELPWEB), true);
						SendDlgItemMessageW(hWnd, IDC_HELPWEB, WM_SETTEXT, 0, (LONG)TextButton);
					}

					if (strncmp(myline, "[TEXTCLOSEBUTTON]", strlen("[TEXTCLOSEBUTTON]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextCButton, myline);
						SendDlgItemMessageW(hWnd, IDC_CLOSE, WM_SETTEXT, 0, (LONG)TextCButton);
					}

					if (strncmp(myline, "[TEXTCONNECTBUTTON]", strlen("[TEXTCONNECTBUTTON]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextCButton, myline);
						SendDlgItemMessageW(hWnd, IDC_CONNECT, WM_SETTEXT, 0, (LONG)TextCButton);
					}

					if (strncmp(myline, "[TITLE]", strlen("[TITLE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextTitle, myline);
						SetWindowTextW(hWnd, TextTitle);
					}

					if (strncmp(myline, "[WEBPAGE]", strlen("[WEBPAGE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Webpage, myline);
						bWebpage = true;
					}

					if (strncmp(myline, "[BALLOON1TITLE]", strlen("[BALLOON1TITLE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon1Title, myline);
						//Balloon1Title[strlen(Balloon1Title) - 1] = '\0';
					}
					if (strncmp(myline, "[BALLOON1A]", strlen("[BALLOON1A]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon1A, myline);
					}
					if (strncmp(myline, "[BALLOON1B]", strlen("[BALLOON1B]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon1B, myline);
					}
					if (strncmp(myline, "[BALLOON1C]", strlen("[BALLOON1C]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon1C, myline);
					}

					if (strncmp(myline, "[BALLOON2TITLE]", strlen("[BALLOON2TITLE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon2Title, myline);
					}
					if (strncmp(myline, "[BALLOON2A]", strlen("[BALLOON2A]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon2A, myline);
					}
					if (strncmp(myline, "[BALLOON2B]", strlen("[BALLOON2B]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon2B, myline);
					}
					if (strncmp(myline, "[BALLOON2C]", strlen("[BALLOON2C]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						toUnicode(Balloon2C, myline);
					}
					if (strncmp(myline, "[DIRECT]", strlen("[DIRECT]")) == 0) {
						direct = true;
					}
					if (strncmp(myline, "[ENTERCODE]", strlen("[ENTERCODE]")) == 0) {
						entercode = true;
						fgets(myline, sizeof(myline), fid);
						toUnicode(TextENTERCODE, myline);
						SendDlgItemMessageW(hWnd, IDC_STATICIDCODE, WM_SETTEXT, 0, (LONG)TextENTERCODE);
					}
					if (strncmp(myline, "[WALLPAPER]", strlen("[WALLPAPER]")) == 0) {
						g_wallpaper_enabled = true;
					}
					if (strncmp(myline, "[DIS_UAC]", strlen("[DIS_UAC]")) == 0) {
						g_dis_uac = true;
					}

					if (strncmp(myline, "[SAVEMODE]", strlen("[SAVEMODE]")) == 0) {
						fgets(myline, sizeof(myline), fid);
						strcpy_s(sfxname, myline);
						sfxname[strlen(myline) - 1] = '\0';
					}
				}
				fclose(fid);
			}
			else 
				EndDialog(hWnd, 0);
			if (i == 1 && direct) {
				strcpy_s(defaultCommandLine, g_var[0]);
				if (entercode)
					m_mytimerid = SetTimer(hWnd, 11, 20, NULL);
				else
					m_mytimerid = SetTimer(hWnd, 11, 2000, NULL);
			}

			ExpandBox(hWnd, !bExpanded);

			return TRUE; // Always True			
		}
						  break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_CLOSE:
				DeleteObject(defaultbrush);
				DeleteObject(selectbrush);
				DeleteObject(hotbrush);
				EndDialog(hWnd, 0); // kill dialog
				PostMessage(hWnd, WM_QUIT, 0, 0);				
				exit(0);
			case IDC_HELPWEB:
				if (bWebpage)
					ShellExecuteW(GetDesktopWindow(), L"open", Webpage, L"", 0, SW_SHOWNORMAL);
				break;
			case IDC_CONNECT:
				connectRemote(hWnd, defaultbrush, hotbrush, selectbrush);
				break;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}



	HBITMAP DoGetBkGndBitmap(IN CONST UINT uBmpResId)
	{
		static HBITMAP hbmBkGnd = NULL;
		if (NULL == hbmBkGnd) {
			hbmBkGnd = (HBITMAP)LoadImage(NULL, "background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			if (NULL == hbmBkGnd)
				hbmBkGnd = (HBITMAP)-1;
		}
		return (hbmBkGnd == (HBITMAP)-1)
			? NULL : hbmBkGnd;
	}

	HBITMAP DoGetBkGndBitmap3(IN CONST UINT uBmpResId)
	{
		static HBITMAP hbmBkGnd = NULL;
		if (NULL == hbmBkGnd) {
			hbmBkGnd = (HBITMAP)LoadImage(NULL, "enter.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			BITMAPINFOHEADER h2;
			h2.biSize = sizeof(h2);
			h2.biBitCount = 0;
			HDC hxdc = CreateDC("DISPLAY", NULL, NULL, NULL);
			GetDIBits(hxdc, hbmBkGnd, 0, 0, NULL, (BITMAPINFO*)&h2, DIB_RGB_COLORS);
			wdd = h2.biWidth;
			htd = h2.biHeight;
			DeleteDC(hxdc);
			if (NULL == hbmBkGnd)
				hbmBkGnd = (HBITMAP)-1;
		}
		return (hbmBkGnd == (HBITMAP)-1)
			? NULL : hbmBkGnd;
	}

	BOOL DoSDKEraseBkGnd(IN CONST HDC hDC, IN CONST COLORREF crBkGndFill)
	{
		HBITMAP hbmBkGnd = DoGetBkGndBitmap(IDB_LOGO64);
		if (hDC || hbmBkGnd) {
			RECT rc;
			if ((ERROR != GetClipBox(hDC, &rc)) && !IsRectEmpty(&rc)) {
				HDC hdcMem = CreateCompatibleDC(hDC);
				if (hdcMem) {
					HBRUSH hbrBkGnd = CreateSolidBrush(crBkGndFill);
					if (hbrBkGnd) {
						HGDIOBJ hbrOld = SelectObject(hDC, hbrBkGnd);
						if (hbrOld) {
							SIZE size = { (rc.right - rc.left), (rc.bottom - rc.top) };
							if (PatBlt(hDC, rc.left, rc.top, size.cx, size.cy, PATCOPY)) {
								HGDIOBJ hbmOld = SelectObject(hdcMem, hbmBkGnd);
								if (hbmOld) {
									StretchBlt(hDC, rc.left, rc.top, size.cx, size.cy,
										hdcMem, rc.left, rc.top, 1, 283, SRCCOPY);
									SelectObject(hdcMem, hbmOld);
								}
							}
							SelectObject(hDC, hbrOld);
						}
						DeleteObject(hbrBkGnd);
					}
					DeleteDC(hdcMem);
				}
			}
			return TRUE;
		}
		return TRUE;
	}

	int toUnicode(wchar_t* buffer, const char* str) {
		int lenW = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
		MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, lenW);
		return lenW;

	}

	void Disbale_UAC_for_admin_run_elevated()
	{
		HKEY hkLocal, hkLocalKey;
		DWORD dw;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies", 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
			return;
		if (RegOpenKeyEx(hkLocal, "System", 0, KEY_READ, &hkLocalKey) != ERROR_SUCCESS) {
			RegCloseKey(hkLocal);
			return;
		}
		LONG pref;
		ULONG type = REG_DWORD;
		ULONG prefsize = sizeof(pref);
		if (RegQueryValueEx(hkLocalKey, "ConsentPromptBehaviorAdmin", NULL, &type, (LPBYTE)&pref, &prefsize) != ERROR_SUCCESS)
			return;
		if (type != REG_DWORD)
			return;
		if (prefsize != sizeof(pref))
			return;
		old_pref = pref;
		RegCloseKey(hkLocalKey);
		RegCloseKey(hkLocal);

		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);
		SHELLEXECUTEINFO shExecInfo;

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = "-dis_auc";
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
	}

	void Disbale_UAC_for_admin()
	{
		HKEY hkLocal, hkLocalKey;
		DWORD dw;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
			0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
			return;
		if (RegOpenKeyEx(hkLocal, "System", 0, KEY_WRITE | KEY_READ, &hkLocalKey) != ERROR_SUCCESS) {
			RegCloseKey(hkLocal);
			return;
		}
		LONG pref;
		ULONG type = REG_DWORD;
		ULONG prefsize = sizeof(pref);
		if (RegQueryValueEx(hkLocalKey, "ConsentPromptBehaviorAdmin", NULL, &type, (LPBYTE)&pref, &prefsize) != ERROR_SUCCESS)
			return;
		if (type != REG_DWORD)
			return;
		if (prefsize != sizeof(pref))
			return;
		old_pref = pref;
		pref = 0;
		RegSetValueEx(hkLocalKey, "ConsentPromptBehaviorAdmin", 0, REG_DWORD, (LPBYTE)&pref, sizeof(pref));
		RegCloseKey(hkLocalKey);
		RegCloseKey(hkLocal);
	}

	void Restore_UAC_for_admin_elevated()
	{
		g_dis_uac = false;
		if (old_pref == 99)
			return;
		HKEY hkLocal, hkLocalKey;
		DWORD dw;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
			0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
			return;
		if (RegOpenKeyEx(hkLocal, "System", 0, KEY_READ, &hkLocalKey) != ERROR_SUCCESS) {
			RegCloseKey(hkLocal);
			return;
		}
		LONG pref;
		ULONG type = REG_DWORD;
		ULONG prefsize = sizeof(pref);
		if (RegQueryValueEx(hkLocalKey, "ConsentPromptBehaviorAdmin", NULL, &type, (LPBYTE)&pref, &prefsize) != ERROR_SUCCESS)
			return;
		if (type != REG_DWORD)
			return;
		if (prefsize != sizeof(pref))
			return;
		RegCloseKey(hkLocalKey);
		RegCloseKey(hkLocal);
		if (pref == old_pref) return;

		char exe_file_name[MAX_PATH];
		char parameters[MAX_PATH];
		char temp[10];
		GetModuleFileName(0, exe_file_name, MAX_PATH);
		strcpy_s(parameters, "-en_auc ");
		strcat_s(parameters, _itoa(old_pref, temp, 10));
		SHELLEXECUTEINFO shExecInfo;
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = GetForegroundWindow();
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = exe_file_name;
		shExecInfo.lpParameters = parameters;
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = NULL;
		ShellExecuteEx(&shExecInfo);
		Sleep(3000);
	}

	void Restore_UAC_for_admin(int i)
	{
		g_dis_uac = false;
		HKEY hkLocal, hkLocalKey;
		DWORD dw;
		old_pref = i;
		if (old_pref == 99) return;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
			0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
			return;
		if (RegOpenKeyEx(hkLocal, "System", 0, KEY_WRITE | KEY_READ, &hkLocalKey) != ERROR_SUCCESS) {
			RegCloseKey(hkLocal);
			return;
		}
		RegSetValueEx(hkLocalKey, "ConsentPromptBehaviorAdmin", 0, REG_DWORD, (LPBYTE)&old_pref, sizeof(old_pref));
		RegCloseKey(hkLocalKey);
		RegCloseKey(hkLocal);
	}

	BOOL DoSDKEraseBkGnd3(IN CONST HDC hDC, IN CONST COLORREF crBkGndFill )
	{
		HBITMAP hbmBkGnd = DoGetBkGndBitmap3(IDB_LOGO64);
		if (hDC && hbmBkGnd){
			RECT rc;
			if ((ERROR != GetClipBox(hDC, &rc)) && !IsRectEmpty(&rc)){
				HDC hdcMem = CreateCompatibleDC(hDC);
				if (hdcMem){
					HBRUSH hbrBkGnd = CreateSolidBrush(crBkGndFill);
					if (hbrBkGnd){
						HGDIOBJ hbrOld = SelectObject(hDC, hbrBkGnd);
						if (hbrOld){
							SIZE size = {
								(rc.right - rc.left), (rc.bottom - rc.top)
							};

							if (PatBlt(hDC, rc.left, rc.top, size.cx, size.cy, PATCOPY)){
								HGDIOBJ hbmOld = SelectObject(hdcMem, hbmBkGnd);
								if (hbmOld){
										StretchBlt(hDC, rc.left, rc.top, size.cx, size.cy, hdcMem, rc.left, rc.top, wdd, htd, SRCCOPY);
										SelectObject(hdcMem, hbmOld);
								}
							}
							SelectObject(hDC, hbrOld);
						}
						DeleteObject(hbrBkGnd);
					}
					DeleteDC(hdcMem);
				}
			}
			return TRUE;
		}
		return false;
	}

	void ExpandBox(HWND hDlg, bool fExpand)
	{
		// if the dialog is already in the requested state, return
		// immediately.
		if (fExpand == bExpanded) return;
		SetFocus(GetDlgItem(hDlg, IDC_IDCODE));
		RECT rcWnd{}, rcDefaultBox{}, rcChild{}, rcIntersection{};
		HWND wndChild = NULL;
		HWND wndDefaultBox = NULL;

		wndDefaultBox = GetDlgItem(hDlg, IDC_SCEXPAND);
		if (wndDefaultBox == NULL) return;

		GetWindowRect(wndDefaultBox, &rcDefaultBox);

		// enable/disable all of the child window outside of the default box.
		wndChild = GetTopWindow(hDlg);

		for (; wndChild != NULL; wndChild = GetWindow(wndChild, GW_HWNDNEXT))
		{
			// get rectangle occupied by child window in screen coordinates.
			GetWindowRect(wndChild, &rcChild);

			if (!IntersectRect(&rcIntersection, &rcChild, &rcDefaultBox))
			{
				EnableWindow(wndChild, fExpand);
			}
		}

		if (!fExpand)  // we are contracting
		{
			_ASSERT(bExpanded);
			GetWindowRect(hDlg, &rcWnd);

			// this is the first time we are being called to shrink the dialog
			// box.  The dialog box is currently in its expanded size and we must
			// save the expanded width and height so that it can be restored
			// later when the dialog box is expanded.

			if (cx == 0 && cy == 0)
			{
				cx = rcWnd.right - rcWnd.left;
				cy = rcDefaultBox.bottom - rcWnd.top;

				// we also hide the default box here so that it is not visible
				ShowWindow(wndDefaultBox, SW_HIDE);
			}

			// shrink the dialog box so that it encompasses everything from the top,
			// left up to and including the default box.
			SetWindowPos(hDlg, NULL, 0, 0,
				rcDefaultBox.right - rcWnd.left,
				rcDefaultBox.bottom - rcWnd.top,
				SWP_NOZORDER | SWP_NOMOVE);

			// record that the dialog is contracted.
			bExpanded = FALSE;
		}
		else // we are expanding
		{
			_ASSERT(!bExpanded);
			SetWindowPos(hDlg, NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE);

			// make sure that the entire dialog box is visible on the user's
			// screen.
			SendMessage(hDlg, DM_REPOSITION, 0, 0);
			bExpanded = TRUE;
		}
	}
}
#endif
