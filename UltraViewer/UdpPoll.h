//---------------------------------------------------------------------------

#ifndef UdpPollH
#define UdpPollH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdEchoUDP.hpp>
#include <IdUDPBase.hpp>
#include <IdUDPClient.hpp>
//---------------------------------------------------------------------------
class TDMUdpPoll : public TDataModule
{
__published:	// IDE-managed Components
	TIdEchoUDP *IdEchoUDP1;
private:	// User declarations
	static TDMUdpPoll* FInstance;
protected:
	__fastcall TDMUdpPoll(TComponent* Owner);
public:		// User declarations
	static TDMUdpPoll* getInstance();
	__fastcall virtual ~TDMUdpPoll();
	bool SendReceive(String localhost);
};
//---------------------------------------------------------------------------
extern PACKAGE TDMUdpPoll *DMUdpPoll;
//---------------------------------------------------------------------------
#endif
