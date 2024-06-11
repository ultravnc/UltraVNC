//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "uMainForm.h"
#include "uAddCard.h"
#include "SettingsManager.h"
#include "CardList.h"
#include "Tabsheet.h"
#include "TabsheetList.h"
#include "DlgPasswordBox.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
	: TForm(Owner)
{
	SettingsManager sm;
	TStyleManager::SetStyle(sm.loadTheme());
	SplitView->Opened = false;
	if (sm.isEncryptionUsed()) {
		TPasswordDialog *dialog = new TPasswordDialog(this, 2);
		if (dialog->ShowModal() == mrOk) {
			globalPassword = dialog->getNewPassword();
			if (!sm.ispasswordCorrect(globalPassword))
				Close();
		}
		delete dialog;
	}

	sections = new TStringList();
	thread = new SizeChangedThread(false);
	pollThread = new PollThread(false);
	try
	{
		sm.getsections(sections);
		for (int i = 0; i < sections->Count; ++i)
		{
			if (sections->Strings[i] == "theme")
				continue;
			if (sections->Strings[i] == "UltraViewer")
				continue;

			if (sm.load(sections->Strings[i], globalPassword)) {
			TfrmCard *frmCard = new TfrmCard(NULL, this, globalPassword); // Create a new instance of your frame
			CardList::getInstance()->addToCardList(sections->Strings[i], frmCard);
			frmCard->Parent = FlowPanel1;
			frmCard->setCustumName(sections->Strings[i]);
			frmCard->copyCardSetting(sm.getCardSetting());
			frmCard->Show();
			}
		}
	}
	__finally
	{
		FlowPanel1->Realign();
	}
}

 __fastcall TMainForm::~TMainForm()
 {
	 delete sections;
	 delete thread;
     delete pollThread;
 }
