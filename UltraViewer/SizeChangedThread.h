//---------------------------------------------------------------------------

#ifndef SizeChangedThreadH
#define SizeChangedThreadH
#include <System.Classes.hpp>

//---------------------------------------------------------------------------
class SizeChangedThread : public TThread
{
private:
	void __fastcall Execute();

public:
	__fastcall SizeChangedThread(bool CreateSuspended) : TThread(CreateSuspended) {}
	__fastcall ~SizeChangedThread();
    bool running = true;
};
#endif
