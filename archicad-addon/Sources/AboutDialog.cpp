#include "AboutDialog.hpp"

#include "AddOnVersion.hpp"
#include "AddOnBuildCommit.hpp"
#include "ResourceIds.hpp"
#include "MigrationHelper.hpp"

#include "ACAPinc.h"

AboutDialog::AboutDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_ABOUT_DIALOG, ACAPI_GetOwnResModule ()),
    okButton (GetReference (), 1),
    versionText (GetReference (), 4)
{
    AttachToAllItems (*this);
    Attach (*this);

    GS::UShort portNumber = 0;
    ACAPI_Command_GetHttpConnectionPort (&portNumber);

    // The commit tells a pull request's or a local build from the release of the same version.
    GS::UniString addOnVersion (ADDON_VERSION);
    const GS::UniString buildCommit (ADDON_BUILD_COMMIT);
    if (!buildCommit.IsEmpty ()) {
        addOnVersion += GS::UniString (" (") + buildCommit + ")";
    }

    GS::UniString versionTextContent = versionText.GetText ();
    GS::UniString versionTextNewContent = GS::UniString::SPrintf (
        versionTextContent,
        addOnVersion.ToPrintf (),
        GS::ValueToUniString (portNumber).ToPrintf ()
    );
    versionText.SetText (versionTextNewContent);
}

void AboutDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &okButton) {
        PostCloseRequest (DG::ModalDialog::Accept);
    }
}
