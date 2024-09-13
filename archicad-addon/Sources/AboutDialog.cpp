#include "AboutDialog.hpp"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"

#include "ACAPinc.h"

AboutDialog::AboutDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_ABOUT_DIALOG, ACAPI_GetOwnResModule ()),
    okButton (GetReference (), 1),
    versionText (GetReference (), 4)
{
    AttachToAllItems (*this);
    Attach (*this);

    GS::UniString versionTextContent = versionText.GetText ();
    versionText.SetText (GS::UniString::SPrintf (versionTextContent, GS::UniString (ADDON_VERSION).ToPrintf ()));
}

void AboutDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &okButton) {
        PostCloseRequest (DG::ModalDialog::Accept);
    }
}
