// *****************************************************************************
// Header file for the FontPopUp Dialog in DG Test Add-On
// *****************************************************************************

#ifndef FONTPOPUP_HPP
#define FONTPOPUP_HPP


// ---------------------------------- Includes ---------------------------------

#include	"DGModule.hpp"
#include	"DGMenu.hpp"
#include	"DGCommandDescriptor.hpp"
#include	"UCModule.hpp"


// --- Class declaration: FontPopUpDialog -------------------------------------------------

class FontPopupDialog : public DG::ModalDialog,
						public DG::PanelObserver,
						public UC::FontPopUpObserver,
						public DG::CheckItemObserver,
						public DG::ButtonItemObserver
{
protected:
	enum Controls {
		FontPopUpID			= 1,
		RichEditID			= 2,
		BoldCheckID			= 3,
		ItalicCheckID		= 4,
		UnderlineCheckID	= 5,
		CloseButtonID		= 6
	};

	enum MenuStrings {
		Restore		= 1,
		Close		= 2
	};

	UC::FontPopUp	fontPopUp;
	DG::RichEdit	richEdit;
	DG::PushCheck	boldCheck;
	DG::PushCheck	italicCheck;
	DG::PushCheck	underlineCheck;
	DG::Button		closeButton;

	DG::Menu			menu;
	DG::Command			restoreCommand;
	DG::Command			closeCommand;
	DG::CommandTable	commandTable;


	virtual void	PanelContextMenuRequested	(const DG::PanelContextMenuEvent& ev, bool* needHelp, bool* processed) override;
	virtual void	CheckItemChanged			(const DG::CheckItemChangeEvent&)	override;
	virtual void	FontPopUpChanged			(const UC::FontPopUpChangeEvent&)	override;
	virtual void	ButtonClicked				(const DG::ButtonClickEvent& ev)	override;
	void			UpdateFont					();

public:
	FontPopupDialog ();
	void            SetText(GS::UniString);
	virtual ~FontPopupDialog ();
};


#endif // FONTPOPUP_HPP
