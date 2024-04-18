//---------------------------------------------------------------------------

#ifndef TabsheetH
#define TabsheetH
#include <Vcl.BaseImageCollection.hpp>
#include <Vcl.Bind.DBEngExt.hpp>
#include <Vcl.Bind.Editors.hpp>
#include <Vcl.Bind.Grid.hpp>
#include <Vcl.Bind.Navigator.hpp>
#include <Vcl.Buttons.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ImageCollection.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include <Vcl.ImgList.hpp>
#include <Vcl.VirtualImage.hpp>
#include <Vcl.VirtualImageList.hpp>
#include <Vcl.WinXCalendars.hpp>
#include <Vcl.WinXCtrls.hpp>
#include <Vcl.TitleBarCtrls.hpp>
//---------------------------------------------------------------------------
enum WHAT {aspectration, nbrMonitord, setMonitor};

class TfraTopPanel;

class MyPanel : public TPanel {
private:
	void __fastcall WMCopyData(TMessage &Message);
protected:
	BEGIN_MESSAGE_MAP
		VCL_MESSAGE_HANDLER(WM_COPYDATA, TMessage, WMCopyData)
	END_MESSAGE_MAP(TPanel)
	//virtual void __fastcall Resize();
public:
	__fastcall MyPanel(TComponent* Owner) : TPanel(Owner) {}
	//void __fastcall Resize();
	double aspectRatio = 1.0;
	int nbrMonitors = 0;
};

void __fastcall MyPanel::WMCopyData(TMessage &Message) {
	COPYDATASTRUCT *data = reinterpret_cast<COPYDATASTRUCT*>(Message.LParam);
	switch(data->dwData) {
		case 0:
			aspectRatio = (*(int*)data->lpData) / 100.0;
			break;
		case 1:
			nbrMonitors =  (*(int*)data->lpData);
			break;
	}
}


class Tabsheet
{
private:
	std::auto_ptr<TPanel> toppanel;
	std::auto_ptr<TfraTopPanel> fraTopPanel;
	std::auto_ptr<TTabControl> customTabControl;
	std::auto_ptr<TPageControl> pageControl;
	String caption;
	static LRESULT CALLBACK PanelWndProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK PanelWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	void sendDataToVNC(HWND hwnd, int data, HWND Handle, int what);
	int oldWidth;
	int oldHeight;
public:		// User declarations
	Tabsheet(TComponent* owner, String classname, String caption, TPageControl *PageControl, bool left = false);
	~Tabsheet();
	std::auto_ptr<TTabSheet> tab;
	HWND hVNCWnd = NULL;
	std::auto_ptr<MyPanel> panel;
	void __fastcall PanelResize(TObject *Sender);
	void updateSize();

	void __fastcall Close();
	void __fastcall RemoveToolbar();
	void __fastcall TopToolbar();
	void __fastcall LeftToolbar();
	HWND  getVNCWnd() {return hVNCWnd;}
	void  SetVNCParent();
};
#endif
