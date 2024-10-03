#pragma once
#include "stdhdrs.h"

class RulesListView
{
private:
	int index;
	void addItem(char* caption);	
	void selectItem(int index);
	HWND hListView = NULL;

	bool isValidNumber(const char* str);
	bool isValidIPAddress(const char* ip);
	bool isEdit;
	char oldrule[256];
	int oldindex;
	
public:
	RulesListView();
	~RulesListView();
	void init(HWND hListView);
	void removeSelectedItem();
	void moveUp();
	void moveDown();
	void updateItem(int index, char* caption);
	void add();
	void edit();
	bool onInitDialog(HWND hwnd);
	bool onCommand(HWND hwnd, UINT cID, UINT nID);
	void onOkButtonClick(HWND hwnd);
	char* getAuthHost();
};

