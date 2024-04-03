//---------------------------------------------------------------------------

#pragma hdrstop

#include "PollThread.h"
#include "CardList.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
void __fastcall PollThread::Execute()
{
	// Put your thread logic here
	Sleep(2000);
	while (running)
	{
		CardList::getInstance()->pollAlive();
		for (int i = 0; i < 100; i++) {
			Sleep(150);
			if (!running) return;
		}

	}
}

__fastcall PollThread::~PollThread()
{
	CardList::getInstance()->stopPoll();
	running = false;
}
