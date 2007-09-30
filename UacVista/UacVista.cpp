// UacVista.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
void Copy_to_Secure_from_temp(char *c);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	Copy_to_Secure_from_temp(lpCmdLine);
	return 0;
}



