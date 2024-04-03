//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "uProposal.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TProposalForm *ProposalForm;
//---------------------------------------------------------------------------
__fastcall TProposalForm::TProposalForm(TComponent* Owner)
	: TForm(Owner)
{
}


void __fastcall TProposalForm::CompleteButtonClick( TObject* Sender )
{
  Close();
}


void __fastcall TProposalForm::CloseButtonClick( TObject* Sender )
{
  Close();
}


void __fastcall TProposalForm::LoadProposal( int ALeadId, const String ADocument )
{
  FLeadId = ALeadId;
  LoadDocument( ADocument );
}


void __fastcall TProposalForm::LoadDocument( const String ADocument )
{
	if ( !( WebBrowser->Document != NULL ) )
	WebBrowser->Navigate( L"about:blank" );
	Variant LDoc = WebBrowser->Document;
	LDoc.OleProcedure(L"Clear");
	LDoc.OleFunction(L"Write", &WideString(ADocument) );
	LDoc.OleProcedure(L"Close");
}

//---------------------------------------------------------------------------
