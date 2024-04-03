//---------------------------------------------------------------------------

#ifndef uProposalH
#define uProposalH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <SHDocVw.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.OleCtrls.hpp>
#include <mshtml.h>
//---------------------------------------------------------------------------
class TProposalForm : public TForm
{
__published:	// IDE-managed Components
	TWebBrowser *WebBrowser;
	TPanel *NavPanel;
	TButton *CompleteButton;
	TButton *CloseButton;
	void __fastcall CompleteButtonClick(TObject *Sender);
	void __fastcall CloseButtonClick(TObject *Sender);
private:	// User declarations
	int FLeadId;
	void __fastcall LoadDocument( const String ADocument );
public:		// User declarations
	void __fastcall LoadProposal( int ALeadId, const String ADocument );
	__fastcall TProposalForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TProposalForm *ProposalForm;
//---------------------------------------------------------------------------
#endif