//---------------------------------------------------------------------------
void __fastcall TMainForm::VCLStylesCBChange( TObject* Sender )
{
	if (!TabsheetList::getInstance()->isTabOpen()) {
		TStyleManager::SetStyle( VCLStylesCB->Text );
		SettingsManager sm;
		sm.saveTheme(VCLStylesCB->Text);
		TabsheetList::getInstance()->isTabOpen();
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::MenuVirtualImageClick( TObject* Sender )
{
  SplitView->Opened = !SplitView->Opened;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::DashboardButtonClick( TObject* Sender )
{
  PageControl->ActivePageIndex = ((TButton*)( Sender ))->Tag;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormResize( TObject* Sender )
{
  if ( MainForm->Width <= 640 )
  {
    if ( SplitView->Opened == true )
	  SplitView->Opened = false;
  }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender)
{
  String StyleName;
  for(String StyleName: TStyleManager::StyleNames) {
	VCLStylesCB->Items->Add( StyleName );
  }
  VCLStylesCB->ItemIndex = VCLStylesCB->Items->IndexOf( TStyleManager::ActiveStyle->Name );

}
//---------------------------------------------------------------------------
void __fastcall TMainForm::AddSessionClick(TObject *Sender)
{
   TAddCard *addCard(new TAddCard (NULL, globalPassword));
	int result = addCard->ShowModal();
	if (addCard->ModalResult == mrOk)
	{
		SettingsManager sm;
		String customname = addCard->getCustomName();
		if (!CardList::getInstance()->existInCardList(customname))
		{
			sm.load(customname, globalPassword);

			TfrmCard  *frmCard =new TfrmCard(NULL, this, globalPassword); // Create a new instance of your frame
			CardList::getInstance()->addToCardList(customname, frmCard);
			frmCard->Parent = FlowPanel1;
			frmCard->setCustumName(customname);
			frmCard->copyCardSetting(sm.getCardSetting());
			frmCard->Show();
			FlowPanel1->Realign();
		}
	}
	delete addCard;
}
 //---------------------------------------------------------------------------
 void TMainForm::addTab(String caption)
{
	if (!TabsheetList::getInstance()->existInTabsheetList(caption))
	{

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		SettingsManager sm;
		sm.load(caption, globalPassword);
		CardSetting * cs = sm.getCardSetting();

		String command = String("vncviewer.exe");
		if (cs->useRepeater) {
			command += (" -proxy" + cs->repeater + " " + cs->idNumber);
		}
		else {
			command += (" " + cs->host);
		}
		command += (" -password " + cs->password);
		command += (" -nostatus -directx -noborder -notoolbar -classname vnc_" + caption);
		if (!cs->user.IsEmpty()) {
			command += (" " + cs->host);
		command += (" -user " + cs->user);
		}

		if (cs->viewOnly) {
			command += (" -viewOnly");
		}

		if (cs->cursorType == 1) {
		   command += (" -nocursorshape");
		}
		else if (cs->cursorType == 3) {
		   command += (" -noremotecursor");
		}

		if (!cs->encoder.IsEmpty()) {
			command += (" -encoding " + cs->encoder);
		}

		if (!cs->user.IsEmpty()) {
			command += (" -user " + cs->user);
		}

		if (cs->disableClipboard) {
		   command += (" -disableclipboard");
		}

		if (!cs->notification.IsEmpty()) {
			command += (" -InfoMsg \""  + cs->notification + "\"");
		}

		if (cs->useEncryption) {
			command += (" -dsmplugin " +  cs->encryptionPlugin );
		}

		if (cs->alternativeKeybaord) {
		   command += (" -JapKeyboard");
		}

		// Start the child process
		if (CreateProcess(NULL,   // No module name (use command line)
			(LPWSTR)command.c_str(),   // Command line
			NULL,                  // Process handle not inheritable
			NULL,                  // Thread handle not inheritable
			FALSE,                 // Set handle inheritance to FALSE
			0,                     // No creation flags
			NULL,                  // Use parent's environment block
			NULL,                  // Use parent's starting directory
			&si,                   // Pointer to STARTUPINFO structure
			&pi))                  // Pointer to PROCESS_INFORMATION structure
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			String classname = "vnc_" +caption;
			Tabsheet *tabsheet = new Tabsheet(this, classname, caption, PageControl);
			TabsheetList::getInstance()->addToTabsheetList(caption, tabsheet);
		}
	}
}
//---------------------------------------------------------------------------


void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
	TabsheetList::getInstance()->closeAll();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::VCLStylesCBMouseEnter(TObject *Sender)
{
	VCLStylesCB->Enabled = (!TabsheetList::getInstance()->isTabOpen());}
//---------------------------------------------------------------------------
void __fastcall TMainForm::VCLStylesCBMouseLeave(TObject *Sender)
{
	VCLStylesCB->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::DashboardTabMouseEnter(TObject *Sender)
{
   //	SplitView->Opened = false;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::PageControlDrawTab(TCustomTabControl *Control, int TabIndex,
		  const TRect &Rect, bool Active)
{
	//Control->Canvas->FillRect(Rect);
	Control->Canvas->Brush->Style = bsClear;

	// Calculate the position to center the text horizontally
	int textWidth = Control->Canvas->TextWidth(PageControl->Pages[TabIndex]->Caption);
	int xPos = Rect.Left + (Rect.Width() - textWidth) / 2;
	int yPos = Rect.Top + 12; // Adjust vertical position as needed

	Control->Canvas->TextOut(xPos, yPos, PageControl->Pages[TabIndex]->Caption);

	if (PageControl->Pages[TabIndex]->Caption == "Sessions    ")  {
		Control->Canvas->Brush->Style = bsSolid;
		return;
	}

	 // Calculate the center of the circle
	int circleRadius = 12; // Adjust circle radius as needed
	ImageList1->Draw(Control->Canvas, Rect.Right - 32, (Rect.Top + (Rect.Height() -32) /2), 0 );

	Control->Canvas->Brush->Style = bsSolid;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::PageControlMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, int X, int Y)
{
	for (int i = 0; i < PageControl->PageCount; ++i)
	{
		TRect buttonRect = PageControl->TabRect(i);
		buttonRect.Left = buttonRect.Right - 32; // Adjust button position as needed
		if (buttonRect.Contains(Point(X, Y)))
		{
			selectedTab = i;
			TPoint point = PageControl->ClientToScreen(Point(X, Y));
			PopupMenu1->Popup(point.X, point.Y);
			break;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::close1Click(TObject *Sender)
{
	String caption = PageControl->Pages[selectedTab]->Caption;
	Tabsheet*  tab = TabsheetList::getInstance()->getTabsheet(caption.TrimRight());
	tab->Close();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::RemoveToolbarClick(TObject *Sender)
{
	String caption = PageControl->Pages[selectedTab]->Caption;
	Tabsheet*  tab = TabsheetList::getInstance()->getTabsheet(caption.TrimRight());
	tab->RemoveToolbar();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Top1Click(TObject *Sender)
{
	String caption = PageControl->Pages[selectedTab]->Caption;
	Tabsheet*  tab = TabsheetList::getInstance()->getTabsheet(caption.TrimRight());
	tab->TopToolbar();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Left1Click(TObject *Sender)
{
	String caption = PageControl->Pages[selectedTab]->Caption;
	Tabsheet*  tab = TabsheetList::getInstance()->getTabsheet(caption.TrimRight());
	tab->LeftToolbar();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::btnPaswordClick(TObject *Sender)
{
	if (globalPassword.IsEmpty()) {

		TPasswordDialog *dialog = new TPasswordDialog(this, 1);
		if (dialog->ShowModal() == mrOk) {
			globalPassword = dialog->getNewPassword();
			SettingsManager sm;
			sm.SettingsManager::AddPassword(globalPassword, "");
		}
	}
	else if (!globalPassword.IsEmpty()) {
		TPasswordDialog *dialog = new TPasswordDialog(this, 0);
		if (dialog->ShowModal() == mrOk) {
			String oldpassword = dialog->getOldPassword();
			if (oldpassword == globalPassword) {
				globalPassword = dialog->getNewPassword();
				SettingsManager sm;
				sm.SettingsManager::AddPassword(globalPassword, oldpassword);
			}
			else
                ShowMessage("Incorrect password");
		}
		delete dialog;
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::PageControlChange(TObject *Sender)
{
	String toCaption = PageControl->ActivePage->Caption;

	if (toCaption == "Sessions    ")  {
		TfrmCard *card = CardList::getInstance()->getCard(toCaption.TrimRight());
		if (oldVNCCard) {
			oldVNCCard->SetVNCParent(hOldVNCWnd);
		}
		
	}
	else if (fromsession){
		Tabsheet*  tab = TabsheetList::getInstance()->getTabsheet(toCaption.TrimRight());
		if(tab)
			tab->SetVNCParent();
			TfrmCard *card = CardList::getInstance()->getCard(toCaption.TrimRight());
		if (card) {
			card->UnSetVNCParent();
		}

	}

}
//---------------------------------------------------------------------------
void __fastcall TMainForm::PageControlChanging(TObject *Sender, bool &AllowChange)
{
	String fromCaption = PageControl->ActivePage->Caption;
	if (fromCaption != "Sessions    "){
		Tabsheet*  tab = TabsheetList::getInstance()->getTabsheet(fromCaption.TrimRight());
        if(tab)
			hOldVNCWnd =  tab->getVNCWnd();
		oldVNCCard = CardList::getInstance()->getCard(fromCaption.TrimRight());
		fromsession = false;
	}
	if (fromCaption == "Sessions    ")
		fromsession = true;
}
//---------------------------------------------------------------------------

