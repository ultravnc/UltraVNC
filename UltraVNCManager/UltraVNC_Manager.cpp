//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
#include <Vcl.Styles.hpp>
#include <Vcl.Themes.hpp>
USEFORM("UdpPoll.cpp", DMUdpPoll); /* TDataModule: File Type */
USEFORM("uAddCard.cpp", AddCard);
USEFORM("TopBarPanel.cpp", fraTopPanel); /* TFrame: File Type */
USEFORM("uMainForm.cpp", MainForm);
USEFORM("uFrmCard.cpp", frmCard); /* TFrame: File Type */
USEFORM("DlgPasswordBox.cpp", PasswordDialog);
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		TStyleManager::TrySetStyle("Windows10 Blue");
		Application->CreateForm(__classid(TMainForm), &MainForm);
		Application->CreateForm(__classid(TDMUdpPoll), &DMUdpPoll);
		Application->CreateForm(__classid(TPasswordDialog), &PasswordDialog);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
