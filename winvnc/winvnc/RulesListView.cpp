// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "RulesListView.h"
#include <CommCtrl.h>
#include "SettingsManager.h"
#include "resource.h"

extern HINSTANCE	hInstResDLL;

RulesListView::RulesListView()
{

}
RulesListView::~RulesListView()
{

}
void RulesListView::init(HWND hListView)
{
	this->hListView =  hListView;
	const char* delimiter = ":";
	if (hListView) {
		ListView_DeleteAllItems(hListView);

		LVCOLUMN lvCol;
		lvCol.mask = LVCF_TEXT | LVCF_WIDTH;
		lvCol.cx = 100;  // Column width
		lvCol.pszText = const_cast<char*>("Action");
		ListView_InsertColumn(hListView, 0, &lvCol);
		lvCol.cx = 300;  // Column width
		lvCol.pszText = const_cast<char*>("Rule");
		ListView_InsertColumn(hListView, 1, &lvCol);
		lvCol.cx = 0;
		lvCol.pszText = const_cast<char*>("string");
		ListView_InsertColumn(hListView, 2, &lvCol);

		index = 0;
		char * authHost = _strdup(settings->getAuthhosts());
		if (authHost != 0) {
			char* token = strtok(authHost, delimiter);
			while (token != nullptr) {

				{
					LVITEM lvItem;
					lvItem.mask = LVIF_TEXT;          // Specifies that we're adding text
					lvItem.iItem = index;             // Item index (position)
					lvItem.iSubItem = 0;
					switch (token[0])
					{
					case '-':
						lvItem.pszText = "Refuse";  // Item text
						ListView_InsertItem(hListView, &lvItem);
						break;
					case '+':
						lvItem.pszText = "Allow";  // Item text
						ListView_InsertItem(hListView, &lvItem);
						break;
					case '?':
						lvItem.pszText = "Query";  // Item text
						ListView_InsertItem(hListView, &lvItem);
						break;
					default:
						lvItem.pszText = "Refuse";  // Item text							
						ListView_InsertItem(hListView, &lvItem);
					}
				}
				char restOfString[1024];
				strcpy_s(restOfString, token + 1);
				ListView_SetItemText(hListView, index, 1, const_cast<char*>(restOfString));
				ListView_SetItemText(hListView, index, 2, const_cast<char*>(token));

				index++;
				token = strtok(nullptr, delimiter);
			}
		}
	}
}

void RulesListView::addItem(char* caption)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT;          // Specifies that we're adding text
	lvItem.iItem = index;             // Item index (position)
	lvItem.iSubItem = 0;
	switch (caption[0])
	{
	case '-':
		lvItem.pszText = "Refuse";  // Item text
		ListView_InsertItem(hListView, &lvItem);
		break;
	case '+':
		lvItem.pszText = "Allow";  // Item text
		ListView_InsertItem(hListView, &lvItem);
		break;
	case '?':
		lvItem.pszText = "Query";  // Item text
		ListView_InsertItem(hListView, &lvItem);
		break;
	default:
		lvItem.pszText = "Refuse";  // Item text							
		ListView_InsertItem(hListView, &lvItem);
	}
	char restOfString[1024];
	strcpy_s(restOfString, caption + 1);
	ListView_SetItemText(hListView, index, 1, const_cast<char*>(restOfString));
	ListView_SetItemText(hListView, index, 2, const_cast<char*>(caption));
	index++;
}

void RulesListView::updateItem(int index, char* caption)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT;          // Specifies that we're adding text
	lvItem.iItem = index;             // Item index (position)
	lvItem.iSubItem = 0;
	switch (caption[0])
	{
	case '-':
		lvItem.pszText = "Refuse";  // Item text
		ListView_SetItemText(hListView, index, 0, "Refuse");
		break;
	case '+':
		lvItem.pszText = "Allow";  // Item text
		ListView_SetItemText(hListView, index, 0, "Allow");
		break;
	case '?':
		lvItem.pszText = "Query";  // Item text
		ListView_SetItemText(hListView, index, 0, "Query");
		break;
	default:
		lvItem.pszText = "Refuse";  // Item text							
		ListView_SetItemText(hListView, index, 0, "Refuse");
	}
	char restOfString[1024];
	strcpy_s(restOfString, caption + 1);
	ListView_SetItemText(hListView, index, 1, const_cast<char*>(restOfString));
	ListView_SetItemText(hListView, index, 2, const_cast<char*>(caption));
}

