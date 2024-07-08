//---------------------------------------------------------------------------


#pragma hdrstop

#include "UdpPoll.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma classgroup "Vcl.Controls.TControl"
#pragma resource "*.dfm"
TDMUdpPoll *DMUdpPoll = nullptr;
TDMUdpPoll *TDMUdpPoll::FInstance = nullptr;
//---------------------------------------------------------------------------
__fastcall TDMUdpPoll::TDMUdpPoll(TComponent* Owner)
	: TDataModule(Owner)
{
}
//---------------------------------------------------------------------------
TDMUdpPoll* TDMUdpPoll::getInstance()
{
	if (!FInstance)
		FInstance = new TDMUdpPoll(NULL);
	return FInstance;
}
//---------------------------------------------------------------------------
__fastcall TDMUdpPoll::~TDMUdpPoll()
{
	// Clean-up code here
	FInstance = nullptr;
}
//---------------------------------------------------------------------------
bool TDMUdpPoll::SendReceive(String localhost)
{
	String host;
	int port;
	int colonPos = localhost.Pos(":");
    if (colonPos > 0) // If ':' is found
	{
        // Extract the host part
		host = localhost.SubString(1, colonPos - 1);
		port = StrToIntDef(localhost.SubString(colonPos + 1, localhost.Length() - colonPos), 0);
	}
    else
    {
        // No ':' found, handle error or default behavior here
		// For example, set default host and port
		host = localhost;
		port = 5900;
	}
    // Send a message to the echo server
	IdEchoUDP1->Host = host; // Change to the IP address or hostname of the echo server
	IdEchoUDP1->Port = port; // Default echo port
	IdEchoUDP1->Send(L"UVNC_alive_check");
	bool returnvalue = false;
	try
	{
		try {
			// Wait for the response
			String Response = IdEchoUDP1->ReceiveString(1000); // Timeout in milliseconds
			if (Response == L"UVNC_alive_check") {
			return true;
			}
		} catch (...) {

		}
	}
	__finally
	{
		returnvalue = false;
	}
	return returnvalue;
}
