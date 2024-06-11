//---------------------------------------------------------------------------

#pragma hdrstop

#include "SizeChangedThread.h"
#include "TabsheetList.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

void __fastcall SizeChangedThread::Execute()
{
	// Put your thread logic here
	while (running)
	{
		TabsheetList::getInstance()->updateSizes();
		Sleep(500); // Sleep for 1 second
	}
}

__fastcall SizeChangedThread::~SizeChangedThread()
{
 	running = false;
}
