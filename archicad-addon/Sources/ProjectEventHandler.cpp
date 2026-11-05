#include "ProjectEventHandler.hpp"
#include "ElementChangeTrackingCommands.hpp"
#include "NotificationCommands.hpp"
#include "MigrationHelper.hpp"

static GSErrCode ProjectEventHandler (API_NotifyEventID notifID, Int32 param)
{
    switch (notifID) {
        case APINotify_New:
        case APINotify_Open:
        case APINotify_Close:
        case APINotify_Quit:
            // The element states of CaptureElementState belong to the open project.
            ReleaseAllElementStates ();
            break;
        default:
            break;
    }

    // Does nothing unless a notification client asked for this event.
    AddElementNotificationClientCommand::ProjectEventHandler (notifID, param);

    return NoError;
}

GSErrCode InstallProjectEventHandler ()
{
    // Installed once with every event any part needs, rather than re-registered when a
    // notification client is added or removed: the handler filters the events itself, and the
    // events only the notification clients need (window and floor changes, Teamwork send and
    // receive) cost no more than a check of the registered clients.
    return ACAPI_ProjectOperation_CatchProjectEvent (
        APINotify_New | APINotify_NewAndReset | APINotify_Open | APINotify_Close | APINotify_Quit |
        APINotify_SendChanges | APINotify_ReceiveChanges |
        APINotify_ChangeWindow | APINotify_ChangeFloor,
        ProjectEventHandler);
}
