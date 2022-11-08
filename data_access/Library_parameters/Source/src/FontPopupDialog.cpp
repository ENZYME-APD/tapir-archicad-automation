// *****************************************************************************
// Source code for the FontPopUp Dialog in DG Test Add-On
// *****************************************************************************


// ---------------------------------- Includes ---------------------------------

#include	"FontPopupDialog.hpp"


#include	"APIEnvir.h"
#include	"ACAPinc.h"					// also includes APIdefs.h

#include	"MDIDs_APICD.h"

#include	"DGTEFontData.hpp"

#include    "DGTestResIDs.hpp"


// --- Class definition: FontPopUpDialog -------------------------------------------------

FontPopupDialog::FontPopupDialog () :
	DG::ModalDialog (ACAPI_GetOwnResModule(), DGTEST_FONTPOPUPDIALOG_RESID, ACAPI_GetOwnResModule()),
	UC::FontPopUpObserver (),
	fontPopUp		(GetReference (), FontPopUpID),
	richEdit		(GetReference (), RichEditID),
	boldCheck		(GetReference (), BoldCheckID),
	italicCheck		(GetReference (), ItalicCheckID),
	underlineCheck	(GetReference (), UnderlineCheckID),
	closeButton		(GetReference (), CloseButtonID),
	menu			("-"),
	restoreCommand	(Restore, MDID_APICD, MDID_APICD_DGTest),
	closeCommand	(Close, MDID_APICD, MDID_APICD_DGTest)
{
	fontPopUp.Attach (*this);
	boldCheck.Attach (*this);
	italicCheck.Attach (*this);
	underlineCheck.Attach (*this);
	closeButton.Attach (*this);
	Attach (*this);


	menu.AddMenuItem (DG::MenuSimpleItem (restoreCommand));
	menu.AddMenuItem (DG::MenuSeparatorItem ());
	menu.AddMenuItem (DG::MenuSimpleItem (closeCommand));

	GS::UniString buffer;
	RSGetIndString (&buffer, DGTEST_FONTPOPUPDIALOG_RESID, Restore, ACAPI_GetOwnResModule (), RST_Localised);
	commandTable.Add (restoreCommand, new DG::CommandDescriptor (restoreCommand, buffer));
	RSGetIndString (&buffer, DGTEST_FONTPOPUPDIALOG_RESID, Close, ACAPI_GetOwnResModule (), RST_Localised);
	commandTable.Add (closeCommand, new DG::CommandDescriptor (closeCommand, buffer));


	fontPopUp.SelectItem (TE::FontFamily ("Arial"));
	richEdit.SetText ("Hello World");
	UpdateFont ();
}


FontPopupDialog::~FontPopupDialog ()
{
	for (auto it = commandTable.EnumerateValues (); it != nullptr; ++it) {	// This clears the command table
		delete *it;
	}
}


void FontPopupDialog::CheckItemChanged (const DG::CheckItemChangeEvent& /*ev*/)
{
	UpdateFont ();
}

void FontPopupDialog::FontPopUpChanged (const UC::FontPopUpChangeEvent& /*ev*/)
{
	UpdateFont ();
}


void FontPopupDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (ev.GetSource () == &closeButton)
		PostCloseRequest (Cancel);
}


void FontPopupDialog::UpdateFont ()
{
	DGTEFontData data;
	if (!richEdit.GetFont (&data)) {
		return;
	};
	data.font.Set (fontPopUp.GetSelectedItem ());
	Int32 fontStyle =
		(boldCheck.IsChecked () ? TE::IFont::FontStyle::bold : 0) |
		(italicCheck.IsChecked () ? TE::IFont::FontStyle::italic : 0) |
		(underlineCheck.IsChecked () ? TE::IFont::FontStyle::underline : 0);

	data.font.SetStyle (fontStyle);
	richEdit.SetFont (&data);
}



void FontPopupDialog::PanelContextMenuRequested(const DG::PanelContextMenuEvent& ev, bool* needHelp, bool* processed)
{
	*needHelp = false;
	if (richEdit.GetNativeRectInScreenSpace ().Contains (ev.GetPosition ())) {
		DG::MousePosData mPos;
		mPos.Retrieve ();


		DG::ContextMenu contextMenu ("-", &menu);
		contextMenu.SetEnabledCommands (commandTable);

		DG::CommandEvent* commandEvent = contextMenu.Display (mPos.GetMouseOffsetInNativeUnits ());
		if (commandEvent != nullptr) {
			ULong cmd = commandEvent->GetCommand ().GetCommandId ();
			switch (cmd) {
			case Restore:
				fontPopUp.SelectItem (TE::FontFamily ("Arial"));
				richEdit.SetText ("Hello World #2");
				boldCheck.Uncheck ();
				italicCheck.Uncheck ();
				underlineCheck.Uncheck ();
				UpdateFont ();
				break;
			case Close:
				PostCloseRequest (Cancel);
				break;
			default:
				break;
			}
			*processed = true;
			delete commandEvent;
		}
	}
}

void FontPopupDialog::SetText(GS::UniString str1) {
	
	richEdit.SetText(str1);
	richEdit.SetFont(DG::Font::Large,DG::Font::Plain);
	//richEdit.HGrow

}