void RulesListView::selectItem(int index)
{
	WPARAM itemIndex = (WPARAM)index;
	ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
	SendMessage(hListView, LVM_ENSUREVISIBLE, itemIndex, FALSE);
	ListView_SetItemState(hListView, itemIndex, LVIS_SELECTED, LVIS_SELECTED);
	ListView_SetItemState(hListView, itemIndex, LVIS_FOCUSED, LVIS_FOCUSED);
	SetFocus(hListView);
}

void RulesListView::removeSelectedItem()
{
	int iSelect = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
	ListView_DeleteItem(hListView, iSelect);

	selectItem(iSelect);
	if (ListView_GetNextItem(hListView, -1, LVNI_SELECTED) == -1) {
		selectItem(iSelect - 1);
	}
	else if (ListView_GetNextItem(hListView, -1, LVNI_SELECTED) == -1) {
		selectItem(iSelect + 1);
	}
	index--;
}

void RulesListView::moveUp()
{
	int si = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
	// If nothing selected
	if ((si == -1) || (si == 0)) {
		return;
	}
	LVITEM lv1;
	lv1.mask = LVIF_TEXT;
	lv1.iItem = si;
	lv1.iSubItem = 2;
	char accessRule[256];
	lv1.pszText = (char*)&accessRule[0];
	lv1.cchTextMax = 256;
	ListView_GetItem(hListView, &lv1);

	LVITEM lv2;
	lv2.mask = LVIF_TEXT;
	lv2.iItem = si-1;
	lv2.iSubItem = 2;
	char accessRule2[256];
	lv2.pszText = (char*)&accessRule2[0];
	lv2.cchTextMax = 256;
	ListView_GetItem(hListView, &lv2);

	updateItem(si - 1, accessRule);
	updateItem(si, accessRule2);
	selectItem(si - 1);
}

void RulesListView::moveDown()
{
	int si = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
	// If nothing selected
	if ((si == -1) || (si == ListView_GetItemCount(hListView) - 1)) {
		return;
	}

	LVITEM lv1;
	lv1.mask = LVIF_TEXT;
	lv1.iItem = si;
	lv1.iSubItem = 2;
	char accessRule[256];
	lv1.pszText = (char*)&accessRule[0];
	lv1.cchTextMax = 256;
	ListView_GetItem(hListView, &lv1);

	LVITEM lv2;
	lv2.mask = LVIF_TEXT;
	lv2.iItem = si + 1;
	lv2.iSubItem = 2;
	char accessRule2[256];
	lv2.pszText = (char*)&accessRule2[0];
	lv2.cchTextMax = 256;
	ListView_GetItem(hListView, &lv2);

	updateItem(si, accessRule2);
	updateItem(si + 1, accessRule);
	selectItem(si + 1);
}


INT_PTR CALLBACK dialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RulesListView* _this;
	BOOL bResult;

	bResult = FALSE;
	if (uMsg == WM_INITDIALOG) {
		_this = (RulesListView*)lParam;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)_this);
		//_this->m_ctrlThis.setWindow(hwnd);
		//_this->updateIcon();
	}
	else {
		_this = (RulesListView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (_this == 0) {
			return FALSE;
		}
	}

	switch (uMsg) {
	case WM_INITDIALOG:
		bResult = _this->onInitDialog(hwnd);
		break;
	case WM_NOTIFY:
		bResult = FALSE;
		break;
	case WM_COMMAND:
		bResult = _this->onCommand(hwnd, LOWORD(wParam), HIWORD(wParam));
		break;
	case WM_CLOSE:
		bResult = FALSE;
		break;
	case WM_DESTROY:
		bResult = FALSE;
		break;
	case WM_DRAWITEM:
		bResult = TRUE;
		break;
	}

	return bResult;
}

bool RulesListView::onInitDialog(HWND hwnd)
{
	if (isEdit) {
		switch (oldrule[0])
		{
		case '-':
			SendDlgItemMessage(hwnd, IDC_DENY,BM_SETCHECK,true,0);
			break;
		case '+':
			SendDlgItemMessage(hwnd, IDC_ALLOW,BM_SETCHECK,true,0);
			break;
		case '?':
			SendDlgItemMessage(hwnd, IDC_QUERY,BM_SETCHECK,true,0);
			break;
			
		}
		char restOfString[1024]{};
		strcpy_s(restOfString, oldrule + 1);
		SetDlgItemText(hwnd, IDC_FIRST_IP, restOfString);
	}
	else {
		SetDlgItemText(hwnd, IDC_FIRST_IP, "");
		SetWindowText(hwnd, "UltraVNC Server - Add IPv4 Access Rule");
	}
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	return true;
}

