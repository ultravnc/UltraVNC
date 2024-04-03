//---------------------------------------------------------------------------

#ifndef PollThreadH
#define PollThreadH
//---------------------------------------------------------------------------
class PollThread : public TThread
{
private:
	void __fastcall Execute();

public:
	__fastcall PollThread(bool CreateSuspended) : TThread(CreateSuspended) {}
	__fastcall ~PollThread();
	bool running = true;
};
#endif
