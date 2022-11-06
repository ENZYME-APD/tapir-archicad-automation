// *****************************************************************************
// Source code for UserItemDialog, used by example Add-Ons to draw native images
// *****************************************************************************

#include	"APIEnvir.h"
#include	"ACAPinc.h"				// also includes APIdefs.h

#include	"UserItemDialog.hpp"

#include	"DGNativeContexts.hpp"

constexpr short BUTTON_HEIGHT = 20;
constexpr short MARGIN = 5;


UserItemDialog::UserItemDialog (short userItemCount,
								short userItemWidth,
								short userItemHeight,
								const NativeImageRendererFunc& rendererFunc):
	DG::ModalDialog (DG::NativePoint (),
					 userItemCount * userItemWidth + (userItemCount + 1) * MARGIN,
					 userItemHeight + BUTTON_HEIGHT + 3 * MARGIN,
					 GS::Guid ()),

	okButton		(GetReference (),
					 DG::Rect (MARGIN,
							   userItemHeight + 2 * MARGIN,
							   userItemCount * (userItemWidth + MARGIN),
							   userItemHeight + 2 * MARGIN + BUTTON_HEIGHT))
{
	for (short ii = 0; ii < userItemCount; ++ii) {
		const DG::Rect userItemRect (ii * userItemWidth + (ii + 1) * MARGIN,
									 MARGIN,
									 (ii + 1) * (userItemWidth + MARGIN),
									 MARGIN + userItemHeight);
		userItems.PushNew (new DG::UserItem (GetReference (), userItemRect));
		nativeImages.PushNew (rendererFunc (*userItems.GetLast (), ii));
	}

	AttachToAllItems (*this);

	okButton.SetText (L("OK"));
	ShowItems ();
}

UserItemDialog::~UserItemDialog ()
{
	DetachFromAllItems (*this);
}

void	UserItemDialog::ButtonClicked (const DG::ButtonClickEvent&)
{
	PostCloseRequest (Accept);
}

void	UserItemDialog::UserItemUpdate (const DG::UserItemUpdateEvent& ev)
{
	const UIndex index = GetUserItemIndex (ev.GetSource ());
	NewDisplay::UserItemUpdateNativeContext context (ev);
	context.DrawImage (nativeImages[index], 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, false);
}

UIndex	UserItemDialog::GetUserItemIndex (const DG::UserItem* userItem) const
{
	return userItems.FindFirst ([&] (const GS::Owner<DG::UserItem>& item) { return item == userItem; });
}