void RulesListView::add()
{
	isEdit = false;
	DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_EDIT_IP_ACESS_CONTROL),
		NULL, (DLGPROC)dialogProc, (LONG_PTR)this);
}
void RulesListView::edit()
{
	int si = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
	oldindex = si;
	// If nothing selected
	if (si == -1) {
		return;
	}
	LVITEM lv1;
	lv1.mask = LVIF_TEXT;
	lv1.iItem = si;
	lv1.iSubItem = 2;
	char accessRule[256];
	lv1.pszText = (char*)&accessRule[0];
	lv1.cchTextMax = 256;
	ListView_GetItem(hListView, &lv1);

	strcpy_s(oldrule, accessRule);
	isEdit = true;
	DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_EDIT_IP_ACESS_CONTROL),
		NULL, (DLGPROC)dialogProc, (LONG_PTR)this);
}

void RulesListView::onOkButtonClick(HWND hwnd)
{
	SetDlgItemText(hwnd, IDC_ERROR_TEXT, "");
	char first[256];
	GetDlgItemText(hwnd, IDC_FIRST_IP, first, 25);
	char rule[256]{};

	if (SendMessage(GetDlgItem(hwnd, IDC_ALLOW), BM_GETCHECK, 0, 0) == BST_CHECKED)
		rule[0] = '+';

	else if (SendMessage(GetDlgItem(hwnd, IDC_DENY), BM_GETCHECK, 0, 0) == BST_CHECKED)
		rule[0] = '-';

	else if (SendMessage(GetDlgItem(hwnd, IDC_QUERY), BM_GETCHECK, 0, 0) == BST_CHECKED)
		rule[0] = '?';
	else {
		SetDlgItemText(hwnd, IDC_ERROR_TEXT, "No action selected");
		return;
	}

	
	strcat_s(rule, first);

	if (!isValidIPAddress(first)) {
		SetDlgItemText(hwnd, IDC_ERROR_TEXT, "Ip address is invalid");
		return;
	}

	if (isEdit)
		updateItem(oldindex, rule);
	else
		addItem(rule);
	
	DestroyWindow(hwnd);
}

bool RulesListView::onCommand(HWND hwnd, UINT cID, UINT nID)
{
	if (nID == BN_CLICKED) {
		switch (cID) {
		case IDOK:
			onOkButtonClick(hwnd);
			break;
		case IDCANCEL:
			DestroyWindow(hwnd);
			break;
		}
	}
	return TRUE;
}

bool RulesListView::isValidNumber(const char* str) {
	int len = strlen(str);
	if (len == 0 || len > 3) return false;
	for (int i = 0; i < len; ++i) {
		if (!isdigit(str[i])) return false;
	}
	int num = std::atoi(str);
	if (num < 0 || num > 255) return false;
	if (len > 1 && str[0] == '0') return false;
	return true;
}

bool RulesListView::isValidIPAddress(const char* ip) {
	char ipCopy[20];
	strncpy_s(ipCopy, ip, 19);
	ipCopy[19] = '\0';

	char* token = std::strtok(ipCopy, ".");
	int count = 0;

	while (token != nullptr) {
		// Check if the token (part between dots) is a valid number
		if (!isValidNumber(token)) {
			return false;
		}

		token = std::strtok(nullptr, ".");
		count++;
	}

	return true;
}

char* RulesListView::getAuthHost()
{
	int itemCount = ListView_GetItemCount(hListView);
	static char result[10000];
	result[0] = '\0';  // Clear buffer for each call
	for (int i = 0; i < itemCount; i++) {
		// Define a LVITEM structure to hold the item's data
		LVITEM lvItem;
		lvItem.iItem = i;           // Specify the item index
		lvItem.iSubItem = 2;        // If you want the first column (subitem)
		lvItem.mask = LVIF_TEXT;    // We want the text

		// Buffer to store the item's text
		char buffer[256];
		lvItem.pszText = buffer;
		lvItem.cchTextMax = sizeof(buffer);
		ListView_GetItem(hListView, &lvItem);
		if (i != 0)
			strcat_s(result, ":");
		strcat_s(result, lvItem.pszText);
	}
	return result;
}



