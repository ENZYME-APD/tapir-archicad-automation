#include "APIEnvir.h"
#include "ACAPinc.h"

#include "DGModule.hpp"
#include "DGFolderDialog.hpp"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "DeveloperTools.hpp"

#include "3DCutPlaneCommands.hpp"
#include "3DProjectionCommands.hpp"
#include "DocumentFrom3DDefaultsCommands.hpp"
#include "3DStyleCommands.hpp"
#include "3DDocumentCommands.hpp"



#include "AboutDialog.hpp"
#include "TapirPalette.hpp"
#include "ScriptUIPalette.hpp"
#include "VersionChecker.hpp"

#include "ApplicationCommands.hpp"
#include "ScriptUICommands.hpp"
#include "DialogCommands.hpp"
#include "ProjectCommands.hpp"
#include "NameResolutionCommands.hpp"
#include "CustomDataCommands.hpp"
#include "ElementCommands.hpp"
#include "ElementChangeTrackingCommands.hpp"
#include "ProjectEventHandler.hpp"
#include "ElementCategoryCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "ElementURLReferenceCommands.hpp"
#include "ElementCreationCommands.hpp"
#include "ExtendedElementCommands.hpp"
#include "Model3DCommands.hpp"
#include "ElementGroupingCommands.hpp"
#include "RenderingCommands.hpp"
#include "AttributeCommands.hpp"
#include "TeamworkCommands.hpp"
#include "IssueCommands.hpp"
#include "LibraryCommands.hpp"
#include "LibraryPartCommands.hpp"
#include "LibraryPartSectionCommands.hpp"
#include "PropertyCommands.hpp"
#include "ClassificationCommands.hpp"
#include "FavoritesCommands.hpp"
#include "MigrationHelper.hpp"
#include "NavigatorCommands.hpp"
#include "DocumentCreationCommands.hpp"
#include "RevisionCommands.hpp"
#include "NotificationCommands.hpp"
#include "DesignOptionCommands.hpp"
#include "IFCCommands.hpp"
#include "SolidElementOperationCommands.hpp"
#include "MEPCommands.hpp"
#include "MEPRoutingParameterCommands.hpp"
#include "MEPPreferenceTableRowCommands.hpp"
#include "AnalyticalCommands.hpp"
#include "KeynoteCommands.hpp"
#include "ProjectDataCommands.hpp"
#include "GraphicOverrideCommands.hpp"
#include "GridElementCommands.hpp"
#include "PropertyObjectCommands.hpp"
#include "GraphicalOverrideCommands.hpp"
#include "PolygonCommands.hpp"
#include "ViewAndPlaceSettingsCommands.hpp"
#include "DrawingPrimitiveCommands.hpp"
#include "LibraryPartCreationCommands.hpp"

template <typename CommandType>
GSErrCode RegisterCommand (CommandGroup& group, const GS::UniString& version, const GS::UniString& description)
{
    GS::Owner<CommandType> command = GS::NewOwned<CommandType> ();
    const GS::String commandName = command->GetName ();
    group.commands.push_back (CommandInfo (
        commandName,
        description,
        version,
        command->GetInputParametersSchema (),
        command->GetRawResponseSchema ())
    );

    GSErrCode err = ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (command.Pass ());
    if (err != NoError) {
        // Initialize only ORs the results together, so without this line nothing tells which
        // command failed. A failed registration can make Archicad crash later, at the first
        // command call through the JSON interface, far from its cause.
        ACAPI_WriteReport (RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_COMMAND_NOT_REGISTERED, ACAPI_GetOwnResModule ()), false, commandName.ToCStr (), static_cast<int> (err));
        return err;
    }
    return NoError;
}

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
    switch (menuParams->menuItemRef.menuResID) {
        case ID_ADDON_MENU:
            switch (menuParams->menuItemRef.itemIndex) {
                case ID_ADDON_MENU_ABOUT:
                    {
                        AboutDialog aboutDialog;
                        aboutDialog.Invoke ();
                    }
                    break;
            }
            break;
        case ID_ADDON_MENU_FOR_PALETTE:
            switch (menuParams->menuItemRef.itemIndex) {
                case ID_ADDON_MENU_PALETTE:
                    {
                        if (TapirPalette::HasInstance () && TapirPalette::Instance ().IsVisible ()) {
                            TapirPalette::Instance ().Hide ();
                        } else {
                            TapirPalette::Instance ().Show ();
                        }
                    }
                    break;
            }
            break;
        case ID_ADDON_MENU_FOR_UPDATE:
            switch (menuParams->menuItemRef.itemIndex) {
                case ID_ADDON_MENU_UPDATE:
                    // The same code as the palette's update button.
                    TapirPalette::Instance ().CheckForUpdates ();
                    break;
            }
            break;
        case ID_ADDON_MENU_FOR_SHORTCUTS:
            switch (menuParams->menuItemRef.itemIndex) {
                case ID_ADDON_MENU_MANAGE_SHORTCUTS:
                    TapirPalette::Instance ().OpenShortcutsDialog ();
                    break;
            }
            break;
        // Each shortcut slot is its own top-level menu group (see ResourceIds.hpp) —
        // RunShortcutSlot takes a 0-based slot index.
        case ID_ADDON_MENU_RUNSCRIPT_1: TapirPalette::Instance ().RunShortcutSlot (0); break;
        case ID_ADDON_MENU_RUNSCRIPT_2: TapirPalette::Instance ().RunShortcutSlot (1); break;
        case ID_ADDON_MENU_RUNSCRIPT_3: TapirPalette::Instance ().RunShortcutSlot (2); break;
        case ID_ADDON_MENU_RUNSCRIPT_4: TapirPalette::Instance ().RunShortcutSlot (3); break;
        case ID_ADDON_MENU_RUNSCRIPT_5: TapirPalette::Instance ().RunShortcutSlot (4); break;
        case ID_ADDON_MENU_RUNSCRIPT_6: TapirPalette::Instance ().RunShortcutSlot (5); break;
    }

    return NoError;
}

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, ID_ADDON_INFO_NAME, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, ID_ADDON_INFO_DESC, ACAPI_GetOwnResModule ());
    envir->addOnInfo.description += GS::UniString (" ") + ADDON_VERSION;
    // Only records the Archicad version: the GitHub lookup is started from Initialize, because CheckEnvironment
    // also runs when Archicad merely lists the Add-Ons and unloads the Add-On right after.
    VersionChecker::CreateInstance (envir->serverInfo.mainVersion);
    return APIAddon_Preload;
}

GSErrCode RegisterInterface (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_FOR_PALETTE, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_FOR_UPDATE, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_FOR_SHORTCUTS, 0, MenuCode_UserDef, MenuFlag_SeparatorBefore);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_RUNSCRIPT_1, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_RUNSCRIPT_2, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_RUNSCRIPT_3, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_RUNSCRIPT_4, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_RUNSCRIPT_5, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_RUNSCRIPT_6, 0, MenuCode_UserDef, MenuFlag_Default);

    // So that 'Enable all informational messages' also re-enables ShowNotificationBubble's bubble
    // after its redrawCountLimit or disabled input turned it off.
    RegisterTapirNotificationBubble ();

    return err;
}

GSErrCode Initialize (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_FOR_PALETTE, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_FOR_UPDATE, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_FOR_SHORTCUTS, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_1, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_2, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_3, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_4, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_5, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_6, MenuCommandHandler);
    err |= TapirPalette::RegisterPaletteControlCallBack ();
    err |= ScriptUIPalette::RegisterPaletteControlCallBack ();
    // The add-on's one project event handler: drops the element states of CaptureElementState when
    // the project closes and passes window, floor and project events to the notification clients.
    err |= InstallProjectEventHandler ();

    // The Add-On is preloaded, so this runs before Archicad opens the first project.
    ApplyDialogAutoCloseFromEnvironment ();

    { // Application Commands
        CommandGroup applicationCommands ("Application Commands");
        err |= RegisterCommand<GetAddOnVersionCommand> (
            applicationCommands, "0.1.0",
            "Retrieves the version of the Tapir Additional JSON Commands Add-On."
        );
        err |= RegisterCommand<GetCommandsCommand> (
            applicationCommands, "2.0.0",
            "Retrieves every command of the Tapir Add-On with its command group, the Tapir version it was introduced in and its description, so a script can check that a command exists before calling it."
        );
        err |= RegisterCommand<GetArchicadLocationCommand> (
            applicationCommands, "0.1.0",
            "Retrieves the location of the currently running Archicad executable."
        );
        err |= RegisterCommand<GetArchicadReleaseInfoCommand> (
            applicationCommands, "1.7.3",
            "Retrieves the main version, release version and build number of the running Archicad, with the caption of its window."
        );
        err |= RegisterCommand<GetArchicadLicenseInfoCommand> (
            applicationCommands, "2.0.0",
            "Retrieves the license the running Archicad runs on: its type (Full, Education, Trial, Demo, Start or BIMcloud Team Client), the raw protection mode with its bits by name, and the highest Software Service Agreement (SSA) level of the license, so a script can warn before it uses a feature the license lacks. The SSA level is not available in Archicad 25."
        );
        err |= RegisterCommand<GetPrinterInfoCommand> (
            applicationCommands, "1.7.3",
            "Retrieves Archicad's current print settings: the print area, grid, fixed text size and scale (the settings PrintView takes)."
        );
        err |= RegisterCommand<GetPrinterAndPlotterInfoCommand> (
            applicationCommands, "1.8.0",
            "Retrieves the printer selected in Archicad, with its name and orientation, and the selected plotter, with its name and model."
        );
        err |= RegisterCommand<QuitArchicadCommand> (
            applicationCommands, "0.1.0",
            "Performs a quit operation on the currently running Archicad instance."
        );
        err |= RegisterCommand<GetPointFromUserCommand> (
            applicationCommands, "1.5.9",
            "Asks the designer to click a point in the current window and returns it. Archicad waits for the click or for Escape, and every other JSON command queues behind this one until then; the call fails when the input is cancelled."
        );
        err |= RegisterCommand<ModelToScreenCommand> (
            applicationCommands, "1.8.0",
            "Converts model coordinates to pixel positions local to the current Archicad view window (not global desktop coordinates), using the current window's size, drawing scale and zoom. In a 3D window the coordinates are projected with the current camera; in other windows z is disregarded. Available from Archicad 29."
        );
        err |= RegisterCommand<ScreenToModelCommand> (
            applicationCommands, "1.8.0",
            "Converts pixel positions local to the current Archicad view window to 2D model coordinates of the current database, using the current window's size, drawing scale and zoom. It is a 2D mapping: it does not reconstruct a 3D point, and several model coordinates can map to the same pixel."
        );
        err |= RegisterCommand<GetElementFromUserCommand> (
            applicationCommands, "1.7.3",
            "Asks the designer to click an element in the current window and returns its id, its type and the clicked point. A click on empty space or on an element of a type not asked for asks again. Archicad waits for the click or for Escape, and every other JSON command queues behind this one until then; cancelling returns cancelled."
        );
        err |= RegisterCommand<GetCurrentToolCommand> (
            applicationCommands, "2.0.0",
            "Returns the tool active in the Toolbox: its element type, its variation (the subtype, e.g. Door, Skylight or a library part subtype of an add-on) and, for the tools of add-ons, its class id, in the form SetCurrentTool accepts. Nothing is returned when the active tool makes no element, e.g. the Arrow or the Marquee."
        );
        err |= RegisterCommand<SetCurrentToolCommand> (
            applicationCommands, "2.0.0",
            "Activates a tool in the Toolbox, given as GetCurrentTool or GetToolboxItems returns it; an empty input activates the Arrow. Archicad switches the tool once it processes its events, so GetCurrentTool may still return the previous tool right after this command."
        );
        err |= RegisterCommand<GetToolboxItemsCommand> (
            applicationCommands, "2.0.0",
            "Lists the tools of the Toolbox, the tools of add-ons included, in the form SetCurrentTool accepts, and the active tool. includeHidden also lists the tools without an icon, e.g. the Slab Accessory."
        );
        err |= RegisterCommand<GetLineFromUserCommand> (
            applicationCommands, "1.8.0",
            "Waits for the user to input a line in the active Archicad window and returns start and end coordinates."
        );
        err |= RegisterCommand<GetArcFromUserCommand> (
            applicationCommands, "2.0.0",
            "Asks the designer to draw an arc in the current window - its center, its start point (which sets the radius) and its end point - with a rubber-band circle shown while drawing, and returns its center, radius, start and end angles, the angle it turns through, its direction and its start and end points. Archicad waits for the clicks or for Escape, and every other JSON command queues behind this one until then; cancelling returns cancelled."
        );
        err |= RegisterCommand<GetPolygonFromUserCommand> (
            applicationCommands, "1.8.0",
            "Waits for the user to input a polygon in the active Archicad window and returns its points and arcs."
        );
        err |= RegisterCommand<GetMagicWandSettingsCommand> (
            applicationCommands, "2.0.0",
            "Returns the settings of Archicad's Magic Wand Settings dialog: how curves are approximated with straight segments (a number of segments along arcs or circles, a largest deviation from the curve, or a segment length) and the trace method. Useful to know how the Magic Wand will trace a curve. Changes nothing in the project."
        );
        err |= RegisterCommand<ConvertImagesCommand> (
            applicationCommands, "2.0.0",
            "Converts base64 encoded images from one image format to another with Archicad's own image converters, for example a PNG preview image to JPEG. The formats are Bitmap, GIF, TIFF, JPEG and PNG, or the MIME type of another format Archicad can read or write. The output is aligned with the input array of images. Changes nothing in the project."
        );
        err |= RegisterCommand<GetCurrentWindowTypeCommand> (
            applicationCommands, "1.0.7",
            "Returns the type of the current (active) window and, when it has one, the databaseId of the database it shows, which ChangeWindow takes to bring the window back after a script switched to another one. The saved view (View Map item) the window was opened from is not returned, as Archicad does not tell which one it was."
        );
        err |= RegisterCommand<GetCurrentWindowDrawingScaleCommand> (
            applicationCommands, "1.7.3",
            "Returns the drawing scale of the current (active) window, the scale shown in the bottom bar, as its denominator: 100.0 for 1:100, 20.0 for 1:20. Unlike GetViewSettings it needs no saved view. It reads the scale of the current database: the floor plan, a section, elevation, interior elevation, detail, worksheet or 3D document window, each of which has its own. Layouts always return 1.0."
        );
        err |= RegisterCommand<SetCurrentWindowDrawingScaleCommand> (
            applicationCommands, "1.7.3",
            "Changes the drawing scale of the current (active) window, the scale shown in the bottom bar, to the given denominator: 100 for 1:100. It changes the scale of the current database: the floor plan, a section, elevation, interior elevation, detail, worksheet or 3D document window, each of which has its own. Layouts are always 1:1 and are rejected. The scale must be between 1 and 30000. The change is undoable."
        );
        err |= RegisterCommand<ChangeWindowCommand> (
            applicationCommands, "1.3.1",
            "Changes the current (active) window to the given window. A databaseId on its own is enough; the window type is taken from the database. The databaseId of a worksheet or detail whose window was never opened opens its Project Map item instead (from Archicad 27). With windowType 'FloorPlan' and a storyIndex it also activates that story on the floor plan. windowType 'Report' on its own opens the Session Report window (from Archicad 26); its text cannot be read through the Archicad API."
        );
        err |= RegisterCommand<GetContainingDatabasesCommand> (
            applicationCommands, "1.7.3",
            "Returns the database (window type, database id and name) that contains each of the given elements, from any database, not only the current one. Pass the windowType and databaseId to ChangeWindow to open it, for example to find an element placed on a layout."
        );
        err |= RegisterCommand<CloseWindowsCommand> (
            applicationCommands, "1.8.0",
            "Closes the windows of the given databases (for example layouts, sections or details opened earlier with ChangeWindow), with one execution result per window; a window Archicad cannot close gets a failed result. Requires Archicad 27 or later."
        );
        err |= RegisterCommand<RebuildCurrentDatabaseCommand> (
            applicationCommands, "1.8.0",
            "Rebuilds the current database, the one the API works on (see ChangeWindow). Requires Archicad 27 or later."
        );
        err |= RegisterCommand<DeleteGhostRecordsCommand> (
            applicationCommands, "1.8.0",
            "Deletes the trace (ghost) record of each given parent database, which removes the trace reference shown in that database's window. A database without a trace reference gets a failed execution result. Not undoable."
        );
        err |= RegisterCommand<GetUserGSIDCommand> (
            applicationCommands, "1.5.6",
            "Get the current registered User-GSID and OrganizationsID. Requires Archicad 27 or later."
        );
        err |= RegisterCommand<ShowAlertCommand>(
            applicationCommands,"1.5.6",
            "Display a dialog with up to three buttons."
        );
        err |= RegisterCommand<ShowNotificationBubbleCommand> (
            applicationCommands, "2.0.0",
            "Shows a non-blocking notification bubble with a message in the current Archicad window, e.g. to report progress or a result without stopping the user as ShowAlert does. The bubble disappears by itself after the timeout. It is shown at a model coordinate or a pixel position of the current view, by default at the center of the current 2D view. Optionally it gets a large icon, is shown at most a given number of times, or is turned off, until the user runs 'Enable all informational messages'. Returns the bubble's settings read back from Archicad. Requires Archicad 29 or later."
        );
        err |= RegisterCommand<GetSpecialFoldersCommand> (
            applicationCommands, "1.5.6",
            "Retrieves the filesystem paths of the special folders of the running Archicad (preferences, cache, data, temporary, application, defaults, templates, help, embedded project library, etc.)."
        );
        err |= RegisterCommand<GetEditModeStateCommand> (
            applicationCommands, "1.8.0",
            "Tells whether Archicad is in a hierarchical editing mode (curtain wall, stair, beam or column editing), and which element is being edited. Read only: it never enters, leaves or cancels an editing mode. Commands that modify the project can fail while such a mode is active. The overall inEditMode flag requires Archicad 29 or later; the command requires Archicad 27 or later."
        );
        err |= RegisterCommand<GetArchicadVisibilityCommand> (
            applicationCommands, "1.8.0",
            "Returns whether the Archicad application is shown or hidden. Supported on Windows only; on macOS it returns an error."
        );
        err |= RegisterCommand<SetArchicadVisibilityCommand> (
            applicationCommands, "1.8.0",
            "Hides or shows the Archicad application, e.g. to keep it out of the way while a batch runs. A script that hides Archicad must show it again, also when the script fails (e.g. in a finally block). Supported on Windows only; on macOS it returns an error."
        );
        err |= RegisterCommand<RunActionCenterChecksCommand> (
            applicationCommands, "1.8.0",
            "Runs the given checks of the Action Center (by default the hotlink check). A successful result means the check ran, not that nothing was found: Archicad does not report the outcome to the caller, it shows the Action Center tab when the check found an issue."
        );
        err |= RegisterCommand<RedrawCurrentDatabaseCommand> (
            applicationCommands, "1.8.0",
            "Redraws the current database, even when its window is in the background. Only the visible region of its window is updated."
        );
        err |= RegisterCommand<GetProductVersionInfoCommand> (
            applicationCommands, "1.7.3",
            "Retrieves the version information of the running Archicad: build number, language code, product family and version text, plus the product name, product type, main/update/hotfix versions, the available domains (Architecture, MEP) and the CEIP project identifier of the open project from Archicad 29. Requires Archicad 28 or later."
        );
        err |= RegisterCommand<Get3DCutawayStatusCommand> (
            applicationCommands, "1.7.3",
            "Returns whether 3D Cutaway is on and whether the cutting planes are shown in the 3D window. In Archicad 25-27 the 3D window may need to be open."
        );
        err |= RegisterCommand<Set3DCutawayStatusCommand> (
            applicationCommands, "1.7.3",
            "Turns 3D Cutaway on or off and shows or hides the cutting planes in the 3D window. In Archicad 25-27 the 3D window may need to be open."
        );
        err |= RegisterCommand<SetDialogAutoCloseCommand> (
            applicationCommands, "1.7.3",
            "Turns the dialog auto close on or off. While it is on, Archicad closes the dialogs it would show with their default button instead of waiting for the user, so automation is not blocked by them. To have it on already while Archicad opens a project (before any command can be sent), set the TAPIR_DIALOG_AUTOCLOSE environment variable to true for the Archicad process before starting it."
        );
        err |= RegisterCommand<GetDialogAutoCloseCommand> (
            applicationCommands, "1.8.0",
            "Tells whether the dialog auto close is on, the same state SetDialogAutoClose writes, so a script can restore it afterwards. The state belongs to the running Archicad process: it is off when Archicad starts (unless the TAPIR_DIALOG_AUTOCLOSE environment variable is true), it stays as last set until Archicad quits, and it is shared by every add-on and script in that Archicad."
        );

        AddCommandGroup (applicationCommands);
    }

    { // Project Commands
        CommandGroup projectCommands ("Project Commands");
        err |= RegisterCommand<GetProjectInfoCommand> (
            projectCommands, "0.1.0",
            "Retrieves information about the currently loaded project."
        );
        err |= RegisterCommand<GetProjectInfoFieldsCommand> (
            projectCommands, "0.1.2",
            "Retrieves the names and values of all project info fields."
        );
        err |= RegisterCommand<SetProjectInfoFieldCommand> (
            projectCommands, "0.1.2",
            "Sets the value of a project info field."
        );
        err |= RegisterCommand<CreateProjectInfoFieldsCommand> (
            projectCommands, "1.5.2",
            "Creates one or more custom project info fields. From Archicad 30 the Project Info group of each field can be given; without it the field goes to the first group."
        );
        err |= RegisterCommand<DeleteProjectInfoFieldsCommand> (
            projectCommands, "1.5.4",
            "Deletes one or more custom project info fields, by their autotext keys or, from Archicad 30, by the ids of the fields of the Project Info groups. Hardcoded fields cannot be deleted."
        );
        err |= RegisterCommand<RenameProjectInfoFieldsCommand> (
            projectCommands, "2.0.0",
            "Renames custom fields of the Project Info groups. Fixed (built-in) fields cannot be renamed. Available from Archicad 30."
        );
        err |= RegisterCommand<MoveProjectInfoFieldsCommand> (
            projectCommands, "2.0.0",
            "Moves fields of the Project Info groups to a group, before another field of it or to its end. A subfield of a calculated field moves with its parent field. Available from Archicad 30."
        );
        err |= RegisterCommand<GetProjectInfoGroupsCommand> (
            projectCommands, "1.8.0",
            "Retrieves the Project Info groups in their order, each with its fields: id, name, value and, when exactly one project info autotext has the name of the field, the autotext key that SetProjectInfoField takes. Available from Archicad 30."
        );
        err |= RegisterCommand<CreateProjectInfoGroupsCommand> (
            projectCommands, "1.8.0",
            "Creates Project Info groups with the given names, at the end of the group list. Available from Archicad 30."
        );
        err |= RegisterCommand<RenameProjectInfoGroupsCommand> (
            projectCommands, "1.8.0",
            "Renames Project Info groups. Available from Archicad 30."
        );
        err |= RegisterCommand<DeleteProjectInfoGroupsCommand> (
            projectCommands, "1.8.0",
            "Deletes Project Info groups. A group that contains fixed (built-in) fields cannot be deleted. Available from Archicad 30."
        );
        err |= RegisterCommand<MoveProjectInfoGroupsCommand> (
            projectCommands, "1.8.0",
            "Moves Project Info groups before another group, or to the end of the group list when no other group is given. Available from Archicad 30."
        );
        err |= RegisterCommand<GetStoriesCommand> (
            projectCommands, "1.1.5",
            "Retrieves information about the story structure of the currently loaded project: index, floor id, name, level, height and, from Archicad 28, the IFC GlobalId of each story."
        );
        err |= RegisterCommand<SetStoriesCommand> (
            projectCommands, "1.1.5",
            "Sets the story sructure of the currently loaded project."
        );
        err |= RegisterCommand<GetAutoTextKeysCommand> (
            projectCommands, "1.5.9",
            "Retrieves the available autotext keys (name and embeddable key): the Project Info, layout, drawing, date and other autotexts Archicad lists, and the property based ones, optionally for a specific element. Embed a key in a Text or Label content by surrounding it with '<' and '>'."
        );
        err |= RegisterCommand<GetAutoTextNameCommand> (
            projectCommands, "1.5.9",
            "Retrieves the display names of one or more autotext keys (as returned inside a '<...>' embedded key), with a direct guid lookup for property-based keys instead of enumerating every property definition."
        );
        err |= RegisterCommand<GetAllAutoTextKeysCommand> (
            projectCommands, "2.0.0",
            "Retrieves every autotext key Archicad knows (not only the property-based ones returned by GetAutoTextKeys) with its current value, resolved without an element context."
        );
        err |= RegisterCommand<InterpretAutoTextsCommand> (
            projectCommands, "2.0.0",
            "Resolves one or more autotexts (e.g. '<LAYOUTID>') to their values, optionally in the context of an element, so a key can be tested before it is written into a Text or Label."
        );
        err |= RegisterCommand<GetNoteAutotextsCommand> (
            projectCommands, "1.8.0",
            "Retrieves the autotexts available in the notes of an element type (e.g. a Dimension), each with the element types it is available for. The autotexts are identified by the numeric value of their API_NoteAutotextID. For a Dimension, the optional dimensionAppearance (Linear by default) chooses the dimension type. Needs Archicad 26 or later."
        );
        err |= RegisterCommand<GetNoteAutoTextKeywordsCommand> (
            projectCommands, "1.7.3",
            "Retrieves the keywords of note autotexts (the autotexts of dimension, label and other element notes) by their numeric API_NoteAutotextID values. Available from Archicad 26."
        );
        err |= RegisterCommand<GetHotlinksCommand> (
            projectCommands, "0.1.0",
            "Gets the file system locations (path) of the hotlink modules and the instance elements placed from each node. The hotlinks can have tree hierarchy in the project."
        );
        err |= RegisterCommand<CreateHotlinkNodesCommand> (
            projectCommands, "1.5.9",
            "Creates hotlink module nodes from source files. A node that already points at the same file is returned instead of duplicated (Archicad 26 and later; 25 cannot see an unplaced node)."
        );
        err |= RegisterCommand<CreateHotlinkInstancesCommand> (
            projectCommands, "1.5.9",
            "Places instances of hotlink module nodes at an origin, rotation and mirroring."
        );
        err |= RegisterCommand<ChangeHotlinkInstancesCommand> (
            projectCommands, "1.5.9",
            "Moves, rotates or mirrors placed hotlink instances by changing their transformation. MoveElements and RotateElements do not work on hotlink instances."
        );
        err |= RegisterCommand<UpdateHotlinksCommand> (
            projectCommands, "1.7.1",
            "Re-reads the source files of hotlink module nodes and refreshes their cached content in the project, as the Hotlink Manager's Update button does."
        );
        err |= RegisterCommand<ChangeHotlinkNodesCommand> (
            projectCommands, "1.7.1",
            "Repoints hotlink module nodes at other source files, as the Hotlink Manager's Relink button does. Follow it with UpdateHotlinks to re-read the content."
        );
        err |= RegisterCommand<BreakHotlinkNodesCommand> (
            projectCommands, "1.8.0",
            "Breaks hotlink nodes, as the Hotlink Manager's Break Hotlink does: the elements of each node's placed instances stay in the project as ordinary elements, no longer tied to the source file."
        );
        err |= RegisterCommand<DeleteHotlinkNodesCommand> (
            projectCommands, "1.8.0",
            "Deletes hotlink nodes together with their placed instances, as the Hotlink Manager's Delete does."
        );
        err |= RegisterCommand<GetHotlinkOwnersOfElementsCommand> (
            projectCommands, "1.8.0",
            "Tells for each element whether it comes from a hotlinked module, and returns the hotlink element that contains it and the hotlink instance it belongs to. Available from Archicad 27."
        );
        err |= RegisterCommand<GetHotlinkProxyElementTableCommand> (
            projectCommands, "1.7.3",
            "Maps the proxy elements of placed hotlink instances to the elements of the source file they were made from, so an element seen in a hotlinked module can be traced back to its source element."
        );
        err |= RegisterCommand<GetHotlinkSourceStatusCommand> (
            projectCommands, "1.8.0",
            "Tells for each hotlink source file whether it is available, missing, not accessible, incompatible or offline. Available from Archicad 27."
        );
        err |= RegisterCommand<OpenProjectCommand> (
            projectCommands, "1.0.7",
            "Opens the given project. The unsaved changes of the open project are discarded without asking: Archicad has no query for them, "
            "so call SaveProject first when they must be kept."
        );
        err |= RegisterCommand<NewProjectCommand> (
            projectCommands, "1.7.3",
            "Starts a new, untitled project from the template set in Archicad's New Project settings, as File > New does; with newAndReset also the working environment is reset, as New and Reset All does. The unsaved changes of the open project are discarded, as with OpenProject."
        );
        err |= RegisterCommand<CloseProjectCommand> (
            projectCommands, "1.3.1",
            "Closes the currently opened project."
        );
        err |= RegisterCommand<SaveProjectCommand> (
            projectCommands, "1.3.1",
            "Saves the currently opened project, in place or to a given path. With overwrite false it refuses to replace an existing file at the path."
        );
        err |= RegisterCommand<SaveAsModuleFileCommand> (
            projectCommands, "1.5.9",
            "Saves the given elements, or the current selection, as a hotlink module (.mod) file. With overwrite false it refuses to replace an existing file."
        );
        err |= RegisterCommand<SaveProjectAsArchiveCommand> (
            projectCommands, "1.6.0",
            "Saves the open project as an archive (.pla) file, with the library parts it uses inside. With overwrite false it refuses to replace an existing file."
        );
        err |= RegisterCommand<GetRenderingScenesCommand> (
            projectCommands, "1.8.0",
            "Gets the names of the rendering scenes of the project (the Scene list of the PhotoRendering Settings palette), the most recently used first, and the name of the current scene."
        );
        err |= RegisterCommand<SetCurrentRenderingSceneCommand> (
            projectCommands, "1.8.0",
            "Makes the given rendering scene the current one, as selecting it in the PhotoRendering Settings palette does."
        );
        err |= RegisterCommand<PhotoRenderCommand> (
            projectCommands, "1.8.0",
            "Renders the 3D model with the current rendering scene, or with the given one, as Document > Creative Imaging > PhotoRender Projection does, and saves the image to a file (png, jpg, bmp, tiff or gif). The image size, resolution and quality come from the scene; a width, height or resolution given here is used for this render only, the scene is not changed. With overwrite false it refuses to replace an existing file."
        );
        err |= RegisterCommand<SaveCurrentWindowAsImageCommand> (
            projectCommands, "1.7.2",
            "Saves what the current window shows as an image file (png, jpg, bmp, tiff or gif), as File > Save as does with a picture format. Master layout windows are not supported: save a layout that uses the master layout instead. With overwrite false it refuses to replace an existing file."
        );
        err |= RegisterCommand<GetCurrentViewImageCommand> (
            projectCommands, "1.7.3",
            "Returns what the 3D window, or the PhotoRendering window, shows as a base64 encoded png or jpg image with its size in pixels, without writing a file. Available from Archicad 30."
        );
        err |= RegisterCommand<GetCalculationUnitsCommand> (
            projectCommands, "1.4.0",
            "Gets the project calculation units."
        );
        err |= RegisterCommand<GetDimensionStandardsCommand> (
            projectCommands, "1.8.0",
            "Gets the dimension standards of the project (Options > Project Preferences > Dimensions) with their name and the unit, decimals and rounding of each dimension type, optionally only the one with the given name. Read-only. Available from Archicad 27."
        );
        err |= RegisterCommand<GetExportToleranceCommand> (
            projectCommands, "1.8.0",
            "Gets the export tolerance: the largest distance allowed between an arc and the straight segments that replace it when curves are segmented for export. Archicad only provides this value while an export is in progress, so outside an export the command returns an error saying so."
        );
        err |= RegisterCommand<GetLast3DDefaultLevelsCommand> (
            projectCommands, "1.8.0",
            "Gets the last 3D default levels of the given element types' defaults."
        );
        err |= RegisterCommand<SetLast3DDefaultLevelsCommand> (
            projectCommands, "1.8.0",
            "Sets the last 3D default levels of the given element types' defaults."
        );
        err |= RegisterCommand<GetProjectPreferencesCommand> (
            projectCommands, "1.7.3",
            "Gets pages of the project preferences (Options > Project Preferences): the working units, the dimensions, the calculation units, the zones and the reference levels, and the Floor Plan Cut Plane Settings. Further pages may follow."
        );
        err |= RegisterCommand<SetProjectPreferencesCommand> (
            projectCommands, "1.7.3",
            "Changes pages of the project preferences (Options > Project Preferences): the working units, the dimensions, the calculation units, the zones and the reference levels, and the Floor Plan Cut Plane Settings. Only the given fields change, the rest of each page keeps its value. In Teamwork the PreferencesDialog lockable object set has to be reserved first (ReserveLockableObjectSet). Available from Archicad 27."
        );
        err |= RegisterCommand<ConvertUnitValuesCommand> (
            projectCommands, "1.8.0",
            "Formats lengths, areas, volumes and angles (given in meters, square meters, cubic meters and radians) as Archicad shows them with the unit and rounding of the given Project Preference, returning the text, its extra accuracy decimals and the unit. The output is aligned with the input array of values. Available from Archicad 27."
        );
        err |= RegisterCommand<GetRenovationStatusNamesCommand> (
            projectCommands, "1.8.0",
            "Gets the names the running Archicad shows for the renovation statuses Existing, New and Demolished (localized). Available from Archicad 27."
        );
        err |= RegisterCommand<GetGeoLocationCommand> (
            projectCommands, "1.1.6",
            "Gets the project location details: the project location, and the survey point's position, geo-referencing parameters, visibility and lock state (the lock from Archicad 26)."
        );
        err |= RegisterCommand<GetSurveyPointTransformationCommand> (
            projectCommands, "1.7.3",
            "Gets the 3x4 matrix that transforms points from the Project Origin coordinate system into the Survey Point coordinate system, row by row, and its inverse when it is invertible. Read only. Available from Archicad 27."
        );
        err |= RegisterCommand<SetGeoLocationCommand> (
            projectCommands, "1.2.9",
            "Sets the project location details: the project location, and the survey point's position, geo-referencing parameters, visibility and lock state (the lock from Archicad 26). Omitted fields stay unchanged."
        );
        err |= RegisterCommand<SetPlaceSettingsCommand> (
            projectCommands, "1.8.0",
            "Changes the place settings of the project: location (longitude, latitude, altitude, north), date, time, summer time and time zone. Only the given fields change. Returns the settings read back, with the sun position Archicad calculated for them. Available from Archicad 27."
        );
        err |= RegisterCommand<FindDimensionStandardCommand> (
            projectCommands, "1.8.0",
            "Finds a dimension standard (Options > Project Preferences > Dimensions) by its name and returns its index, name and guid. Available from Archicad 27."
        );
        err |= RegisterCommand<GetProjectNotesCommand> (
            projectCommands, "1.7.3",
            "Gets the Project Notes of the project (File > Info > Project Notes): client, project name, company, address, architect, draftsmen, project status, date of issue, keywords and notes."
        );
        err |= RegisterCommand<SetProjectNotesCommand> (
            projectCommands, "1.7.3",
            "Changes the Project Notes of the project (File > Info > Project Notes). Only the given fields are changed; texts longer than the field Archicad stores are truncated. architect and draftsmen are full names that Archicad keeps as name parts: a new name is stored in the given name with the other parts cleared, and the parts of a name that is not given or not changed are kept."
        );
        err |= RegisterCommand<GetSunSettingsCommand> (
            projectCommands, "1.8.0",
            "Gets the project's date and time (Options > Project Preferences > Location Settings), the sun position Archicad calculates for them at the project location, and the sun and ambient light settings."
        );
        err |= RegisterCommand<CalculateSunPositionCommand> (
            projectCommands, "1.8.0",
            "Calculates the sun position (azimuth and altitude in degrees) for a date and time, at the project location unless another one is given, without changing the project."
        );
        err |= RegisterCommand<GetCoordinateOriginsCommand> (
            projectCommands, "2.0.0",
            "Gets the offset of the virtual coordinate system origin from the world origin, and the user origin of the current database. Use it to tell which origin the coordinates of different sources (elements, views, survey point) are relative to."
        );
        err |= RegisterCommand<PrintViewCommand> (
            projectCommands, "1.3.1",
            "Prints from the current view."
        );
        err |= RegisterCommand<RebuildViewCommand> (
            projectCommands, "1.5.0",
            "Rebuilds the current view."
        );
        err |= RegisterCommand<GetAutoIntersectCommand> (
            projectCommands, "1.8.0",
            "Gets whether Auto-Intersection (Options > Auto-Intersection) is on. It changes what the wall and beam creation commands produce: with it on, the new elements are joined at their intersections."
        );
        err |= RegisterCommand<SetAutoIntersectCommand> (
            projectCommands, "1.8.0",
            "Turns Auto-Intersection on or off. It is a setting of the running Archicad, not of the project; remember to restore the previous state afterwards."
        );
        err |= RegisterCommand<ResolveNamesCommand> (
            projectCommands, "2.0.0",
            "Looks up words or phrases (for example from a user's question) among the names of the open project before they are passed to other commands: stories, layers, layer combinations, building materials, composites, profiles, surfaces, fills, lines, zone categories, element types (by API name and, from Archicad 27, by the name the Archicad interface shows), properties (by name, and by group and name together) and classifications (systems by name and version, items of every system by name and ID). For each name it returns 'found' with the items named exactly so, 'closeMatches' with the nearest names, best first (ranked by differences in case, accents, spacing and separators, shared words, one name containing the other and a few differing characters), or 'notFound' when nothing in the project is close, meaning the project has no such item and it must not be guessed. Each match carries what the other commands take: the story index, the attribute id and index, the elementType, the propertyId (with the property's group, value type and the classification items a custom property is available for), or the classificationSystemId and classificationItemId. Matching is lexical and deterministic. Read only."
        );
        AddCommandGroup (projectCommands);
    }

    { // Custom Data Commands
        CommandGroup customDataCommands ("Custom Data Commands");
        err |= RegisterCommand<SetElementUserDataCommand> (
            customDataCommands, "1.8.0",
            "Stores string values under keys on elements, for scripts to keep their own state per element (external database ids, sync timestamps, markers). The data is saved with the project as the user data of the Tapir add-on on the element and appears nowhere in the Archicad UI. The given keys are merged into what the element already holds: a key that exists gets the new value, other keys are left as they are, and the keys listed in keysToDelete are removed. Prefix the keys with the script's name so that scripts do not overwrite each other's data. One undo step per call; in a Teamwork project the element has to be reserved."
        );
        err |= RegisterCommand<GetElementUserDataCommand> (
            customDataCommands, "1.8.0",
            "Reads the values stored on elements by SetElementUserData: every key of each element, or only the given keys."
        );
        AddCommandGroup (customDataCommands);
    }

    { // Element Commands
        CommandGroup elementCommands ("Element Commands");
        err |= RegisterCommand<GetSelectedElementsCommand> (
            elementCommands, "0.1.0",
            "Gets the list of the currently selected elements."
        );
        err |= RegisterCommand<GetElementsByTypeCommand> (
            elementCommands, "1.0.7",
            "Returns the identifier of every element of the given type on the plan. It works for any type. Use the optional filter parameter for filtering. Objects that belong to another element (zone stamps, the parts of stairs and railings) are left out unless the IncludeSubElemObjects filter is given."
        );
        err |= RegisterCommand<GetAllElementsCommand> (
            elementCommands, "1.0.7",
            "Returns the identifier of all elements on the plan. Use the optional filter parameter for filtering. Objects that belong to another element (zone stamps, the parts of stairs and railings) are left out unless the IncludeSubElemObjects filter is given."
        );
        err |= RegisterCommand<CaptureElementStateCommand> (
            elementCommands, "1.8.0",
            "Captures the current state of the elements of the project and returns a stateId for it, so that GetElementChangesSince can later list what was created, modified and deleted since. The optional type selects how changes are detected (modification stamps by default, or the 3D model). The state is kept in a file in the temporary folder for the Archicad session only: it is lost when the project is closed or Archicad restarts, and ReleaseElementState frees it earlier."
        );
        err |= RegisterCommand<GetElementChangesSinceCommand> (
            elementCommands, "1.8.0",
            "Returns the new, modified and deleted elements between a state captured by CaptureElementState and either another captured state (toStateId) or the current state of the project. Scripts that keep an external copy of the model use it to sync only what changed instead of re-reading every element. The changes come from Archicad's element difference generator, which tracks the model elements: 2D elements such as texts, labels and hatches can be missing from the lists, so read those directly. Fails for a stateId that is unknown - never captured, released, or lost because the project was closed or Archicad restarted."
        );
        err |= RegisterCommand<ReleaseElementStateCommand> (
            elementCommands, "1.8.0",
            "Frees a state captured by CaptureElementState. Every state is freed anyway when the project is closed or Archicad quits."
        );
        err |= RegisterCommand<GetElementTypeNamesCommand> (
            elementCommands, "1.7.3",
            "Gets the localized names of the given element types, as the running Archicad shows them (for example 'Wand' for Wall in German Archicad). The output is aligned with the input array of element types. Available from Archicad 27."
        );
        err |= RegisterCommand<GetElementsInCreationOrderCommand> (
            elementCommands, "1.8.0",
            "Returns the identifier of every element of the current database, or of the given databases, in the order the elements were created."
        );
        err |= RegisterCommand<ChangeSelectionOfElementsCommand> (
            elementCommands, "1.0.7",
            "Adds/removes a number of elements to/from the current selection."
        );
        err |= RegisterCommand<DeselectAllCommand> (
            elementCommands, "2.0.0",
            "Clears the selection in the current window."
        );
        err |= RegisterCommand<GetMarqueeCommand> (
            elementCommands, "2.0.0",
            "Gets the marquee of the current window: its shape (rectangle, rotated rectangle or polygon), whether it is multi-story, and its outline as a polygon. Returns the type 'none' when there is no marquee."
        );
        err |= RegisterCommand<SetMarqueeCommand> (
            elementCommands, "2.0.0",
            "Draws a marquee in the current window from a polygon or a box (optionally rotated), single- or multi-story. Called without polygon and box it removes the marquee."
        );
        err |= RegisterCommand<FilterElementsCommand> (
            elementCommands, "1.0.7",
            "Keeps the elements that match all the given criteria: Archicad's flag filters (filters), layer (layerIndices), home story (floorIndices), element type (elementTypes) and hotlink (fromHotlink, hotlinkIds). Within one criterion any of the listed values matches; an omitted criterion does not filter."
        );
        err |= RegisterCommand<GetElementsByCriteriaCommand> (
            elementCommands, "2.0.0",
            "Returns the elements, or with countOnly only their number, that match all the given criteria: element types, home stories, layers, Archicad's element filter flags and property criteria (a value compared in internal units, or a value status such as hasValue, isUserUndefined, isNotAvailable, isDefault or hasCustomValue). An unknown type, property or operator is an error, not ignored. Objects that belong to another element (zone stamps, the parts of stairs and railings) are left out unless the IncludeSubElemObjects filter is given."
        );
        err |= RegisterCommand<GetDetailsOfElementsCommand> (
            elementCommands, "1.5.7",
            "Gets the details of elements (geometry parameters etc). When elements is omitted, all elements matching the optional server-side filter (types, floorIndices, layerIndices) are queried; an empty elements list returns no details. Use the optional fields parameter to return only the fields you need (e.g. elementId, type, id, floorIndex) and skip the computation of the others (for example floorPlanPolygons). Use the optional additionalFields parameter to also get the info string, the compound info string, the library part unique ID and the creator tool unique ID. An element of another database (a layout, a section, a worksheet ...) returns only its header fields (type, floor index, layer, draw index) with fromOtherDatabase set to true."
        );
        err |= RegisterCommand<SetDetailsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Sets the details of the given elements. Every element type: Element ID (id), story (floorIndex), layer (layerIndex) and display order (drawIndex). In typeSpecificDetails: Wall - start and end point, height, base offset, and the start and end thickness of a trapezoid wall; Zone - the ZoneSettings fields (the same as ModifyZones); Line - start and end point, pen, line type, room separator; Arc and Circle - origin, radius, angle, ratio, start and end angle (arcs only), reflected, pen, line type, room separator; Polyline - points and arcs, pen, line type, room separator; Spline - pen, line type, room separator; Hatch - outline, holes, contour pen, fill pen and background pen or colors, fill or building material, area display, room special and fill orientation; Hotspot - position, height, pen; Text - text, position, angle, height, justification; Label - the text of a text label; Drawing - position, angle, ratio, model offset, clip polygon (isCutWithFrame), name, ID and numbering, scale in Autotext, border, title library part, pen table (penTableUsageMode, penTableIndex), colorMode, isTransparentBackground, rasterizeDPI and the anchor (anchorPoint, useOwnOriginAsAnchor). Not written, because the Archicad API does not take them: the geometry of a Spline, the source of a Drawing (use ChangeDrawingLink), the Drawing Manager's update type and stored in project, and often a placed Drawing's drawingScale, which comes from its source view (the element's result then names the kept value). Other wall, slab, column, beam, stair and object settings are written by their Modify commands. An element Archicad does not let you edit (its layer is hidden or locked, it is locked, or it is not reserved by you in teamwork) gets a failed execution result naming the reason; a Drawing setting Archicad kept unchanged is named in a failed execution result too."
        );
        err |= RegisterCommand<SetElementInfoStringsCommand> (
            elementCommands, "1.8.0",
            "Sets the info string of the given elements: the Element ID shown in the Info Box, which GetDetailsOfElements returns as id. Returns one execution result per element."
        );
        err |= RegisterCommand<GetElementCategoriesCommand> (
            elementCommands, "1.7.3",
            "Lists the element categories of the project (Structural Function, Position, Renovation Status, Renovation Filter and, in the German version, BRI) with their possible values. The names of the categories and values are localized: identify a category by its type and a value by its categoryValueId, never by name."
        );
        err |= RegisterCommand<GetCategoryValuesOfElementsCommand> (
            elementCommands, "1.7.3",
            "Gets the category values (Structural Function, Position, Renovation Status ...) of the given elements, optionally only for the given category types. An element that has none of the requested categories (an annotation, for example) gets an error item. The names are localized: use the type and the categoryValueId in scripts."
        );
        err |= RegisterCommand<GetCategoryValuesOfDefaultsCommand> (
            elementCommands, "1.8.0",
            "Gets the category values (Structural Function, Position, Renovation Status ...) of the tool defaults of the given element types, the values new elements of the type get, optionally only for the given category types. A category the default has no value in is listed without categoryValueId; an element type without any of the requested categories gets an error item. The names are localized: use the type and the categoryValueId in scripts."
        );
        err |= RegisterCommand<SetCategoryValuesOfElementsCommand> (
            elementCommands, "1.7.3",
            "Sets the category values (Structural Function, Position, Renovation Status ...) of the given elements, one element and one category per item, in a single undoable step. The value must belong to the given category, as GetElementCategories lists them; an element without that category gets a failed execution result. The names are localized: use the type and the categoryValueId in scripts. With dryRun true nothing is changed and no undo step is created: the response has dryRun set to true, and each item that passes the checks made before writing gets wouldChange, with the element's value in the category now (oldValue) and the value a real run would set (newValue)."
        );
        err |= RegisterCommand<Get3DBoundingBoxesCommand> (
            elementCommands, "1.1.2",
            "Get the 3D bounding box of elements. The bounding box is calculated from the global origin in the 3D view. The output is the array of the bounding boxes respective to the input array of elements. An element without 3D geometry of its own gets an error item instead of a bounding box. Walls, Slabs, Roofs, Zones and Stairs are measured on their 3D model, so the box of a Wall includes its connections to other walls. The current window does not need to show the element: it works with a layout or a section open as well."
        );
        err |= RegisterCommand<Get2DBoundingBoxesCommand> (
            elementCommands, "2.0.0",
            "Get the 2D bounding box of elements as Archicad draws them, in the coordinates of the database the element is in: the floor plan, a section, a worksheet, a layout or a master layout. Works for 2D elements (Line, PolyLine, Arc, Hatch, Text, Label, Dimension, Drawing ...) as well as for model elements; a Text's box covers its drawn lines, a Drawing's box its frame with the title. The current window does not need to show the element. The output is the array of the bounding boxes respective to the input array of elements."
        );
        err |= RegisterCommand<GetLibPartTransformationsCommand> (
            elementCommands, "1.8.0",
            "Gets the placement transformation of library part instances (Window, Door, Object, Lamp, Zone) as Archicad computes it: the location, the rotation angle and the x/y scaling. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetSnapPointsOfElementsCommand> (
            elementCommands, "2.0.0",
            "Gets the snap points (hotspots) of the given elements, of any element type: the points of an element that the cursor snaps to and dimensions attach to (for example the endpoints of a Wall's reference line or the vertices of a Slab), in world coordinates, each with the neig kind and subindex identifying which point of the element it is. For the 2D hotspots of library part instances see GetHotspotsOfElements. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<ConvertRevealOffsetsCommand> (
            elementCommands, "1.7.3",
            "Converts the reveal depth offset of windows and doors placed in walls between the two references Archicad uses: measured from the core skin of a composite wall, or from the wall side. Give exactly one of coreOffset or sideOffset per opening; both values are returned. For walls with a basic structure the two values are equal. The output is aligned with the input array of openings. Available from Archicad 27."
        );
        err |= RegisterCommand<Get3DGeometryOfElementsCommand> (
            elementCommands, "1.7.2",
            "Gets the 3D geometry of elements as the solid bodies of their 3D model: a vertex list in world coordinates and the planar faces (with their holes) referencing it, each face with its surface attribute when that is a project attribute. The bodies of the subelements of a Stair, Railing or Curtain Wall, and of the segments of a Column or Beam, are included. The output is aligned with the input array of elements. The 3D model of the elements is generated as needed, so many elements can take a while."
        );
        err |= RegisterCommand<GetElementConnectionsCommand> (
            elementCommands, "1.8.0",
            "Gets the connections among the given elements: each pair of touching elements once, with the polygons of the surface where they touch, in world coordinates. The 3D model of the elements is generated with separate components in a temporary sight, so the 3D window is not changed. Also returns, aligned with the input array of elements, whether each element was found. Available from Archicad 28."
        );
        err |= RegisterCommand<GetBodyMaterialsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets the building material of each body of the 3D model of the given elements. The 3D model is generated with separate components in a temporary sight, so a composite structure has one body per skin and the 3D window is not changed. The output is aligned with the input array of elements. Available from Archicad 28."
        );
        err |= RegisterCommand<GetCutPolygonsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Cuts each body of the 3D model of the given elements with a plane and returns, for each body, its building material, the area of the cut surface and its polygons (outline and holes) in 2D coordinates of the plane. From Archicad 28 the 3D model is generated with separate components in a temporary sight, so a composite structure has one body per skin and the 3D window is not changed. Before Archicad 28 the bodies are those of the element's 3D model (a composite structure is one body) and their building materials are left out. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetElementAtCoordinateCommand> (
            elementCommands, "1.7.3",
            "Finds the element of the given polygonal type (for example Slab, Roof, Mesh, Zone or Hatch) at a coordinate of the floor plan of the current database, the way a click there would. Use the optional filters parameter (the values of FilterElements) to consider only, for example, the elements visible by layer, and the optional z parameter to give the elevation of the point. Returns found: false when there is no such element. Useful together with GetPointFromUser."
        );
        err |= RegisterCommand<GetSkylightHolePolygonsCommand> (
            elementCommands, "1.8.0",
            "Gets the polygon of the hole each given Skylight cuts into its host Roof or Shell: the outline and any holes of it as 2D coordinates. The output is aligned with the input array of elements; an element that is not a Skylight gets an error item."
        );
        err |= RegisterCommand<GetVisibleSkinStatusCommand> (
            elementCommands, "1.8.0",
            "Checks whether elements have a visible skin in a given Partial Structure Display mode (EntireStructure, CoreOnly, WithoutFinishes or StructureOnly), e.g. a finish-only slab has none in CoreOnly. The current window is not changed. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetRoofPlanesCommand> (
            elementCommands, "2.0.0",
            "Gets the single planes of roofs, without changing the roofs. A multi-plane roof is decomposed by Archicad into its planes, each returned with its part identifier and its details as a single-plane roof (slope, pivot line, level, outline with arcs and holes); a single-plane roof is returned as its only plane. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetCurtainWallAbsoluteBottomsCommand> (
            elementCommands, "1.8.0",
            "Gets the absolute bottom level of curtain walls or their subelements (segments, frames, panels, junctions, accessories). The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetQuantitiesOfElementsCommand> (
            elementCommands, "1.7.2",
            "Gets the quantities of the given elements calculated by Archicad (volumes, surfaces, lengths etc.), together with the volume and projected area per building material (one item per skin of composite and complex profile elements, one item for basic elements). Supported types: Wall, Column, Beam, Slab, Roof, Shell, Morph, Zone, ColumnSegment, BeamSegment. Use the optional minOpeningSize parameter to control the conditional values, and calculateInOneCall to calculate all the elements in one Archicad call."
        );
        err |= RegisterCommand<TriangulatePolygonsOfElementsCommand> (
            elementCommands, "1.7.3",
            "Triangulates the plan polygon of the given elements, holes excluded, as Archicad does it: the outline of a Slab, Zone, Hatch or Mesh and the polygon of a Roof. Each triangle is given by the 2D coordinates of its three vertices. With convex: true, the faces of the 3D model of the elements (of any type) are decomposed into convex polygons instead, given by the 3D coordinates of their vertices. The output is aligned with the input array of elements; an element of another type gets an error item."
        );
        err |= RegisterCommand<GetTextureCoordinatesCommand> (
            elementCommands, "1.8.0",
            "Gets the texture coordinates (u, v) of the faces of the 3D model of the given elements: per solid body, per face, at the first vertex of the face. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetSurfaceTypesOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets the surface type of every polygon of the 3D model of the given elements, as ACAPI_Polygon_PolygonId2SurfaceType returns it: for each element its polygons with their body index, polygon index, polygon id and the numeric API_SurfaceTypeID value. The model is generated with separate components in a temporary sight, so the 3D window is not changed. The output is aligned with the input array of elements. Available from Archicad 28."
        );
        err |= RegisterCommand<Get3DModelCountsCommand> (
            elementCommands, "1.8.0",
            "Gets the number of components of each type (bodies, polygons, polygon edges, edges, vertices, normal vectors, materials and lights) in the 3D model Archicad currently holds. Bodies, materials and lights are counted with ACAPI_ModelAccess_GetNum; the polygons, polygon edges, edges, vertices and normal vectors, which that function refuses to count, are summed over the bodies. A type that cannot be counted gets an error item. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteUserDataOfElementsCommand> (
            elementCommands, "2.0.0",
            "Deletes the user data this add-on stored on the given elements. Archicad keeps the user data of an element per add-on, so the data of other add-ons is left as it is. All deletions are one undo step. The output has one execution result per element, aligned with the input array of elements. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each element that exists gets wouldDelete."
        );
        err |= RegisterCommand<GetSurfaceQuantitiesOfElementsCommand> (
            elementCommands, "1.7.3",
            "Gets the exposed surface areas of the given elements calculated by Archicad: for each element one item per surface attribute and building material pair with the area that is not covered by the covering elements (for example the wall area behind a slab is taken off the wall's surface). The covering elements are the optional coverElements; by default every construction element of the project. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetSubelementsOfHierarchicalElementsCommand> (
            elementCommands, "1.0.6",
            "Gets the subelements of the given hierarchical elements. From Archicad 28 the subelements of external hierarchical elements (for example MEP elements or elements of other add-ons) are returned as externalSubelements."
        );
        err |= RegisterCommand<GetHierarchicalElementOwnersCommand> (
            elementCommands, "1.7.3",
            "Gets the owner of each given element in its hierarchical element: the root (e.g. the Curtain Wall, Stair or Railing of a frame, tread or post) or the immediate parent. Also returns whether the given element is a main element, a subelement or not part of a hierarchical element."
        );
        err |= RegisterCommand<EditHierarchicalElementCommand> (
            elementCommands, "1.8.0",
            "Opens the given hierarchical element (Curtain Wall, Stair, Railing, Beam or Column) in its hierarchical 3D editing mode, as the Edit button of the element does. Fails when Archicad is already in an editing mode. Limitation: while the editing mode is open, Archicad refuses every JSON command, this add-on's included, with error 4001 (ongoing user input), FinishHierarchicalEditing included, so the mode can only be left in Archicad, with the button of its edit mode palette (reported for Archicad 29). Requires Archicad 27 or later."
        );
        err |= RegisterCommand<FinishHierarchicalEditingCommand> (
            elementCommands, "1.8.0",
            "Finishes the current hierarchical editing mode (Curtain Wall, Stair, Railing, Beam or Column): accept true keeps the changes (OK), accept false discards them (Cancel). The accept parameter is required, so changes are never discarded silently. Fails when Archicad is not in such an editing mode. Limitation: while an editing mode is open, Archicad refuses JSON commands with error 4001 (ongoing user input) before they run, so this command cannot leave a mode opened by EditHierarchicalElement or in Archicad (reported for Archicad 29); leave it in Archicad instead. Requires Archicad 27 or later."
        );
        err |= RegisterCommand<GetConnectedElementsCommand> (
            elementCommands, "1.1.4",
            "Gets the elements hosted by (connected to) the given owner elements, filtered to the given element type: for example the Windows or Doors of a Wall, the frames, panels, junctions and accessories of a Curtain Wall, the risers, treads and structures of a Stair, or the posts, rails and panels of a Railing."
        );
        err |= RegisterCommand<GetElementsInDrawOrderCommand> (
            elementCommands, "1.7.3",
            "Returns the identifier of every element of the current database, or of the given databases, in Archicad's drawing order (the order set by Bring to Front and Send to Back), as ACAPI_DrawOrder_DrawOrderGetNext walks it. Use the optional elementType and filters parameters for filtering. Objects that belong to another element (zone stamps, the parts of stairs and railings) are left out unless the IncludeSubElemObjects filter is given."
        );
        err |= RegisterCommand<GetDrawingPrimitivesOfElementsCommand> (
            elementCommands, "1.8.0",
            "Returns the 2D drawing primitives (points, lines, arcs, texts, polylines, triangles, polygons, pictures) Archicad draws each given floor plan element from, in drawing order, as ACAPI_DrawingPrimitive_ShapePrimsExt decomposes it on every story and unclipped. Control items (type Ctrl_*) mark the beginning and the end of sub-parts such as hatches and texts. Texts are returned without their content and polylines without their vertices. A Ctrl_ElementRef item names the element it refers to: the drawing of a Wall ends with one for each Window and Door in it, while Windows and Doors give no primitives of their own. The floor plan must be the current database. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetStoryControlPrimitivesCommand> (
            elementCommands, "1.8.0",
            "Returns the 2D drawing primitives of the story levels (story lines and their markers) drawn in the given section, elevation or interior elevation database, as ACAPI_DrawingPrimitive_DrawStoryControl draws them, in the same format as GetDrawingPrimitivesOfElements. The databaseId can be obtained with GetDatabaseIdFromNavigatorItemId."
        );
        err |= RegisterCommand<GetShapePrimitivesOfElementsCommand> (
            elementCommands, "2.0.0",
            "Returns the 2D drawing primitives of each given floor plan element in the same format as GetDrawingPrimitivesOfElements, but drawn as ACAPI_DrawingPrimitive_ShapePrims draws it by default (on the current story, clipped as Archicad clips it) instead of on every story and unclipped. With includeMarkers the objects attached to the elements (window and door markers, drawing titles, ...) are drawn with them or left out; Archicad's setting is changed back afterwards. An element on another story gets an error item. The floor plan must be the current database. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetSectionElementsCommand> (
            elementCommands, "1.5.8",
            "Gets the elements drawn in the given section, elevation or interior elevation databases, each with the owner element it was generated from. This is the only command that returns raw section element identifiers - every other listing command converts them to their owner - so it is the way to obtain the sectionElementId that CreateAssociativeDimensionsOnSection requires."
        );
        err |= RegisterCommand<GetRelationsOfElementsCommand> (
            elementCommands, "1.5.7",
            "Gets the type-specific relations of the given elements: endpoint and reference line connections of walls, beams and beam segments, boundary elements and boundary sections of zones, the zones on the two sides of windows, doors, skylights and curtain wall panels, and the zones connected to roofs and shells. Available from Archicad 26."
        );
        err |= RegisterCommand<LinkElementsCommand> (
            elementCommands, "1.7.3",
            "Creates persistent links between pairs of elements, each from one element to another, with an optional integer flags value per link. Scripts can record their own relations between elements this way (for example which label documents which column) instead of storing GUIDs in properties. The links are stored in the project and have no effect on geometry; they are independent from groups, hotlinks and solid element operations. A link is directional: it is returned by GetLinksOfElements for the element it starts from only, and removed by UnlinkElements. Undoable."
        );
        err |= RegisterCommand<UnlinkElementsCommand> (
            elementCommands, "1.7.3",
            "Removes the links created by LinkElements between the given pairs of elements. The direction matters: the link starting from fromElementId and pointing to toElementId is removed. Undoable."
        );
        err |= RegisterCommand<GetLinksOfElementsCommand> (
            elementCommands, "1.7.3",
            "Gets the links created by LinkElements that start from the given elements: the linked element and the flags of each link. Links pointing to an element are not returned; query the elements they start from. Returns an error item for an element that does not exist."
        );
        err |= RegisterCommand<GetZoneBoundariesCommand> (
            elementCommands, "1.2.3",
            "Gets the boundaries of the given Zones (connected elements, neighbour zones, etc.). Accepts either a single zoneElementId or a list of zones. Prefer the list: the expensive boundary recalculation runs once per call, so querying many Zones in one call is much faster than calling the command once per Zone."
        );
        err |= RegisterCommand<GetZoneReductionsCommand> (
            elementCommands, "1.7.3",
            "Gets the area reductions of the given Zones, as the Zone Stamp's area calculation uses them: the polygons of the Zone covered by Walls, Columns, reducing Fills, sloping Roofs and low ceilings, each with its type, the percentage it is reduced by, its area and its outline with holes. Area calculations by local standards need these parts, which GetQuantitiesOfElements only gives as totals. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetPropertyObjectsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets the property objects of the given elements: the library parts of the old listing system (used by projects migrated from older Archicad versions) linked to each element by criteria or individually, with the library part's index (the same as the index returned by GetAvailableLibraryParts), name and guid. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetDescriptorsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets the descriptors of the given elements: the listing descriptors in the property objects of each element, with the library part that declares them (index, name and guid), their listing database set and index, and their status (Normal, Deleted or Local). The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<GetLinkedPropertyObjectsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets how property objects (the library parts of the old listing system) are linked to the given elements, or to the element defaults of the given element types: whether the property objects matching their criteria are linked, which of them match, and which one is assigned individually, each with the library part's index (the same as the index returned by GetAvailableLibraryParts), name and guid. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<SetLinkedPropertyObjectsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Sets how property objects (the library parts of the old listing system) are linked to the given elements, or to the element defaults of the given element types: whether the property objects matching their criteria are linked, and which one is assigned individually (0 removes the individual assignment). A link that is not given stays unchanged. All changes are made in one undoable step."
        );
        err |= RegisterCommand<UpdateZonesCommand> (
            elementCommands, "1.5.4",
            "Updates Zones (recalculates their geometry, updates their Zone Stamps and the connected elements): the selected Zones, every Zone when nothing is selected, or the Zones given in zones. Archicad runs the update right after the command has returned, on the Zones selected at that moment, so the new geometry can only be read with a later command."
        );
        err |= RegisterCommand<SetZoneLabelingSettingsCommand> (
            elementCommands, "1.7.3",
            "Turns the automatic labeling of Zones on or off and selects the Label Favorite it places. Only the given settings are changed. Available from Archicad 28."
        );
        err |= RegisterCommand<GetZoneLabelingSettingsCommand> (
            elementCommands, "1.8.0",
            "Returns whether the automatic labeling of Zones is on and the name of the Label Favorite it places. Available from Archicad 28."
        );
        err |= RegisterCommand<GetCollisionsCommand> (
            elementCommands, "1.2.2",
            "Detect collisions between the given two groups of elements."
        );
        err |= RegisterCommand<HighlightElementsCommand> (
            elementCommands, "1.0.3",
            "Highlights the elements given in the elements array. In case of empty elements array removes all previously set highlights. On Archicad 25 nonHighlightedColor is ignored."
        );
        err |= RegisterCommand<MoveElementsCommand> (
            elementCommands, "1.0.2",
            "Moves elements with a given vector. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<AlignElementsCommand> (
            elementCommands, "2.0.0",
            "Aligns elements on the floor plan or in the current 2D view to one edge of their plan bounding boxes (left, right, top or bottom) or to their middle (centerX, centerY), either of the whole set or of a reference element, by moving them in one undo step. The bounding boxes are those Get2DBoundingBoxes returns. Supported types: Wall, Column, Beam, Slab, Roof, Shell, Morph, Mesh, Zone, Object, Lamp, Stair, Railing, CurtainWall, Line, PolyLine, Arc, Circle, Spline, Hatch, Text and Hotspot. An element of another type (doors, windows and openings, which only slide along their host, subelements, dimensions, labels, ...) or one that cannot be edited gets a failed execution result naming the reason, and the other elements are still aligned."
        );
        err |= RegisterCommand<DistributeElementsCommand> (
            elementCommands, "2.0.0",
            "Distributes 3 or more elements on the floor plan or in the current 2D view evenly along X (horizontal) or Y (vertical), with equal distances between the middles of their plan bounding boxes (centers) or equal space between the boxes (gaps), by moving them in one undo step. The elements are taken in the order of their middles along the direction, and the two outermost stay where they are. The bounding boxes are those Get2DBoundingBoxes returns. Supported types: Wall, Column, Beam, Slab, Roof, Shell, Morph, Mesh, Zone, Object, Lamp, Stair, Railing, CurtainWall, Line, PolyLine, Arc, Circle, Spline, Hatch, Text and Hotspot. An element of another type (doors, windows and openings, which only slide along their host, subelements, dimensions, labels, ...) or one that cannot be edited gets a failed execution result naming the reason, and the other elements are still distributed."
        );
        err |= RegisterCommand<SplitElementsCommand> (
            elementCommands, "2.0.0",
            "Splits each given element along a line (the infinite line through its begin and end point, like Archicad's Split command). The original element keeps one part, copies of it (Drag a Copy, so they keep the layer, classifications, properties and every other setting) get the others; all are returned, the original first. Supports straight walls without doors, windows or openings, straight unslanted single segment beams without holes or openings, and lines (their arrows stay at the original ends), which are split in two, the original keeping the part from its begin point; and slabs, meshes without level lines or points, fills and single contour polylines, whose polygon (holes and arcs included) or points are cut along the line into one part per piece, the original keeping the largest piece (a polyline: the piece with its begin point). Slab edges keep their trim and side surface, the edges along the line are vertical; the corners a mesh gets on the line take their height from the edge they lie on. An element the line does not cross is reported as notCut, an unsupported one as an error naming its type. One undo step."
        );
        err |= RegisterCommand<CopyElementsToDatabaseCommand> (
            elementCommands, "2.0.0",
            "Copies 2D elements (lines, polylines, arcs, circles, splines, hotspots, texts, hatches, independent labels) from one database into another, e.g. from a worksheet into another worksheet, a detail or a layout, optionally moved by an offset. Returns the new elements in the order of the given ones."
        );
        err |= RegisterCommand<ElevateElementsCommand> (
            elementCommands, "2.0.0",
            "Moves elements vertically by a given offset in meters, like Edit > Move > Elevate. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<RotateElementsCommand> (
            elementCommands, "1.5.3",
            "Rotates elements around a reference point. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<MirrorElementsCommand> (
            elementCommands, "1.7.3",
            "Mirrors elements across an axis given by two points on the floor plan, optionally as a copy. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<CopyElementsCommand> (
            elementCommands, "2.0.0",
            "Copies elements by an offset, like dragging a copy in Archicad, keeping all their settings. With count, copy n is moved by n times the offset, which makes a linear array. Returns the new elements of each given element, in the order of the given ones; an element Archicad cannot copy (a part of a curtain wall, stair or railing, a locked element or one on a hidden or locked layer, ...) gets an error naming the reason, and the other elements are still copied. All copies are one undo step."
        );
        err |= RegisterCommand<RotateCopyElementsCommand> (
            elementCommands, "2.0.0",
            "Copies elements rotated around a center on the floor plan by an angle in radians, like rotating a copy in Archicad, keeping all their settings. With count, copy n is rotated by n times the angle, which makes a circular array. Returns the new elements of each given element, in the order of the given ones; an element Archicad cannot copy gets an error naming the reason, and the other elements are still copied. All copies are one undo step."
        );
        err |= RegisterCommand<ScaleElementsCommand> (
            elementCommands, "2.0.0",
            "Scales elements on the floor plan or in the current 2D view by a factor around a center point with Archicad's Resize edit, in one undo step. Resize scales X and Y by the same ratio, so factorY, if given, must equal factor. Supported types: Wall, Column, Beam, Slab, Roof, Mesh, Zone, Line, PolyLine, Arc, Circle, Spline, Hatch and Hotspot. An element of another type (library parts such as objects, lamps, doors and windows, stairs, railings, curtain walls, shells, morphs, texts, subelements, dimensions, labels, ...) or one that cannot be edited gets a failed execution result naming the reason, and the other elements are still scaled."
        );
        err |= RegisterCommand<DeleteElementsCommand> (
            elementCommands, "1.2.1",
            "Deletes elements. The zones of the call are deleted before all the other elements, so an automatic zone deleted together with its bounding walls is gone before its boundary is; deleting the boundary of an automatic zone without the zone itself can open Archicad's modal 'No closed polygon found' dialog, which blocks the command until someone clicks it in Archicad. Returns an execution result for each input element: an element that could not be deleted (for example because its layer is locked) or a guid that is not in the project gets a failed execution result instead of being skipped silently, and does not stop the other elements from being deleted. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each element that passes the checks made before deleting gets wouldDelete, so a script can preview a deletion before making it."
        );
        err |= RegisterCommand<DeleteSelectedElementsCommand> (
            elementCommands, "1.7.3",
            "Deletes the currently selected elements in one undo step, the same as pressing the Delete key. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true and lists the selected elements, with wouldDelete when there is at least one, or fails when one of them cannot be edited (Archicad then deletes none of them)."
        );
        err |= RegisterCommand<LockElementsCommand> (
            elementCommands, "1.5.2",
            "Locks the given elements. Manual lock, not teamwork!"
        );
        err |= RegisterCommand<UnlockElementsCommand> (
            elementCommands, "1.5.2",
            "Unlocks the given elements. Manual lock, not teamwork!"
        );
        err |= RegisterCommand<RefreshElementsCommand> (
            elementCommands, "1.8.0",
            "Refreshes (rebuilds) the given elements, for example placed Objects, Windows and Doors that still show the old geometry after their library part's script or embedded file changed. Set force to true to run the parameter script of GDL-based elements too. The elements are looked up in the current database (floor plan, section or detail window). Returns an execution result for each input element. The refresh cannot be undone."
        );
        err |= RegisterCommand<GetGDLParametersOfElementsCommand> (
            elementCommands, "1.5.7",
            "Gets all the GDL parameters (name, type, value) of the given elements."
        );
        err |= RegisterCommand<SetGDLParametersOfElementsCommand> (
            elementCommands, "1.5.7",
            "Sets the given GDL parameters of the given elements. An element Archicad does not let the user edit (its layer locked, the element locked, or not reserved by the user in Teamwork) gets a failed execution result naming that reason, whatever the parameter. On a Curtain Wall Frame, A, B, ac_frameWidth and ac_frameDepth mirror the frame's contour, which Archicad derives from the frame's settings, so they are rejected instead of reporting a change that does not take effect; a frame without a library part (an invisible Boundary frame, for example) has no GDL parameters. The parameters are read back after the change, and a written parameter Archicad did not keep (e.g. one it sets from the element's own settings, like AC_TextSize_1 of a label) gives a failed execution result naming it. With dryRun true no element is changed and no undo step is created: the parameters are written to the element's parameter list and its Parameter Script runs as in a real run, the response has dryRun set to true, and each element that passes the checks made before changing it gets wouldChange, listing the parameters a real run would change (the given ones and the ones the Parameter Script changes with them) with their old and new values."
        );
        err |= RegisterCommand<GetMarkerParametersOfElementsCommand> (
            elementCommands, "1.7.1",
            "Gets the marker library part and the marker's GDL parameters (name, type, value) of the given Window and Door elements. The marker is the stamp drawn next to the opening on the floor plan; its parameters are separate from the opening's own GDL parameters, which GetGDLParametersOfElements returns."
        );
        err |= RegisterCommand<SetMarkerParametersOfElementsCommand> (
            elementCommands, "1.7.1",
            "Sets the given GDL parameters of the marker of the given Window and Door elements. The opening's own settings and the other marker parameters are left as they are."
        );
        err |= RegisterCommand<GetURLReferencesOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets the web link (URL reference) of the given elements: a URL with a description, shown in Archicad in the element's settings under Element Information > Link. An element without a link gets the error APIERR_NOUSERDATA."
        );
        err |= RegisterCommand<SetURLReferencesOfElementsCommand> (
            elementCommands, "1.8.0",
            "Sets the web link (URL reference) of the given elements: a URL with a description, shown in Archicad in the element's settings under Element Information > Link. Fails with APIERR_MOREUSER for an element that already holds a link."
        );
        err |= RegisterCommand<ChangeLibraryPartOfElementsCommand> (
            elementCommands, "1.8.0",
            "Changes the library part of the given Object, Lamp, Window, Door, Skylight, Zone and symbol Label elements to another library part of the same type, given by name, index or guid, e.g. to replace missing library parts with ones from the loaded libraries. The elements themselves (guid, properties, classifications) are kept. Components of stairs and railings are not supported, even when they are reported as Objects. Optionally keeps the values of the GDL parameters with the same name and type, and the size (A and B)."
        );
        err |= RegisterCommand<GetCalculatedParametersOfCurtainWallSubElementsCommand> (
            elementCommands, "2.0.0",
            "Gets the GDL parameters (name, type, value) of the given Curtain Wall Frame, Panel, Junction and Accessory elements as Archicad calculated them for their place in the curtain wall. GetGDLParametersOfElements returns the parameters the sub-element was set up with instead."
        );
        err |= RegisterCommand<GetHotspotsOfElementsCommand> (
            elementCommands, "1.8.0",
            "Gets the 2D hotspots of the given library part instances (Objects, Lamps, Windows, Doors, Skylights, Zones, Curtain Wall parts, etc.), e.g. to dimension to them."
        );
        err |= RegisterCommand<SetMarkerLibraryPartOfElementsCommand> (
            elementCommands, "2.0.0",
            "Changes which library part the marker of the given Window and Door elements uses, given by its name (find the available ones with GetAvailableLibraryParts); only a marker library part for the opening's type is accepted. The marker gets the new library part's default parameters, which SetMarkerParametersOfElements can change afterwards; the opening's own settings are left as they are."
        );
        err |= RegisterCommand<RunParameterScriptOfElementsCommand> (
            elementCommands, "1.7.3",
            "Runs the parameter script of the library part of the given elements (objects, lamps, windows, doors, etc.), so that parameter values that depend on other parameters are updated, e.g. after SetGDLParametersOfElements."
        );
        err |= RegisterCommand<CreateColumnsCommand> (
            elementCommands, "1.0.3",
            "Creates Column elements based on the given parameters. Takes the same cut fill pens and cover fill as ModifyColumns, besides the position, size, slant, cross section and veneer; "
            "settings not given come from the Column tool defaults or the favorite. Not written by either command: the surfaces of the column segments, and the cut fill pens before Archicad 27."
        );
        err |= RegisterCommand<CreateWallsCommand> (
            elementCommands, "1.4.0",
            "Creates Wall elements based on the given parameters. Takes the same wall settings as ModifyWalls, applied by the same code, except geometryType and profileType, which the new wall takes from the favorite or the Wall tool defaults (its plan outline is always Straight), and bottomOffset, which comes from zCoordinate. Neither command can write a Polygonal outline (it needs an outline polygon, which these commands do not take), connectionPriority or junctionSequence (Archicad ignores writes to them; GetDetailsOfElements reads them), and on Archicad 29 any story visibility other than 'Home story only' and 'All relevant stories'."
        );
        err |= RegisterCommand<CreateBeamsCommand> (
            elementCommands, "1.4.0",
            "Creates Beam elements based on the given parameters. Takes the same cut fill pens and cover fill as ModifyBeams, besides the position, shape, slant and cross section; "
            "settings not given come from the Beam tool defaults or the favorite. Not written by either command: the surfaces of the beam segments, and the cut fill pens before Archicad 27."
        );
        err |= RegisterCommand<CreateStairsCommand> (
            elementCommands, "1.5.0",
            "Creates Stair elements based on the given baseline and parameters, optionally with the segment type of each baseline edge "
            "and the turn at each baseline point (landings and winders). Accepts the same settings as ModifyStairs: the step values with their locks, "
            "the finish and structure visibility, the extra treads, the walking line, the numbering, the walking direction, and the tread, riser "
            "and structure settings of the flights and the landings; settings not given come from the Stair tool defaults or the favorite."
        );
        err |= RegisterCommand<SolveStairBaselineCommand> (
            elementCommands, "1.7.3",
            "Runs the Archicad stair solver on the given Stair baselines and parameters without creating any element. "
            "Returns for each one whether the baseline was already correct, could be solved or could not be solved, "
            "with the baseline and the main stair settings after solving. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateSlabsCommand> (
            elementCommands, "1.0.3",
            "Creates Slab elements based on the given parameters. Takes the same surfaces (top, side, bottom), cut fill pens and floor fill as ModifySlabs, besides the outline, holes, level and structure; "
            "settings not given come from the Slab tool defaults or the favorite. Not written by either command: the surface of a single edge, and the cut fill pens before Archicad 27; "
            "of the floor fill orientation Archicad applies only the type through the API."
        );
        err |= RegisterCommand<CreateWindowsCommand> (
            elementCommands, "1.4.0",
            "Creates Window elements in host walls based on the given parameters."
        );
        err |= RegisterCommand<CreateDoorsCommand> (
            elementCommands, "1.4.0",
            "Creates Door elements in host walls based on the given parameters."
        );
        err |= RegisterCommand<CreateOpeningsCommand> (
            elementCommands, "1.4.0",
            "Creates Opening elements in the given host elements."
        );
        err |= RegisterCommand<GetOpeningDefaultCommand> (
            elementCommands, "1.7.3",
            "Returns the settings of the Opening tool defaults: the extrusion parameters (as GetOpeningDetails returns them) and the floor plan parameters (as GetOpeningFloorPlanParameters returns them). Available from Archicad 29."
        );
        err |= RegisterCommand<SetOpeningDefaultCommand> (
            elementCommands, "1.8.0",
            "Changes the settings of the Opening tool defaults, with the names GetOpeningDefault returns. Only the given settings change. Available from Archicad 29."
        );
        err |= RegisterCommand<PlaceOpeningsCommand> (
            elementCommands, "1.8.0",
            "Places openings with the Opening tool settings at points of their parent elements. A point off the parent element's surface is moved to the closest point of the surface. Each opening can be given its own extrusion and floor plan settings, which change only that opening and not the tool's settings. Available from Archicad 29."
        );
        err |= RegisterCommand<GetOpeningFloorPlanParametersCommand> (
            elementCommands, "1.7.3",
            "Returns the floor plan parameters of Opening elements: the floor plan display mode, the divided opening mode, the open edge style, the reference axis, the cut surface and outline lines, the cover fills and the cut and uncut symbols. Available from Archicad 29."
        );
        err |= RegisterCommand<SetOpeningFloorPlanParametersCommand> (
            elementCommands, "1.7.3",
            "Changes the floor plan parameters of Opening elements returned by GetOpeningFloorPlanParameters, and sets or removes their cut and uncut symbols. Available from Archicad 29."
        );
        err |= RegisterCommand<GetOpeningSymbolParametersCommand> (
            elementCommands, "1.8.0",
            "Returns the GDL parameters of the cut and uncut floor plan symbols of Opening elements, in the format of GetGDLParametersOfElements. A symbol the opening does not have is left out. Available from Archicad 29."
        );
        err |= RegisterCommand<SetOpeningSymbolParametersCommand> (
            elementCommands, "1.8.0",
            "Changes the GDL parameters of the cut and uncut floor plan symbols of Opening elements, in the format of SetGDLParametersOfElements. Only the given parameters are changed. Available from Archicad 29."
        );
        err |= RegisterCommand<GetOpeningDetailsCommand> (
            elementCommands, "1.7.3",
            "Returns the details of Opening elements: whether they exist, the parent element, the cut and connected elements, the geometry (anchor point, extrusion direction, axes, polygon) and the extrusion parameters (constraint, limit type, linked status, shape type, size, anchor, offsets, surface overrides). Available from Archicad 29."
        );
        err |= RegisterCommand<SetOpeningDetailsCommand> (
            elementCommands, "1.7.3",
            "Changes the details of Opening elements: the parent element, the cut elements, the polygon, a transformation of the geometry and the extrusion parameters (constraint, limit type, linked status, shape type, size, anchor, offsets, surface overrides) returned by GetOpeningDetails. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateMorphsCommand> (
            elementCommands, "1.4.0",
            "Creates Morph elements from simple box definitions or arbitrary bodies. Each item may name a favorite (favoriteName) to start from, a layer (layerIndex) and an Element ID (id); the given geometry and floor are kept."
        );
        err |= RegisterCommand<CreateRoofsCommand> (
            elementCommands, "1.4.0",
            "Creates Roof elements based on footprint, level and roof profile data. Creates a multi-plane roof by default; pass 'pivotLine' (and optionally 'angle') to create a single-plane roof instead."
        );
        err |= RegisterCommand<CreateShellsCommand> (
            elementCommands, "2.0.0",
            "Creates extruded, revolved and ruled Shell elements from profile polylines, with thickness, structure, an optional clipping contour and holes."
        );
        err |= RegisterCommand<CreateSkylightsCommand> (
            elementCommands, "2.0.0",
            "Creates Skylight elements with a roof or shell owner and a floor plan anchor position. It takes the same settings as ModifySkylights (anchor point, fix mode, width, height, azimuth and elevation angles, mirroring, floor plan display, pens, line types, section fill, surface and the library part flags) and optionally a library part; the settings not given come from the Skylight tool defaults or a favorite. Surface placement is handled by Archicad and has not yet been verified for this command."
        );
        err |= RegisterCommand<ModifySkylightsCommand> (
            elementCommands, "2.0.0",
            "Modifies Skylight elements: anchor position and anchor point, fix mode, width, height, azimuth and elevation angles, mirroring, floor plan display option, pens, line types, section fill and pens, surface and the use-the-library-part's-settings flags - the same settings CreateSkylights takes and GetDetailsOfElements returns. Only the given settings change. Not written by this command: the owner roof or shell, the anchor level and the vertical link (Archicad places the skylight on its owner), the plane of a multi-plane roof it sits on, the dimension marker (a separate element) and the library part's own parameters (use SetGDLParametersOfElements). Archicad may set the elevation angle again from the owner."
        );
        err |= RegisterCommand<CreateRailingsCommand> (
            elementCommands, "1.8.0",
            "Creates Railing elements along a reference polyline. Posts, rails, handrails, toprails, panels and balusters come from the favorite, from an existing railing (templateElementId) or from the Railing tool defaults; Archicad generates one node per vertex and one segment per pair of consecutive vertices."
        );
        err |= RegisterCommand<CreateRailingsFromPolylineCommand> (
            elementCommands, "2.0.0",
            "Creates Railing elements along a 3D reference polyline: each node has its own elevation and segments can be arcs. Posts, rails, handrails, toprails, panels and balusters come from the favorite or from the Railing tool defaults."
        );
        err |= RegisterCommand<CreateCurtainWallsCommand> (
            elementCommands, "1.7.2",
            "Creates Curtain Wall elements along a reference polyline with a column (primary) and row (secondary) pattern. The frame and panel classes come from the favorite or from the Curtain Wall tool defaults; the pattern cells can assign them to the grid."
        );
        err |= RegisterCommand<CreateAssociativeDimensionsCommand> (
            elementCommands, "1.4.0",
            "Creates associative dimensions from explicit witness point references: linear by default, or cumulative, baseline or elevation dimensions (appearance). A witness point can also be static, given by its coordinate instead of an element."
        );
        err |= RegisterCommand<CreateAssociativeDimensionsOnSectionCommand> (
            elementCommands, "1.4.0",
            "Creates associative dimensions on section elements using common wall, slab, beam, column, door and window presets: linear by default, or cumulative, baseline or elevation dimensions (appearance), e.g. level marks. The preset points of multiple section elements can be merged into one continuous dimension chain via sectionElementIds. For Opening tool elements, which Archicad does not associate with dimensions, the OpeningEdges preset places fixed points at their edges instead."
        );
        err |= RegisterCommand<CreateWallThicknessDimensionsCommand> (
            elementCommands, "1.4.0",
            "Creates associative wall thickness dimensions for the given walls."
        );
        err |= RegisterCommand<ModifyAssociativeDimensionsCommand> (
            elementCommands, "2.0.0",
            "Changes the settings of existing linear dimensions (the Dimension tool's elements, also in sections): the dimension type, the elevation origin, the line pen, the marker type and pen, the text height, pen and frame and the witness line form and value, with the same keys as the dimension creation commands. The text and witness line settings are applied to every witness point. Not written: the witness points, the dimension line position and direction (use MoveElements), the marker size, the text content and position, the witness extension line, the layout and the hole and composite wall dimensioning settings."
        );
        err |= RegisterCommand<GetDimensionDataCommand> (
            elementCommands, "1.5.0",
            "Gets the data of existing dimension chains: the line pen, the marker type and pen and the default witness line, and per witness point the coordinates, measured value, witness line and the text height, pen and frame."
        );
        err |= RegisterCommand<GetOpeningTransformationsCommand> (
            elementCommands, "1.7.3",
            "Gets the placement of Opening elements, Windows, Doors and Skylights without changing them: a global-to-local transformation matrix, its inverse, and the origin and axes in world coordinates derived from the inverse. For Windows, Doors and Skylights the local-to-global matrix is the one Archicad returns, and mirrored ones give a left-handed matrix (isMirrored). For Opening elements it is built from the opening geometry (Archicad 29 and later): origin at the anchor point, the X and Y axes of the opening and Z = X × Y, which points against the extrusion direction. Needs Archicad 27 or later."
        );
        err |= RegisterCommand<CreateZonesCommand> (
            elementCommands, "1.1.8",
            "Creates Zone elements based on the given parameters. Takes the same zone settings as ModifyZones (name, number, category, zone stamp, levels, contour line and cover fill), applied by the same helpers; the polygon or reference point is only set here. An item whose area would overlap an existing Zone on the same story fails with an error naming that Zone instead of opening Archicad's modal overlap warning, unless its allowOverlap is true."
        );
        err |= RegisterCommand<CreatePolylinesCommand> (
            elementCommands, "1.1.5",
            "Creates Polyline elements based on the given parameters."
        );
        err |= RegisterCommand<CreateLineElementsCommand> (
            elementCommands, "1.5.6",
            "Creates Line elements based on the given parameters."
        );
        err |= RegisterCommand<CreateArcsCommand> (
            elementCommands, "1.5.6",
            "Creates Arc elements based on the given parameters."
        );
        err |= RegisterCommand<CreateCirclesCommand> (
            elementCommands, "1.5.6",
            "Creates Circle elements based on the given parameters."
        );
        err |= RegisterCommand<CreateHotspotsCommand> (
            elementCommands, "1.5.6",
            "Creates Hotspot elements based on the given parameters."
        );
        err |= RegisterCommand<CreateHatchesCommand> (
            elementCommands, "1.5.6",
            "Creates Hatch elements based on the given parameters. A hatch can have holes, in the same format SetDetailsOfElements accepts for hatches."
        );
        err |= RegisterCommand<CreateSplinesCommand> (
            elementCommands, "1.5.6",
            "Creates Spline elements based on the given parameters."
        );
        err |= RegisterCommand<CreateObjectsCommand> (
            elementCommands, "1.5.7",
            "Creates Object elements based on the given parameters. A library part named in libraryPartName starts from its own default size and GDL parameters, not from those of the part the Object tool holds; the dimensions and the GDL parameters given (by name, in parameters) override them. layerIndex places the element on a layer. With databaseId the objects are placed in that database, e.g. a worksheet, instead of the current one."
        );
        err |= RegisterCommand<CreateLampsCommand> (
            elementCommands, "1.5.7",
            "Creates Lamp elements based on the given parameters. A library part named in libraryPartName starts from its own default size and GDL parameters, not from those of the part the Lamp tool holds; the dimensions and the GDL parameters given (by name, in parameters) override them. layerIndex places the element on a layer. With databaseId the lamps are placed in that database, e.g. a worksheet, instead of the current one."
        );
        err |= RegisterCommand<GetGridElementDefaultsCommand> (
            elementCommands, "1.7.2",
            "Returns the Grid Tool defaults: library part, size and GDL parameters of a new grid element. Needs Archicad 26 or later."
        );
        err |= RegisterCommand<GetGridElementsCommand> (
            elementCommands, "1.7.2",
            "Lists the grid elements (placed with the Grid Tool or created by CreateGridElements) with their position, angle, size, library part and optionally GDL parameters. Needs Archicad 26 or later."
        );
        err |= RegisterCommand<CreateGridElementsCommand> (
            elementCommands, "1.7.2",
            "Creates grid elements, the elements the Grid Tool places, from begin and end coordinates, with optional story, layer and GDL parameters. Needs Archicad 26 or later."
        );
        err |= RegisterCommand<CreateMeshesCommand> (
            elementCommands, "1.1.9",
            "Creates Mesh elements based on the given parameters."
        );
        err |= RegisterCommand<CreateLabelsCommand> (
            elementCommands, "1.2.5",
            "Creates Label elements based on the given parameters."
        );
        err |= RegisterCommand<CreateTextsCommand> (
            elementCommands, "1.5.0",
            "Creates standalone Text elements based on the given parameters."
        );
        err |= RegisterCommand<CreateLevelDimensionsCommand> (
            elementCommands, "1.7.2",
            "Creates Level Dimension elements (the Level Dimension tool) based on the given parameters. With a parentElementId the dimension is associative: it shows the height of the parent element (Mesh, Slab, Roof ...) at its position and follows the element when it changes."
        );
        err |= RegisterCommand<ModifyLevelDimensionsCommand> (
            elementCommands, "2.0.0",
            "Changes existing Level Dimension elements: the position of the marker and the settings CreateLevelDimensions takes (pen, text height and pen, marker type and size, plus sign, angle, origin), with the same keys. Not written: the parent element and the static level (delete and recreate the dimension to change them), the text content and the hide-with-base-element setting."
        );
        err |= RegisterCommand<ModifyWallsCommand> (
            elementCommands, "1.4.0",
            "Modifies Wall elements based on the given parameters. Takes the same wall settings as CreateWalls, applied by the same code, plus geometryType (Straight or Trapezoid), profileType and bottomOffset. Neither command can write a Polygonal outline (it needs an outline polygon, which these commands do not take), connectionPriority or junctionSequence (Archicad ignores writes to them; GetDetailsOfElements reads them), and on Archicad 29 any story visibility other than 'Home story only' and 'All relevant stories'."
        );
        err |= RegisterCommand<ModifyBeamsCommand> (
            elementCommands, "1.4.0",
            "Modifies Beam elements based on the given parameters. CreateBeams takes the same cut fill pens and cover fill, and the same absolute zCoordinate of the reference line (with an optional floorIndex). Not written: the surfaces of the beam segments, and the cut fill pens before Archicad 27."
        );
        err |= RegisterCommand<ModifySlabsCommand> (
            elementCommands, "1.4.0",
            "Modifies Slab elements based on the given parameters. CreateSlabs takes the same surfaces, cut fill pens and floor fill. Not written: the surface of a single edge, and the cut fill pens before Archicad 27. Of the cover fill orientation, Archicad applies only the type to a Slab through the API; a given origin or distortion matrix that did not stick gets a failed execution result, while the other details are applied."
        );
        err |= RegisterCommand<ModifyColumnsCommand> (
            elementCommands, "1.4.0",
            "Modifies Column elements based on the given parameters. CreateColumns takes the same cut fill pens and cover fill. Not written: the surfaces of the column segments, and the cut fill pens before Archicad 27."
        );
        err |= RegisterCommand<ModifyWindowsCommand> (
            elementCommands, "1.4.0",
            "Modifies Window elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyDoorsCommand> (
            elementCommands, "1.4.0",
            "Modifies Door elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyOpeningsCommand> (
            elementCommands, "2.0.0",
            "Modifies Opening elements: the width, the height and the base point CreateOpenings takes. Needs Archicad 28 or earlier; in Archicad 29 and later every item fails with a message pointing to SetOpeningDetails, which changes the size and the position of an opening there."
        );
        err |= RegisterCommand<ModifyMorphsCommand> (
            elementCommands, "1.4.0",
            "Modifies Morph elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyRoofsCommand> (
            elementCommands, "1.4.0",
            "Modifies single-plane and multi-plane Roof elements based on the given parameters. The plan outline, level, thickness and structure apply to both; angle, pivotLine and edgeTrims (the cut of each polygon edge) only to single-plane roofs; levels, eavesOverhang and pivotEdges only to multi-plane roofs."
        );
        err |= RegisterCommand<ModifyShellsCommand> (
            elementCommands, "1.8.0",
            "Modifies the structure (Basic or Composite, building material or composite) of Shell elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyMeshesCommand> (
            elementCommands, "1.5.4",
            "Modifies the attributes of Mesh elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyRailingsCommand> (
            elementCommands, "2.0.0",
            "Modifies Railing elements in place: the height of their segments, the bottom offset and the home story. The reference line, the nodes and segments, and the posts, rails and panels stay as they are. Each railing is read back after the change, and a value Archicad did not apply gets a failed execution result."
        );
        err |= RegisterCommand<ModifyObjectsCommand> (
            elementCommands, "1.5.7",
            "Modifies Object elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyLampsCommand> (
            elementCommands, "1.5.7",
            "Modifies Lamp elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyTextsCommand> (
            elementCommands, "1.5.9",
            "Modifies standalone Text elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyLabelsCommand> (
            elementCommands, "1.5.9",
            "Modifies Label elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyCurtainWallsCommand> (
            elementCommands, "1.8.0",
            "Modifies Curtain Wall elements in place, keeping their GUID, properties and classifications: the height, the bottom elevation and story, the reference line (with the same number of segments), and the column (primary) and row (secondary) pattern with its cell assignments, in the fields of CreateCurtainWalls."
        );
        err |= RegisterCommand<ModifyCurtainWallFramesCommand> (
            elementCommands, "2.0.0",
            "EXPERIMENTAL. Modifies single Curtain Wall Frames by their GUID: the frame contour (a1, a2, b1, b2), the frame class and the frame type. ACAPI_Element_Change does not take a frame on its own, so the change is written through the memo of the owner curtain wall, and read back to check that Archicad applied it."
        );
        err |= RegisterCommand<ModifyStairsCommand> (
            elementCommands, "2.0.0",
            "Modifies Stair elements in place, keeping their baseline and position: the flight width, total height, number of steps, riser height and going with their locks, the finish and structure visibility, "
            "the extra top and bottom treads, the walking line position and offset, the step numbering, the walking direction, the uniform finish, monolith and draining settings, "
            "and for the flights and the landings the tread and riser settings (presence, thickness, offset, nosing, riser cross section and position) and the structure of each side (presence, type, underside shape and thickness). "
            "CreateStairs accepts the same settings. Only the given settings are changed. Archicad solves the stair again within its baseline, so it may adjust the step values; read them back with GetDetailsOfElements. "
            "Not written by this command: the baseline with its landings and winders, the top link, the floor plan display (symbol and marker layout), the headroom and the stair rules; "
            "the subelements (treads, risers, structures) take the per-part settings above and cannot be written one by one through the API."
        );
        err |= RegisterCommand<ChangeTextContentCommand> (
            elementCommands, "1.8.0",
            "Replaces a range of the content of Text elements and text Labels, keeping the styles of the runs and the autotexts in the rest of the content."
        );
        err |= RegisterCommand<ModifyZonesCommand> (
            elementCommands, "1.8.0",
            "Modifies Zone elements: name, number, category, zone stamp, height, base offset, top link, floor thickness, contour line and cover fill - the same settings as CreateZones, applied by the same helpers. Only the given fields are changed. The polygon of a manual zone cannot be changed here (the change takes no polygon memo), nor the reference point of an automatic one (SetDetailsOfElements moves it)."
        );
        err |=RegisterCommand<MeasureTextsCommand> (
            elementCommands, "1.7.3",
            "Measures texts before placing them: the length of every line for a given font, style and height, on paper in millimetres or in the model at the current drawing scale, and the font's capital size factor. Creates nothing."
        );
        err |= RegisterCommand<GetElementPreviewImageCommand> (
            elementCommands, "1.2.7",
            "Returns the preview image of the given element."
        );
        err |= RegisterCommand<GetRoomImageCommand> (
            elementCommands, "1.2.7",
            "Returns the room image of the given zone."
        );
        err |= RegisterCommand<GetModelExtentCommand> (
            elementCommands, "2.0.0",
            "Returns the 3D extent (xMin, yMin, zMin, xMax, yMax, zMax) of the current database, the region Archicad's Fit in Window would show."
        );
        err |= RegisterCommand<AddElementNotificationClientCommand> (
            elementCommands, "1.2.8",
            "Sets up a new notification client to receive element events."
        );
        err |= RegisterCommand<RemoveElementNotificationClientCommand> (
            elementCommands, "1.2.8",
            "Removes an element notification client."
        );
        err |= RegisterCommand<RegularizePolygonsCommand> (
            elementCommands, "1.7.3",
            "Regularizes polygons with Archicad's own polygon regularization, before they are used to create slabs, zones or fills: removes duplicate points, fixes the orientation, resolves self-intersections and touching holes. Each input polygon gives 0, 1 or more valid polygons in the format of CreateSlabs, optionally with the input vertex and edge each result vertex and edge came from. Changes nothing in the project."
        );
        err |= RegisterCommand<RegularizePolylinesCommand> (
            elementCommands, "1.7.3",
            "Regularizes polylines with Archicad's own polyline regularization: removes repeated consecutive points. The result is in the format of CreatePolylines and may be empty, optionally with the input point and segment each result point and segment came from. Changes nothing in the project."
        );
        err |= RegisterCommand<GetSplineMaxDistancesCommand> (
            elementCommands, "2.0.0",
            "Measures how far the given 2D coordinates are from a segment of a Spline element, between two of its fit points, with its own Bezier handles: returns the largest distance, for example to check how well a polyline follows the spline. The output is aligned with the input array. Changes nothing in the project."
        );
        AddCommandGroup (elementCommands);
    }

    { // Element grouping Commands
        CommandGroup elementGroupingCommands ("Element grouping Commands");
        err |= RegisterCommand<CreateGroupsCommand> (
            elementGroupingCommands, "1.4.0",
            "Creates groups of the passed elements"
        );
        err |= RegisterCommand<UngroupElementsCommand> (
            elementGroupingCommands, "1.7.3",
            "Ungroups the given groups one level, like Edit > Grouping > Ungroup: each group is dissolved, the elements and subgroups it directly contains stay. A group nested in another group is reported as failed; ungroup the containing group first. Returns an execution result per group."
        );
        err |= RegisterCommand<GetGroupsOfElementsCommand> (
            elementGroupingCommands, "1.5.6",
            "Gets the identifier of the group that directly contains each given element. Returns an error for elements that are not part of any group."
        );
        err |= RegisterCommand<GetElementsOfGroupsCommand> (
            elementGroupingCommands, "1.5.6",
            "Gets the elements directly contained by each given group."
        );
        err |= RegisterCommand<GetMainGroupsCommand> (
            elementGroupingCommands, "1.8.0",
            "Gets the top level group of each given group. A group that is not nested in another group is its own top level group."
        );
        err |= RegisterCommand<GetAllElementsOfGroupsCommand> (
            elementGroupingCommands, "1.8.0",
            "Gets all the elements of each given group, including the elements of its nested groups."
        );
        err |= RegisterCommand<GetAutoGroupModeCommand> (
            elementGroupingCommands, "1.8.0",
            "Gets the current state of the Auto Group mode."
        );
        err |= RegisterCommand<GetElementSetDataCommand> (
            elementGroupingCommands, "1.8.0",
            "Gets the elements and the custom data of each given element set. Element sets are created by add-ons to link elements together; the custom data is returned as Base64 encoded bytes."
        );
        err |= RegisterCommand<GetUserDataOfGroupsCommand> (
            elementGroupingCommands, "1.7.3",
            "Gets the user data an add-on attached to each given group: its version, platform sign, flags and bytes (base64 encoded), and, when its version is 1 (the layout SetUserDataOfGroups writes), its key-value pairs. Returns an error for groups that have no user data."
        );
        err |= RegisterCommand<SetUserDataOfGroupsCommand> (
            elementGroupingCommands, "1.8.0",
            "Stores string values under keys in the user data of groups, for scripts to keep their own state on a group. The given pairs are merged into what each group already holds: a key that exists gets the new value, other keys are left as they are, and the keys listed in keysToDelete are removed. Prefix the keys with the script's name so that scripts do not overwrite each other's data. User data in another layout is left unchanged and reported as an error. One undo step per call."
        );
        err |= RegisterCommand<GetSuspendGroupsModeCommand> (
            elementGroupingCommands, "1.5.6",
            "Gets the current state of the Suspend Groups mode."
        );
        err |= RegisterCommand<SetSuspendGroupsModeCommand> (
            elementGroupingCommands, "1.5.6",
            "Turns the Suspend Groups mode on or off. Suspend groups to perform operations on elements that are part of a group; remember to restore the previous state afterwards."
        );
        err |= RegisterCommand<CreateElementSetsCommand> (
            elementGroupingCommands, "1.8.0",
            "Creates element sets of the given elements. An element set is a hidden, persistent collection of elements (it is not a group and has no effect on selection or editing)."
        );
        err |= RegisterCommand<DeleteElementSetsCommand> (
            elementGroupingCommands, "1.8.0",
            "Deletes the given element sets. The elements of a set are not deleted. Only element sets created by the Tapir Add-On can be deleted; sets created by other add-ons are reported as failed. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each element set that a real run would delete gets wouldDelete."
        );
        err |= RegisterCommand<GetElementSetsOfElementsCommand> (
            elementGroupingCommands, "1.7.3",
            "Gets the element sets that contain each given element. Only element sets created by the Tapir Add-On are examined; sets created by other add-ons are not found."
        );
        AddCommandGroup (elementGroupingCommands);
    }

    { // Favorites Commands
        CommandGroup favoritesCommands ("Favorites Commands");
        err |= RegisterCommand<GetFavoritesByTypeCommand> (
            favoritesCommands, "1.2.2",
            "Returns a list of the names of all favorites with the given element type"
        );
        err |= RegisterCommand<GetAllFavoritesCommand> (
            favoritesCommands, "1.7.3",
            "Returns every Favorite of the project, or only those of the given element type, with its name and element type. "
            "Available from Archicad 29."
        );
        err |= RegisterCommand<GetFavoritePreviewImageCommand> (
            favoritesCommands, "1.2.7",
            "Returns the preview image of the given favorite."
        );
        err |= RegisterCommand<GetFavoriteDetailsCommand> (
            favoritesCommands, "1.8.0",
            "Returns the classifications, property values and element category values stored in the given favorites, "
            "so they can be audited before elements are placed. Each property value comes with its evaluation status "
            "and default-value flag; the value itself is only returned when it has been evaluated."
        );
        err |= RegisterCommand<ApplyFavoritesToElementDefaultsCommand> (
            favoritesCommands, "1.1.2",
            "Apply the given favorites to element defaults."
        );
        err |= RegisterCommand<CreateFavoritesFromElementsCommand> (
            favoritesCommands, "1.1.2",
            "Create favorites from the given elements."
        );
        err |= RegisterCommand<ImportFavoritesCommand> (
            favoritesCommands, "1.5.0",
            "Import Favorites from a .prefs file or folder into the current project. From Archicad 29 they can also be "
            "imported from a folder of Favorite files (xml format, all of them or the given fileNames) or from a legacy "
            ".prf file (legacyPRF format), with the conflict policies Error, Skip, OverwriteExisting and RenameNew. "
            "In a Teamwork project the Favorites lockable object set must be reserved first (see ReserveLockableObjectSet)."
        );
        err |= RegisterCommand<ExportFavoritesCommand> (
            favoritesCommands, "1.5.0",
            "Export the project's Favorites to a .prefs file or folder. From Archicad 29 they can also be "
            "exported to a folder of Favorite files (xml format) or to a legacy .prf file (legacyPRF format). "
            "With overwrite false an export to a single file refuses to replace an existing file."
        );
        err |= RegisterCommand<ApplyFavoritesToElementsCommand> (
            favoritesCommands, "1.5.4",
            "Apply the given favorites to existing elements. Only settings-type parameters are changed - "
            "geometry (position, floor, and dimensions such as a Wall's height) is left untouched, so applying "
            "a Favorite never moves the target element. A Window or Door keeps its wall, marker and placement, "
            "and takes the Favorite's library part and parameters, size included. For the "
            "hierarchical types (Stair, Railing, Curtain Wall) the settings of the sub-elements are not applied, "
            "because they are inseparable from the Favorite's own geometry. By default settings, classifications, "
            "categories and properties are all applied; each can be opted out of individually. "
            "keepFields lists fields of the target (its ID, layer, an opening's width, height and sill "
            "height, a Wall's reference line location, height and top link) that are copied back after "
            "the Favorite has been applied, so that e.g. only the library part or the wall type changes. "
            "With favorite instead of favoritesToApply, one Favorite is read once and applied to the given elements, "
            "or to every element of its type that matches the filter."
        );
        err |= RegisterCommand<UpdateFavoritesFromElementsCommand> (
            favoritesCommands, "1.5.4",
            "Update existing favorites from the given elements."
        );
        err |= RegisterCommand<RenameFavoritesCommand> (
            favoritesCommands, "1.5.4",
            "Rename existing favorites."
        );
        err |= RegisterCommand<DeleteFavoritesCommand> (
            favoritesCommands, "1.5.4",
            "Delete existing favorites."
        );
        err |= RegisterCommand<GetFavoriteFoldersCommand> (
            favoritesCommands, "1.8.0",
            "Returns the folder tree of the Favorites palette from its root folder: the name and path of every folder "
            "and the names of the Favorites in it. Available from Archicad 29."
        );
        err |= RegisterCommand<ReadFavoritesFileCommand> (
            favoritesCommands, "2.0.0",
            "Read the Favorites of a .prefs file or folder without importing them, returning the name, "
            "element type and folder of each. Nothing in the project changes, so the result can be "
            "compared with the project's Favorites before ImportFavorites."
        );
        err |= RegisterCommand<CreateFavoriteFolderCommand> (
            favoritesCommands, "1.8.0",
            "Creates a folder in the Favorites palette, in the parent folder given by its path. Returns the new folder. "
            "Available from Archicad 29."
        );
        err |= RegisterCommand<RenameFavoriteFolderCommand> (
            favoritesCommands, "1.8.0",
            "Renames a folder of the Favorites palette given by its path. The root folder cannot be renamed. Returns the renamed folder. "
            "Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteFavoriteFolderCommand> (
            favoritesCommands, "1.8.0",
            "Deletes a folder of the Favorites palette given by its path. Archicad deletes the folder's Favorites and subfolders with it, "
            "so a folder that is not empty is deleted only if recursive is true. The root folder cannot be deleted. "
            "Returns the deleted folder. Available from Archicad 29."
        );
        err |= RegisterCommand<GetFavoriteSettingsCommand> (
            favoritesCommands, "1.7.3",
            "Reads the stored settings of the given favorites without applying them or placing an element: "
            "the element type, the folder, the library part and GDL parameters, the marker (library part and "
            "parameters) of Window and Door favorites, and the sub-elements. A favorite without a marker has no "
            "marker field, which is not an error. The other settings of the element (pens, layer, structure, etc.) "
            "are not returned."
        );
        err |= RegisterCommand<CreateFavoriteCommand> (
            favoritesCommands, "1.8.0",
            "Creates a Favorite in the given folder from the settings of another Favorite or of the current Opening tool "
            "default. conflictPolicy says what happens when the name is taken: fail, skip, overwrite the existing "
            "Favorite's settings or create the new one under a free name. Available from Archicad 29."
        );
        err |= RegisterCommand<ChangeFavoriteCommand> (
            favoritesCommands, "1.8.0",
            "Gives an existing Favorite the settings of another Favorite or of the current Opening tool default. "
            "Available from Archicad 29."
        );
        err |= RegisterCommand<ApplyFavoriteToToolDefaultCommand> (
            favoritesCommands, "1.8.0",
            "Makes the settings of the given Favorite the settings of its tool, and returns the element type of that tool. "
            "Available from Archicad 29."
        );
        err |= RegisterCommand<MoveFavoriteFolderCommand> (
            favoritesCommands, "1.8.0",
            "Moves a folder of the Favorites palette, with its Favorites and subfolders, into another folder. Folders are "
            "given as their path of folder names from the root. Returns the new path of the folder. Available from Archicad 29."
        );
        err |= RegisterCommand<GetFavoriteContentsCommand> (
            favoritesCommands, "2.0.0",
            "Returns the Favorites of a folder of the Favorites palette and of its subfolders (every Favorite when no folder "
            "is given), each with its folder path, classifications, element category values and property values, read through "
            "the Favorite manager. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteFavoritesWithManagerCommand> (
            favoritesCommands, "2.0.0",
            "Deletes the given Favorites through the Favorite manager, each found by name, optionally only in the given folder. "
            "One execution result for each. Available from Archicad 29."
        );
        err |= RegisterCommand<MoveFavoritesCommand> (
            favoritesCommands, "1.8.0",
            "Moves Favorites given by their names into a folder of the Favorites palette given by its path. "
            "Returns an execution result for each Favorite. Available from Archicad 29."
        );
        err |= RegisterCommand<GetFavoriteDefaultsCommand> (
            favoritesCommands, "2.0.0",
            "Reads the element default stored in each given Favorite through the Favorite manager, without applying it or "
            "placing an element: the element type, the layer, the element ID, the folder, the library part and GDL parameters, "
            "the marker of every element type that has one, and the sub-elements with each of their kinds (Curtain Wall "
            "frames and panels, Stair treads and risers, Railing posts and rails, markers ...). Available from Archicad 29."
        );
        err |= RegisterCommand<SearchFavoritesCommand> (
            favoritesCommands, "2.0.0",
            "Searches project favorites by keyword (matching favorite name or folder) and/or element type."
        );
        AddCommandGroup (favoritesCommands);
    }

    { // Property Commands
        CommandGroup propertyCommands ("Property Commands");
        err |= RegisterCommand<GetAllPropertiesCommand> (
            propertyCommands, "1.1.3",
            "Returns all user defined and built-in properties. Optionally only the given properties or property types, and without the availability lists."
        );
        err |= RegisterCommand<SearchPropertiesCommand> (
            propertyCommands, "2.0.0",
            "Searches properties by name or group substring with optional filtering by property type, value type, and collection type."
        );
        err |= RegisterCommand<GetPropertyValuesOfElementsCommand> (
            propertyCommands, "1.0.6",
            "Returns the property values of the elements for the given property. It works for subelements of hierarchal elements also. With includeAvailability it also tells whether each property is available for the element."
        );
        err |= RegisterCommand<SetPropertyValuesOfElementsCommand> (
            propertyCommands, "1.0.6",
            "Sets the property values of elements. It works for subelements of hierarchal elements also. With valueStatus an element's value can also be reset to the default value of the property (Default) or made undefined (UserUndefined, from 1.8.1). With dryRun true nothing is changed and no undo step is created: the response has dryRun set to true, and each item that passes the checks made before writing gets wouldChange, with the element's value now (oldValue) and the value a real run would write (newValue), both as GetPropertyValuesOfElements gives them."
        );
        err |= RegisterCommand<GetComponentsOfElementsCommand> (
            propertyCommands, "2.0.0",
            "Returns the components of the elements: the building material skins of composite walls, slabs, roofs and shells, and the parts of complex profile walls, beams and columns. The components have their own property values (building material, thickness, area, volume and the properties of the building material); read them with GetPropertyValuesOfElementComponents."
        );
        err |= RegisterCommand<GetPropertyDefinitionsOfElementComponentsCommand> (
            propertyCommands, "2.0.0",
            "Returns the property definitions available on the given element components, optionally filtered to user defined or built-in ones."
        );
        err |= RegisterCommand<GetPropertyValuesOfElementComponentsCommand> (
            propertyCommands, "2.0.0",
            "Returns the property values of the given element components for the given properties, in the shape of GetPropertyValuesOfElements. Use GetComponentsOfElements to list the components of an element."
        );
        err |= RegisterCommand<SetPropertyValuesOfElementDefaultsCommand> (
            propertyCommands, "2.0.0",
            "Sets the property values of the tool defaults (the settings new elements get) of element types, so every element placed afterwards gets them, without saving and applying a favorite."
        );
        err |= RegisterCommand<GetPropertyValuesOfAttributesCommand> (
            propertyCommands, "1.1.8",
            "Returns the property values of the attributes for the given property. With includeStatus it also returns whether each property is available, editable and visible for the attribute (from 1.7.3)."
        );
        err |= RegisterCommand<SetPropertyValuesOfAttributesCommand> (
            propertyCommands, "1.1.8",
            "Sets the property values of attributes."
        );
        err |= RegisterCommand<SetBuiltInPropertyValuesOfAttributesCommand> (
            propertyCommands, "1.8.0",
            "Sets built-in property values of attributes, writing the values of each attribute together. Custom properties are refused; set them with SetPropertyValuesOfAttributes. Returns one execution result per value."
        );
        err |= RegisterCommand<SetPropertyValueOfAttributesCommand> (
            propertyCommands, "2.0.0",
            "Sets one property value on several attributes (for example Building Materials) in one undo step. If the property is not available for any of the given attributes, no attribute is changed."
        );
        err |= RegisterCommand<GetPropertyDefinitionsOfAttributesCommand> (
            propertyCommands, "1.7.3",
            "Returns the property definitions of the given attributes (for example Building Materials): the user defined, the fundamental built-in, or all of them."
        );
        err |= RegisterCommand<GetPropertiesOfToolDefaultsCommand> (
            propertyCommands, "1.8.0",
            "Returns the property definitions of the tool defaults (the settings new elements get) of the given element types, filtered by kind (all by default), and optionally the value of each property in the tool default."
        );
        err |= RegisterCommand<AddPropertyToAttributesCommand> (
            propertyCommands, "1.7.3",
            "Makes an existing custom property available for the given attributes (for example Building Materials), in one undo step. Available in Archicad 25, 26 and 27 only."
        );
        err |= RegisterCommand<RemovePropertyFromAttributesCommand> (
            propertyCommands, "1.7.3",
            "Makes a custom property unavailable for the given attributes (for example Building Materials), in one undo step. Available in Archicad 25, 26 and 27 only. With dryRun true nothing is changed and no undo step is created: the response has dryRun set to true, and wouldDelete when the property and every attribute exist."
        );
        err |= RegisterCommand<CreatePropertyGroupsCommand> (
            propertyCommands, "1.0.7",
            "Creates Property Groups based on the given parameters."
        );
        err |= RegisterCommand<DeletePropertyGroupsCommand> (
            propertyCommands, "1.0.9",
            "Deletes the given Custom Property Groups. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each property group that exists gets wouldDelete."
        );
        err |= RegisterCommand<CreatePropertyDefinitionsCommand> (
            propertyCommands, "1.0.9",
            "Creates Custom Property Definitions based on the given parameters."
        );
        err |= RegisterCommand<DeletePropertyDefinitionsCommand> (
            propertyCommands, "1.0.9",
            "Deletes the given Custom Property Definitions. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each property that exists gets wouldDelete."
        );
        err |= RegisterCommand<UpdatePropertyDefinitionsCommand> (
            propertyCommands, "1.5.4",
            "Updates existing Custom Property Definitions in place, keeping their guid: name, description, group, default value or expressions, availability, and enum options (add, rename, remove, reorder)."
        );
        err |= RegisterCommand<UpdatePropertyGroupsCommand> (
            propertyCommands, "1.6.0",
            "Updates the name and/or description of existing Custom Property Groups, keeping their guid."
        );
        err |= RegisterCommand<ImportPropertiesXmlCommand> (
            propertyCommands, "1.6.0",
            "Imports a Property Manager XML export, with the given policy for names that already exist. Returns the property definitions it created and removed."
        );
        err |= RegisterCommand<GetPropertyValuesOfElementDefaultsCommand> (
            propertyCommands, "2.0.0",
            "Returns the values of the given properties in the current tool default of an element type, which new elements of that type get. It reads the native tool default of the generic variation of the type, not a Favorite that is not applied. A property that is not available for the tool default is reported as an error."
        );
        err |= RegisterCommand<ModifyPropertyValueOfElementsCommand> (
            propertyCommands, "1.8.0",
            "Sets one property value on all the given elements in a single step. If the property is not available for any of the elements, none of them is changed."
        );
        err |= RegisterCommand<GetPropertyExpressionReferencesCommand> (
            propertyCommands, "2.0.0",
            "Returns the strings that refer to the given properties in property expressions."
        );
        err |= RegisterCommand<CheckPropertyValuesCommand> (
            propertyCommands, "2.0.0",
            "Checks whether the given values are acceptable for the given property definitions, without changing anything."
        );
        err |= RegisterCommand<GetPropertyEditabilityCommand> (
            propertyCommands, "2.0.0",
            "Tells whether the values of the given properties can be modified, and whether the properties are visible, for the given elements. Also tells whether they can be modified in the tool defaults of element types (from Archicad 26) and in the default values of the property definitions themselves. Unknown elements, element types and properties are reported as errors, not as locked properties."
        );
        err |= RegisterCommand<GetPropertyAvailabilityOfDefaultsCommand> (
            propertyCommands, "1.8.0",
            "Tells whether the given properties are available and whether they are visible in the current tool default of an element type, before creating new elements. It checks the native tool default only, not a Favorite that is not applied. Unknown properties are reported as errors, not as unavailable. Available from Archicad 26."
        );
        err |= RegisterCommand<MakePropertyAvailableForElementsCommand> (
            propertyCommands, "1.7.3",
            "Makes a property definition available for all the given elements. Available in Archicad 25, 26 and 27."
        );
        err |= RegisterCommand<MakePropertyUnavailableForElementsCommand> (
            propertyCommands, "1.7.3",
            "Makes a property definition unavailable for all the given elements. Available in Archicad 25, 26 and 27."
        );
        AddCommandGroup (propertyCommands);
    }


    { // Classification Commands
        CommandGroup classificationCommands ("Classification Commands");
        err |= RegisterCommand<GetClassificationsOfElementsCommand> (
            classificationCommands, "1.0.7",
            "Returns the classification of the given elements in the given classification systems. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<GetElementsByClassificationCommand> (
            classificationCommands, "1.8.0",
            "Returns the elements classified with the given classification items, one list per item, so elements can be filtered by classification in a single call."
        );
        err |= RegisterCommand<SetClassificationsOfElementsCommand> (
            classificationCommands, "1.0.7",
            "Sets the classifications of elements. In order to set the classification of an element to unclassified, omit the classificationItemId field. It works for subelements of hierarchal elements also. With dryRun true nothing is changed and no undo step is created: the response has dryRun set to true, and each item that passes the checks made before writing gets wouldChange, with the element's classification in the system now (oldValue) and the one a real run would set (newValue)."
        );
        err |= RegisterCommand<GetClassificationsOfAttributesCommand> (
            classificationCommands, "1.8.0",
            "Returns the classifications of the given attributes (building materials, composites, surfaces, etc.) in every classification system they are classified in."
        );
        err |= RegisterCommand<SetClassificationsOfAttributesCommand> (
            classificationCommands, "1.8.0",
            "Sets the classifications of attributes (building materials, composites, surfaces, etc.). The given item replaces the attribute's current item in the same classification system. In order to set the classification of an attribute to unclassified in a system, omit the classificationItemId field."
        );
        err |= RegisterCommand<GetClassificationOfAttributesInSystemCommand> (
            classificationCommands, "1.8.0",
            "Returns the classification of the given attributes (for example Building Materials) in the given classification system. classificationItemId is omitted for an unclassified attribute."
        );
        err |= RegisterCommand<GetClassificationsOfElementDefaultsCommand> (
            classificationCommands, "1.7.3",
            "Returns the classification of the tool defaults (the settings new elements get) of the given element types in the given classification systems."
        );
        err |= RegisterCommand<SetClassificationsOfElementDefaultsCommand> (
            classificationCommands, "1.7.3",
            "Sets the classifications of the tool defaults (the settings new elements get) of the given element types. In order to set a default to unclassified, omit the classificationItemId field."
        );
        err |= RegisterCommand<GetClassificationsOfToolDefaultsCommand> (
            classificationCommands, "1.8.0",
            "Returns every classification of the tool defaults (the settings new elements get) of the given element types, in all classification systems at once. Unlike GetClassificationsOfElementDefaults it needs no list of classification systems; systems in which a default is unclassified are left out."
        );
        err |= RegisterCommand<GetAllClassificationSystemsCommand> (
            classificationCommands, "1.8.0",
            "Returns the classification systems of the project with their guid, name, description, source, version and date. Same output as the official GetAllClassificationSystems command, so scripts can rely on Tapir commands only."
        );
        err |= RegisterCommand<CreateClassificationSystemsCommand> (
            classificationCommands, "1.5.0",
            "Creates Classification Systems including Classification Items based on the given parameters."
        );
        err |= RegisterCommand<CreateClassificationItemsCommand> (
            classificationCommands, "1.5.0",
            "Creates Classification Items in the given Classification Systems based on the given parameters."
        );
        err |= RegisterCommand<DeleteClassificationSystemsCommand> (
            classificationCommands, "1.5.2",
            "Deletes the given Classification Systems. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each classification system that exists gets wouldDelete."
        );
        err |= RegisterCommand<DeleteClassificationItemsCommand> (
            classificationCommands, "1.5.2",
            "Deletes the given Classification Items. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and each classification item that exists gets wouldDelete."
        );
        err |= RegisterCommand<UpdateClassificationSystemsCommand> (
            classificationCommands, "1.6.0",
            "Updates the name, description, source, version and/or date of existing Classification Systems, keeping their guid."
        );
        err |= RegisterCommand<UpdateClassificationItemsCommand> (
            classificationCommands, "1.6.0",
            "Updates the id (code), name and/or description of existing Classification Items, keeping their guid and so the elements classified with them. Items cannot be moved to another parent."
        );
        err |= RegisterCommand<GetClassificationItemParentsCommand> (
            classificationCommands, "1.8.0",
            "Returns the parent item and the classification system of the given Classification Items; a top level item has no parent. The output is aligned with the input array of items."
        );
        err |= RegisterCommand<ImportClassificationsXmlCommand> (
            classificationCommands, "1.6.0",
            "Imports a Classification Manager XML export, with the given policies for systems and items that already exist. Returns the systems and items it created and removed."
        );
        err |= RegisterCommand<GetClassificationItemVisibilityCommand> (
            classificationCommands, "1.8.0",
            "Tells whether the given classification items are visible (offered) for the given elements, element tool defaults and attributes, such as building materials. Non-existing targets and classification items are reported as errors. Visibility does not tell whether the target is classified with the item, and an item can be visible even if its parent item is not."
        );
        AddCommandGroup (classificationCommands);
    }

    { // Attribute Commands
        CommandGroup attributeCommands ("Attribute Commands");
        err |= RegisterCommand<GetAttributesByTypeCommand> (
            attributeCommands, "1.1.3",
            "Returns the details of every attribute of the given type, including the path of the attribute folder holding it (Archicad 26 and newer). From Archicad 30 an attribute that Archicad keeps a creation record of also carries creationInfo: how (method), from where (source), by whom and when it was created."
        );
        err |= RegisterCommand<GetAttributeIndexesCommand> (
            attributeCommands, "2.0.0",
            "Reports for each given attribute type how many attributes there are, the first valid and the highest index in use, and the free indexes between them (for example those of deleted attributes). Available from Archicad 27."
        );
        err |= RegisterCommand<GetAttributesFromProjectCommand> (
            attributeCommands, "1.7.3",
            "Reads the attributes of the given types from another project or template file without opening it, for example to compare them with the attributes of the open project. Nothing is imported into the open project. The names are the legacy 8-bit names, so non-ASCII characters may not come through. Archicad does not read Layer, Surface, BuildingMaterial and MEPSystem attributes from a file that is not open: these types get an error item instead of an empty list. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteAttributesCommand> (
            attributeCommands, "1.5.4",
            "Deletes the given attributes. Returns an execution result for each input item. With dryRun true nothing is deleted: the response has dryRun set to true, and each attribute that passes the checks made before deleting (valid attribute type, attribute exists) gets wouldDelete."
        );
        err |= RegisterCommand<ImportAttributesFromProjectCommand> (
            attributeCommands, "2.0.0",
            "Imports Line, Fill, ZoneCategory, PenTable, OperationProfile, Composite, Profile and LayerCombination attributes from another project or template file into the open project, with their complete definition (symbol and percent fills included), like Append or Overwrite by name in Archicad's Attribute Manager. The lines and fills a composite or profile uses are matched by name; its building materials and surfaces, and the layers of a layer combination, keep their index only with referencesByIndex, since Archicad does not read these types from a closed file. An attribute with a reference that cannot be mapped is not imported. A new attribute gets a new guid: Archicad does not hand out the guids stored in the source file. An attribute whose name already exists is skipped, or overwritten in place with overwriteExisting. dryRun reports what would happen without changing anything. The names are the legacy 8-bit names of the source file. Available from Archicad 27."
        );
        err |= RegisterCommand<GetUsedAttributesCommand> (
            attributeCommands, "1.8.0",
            "Gets how many attributes of each type are in use (layers, lines, fills, composites, surfaces, ...), as Archicad reports them for the usedAttributes JSON query: the counts in usedAttributes, the report as text in raw. Available from Archicad 27."
        );
        err |= RegisterCommand<ConsolidateLinesCommand> (
            attributeCommands, "1.8.0",
            "Runs the Line Consolidation menu command of Archicad on the current window, as the menu does, and reports whether it ran together with what Archicad answered. Available from Archicad 27."
        );
        err |= RegisterCommand<ConsolidateFillsCommand> (
            attributeCommands, "1.8.0",
            "Runs the Fill Consolidation menu command of Archicad on the current window, as the menu does, and reports whether it ran together with what Archicad answered. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateAttributeFoldersCommand> (
            attributeCommands, "1.7.3",
            "Creates attribute folders, each given by attribute type and path (the folder names from the root folder down). Missing parent folders are created too; a folder that already exists is kept and reported as success. Available from Archicad 26."
        );
        err |= RegisterCommand<DeleteAttributeFoldersCommand> (
            attributeCommands, "1.7.3",
            "Deletes attribute folders, each given by attribute type and path, together with all the subfolders and deletable attributes they contain (a folder holding an attribute that cannot be deleted is kept). The root folder cannot be deleted. With dryRun true nothing is deleted: the response has dryRun set to true, and each folder that exists and is not the root folder gets wouldDelete. Available from Archicad 26."
        );
        err |= RegisterCommand<RenameAttributeFoldersCommand> (
            attributeCommands, "1.7.3",
            "Renames attribute folders, each given by attribute type and path, to the given new names. The root folder cannot be renamed. Available from Archicad 26."
        );
        err |= RegisterCommand<MoveAttributesAndFoldersCommand> (
            attributeCommands, "1.7.3",
            "Moves attributes and attribute folders (with their contents) into the target folder, which must be of the same attribute type; an empty target path is the root folder. Returns one execution result per attribute and per folder; an attribute or folder that is already directly in the target folder counts as moved. Available from Archicad 26."
        );
        err |= RegisterCommand<ReindexLayersCommand> (
            attributeCommands, "2.0.0",
            "Moves layers to new indices. Archicad cannot renumber an attribute, so each layer is recreated at its new index (free indices below it are filled with temporary layers, which are deleted again), gets the old layer's state in every layer combination, and then every element of the old layer is moved to it, in the floor plan and in every view and layout database, with the tool defaults on that layer. Only when the old layer is empty everywhere is it deleted, and the new layer gets its name, flags and folder. If an element cannot be moved (it is locked, not reserved in Teamwork, or comes from a hotlinked module), that layer is left as it was. The new layer has a new attribute id; Favorites that use the layer are not updated. Archicad keeps the shown, locked and wireframe state of a layer per open window: the new layer gets the old one's state in the current window and in every layer combination, and starts shown and unlocked in the other open windows. The model elements that sections, elevations, interior elevations and 3D documents show of the layer come back on the new layer when those views are rebuilt (listed as viewsToRebuild). dryRun lists the elements and tool defaults on each layer without changing anything. The whole call is one undo step. Available from Archicad 27."
        );
        err |= RegisterCommand<ReindexAttributesCommand> (
            attributeCommands, "2.0.0",
            "Moves fills, line types, surfaces, building materials and zone categories to new indices, the way ReindexLayers moves layers. Archicad cannot renumber an attribute, so the attribute is recreated at its new index (free indices below it are filled with temporary attributes, which are deleted again) and every reference to the old index is rewritten to the new one: the fields of the elements in the floor plan and in every view and layout, the parameters of library part based elements, the memo data of roofs, slabs, meshes, morphs, curtain walls, beams, columns, stairs and interior elevations, the tool defaults, the Favorites (with their sub-elements: a curtain wall's frame and panel classes, a stair's structures), the other attributes that refer to it (composites, building materials, surfaces, profiles, MEP systems) and the graphic override rules. Only then is the old attribute deleted. The new attribute has a new attribute id (Archicad does not keep a given id); its name, definition and attribute folder are the old ones. Elements on a hidden or locked layer cannot be changed, so their layers are shown and unlocked for the move and put back afterwards (in the current window, as Archicad keeps these flags per window). If an element still cannot be changed (it is locked itself, or not reserved in Teamwork), everything is put back and that attribute is not moved. Elements from hotlinked modules cannot be changed and are listed; their module file has to be reindexed on its own. Indices typed into GDL scripts are not found, and the frame and panel classes of curtain wall Favorites and of the Curtain Wall tool default keep the old index (Archicad does not take them from ACAPI_Favorite_Change, so such Favorites are left as they are and named in the warnings). dryRun lists every reference without changing anything. The whole call is one undo step. Pens, layers (see ReindexLayers) and the other attribute types are not supported. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteAndReplaceAttributesCommand> (
            attributeCommands, "2.0.0",
            "Deletes fills, line types, surfaces or building materials and replaces each by another attribute of the same type wherever it was used, like Delete and Replace in the Attribute Manager. Every reference is rewritten to the replacement, the way ReindexAttributes rewrites them: the fields of the elements in the floor plan and in every view and layout, the parameters of library part based elements, their memo data, the tool defaults, the Favorites, the other attributes that refer to it (composites, building materials, surfaces, profiles, MEP systems) and the graphic override rules. Elements on a hidden or locked layer are changed too: their layers are shown and unlocked for the change and put back afterwards (in the current window). If an element cannot be changed (it is locked itself, or not reserved in Teamwork), everything is put back and that attribute is not deleted. Elements from hotlinked modules cannot be changed and are listed. Indices typed into GDL scripts are not found, and the frame and panel classes of curtain wall Favorites and of the Curtain Wall tool default keep the deleted attribute (named in the warnings). If a tool default, Favorite, attribute or rule cannot be rewritten to the replacement, it keeps a copy of the deleted attribute with its name and a new attribute id, named in the warnings. The whole call is one undo step. With dryRun true nothing is changed and no undo step is created: the response has dryRun set to true, and each attribute that passes the checks gets wouldDelete and lists every reference a real run would rewrite (elements, tool defaults, Favorites, other attributes, graphic override rules, elements from hotlinked modules). Available from Archicad 27."
        );
        err |= RegisterCommand<GetUnusedAttributesCommand> (
            attributeCommands, "2.0.0",
            "Lists the unused layers, fills, surfaces, building materials, composites, profiles, line types, pen tables and zone categories (index, id and name), for the given types or all of them. The Archicad API has no function that tells which attributes are unused (only how many per type), so every reference is collected: the elements of the floor plan and of every view and layout (hotlinked ones too), with their library part parameters and memo data, the tool defaults, the Favorites, the other attributes (composites, building materials, surfaces, profiles, MEP systems) and the graphic override rules, the way ReindexAttributes finds them; for layers the layer of each element, tool default and Favorite; for composites and profiles the walls, slabs, roofs, shells, column and beam segments whose structure type uses them; for pen tables the current pen table and the drawings set to a pen table of their own. An attribute that only an unused attribute refers to counts as used. Indices typed into GDL scripts and the pen tables saved with views are not found. Each type also reports Archicad's own count of the attributes in use (archicadUsedCount, see GetUsedAttributes), so a difference shows. Read only. Available from Archicad 27."
        );
        err |= RegisterCommand<GetMissingAttributesCommand> (
            attributeCommands, "2.0.0",
            "Lists the references of placed elements to layers, fills, surfaces, building materials, composites, profiles, line types, pen tables and zone categories that do not exist in the project, for the given types or all of them, and of the tool defaults, Favorites, graphic override rules, Model View Options combinations and attributes: the structured counterpart of the missing attributes report of Archicad's GDL palette. Every element of the floor plan and of every view and layout (hotlinked ones too) is checked, and so is every tool default and Favorite (with its sub-elements): its layer, its attribute fields and memo data as ReindexAttributes finds them, its composite or profile when its structure type uses one, a drawing's own pen table, and its library part parameters of attribute types; of every graphic override rule the line type, fill and surface it overrides with; of every Model View Options combination the surfaces and line types of the stair and railing schemes and the library part settings' GDL parameters of attribute types; of every composite the building materials of its skins and the line types of its separators, of every building material its cut fill and cut surface, of every surface the fill of its vectorial hatching, of every MEP system its fill, center line type and surfaces, and of every profile the building material, surface or fill of each skin and its contour, cut end and edge line types. The sources input limits the check to elements, tool defaults, Favorites, graphic override rules, Model View Options or attributes. Each item names the source; for an element the element (and the element it is part of, for a curtain wall frame, a column segment, ...) and the story or view it sits on, for a tool default the tool, for a Favorite its name and tool, for a graphic override rule its guid and name, for a Model View Options combination its guid and name (and the library part option set), for an attribute its type, guid and name; then the attribute type, the missing index and the field or GDL parameter that holds it. Index 0 counts as no attribute. Read only. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateLayersCommand> (
            attributeCommands, "1.0.3",
            "Creates or overwrites Layer attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateLayerCombinationsCommand> (
            attributeCommands, "1.2.4",
            "Creates or overwrites Layer Combination attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateLinesCommand> (
            attributeCommands, "1.5.4",
            "Creates or overwrites Line attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateFillsCommand> (
            attributeCommands, "1.5.4",
            "Creates or overwrites Fill attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateZoneCategoriesCommand> (
            attributeCommands, "1.5.4",
            "Creates or overwrites Zone Category attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateMEPSystemsCommand> (
            attributeCommands, "1.5.4",
            "Creates or overwrites MEP System attributes (domain, pens, fill, center line type, surface and insulation surface) based on the given parameters."
        );
        err |= RegisterCommand<CreatePenTablesCommand> (
            attributeCommands, "1.5.4",
            "Creates or overwrites Pen Table attributes based on the given parameters."
        );
        err |= RegisterCommand<ModifyPensCommand> (
            attributeCommands, "1.7.2",
            "Changes the color, width and description of single pens of the Pen Table in effect for the model views (the default) or the layouts, keeping every other pen as it is. Only the given fields of a listed pen are changed. In a Teamwork project the PenTables lockable object set must be reserved first (see ReserveLockableObjectSet)."
        );
        err |= RegisterCommand<CreateProfilesCommand> (
            attributeCommands, "1.5.4",
            "Creates or overwrites Profile attributes as a copy of an existing Profile's geometry, based on the given parameters."
        );
        err |= RegisterCommand<CreateBuildingMaterialsCommand> (
            attributeCommands, "1.0.1",
            "Creates or overwrites Building Material attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateCompositesCommand> (
            attributeCommands, "1.0.2",
            "Creates or overwrites Composite attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateSurfacesCommand> (
            attributeCommands, "1.2.2",
            "Creates or overwrites Surface attributes based on the given parameters."
        );
        err |= RegisterCommand<GetBuildingMaterialPhysicalPropertiesCommand> (
            attributeCommands, "0.1.3",
            "Retrieves the physical properties of the given Building Materials."
        );
        err |= RegisterCommand<GetLayerCombinationsCommand> (
            attributeCommands, "1.2.4",
            "Returns the details of layer combination attributes."
        );
        err |= RegisterCommand<GetCurrentLayerCombinationCommand> (
            attributeCommands, "1.7.3",
            "Returns the layer combination in effect in the current window. No layer combination is returned when the current state of the layers matches none."
        );
        err |= RegisterCommand<ActivateLayerCombinationCommand> (
            attributeCommands, "2.0.0",
            "Activates the given layer combination in the current window, like choosing it in the Quick Layers palette. Saved views are not changed."
        );
        err |= RegisterCommand<GetLayerOfElementDefaultsCommand> (
            attributeCommands, "2.0.0",
            "Returns the layer of the tool defaults of the given element types: the layer new elements of that type are placed on."
        );
        err |= RegisterCommand<SetLayerOfElementDefaultsCommand> (
            attributeCommands, "2.0.0",
            "Sets the layer of the tool defaults of the given element types, so new elements of that type are placed on it. Only the layer of the defaults is changed."
        );
        err |= RegisterCommand<RenameAttributesCommand> (
            attributeCommands, "2.0.0",
            "Renames attributes of any type (layers, layer combinations, composites, surfaces, ...). Each attribute keeps its index, its id and its settings. A new name that another attribute of the same type already has is refused, and so is renaming the attributes Archicad does not allow to change (the built-in Archicad Layer, the Solid and Empty fills)."
        );
        err |= RegisterCommand<GetLinesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Line attributes."
        );
        err |= RegisterCommand<GetFillsCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Fill attributes."
        );
        err |= RegisterCommand<GetZoneCategoriesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Zone Category attributes."
        );
        err |= RegisterCommand<GetMEPSystemsCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given MEP System attributes (domain, pens, fill, center line type, surface and insulation surface)."
        );
        err |= RegisterCommand<GetPenTablesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Pen Table attributes."
        );
        err |= RegisterCommand<GetCurrentPenTableCommand> (
            attributeCommands, "1.7.3",
            "Returns the current Pen Table (its index, attribute identifier and name) and its number of pens. Pass the returned attributeId to GetPenTables to read the pens."
        );
        err |= RegisterCommand<GetPensCommand> (
            attributeCommands, "1.8.0",
            "Returns the given pens, or all of them, in effect in the current window: index, color, width and description. It also works when the pens in effect are not a Pen Table attribute. Available from Archicad 27."
        );
        err |= RegisterCommand<SetPensCommand> (
            attributeCommands, "2.0.0",
            "Changes the color, width and description of the given pens in effect in the current window, the pens GetPens returns, in one undo step. Only the given fields of a listed pen are changed; each pen gets its own execution result. In a Teamwork project the PenTables lockable object set must be reserved first (see ReserveLockableObjectSet). Available from Archicad 27."
        );
        err |= RegisterCommand<GetProfilesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Profile attributes."
        );
        err |= RegisterCommand<GetCompositesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Composite attributes."
        );
        err |= RegisterCommand<GetSurfacesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Surface attributes."
        );
        err |= RegisterCommand<GetLayersCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Layer attributes."
        );
        err |= RegisterCommand<GetBuildingMaterialsCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Building Material attributes."
        );
        err |= RegisterCommand<GetFontsByNameCommand> (
            attributeCommands, "1.7.3",
            "Finds loaded fonts by name through Archicad's font search and returns their index (as Text and Label fields reference fonts in fontIndex) and name, or an error for a name no loaded font has."
        );
        err |= RegisterCommand<GetGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.2",
            "Gets all graphic override rules of the project, with their rule group, criterion and style. With includeCriterion the criterion also comes as structured conditions, which CreateGraphicOverrideRules and ModifyGraphicOverrideRules take back. Available from Archicad 27."
        );
        err |= RegisterCommand<GetGraphicOverrideRulesByNameCommand> (
            attributeCommands, "1.7.3",
            "Gets graphic override rules by name, with their rule group, criterion and style. The rule group narrows the search; without it the rule is searched in every rule group. With includeCriterion the criterion also comes as structured conditions. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.2",
            "Creates graphic override rules. The criterion is given as Archicad's criterion XML or as structured conditions. The rule group is created when no rule group has the given name. Available from Archicad 27."
        );
        err |= RegisterCommand<ModifyGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.2",
            "Modifies the name, criterion or style of graphic override rules. The criterion is given as Archicad's criterion XML or as structured conditions. A given style replaces the whole style of the rule. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.2",
            "Deletes graphic override rules. They are also removed from the combinations that contain them. Available from Archicad 27."
        );
        err |= RegisterCommand<GetGraphicOverrideCombinationsCommand> (
            attributeCommands, "1.7.2",
            "Gets all graphic override combinations of the project with the rules they contain. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateGraphicOverrideCombinationsCommand> (
            attributeCommands, "1.7.2",
            "Creates graphic override combinations from existing rules. Available from Archicad 27."
        );
        err |= RegisterCommand<ModifyGraphicOverrideCombinationsCommand> (
            attributeCommands, "1.8.0",
            "Modifies the name and/or the rules of graphic override combinations. Omitted fields are kept. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteGraphicOverrideCombinationsCommand> (
            attributeCommands, "1.7.2",
            "Deletes graphic override combinations. Their rules are kept. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.8.0",
            "Creates empty graphic override rule groups. Available from Archicad 27."
        );
        err |= RegisterCommand<GetGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.7.2",
            "Gets all graphic override rule groups of the project with the rules they contain, empty groups included. Available from Archicad 27."
        );
        err |= RegisterCommand<SetGraphicOverrideRuleOrderCommand> (
            attributeCommands, "1.8.0",
            "Sets the order of the rules in graphic override rule groups, found by name. The listed rules come first, in the given order; the other rules of the group keep their order after them. Available from Archicad 27."
        );
        err |= RegisterCommand<RenameGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.7.3",
            "Renames graphic override rule groups, found by name. Their rules are kept. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.7.2",
            "Deletes graphic override rule groups, found by name. A group that still contains rules is kept unless its rules are moved to another group or deleted with it. Available from Archicad 27."
        );
        err |= RegisterCommand<SetGraphicOverrideRuleGroupOrderCommand> (
            attributeCommands, "1.8.0",
            "Sets the order of the graphic override rule groups, found by name. The groups not listed keep their order and follow the listed ones. Available from Archicad 27."
        );
        err |= RegisterCommand<MoveGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.3",
            "Moves graphic override rules to another rule group, or to another place in their own group. Available from Archicad 27."
        );
        err |= RegisterCommand<GetUserDataOfAttributesCommand> (
            attributeCommands, "1.8.0",
            "Reads the key-value pairs stored on attributes by SetUserDataOfAttributes. An attribute without user data returns no pairs; one holding user data in another layout returns an error."
        );
        err |= RegisterCommand<SetUserDataOfAttributesCommand> (
            attributeCommands, "1.8.0",
            "Stores string values under keys in the user data of attributes, for scripts to keep their own state on an attribute. The given pairs are merged into what each attribute already holds: a key that exists gets the new value, other keys are left as they are, and the keys listed in keysToDelete are removed. Prefix the keys with the script's name so that scripts do not overwrite each other's data. User data in another layout is left unchanged and reported as an error. Archicad does not record this change as an undo step, so Undo does not revert it."
        );
        err |= RegisterCommand<DeleteUserDataOfAttributesCommand> (
            attributeCommands, "1.8.0",
            "Removes the user data of attributes, every key at once. Archicad does not record this change as an undo step, so Undo does not revert it. With dryRun true nothing is removed: the response has dryRun set to true, and each attribute that exists gets wouldDelete."
        );
        AddCommandGroup (attributeCommands);
    }

    { // IFC Commands
        CommandGroup ifcCommands ("IFC Commands");
        err |= RegisterCommand<IFCFileOperationCommand> (
            ifcCommands, "1.2.6",
            "Executes an IFC file operation: opens or merges an IFC file, or saves the project as an IFC file. A save can name the export translator to use, a merge the story the IFC file's first storey is placed on. With overwrite false a save refuses to replace an existing file."
        );
        err |= RegisterCommand<GetIFCExportTranslatorsCommand> (
            ifcCommands, "1.6.0",
            "Lists the IFC export translators of the project in the order of Archicad's translator list. preview marks the first one, which the Archicad API documents as the preview translator; no API identifies the preview translator otherwise, and in some projects the IFC Translators dialog shows another one as preview. Pass one of the names to IFCFileOperation as translatorName."
        );
        err |= RegisterCommand<GetElementsByIFCIdsCommand> (
            ifcCommands, "1.5.1",
            "Retrieves the elements by the given IFC identifiers. Accepts IFC GlobalIds in every Archicad version."
        );
        err |= RegisterCommand<GetIFCIdsOfElementsCommand> (
            ifcCommands, "1.5.1",
            "Retrieves the IFC identifiers of the given elements. Before Archicad 28 it also returns their IFC GlobalIds."
        );
        err |= RegisterCommand<GetIFCTypeOfElementsCommand> (
            ifcCommands, "1.5.1",
            "Retrieves the IFC types of the given elements."
        );
        err |= RegisterCommand<GetIFCPropertiesOfElementsCommand> (
            ifcCommands, "1.5.1",
            "Retrieves the IFC properties of the given elements."
        );
        err |= RegisterCommand<GetIFCAttributesOfElementsCommand> (
            ifcCommands, "1.7.3",
            "Retrieves the IFC attributes (Name, Description, ObjectType, Tag, PredefinedType, LongName, ...) of the given elements, subelements included: the name, the IFC value type and the value of each attribute. On Archicad 25, 26 and 27 storedOnly returns only the attributes stored on the elements."
        );
        err |= RegisterCommand<SetIFCAttributesOfElementsCommand> (
            ifcCommands, "1.7.3",
            "Sets the values of IFC attributes (Name, Description, ObjectType, Tag, ...) of elements. The attribute must be one GetIFCAttributesOfElements returns for the element; its IFC value type is kept. Available only in Archicad 25, 26 and 27 for now."
        );
        err |= RegisterCommand<SetIFCPropertiesOfElementsCommand> (
            ifcCommands, "1.7.0",
            "Sets local IFC properties on elements by creating or modifying single value (IfcPropertySingleValue) properties in the given property sets. Available only in Archicad 25, 26 and 27 for now."
        );
        err |= RegisterCommand<GetIFCClassificationReferencesOfElementsCommand> (
            ifcCommands, "1.8.0",
            "Retrieves the IFC classification references of the given elements, subelements included: identification, name, location, description and sort (from Archicad 29), the name of the IfcRelAssociatesClassification (from Archicad 28), the referenced classification (name, source, edition, edition date, description and location) and whether the reference comes from the preview translator or is stored locally. On Archicad 25, 26 and 27 the source is not given and storedOnly returns only the references stored on the elements."
        );
        err |= RegisterCommand<SetIFCClassificationReferencesOfElementsCommand> (
            ifcCommands, "1.8.0",
            "Stores IFC classification references (identification, name, location and the referenced classification with its name, edition, edition date, description and location) on elements. Available only in Archicad 25, 26 and 27 for now."
        );
        err |= RegisterCommand<RemoveIFCClassificationReferencesOfElementsCommand> (
            ifcCommands, "1.8.0",
            "Removes IFC classification references stored on elements, given by the name of their classification and optionally their identification; without identification every stored reference of that classification is removed. Available only in Archicad 25, 26 and 27 for now. With dryRun true nothing is removed and no undo step is created: the response has dryRun set to true, and each element that has a matching stored reference gets wouldDelete."
        );
        err |= RegisterCommand<RemoveIFCPropertiesOfElementsCommand> (
            ifcCommands, "1.7.3",
            "Removes local IFC properties, given by property set and property name, from elements. Available only in Archicad 25, 26 and 27 for now. With dryRun true nothing is removed and no undo step is created: the response has dryRun set to true, and each element that has the local IFC property gets wouldDelete."
        );
        err |= RegisterCommand<GetIFCSpatialStructureCommand> (
            ifcCommands, "2.0.0",
            "Returns the IfcProject, IfcSite and IfcBuilding objects of the project's IFC model (hash value, IFC type and GlobalId), and for the given elements the index of the story they are assigned to in the IFC model. Available from Archicad 28."
        );
        err |= RegisterCommand<GetIFCAssignmentsCommand> (
            ifcCommands, "2.0.0",
            "Returns the IFC assignments of the project: the root IFC groups, and for each object the objects assigned to it (children) and the objects it is assigned to (parents). Each object is given by its hash value, its element where it belongs to one, its IFC type and its GlobalId. Available from Archicad 28."
        );
        AddCommandGroup (ifcCommands);
    }

    { // Library Commands
        CommandGroup libraryCommands ("Library Commands");
        err |= RegisterCommand<GetLibrariesCommand> (
            libraryCommands, "1.0.1",
            "Gets the list of loaded libraries. From Archicad 28 a library package also gets its package information (ID, versions, minimum Archicad version and dependencies)."
        );
        err |= RegisterCommand<ReloadLibrariesCommand> (
            libraryCommands, "1.0.0",
            "Executes the reload libraries command."
        );
        err |= RegisterCommand<AddFilesToEmbeddedLibraryCommand> (
            libraryCommands, "1.2.2",
            "Adds the given files into the embedded library. With the overwriteExisting flag an embedded library item already existing on an outputPath is replaced (Archicad 27 or newer)."
        );
        err |= RegisterCommand<DeleteEmbeddedLibraryItemsCommand> (
            libraryCommands, "1.6.0",
            "Deletes the given items from the embedded library. The path of an item is the same relative path that AddFilesToEmbeddedLibrary takes as outputPath. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateLibraryPartsCommand> (
            libraryCommands, "1.8.0",
            "Creates GDL library parts in the embedded library from their scripts (master, 2D, 3D, parameter, interface, properties) and parameter definitions, under the given parent subtype. The new parts are placeable right away, e.g. with CreateObjects. With the overwriteExisting flag an embedded library item already existing on the path of a new part is replaced (Archicad 27 or newer)."
        );
        err |= RegisterCommand<CreateFolderInEmbeddedLibraryCommand> (
            libraryCommands, "1.7.3",
            "Creates a folder, with any missing parent folders, inside the embedded library. The folderPath is relative to the embedded library, like the outputPath of AddFilesToEmbeddedLibrary."
        );
        err |= RegisterCommand<CopyFilesIntoEmbeddedLibraryCommand> (
            libraryCommands, "1.7.3",
            "Copies the given files, keeping their names, into a folder of the embedded library with one library operation. The target folder is created when it is missing. With overwriteIfExists an item of the same name in that folder is replaced."
        );
        err |= RegisterCommand<SetLibrariesCommand> (
            libraryCommands, "1.5.9",
            "Makes the given folders the project's local libraries; built-in, embedded, server and web libraries are kept. Set the libraries before opening a file that needs them and the missing-library dialog does not appear."
        );
        err |= RegisterCommand<AddLibrariesCommand> (
            libraryCommands, "1.5.9",
            "Adds the given folders to the project's local libraries, skipping any already loaded."
        );
        err |= RegisterCommand<GetAvailableLibraryPartsCommand> (
            libraryCommands, "1.5.0",
            "Lists library parts currently available to the project. Filter by typeId (e.g. 'Door', 'Window', 'Object', 'Lamp')."
        );
        err |= RegisterCommand<GetLibraryPartSectionsCommand> (
            libraryCommands, "2.0.0",
            "Lists the sections of library parts given by index: their four-character type code, sub-identifier, size and whether they are text (scripts, comments) or binary (previews)."
        );
        err |= RegisterCommand<GetLibraryPartSectionCommand> (
            libraryCommands, "2.0.0",
            "Reads the text of a text section (a script or the comment) of a library part given by index, without opening it in the GDL editor. Takes the password of a protected library part."
        );
        err |= RegisterCommand<GetLibrariesOfLibPartsCommand> (
            libraryCommands, "2.0.0",
            "Gets the library of each given library part (by file name or guid): display name, location, embedded/built-in/package/local/missing flags and the Library Manager tree path. Available from Archicad 29."
        );
        err |= RegisterCommand<GetLibraryPartTypeDetailsCommand> (
            libraryCommands, "2.0.0",
            "Returns the type details of the given library parts (ACAPI_LibraryPart_GetDetails): full unique ID and its direct parent's, type, the detail flags of objects, lamps, labels and zones, the frame and orientation settings of doors and windows, and the element type and variation of the tool that places the library part. For their path, library, GUID, license and scripts see GetLibraryPartDetails."
        );
        err |= RegisterCommand<IsLibraryPartSubtypeOfCommand> (
            libraryCommands, "2.0.0",
            "Checks for each of the given library parts whether the given ancestor is anywhere up its subtype hierarchy (e.g. whether a part is a kind of furniture), independent of its name. A library part is not a subtype of itself. With matchAncestorByMainId any revision of the ancestor matches."
        );
        err |= RegisterCommand<GetGDLParametersOfLibraryPartsCommand> (
            libraryCommands, "2.0.0",
            "Gets the default GDL parameters (name, display name, type, array dimensions, value, flags) of the given library parts by name, read from the library part without placing it or changing any defaults. The output is in the format of GetGDLParametersOfElements, without isLocked and possibleValues, which need a placed element. To read a part by its file name, as SetLibraryPartParameters writes it, use GetLibraryPartParameterValues."
        );
        err |= RegisterCommand<GetMissingLibPartNamesCommand> (
            libraryCommands, "2.0.0",
            "Gets the library part name of the given placed Object, Lamp, Window or Door elements, even if the library containing the part is not loaded. Use it to find the parts of elements that are shown as dots because their library part is missing."
        );
        err |= RegisterCommand<GetUnusedLibraryPartsCommand> (
            libraryCommands, "2.0.0",
            "Lists the loaded library parts that no element of the project is placed from. Read only. The Archicad API does not tell which library parts are used, so every element of the floor plan and of every view and layout is checked: objects, lamps, doors, windows, skylights, zone stamps, symbol labels, drawing titles, curtain wall parts, and the markers of doors, windows, skylights, sections, elevations, details and worksheets. A library part used only in another way (called as a macro, linked as a property object, as an interior elevation marker, in stairs and railings, in the tool defaults or in Favorites) is listed as unused. Library parts that cannot be placed (e.g. macros) are only listed with includeNotPlaceable."
        );

        err |= RegisterCommand<FindLibraryPartsCommand> (
            libraryCommands, "2.0.0",
            "Finds loaded library parts with the Library Manager: all of them, GDL objects, images or text files, optionally only those whose name contains a text. Returns the name, path and index of each. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartDetailsCommand> (
            libraryCommands, "2.0.0",
            "Gets the details of loaded library parts, by name or index: path, library tree path, containing library, and for GDL objects the GUID, subtype, license, UI default flag, which scripts and pictures they have, and optionally the interface and migration scripts. Can also tell whether each one is in the subtype of another library part. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyLibraryListCommand> (
            libraryCommands, "2.0.0",
            "Adds libraries by path and removes loaded libraries by name or path, in one change of the library list. Returns the result of each addition and removal. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartMetadataCommand> (
            libraryCommands, "2.0.0",
            "Returns the description, keywords, author and placeable, password protected and hierarchical pages flags of loaded library parts, given by file name. Use it to search library parts by description or keywords and to tell placeable parts from macros. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartOriginalLocationsCommand> (
            libraryCommands, "1.8.0",
            "Gets the original location of the given library parts, the file they came from, e.g. before they were embedded into the project. The library parts are given by the index GetAvailableLibraryParts returns. Available from Archicad 27."
        );
        err |= RegisterCommand<IsUsableGDLStringCommand> (
            libraryCommands, "1.8.0",
            "Checks whether each of the given strings is usable in GDL, e.g. before writing it into a GDL parameter or script. Returns one boolean for each string."
        );
        err |= RegisterCommand<GetGDLDateCommand> (
            libraryCommands, "2.0.0",
            "Gets the date GDL scripts see: year, month, day, hour, minute and second."
        );
        err |= RegisterCommand<SetGDLDateCommand> (
            libraryCommands, "2.0.0",
            "Sets the date GDL scripts see: year, month, day, hour, minute and second, as GetGDLDate returns them. Useful to test date-dependent library parts; set the value read with GetGDLDate back to restore it."
        );
        err |= RegisterCommand<CheckLibrariesCommand> (
            libraryCommands, "1.8.0",
            "Checks the loaded libraries with the Archicad API's library check. Returns the error the API reports, if any."
        );
        err |= RegisterCommand<ResetLibrariesCommand> (
            libraryCommands, "1.8.0",
            "Resets the loaded libraries with the Archicad API's library reset."
        );
        err |= RegisterCommand<SetLibraryPartScriptsCommand> (
            libraryCommands, "1.7.3",
            "Sets and deletes the master, parameter, 2D, 3D, properties, interface and forward and backward migration scripts of loaded library parts. Only the given scripts are changed; an empty script makes the section empty, deleteScripts removes it. A library part that is not editable (e.g. a part of a library package or a read-only library) is refused and left unchanged. Available from Archicad 28."
        );
        err |= RegisterCommand<SetLibraryPartMetadataCommand> (
            libraryCommands, "1.8.0",
            "Sets the author, keywords, license, license version, password and the placeable, UI default, subtype and hierarchical pages flags of loaded library parts. Only the given fields are changed. A library part that is not editable (e.g. a part of a library package or a read-only library) is refused and left unchanged. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartPicturesCommand> (
            libraryCommands, "1.8.0",
            "Gets the preview picture and the GDL pictures of a loaded library part as base64 encoded PNGs, and from Archicad 29 also their dark user interface variants. Available from Archicad 28."
        );
        err |= RegisterCommand<SetLibraryPartPicturesCommand> (
            libraryCommands, "1.8.0",
            "Deletes the preview picture, the GDL pictures (and from Archicad 29 their dark variants) or the 2D symbol of a loaded library part, or copies the 2D symbol from another library part. A library part that is not editable is refused and left unchanged. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartCustomSectionCommand> (
            libraryCommands, "1.8.0",
            "Gets a custom section of a loaded library part by its section identifier and index: whether it exists, its data version and its content as base64. Available from Archicad 28."
        );
        err |= RegisterCommand<SetLibraryPartCustomSectionCommand> (
            libraryCommands, "1.8.0",
            "Creates, replaces or deletes a custom section of a loaded library part. The content is given as base64. A library part that is not editable is refused and left unchanged. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateLibraryPartCommand> (
            libraryCommands, "1.8.0",
            "Creates a new library part file (.gsm) of the given loaded subtype, inheriting its parameters, and reports whether it is loaded with its index and names. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartParameterValuesCommand> (
            libraryCommands, "2.0.0",
            "Returns the parameters of loaded library parts, given by their file name as GetAvailableLibraryParts returns it, with their default values and descriptions, in the format of GetGDLParametersOfElements: the read counterpart of SetLibraryPartParameters. Unlike GetGDLParametersOfLibraryParts, which finds a part by its name and also returns the index, flags and value descriptions of each parameter, it reads through the Library Manager. Available from Archicad 28."
        );
        err |= RegisterCommand<UpdateLibraryPartSectionsCommand> (
            libraryCommands, "2.0.0",
            "Replaces the text of existing sections (scripts, comment, keywords, copyright) of a loaded library part, found by its name. Each section is read back after the update, and the output has one execution result per section."
        );
        err |= RegisterCommand<GetLibraryPart2DPrimitivesCommand> (
            libraryCommands, "2.0.0",
            "Gets the lines, arcs and polygons (with their holes, and whether they are fills) of the 2D symbol of a loaded library part, drawn with its default parameters or with the parameters of a placed instance."
        );
        err |= RegisterCommand<SetLibraryPartPictureImagesCommand> (
            libraryCommands, "2.0.0",
            "Sets the preview picture and the GDL pictures of a loaded library part from base64 encoded PNGs, and from Archicad 29 also their dark user interface variants. All pictures are set in one step; an invalid picture changes nothing. A library part that is not editable is refused and left unchanged. Available from Archicad 28."
        );
        err |= RegisterCommand<SetLibraryPartParametersCommand> (
            libraryCommands, "2.0.0",
            "Sets the parameter values, and optionally the parameter descriptions, of loaded library parts, in the format of SetGDLParametersOfElements. Only the given parameters are changed; with merge, the changed list is merged into the parameters of the part instead of replacing them. A library part that is not editable, or with an invalid value given, is left unchanged. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartScriptsCommand> (
            libraryCommands, "2.0.0",
            "Gets the master, parameter, 2D, 3D and properties scripts of loaded library parts, given by name or by an element placed from them, with the unique ID of each part. A script section the part does not have is reported as not present. Password protected parts need their password. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartFileInfoCommand> (
            libraryCommands, "2.0.0",
            "Reads library part files (.gsm) without loading them into the libraries of the project: the unique ID of the part, the unique ID of its subtype and its master, parameter, 2D, 3D and properties scripts. Password protected parts need their password. Available from Archicad 28."
        );
        err |= RegisterCommand<CopyLibraryPartsCommand> (
            libraryCommands, "2.0.0",
            "Copies loaded library parts, given by name or by an element placed from them, under a new file name into a folder, by default the embedded library. The copy gets a new unique ID unless the main GUID is kept. An existing file is never overwritten. The libraries are reloaded once afterwards, so the copies can be found by name. Available from Archicad 28."
        );
        err |= RegisterCommand<GetLibraryPartIdentitiesCommand> (
            libraryCommands, "2.0.0",
            "Gets the unique ID (main and revision GUID), the reference GUID and the Teamwork lock status of library parts given by index or file path, and whether they are in a server library."
        );
        err |= RegisterCommand<CompareLibraryPartUnIdsCommand> (
            libraryCommands, "2.0.0",
            "Compares two library part unique IDs with Archicad's own comparison and tells whether their main and revision GUIDs are the same."
        );
        err |= RegisterCommand<GetLastValidEmbeddedLibraryPartNameCommand> (
            libraryCommands, "2.0.0",
            "Gets the name and file path of the last valid library part of the embedded library."
        );
        err |= RegisterCommand<SearchLibraryPartsCommand> (
            libraryCommands, "2.0.0",
            "Finds the loaded library parts derived from an ancestor library part (by its unique ID) whose name matches a pattern with * wildcards. Archicad returns at most 50 of them."
        );
        err |= RegisterCommand<OpenLibraryPartInEditorCommand> (
            libraryCommands, "1.8.0",
            "Opens a loaded library part, given by its fileName or documentName as returned by GetAvailableLibraryParts, in Archicad's library part editor. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateLibraryPartFromElementsCommand> (
            libraryCommands, "2.0.0",
            "Creates a library part in the embedded library (or in a given local library folder, which is added to the loaded libraries when needed) whose 2D symbol is a copy of the given 2D drawing elements (lines, polylines, arcs, circles, splines, fills, texts and hotspots), under the given parent subtype, with optional master, 2D, 3D and parameter scripts, comment and keywords. A and B are taken from the extent of the symbol. The elements themselves are not changed."
        );
        err |= RegisterCommand<InterpretGDLScript3DCommand> (
            libraryCommands, "2.0.0",
            "Runs a GDL 3D script, given as text or as the master and 3D scripts of a loaded library part with its default parameters, with the settings of the GDL editor, and returns whether it ran without a GDL error, how many 3D model elements it generated and their bounding box. The API returns no GDL error message or line number."
        );
        err |= RegisterCommand<GetGDLUserGlobalsCommand> (
            libraryCommands, "2.0.0",
            "Returns the current GDL global variables the GDL interpreter gets, with their names, types and values. Available from Archicad 28."
        );
        AddCommandGroup (libraryCommands);
    }

    { // Teamwork Commands
        CommandGroup teamworkCommands ("Teamwork Commands");
        err |= RegisterCommand<TeamworkSendCommand> (
            teamworkCommands, "0.1.0",
            "Performs a send operation on the currently opened Teamwork project."
        );
        err |= RegisterCommand<TeamworkReceiveCommand> (
            teamworkCommands, "0.1.0",
            "Performs a receive operation on the currently opened Teamwork project."
        );
        err |= RegisterCommand<GetTeamworkProjectInfoCommand> (
            teamworkCommands, "1.8.0",
            "Tells whether the opened project is a Teamwork project and whether it is online, and returns its BIMcloud server, project name and the current user (id and name)."
        );
        err |= RegisterCommand<ReserveElementsCommand> (
            teamworkCommands, "1.1.4",
            "Reserves elements in Teamwork mode. GUIDs that are not elements (navigator items, layouts, databases) cannot be reserved with it: they are returned in notElements and the execution result fails."
        );
        err |= RegisterCommand<ReleaseElementsCommand> (
            teamworkCommands, "1.1.4",
            "Releases elements in Teamwork mode. Fails, naming them, when any of the given GUIDs is not an element; the other elements are released."
        );
        err |= RegisterCommand<GetReservationStatusOfElementsCommand> (
            teamworkCommands, "1.7.3",
            "Returns the Teamwork reservation status of elements without reserving them: whether each element is free, reserved by the current user or reserved by another user, with the reserving user. New elements stay reserved by the user who created them until they are released."
        );
        err |= RegisterCommand<GrantElementsCommand> (
            teamworkCommands, "1.7.3",
            "Hands over the reservation of elements to another Teamwork user, given by user id, without a dialog. Available from Archicad 27."
        );
        err |= RegisterCommand<ReserveLockableObjectSetCommand> (
            teamworkCommands, "1.7.1",
            "Reserves a non-element lockable object set (attributes, favorites, project info, project preferences, etc.) in Teamwork mode. The object set is given by its Archicad API name, e.g. 'Composites', 'LayerSettingsDialog' or 'ProjectInfo'."
        );
        err |= RegisterCommand<ReleaseLockableObjectSetCommand> (
            teamworkCommands, "1.7.1",
            "Releases a non-element lockable object set reserved with ReserveLockableObjectSet in Teamwork mode."
        );
        err |= RegisterCommand<GetLockableObjectSetStatusCommand> (
            teamworkCommands, "1.7.1",
            "Returns the Teamwork reservation status of a non-element lockable object set and the users who hold it."
        );
        err |= RegisterCommand<ReserveHotlinkCacheManagementCommand> (
            teamworkCommands, "1.7.1",
            "Reserves the Hotlink and XRef management in Teamwork mode, which is needed before changing hotlink nodes."
        );
        err |= RegisterCommand<ReleaseHotlinkCacheManagementCommand> (
            teamworkCommands, "1.7.1",
            "Releases the Hotlink and XRef management in Teamwork mode."
        );
        err |= RegisterCommand<GetTeamworkStatusCommand> (
            teamworkCommands, "1.7.3",
            "Returns whether the opened project is a Teamwork project and is online, the members of the shared project and who has reserved the Hotlink and XRef management. Works in a solo project too, then isShared and isOnline are false."
        );
        err |= RegisterCommand<GetTeamworkOwnersOfDatabasesCommand> (
            teamworkCommands, "1.8.0",
            "Returns the Teamwork owner (user id and name) of each given database, e.g. of a layout or a view database. The user id is 0 when nobody owns the database."
        );
        err |= RegisterCommand<UploadLibrariesToBIMcloudCommand> (
            teamworkCommands, "1.8.0",
            "Uploads local library folders or container files (.lcf) to a folder of a BIMcloud or Teamwork server, as BIMcloud libraries. Returns a result per library. Available from Archicad 29."
        );
        err |= RegisterCommand<RemoveLibrariesFromBIMcloudCommand> (
            teamworkCommands, "1.8.0",
            "Removes libraries, given by name, from a folder of a BIMcloud or Teamwork server. Returns a result per library. Available from Archicad 29."
        );
        err |= RegisterCommand<GetTeamworkRightsOfObjectsCommand> (
            teamworkCommands, "2.0.0",
            "Returns for each given object, for example an element, whether the current Teamwork user has the right to create objects of its kind and to delete or modify it."
        );
        err |= RegisterCommand<GetTeamworkAccessRightsCommand> (
            teamworkCommands, "2.0.0",
            "Returns whether the role of the current user has the given Teamwork access rights (API_TWAccessRights names without the API prefix, e.g. LayersCreate), all of them when no names are given."
        );
        AddCommandGroup (teamworkCommands);
    }

    { // Navigator Commands
        CommandGroup navigatorCommands ("Navigator Commands");
        err |= RegisterCommand<PublishPublisherSetCommand> (
            navigatorCommands, "0.1.0",
            "Performs a publish operation on the currently opened project. Only the given publisher set will be published."
        );
        err |= RegisterCommand<GetPublisherSetsCommand> (
            navigatorCommands, "2.0.0",
            "Returns the publisher sets of the project with their saved output: the local or LAN path, the Teamwork file server path, or the Internet host and directory name."
        );
        err |= RegisterCommand<UpdateDrawingsCommand> (
            navigatorCommands, "1.1.4",
            "Performs a drawing update on the given elements."
        );
        err |= RegisterCommand<GetDrawingContentBoxesCommand> (
            navigatorCommands, "1.7.3",
            "Returns the full content box of the given Drawing elements, independent of their frames, as ACAPI_Drawing_GetFullDrawingContentBox reports it. Available from Archicad 27."
        );
        err |= RegisterCommand<GetDatabaseIdFromNavigatorItemIdCommand> (
            navigatorCommands, "1.1.4",
            "Gets the ID of the database associated with the supplied navigator item id"
        );
        err |= RegisterCommand<GetDatabasesCommand> (
            navigatorCommands, "1.7.3",
            "Lists every database of the given types (sections, elevations, interior elevations, details, worksheets, 3D documents, layouts, master layouts) "
            "with its database ID, name and reference ID, without walking the navigator trees."
        );
        err |= RegisterCommand<CreateDetailsCommand> (
            navigatorCommands, "1.4.0",
            "Creates independent Detail databases. Returns the databaseId and the Project Map navigatorItemId of each new detail."
        );
        err |= RegisterCommand<CreateWorksheetsCommand> (
            navigatorCommands, "1.4.0",
            "Creates independent Worksheet databases. Returns the databaseId and the Project Map navigatorItemId of each new worksheet."
        );
        err |= RegisterCommand<CreateDetailMarkersCommand> (
            navigatorCommands, "2.0.0",
            "Places Detail markers with their clip polygon in the current floor plan, section or elevation, each with a new Detail viewpoint, like the Detail tool. GetDetailsOfElements reads them back."
        );
        err |= RegisterCommand<CreateWorksheetMarkersCommand> (
            navigatorCommands, "2.0.0",
            "Places Worksheet markers with their clip polygon in the current floor plan, section or elevation, each with a new Worksheet viewpoint, like the Worksheet tool. GetDetailsOfElements reads them back."
        );
        err |= RegisterCommand<ModifyDetailMarkersCommand> (
            navigatorCommands, "2.0.0",
            "Changes the name, the reference ID, the clip polygon, the boundary's pen and line type, the marker symbol's angle, always-horizontal setting and pen of Detail and Worksheet markers. Not written: the base point (Archicad places it at the center of the clip polygon's bounding box when the marker is created), the viewpoint the marker refers to, and the marker symbol's other settings, which are parameters of the library part placed as the marker (markerId in GetDetailsOfElements). A value Archicad did not apply gets a failed execution result."
        );
        err |= RegisterCommand<CreateLayoutCommand> (
            navigatorCommands, "1.4.0",
            "Creates Layouts and their backing master layouts. Archicad inserts a new layout at the first "
            "place of its subset, renumbering the sheets after it; previousNavigatorItemId or position "
            "'last' places it after an existing sheet or at the end instead. A custom layout number does "
            "not leave the layout out of the numbering; layoutParameters.doNotIncludeInNumbering does. "
            "A layout that was created but could not be placed is returned with both its databaseId and the error."
        );
        err |= RegisterCommand<CreateLayoutSubsetCommand> (
            navigatorCommands, "1.4.0",
            "Creates Layout Book subsets."
        );
        err |= RegisterCommand<GetLayoutSubsetSettingsCommand> (
            navigatorCommands, "1.7.3",
            "Gets the settings of existing Layout Book subsets: the same fields CreateLayoutSubset accepts (name, ID prefix, numbering style, ...)."
        );
        err |= RegisterCommand<CreateDrawingsCommand> (
            navigatorCommands, "1.4.0",
            "Creates Drawing elements on the specified or active layout from navigator items."
        );
        err |= RegisterCommand<ChangeDrawingLinkCommand> (
            navigatorCommands, "1.5.7",
            "Relinks a Drawing to a different source navigator item. Archicad has no in-place relink "
            "API, so this recreates the Drawing against the new source and deletes the original - the "
            "returned elementId is a NEW guid, not the input one. The Drawing Title marker's own "
            "position is not preserved (undocumented Archicad limitation). The new source view is "
            "made current before the Drawing is created (Archicad 27 or later), and the window and "
            "story that were active when the command started are restored afterwards; the view "
            "settings (zoom, layers, scale) shown in that window are not restored."
        );
        err |= RegisterCommand<GetLayoutSettingsCommand> (
            navigatorCommands, "1.1.7",
            "Gets settings of layouts, including their master layout and Layout Info Panel custom data fields."
        );
        err |= RegisterCommand<SetLayoutSettingsCommand> (
            navigatorCommands, "1.1.7",
            "Sets settings of layouts, including their master layout and Layout Info Panel custom data fields. In Teamwork the layout settings must be reserved first in Archicad (Layout Book > Reserve Layout Settings); the Archicad API cannot reserve them, and ReserveElements does not. GetTeamworkOwnersOfDatabases shows who holds a layout."
        );
        err |= RegisterCommand<GetLayoutCustomSchemeCommand> (
            navigatorCommands, "1.1.7",
            "Gets the Layout Info Panel custom field definitions (name and key) from Book Settings."
        );
        err |= RegisterCommand<GetDrawingNumberingGridLinesCommand> (
            navigatorCommands, "1.8.0",
            "Gets the lines of the grid of the grid-based drawing numbering of the current layout "
            "(start and end point, pen index, line type index), for example to find which grid cell "
            "a drawing on the layout sits in. The current window must be a layout (see ChangeWindow), "
            "and only a layout that uses grid-based drawing numbering has such a grid."
        );
        err |= RegisterCommand<GetModelViewOptionsCommand> (
            navigatorCommands, "1.1.4",
            "Gets all model view options"
        );
        err |= RegisterCommand<FindModelViewOptionCommand> (
            navigatorCommands, "1.8.0",
            "Finds a model view option combination by id or by name and returns its index, id, name and flags. The index is the one SetViewOptions takes to switch the model view options of a window to the combination. Available from Archicad 27."
        );
        err |= RegisterCommand<GetModelViewOptionDetailsCommand> (
            navigatorCommands, "2.0.0",
            "Gets the settings stored in model view option combinations (Document > Model View > Model View Options), all of them or the ones given by id or by name: the model view options in the fields of GetViewOptions, and the options library parts add to the combination (gdlOptions) with their GDL parameter values. Read-only: the Archicad API has no function to change, rename or create a stored combination; SetViewOptions changes the options of the current window. Available from Archicad 27."
        );
        err |= RegisterCommand<GetViewOptionsCommand> (
            navigatorCommands, "1.8.0",
            "Gets the model view options and on-screen view options of the current plan window, and the index and name of the model view option combination it applies."
        );
        err |= RegisterCommand<SetViewOptionsCommand> (
            navigatorCommands, "1.8.0",
            "Switches the current plan window to a model view option combination by index, and/or changes the given model view options and on-screen view options. With an index, the model view options stored in the combination apply."
        );
        err |= RegisterCommand<GetViewSettingsCommand> (
            navigatorCommands, "1.1.4",
            "Gets the view settings of navigator items"
        );
        err |= RegisterCommand<SetViewSettingsCommand> (
            navigatorCommands, "1.1.4",
            "Sets the view settings of navigator items"
        );
        err |= RegisterCommand<GetActiveGraphicalOverrideCombinationCommand> (
            navigatorCommands, "1.7.2",
            "Gets the graphic override combination applied to the current window, with its id and name. The response is empty when no combination is applied. This is the combination of the window itself, not the one stored in a saved view (see GetViewSettings). Available from Archicad 30."
        );
        err |= RegisterCommand<SetActiveGraphicalOverrideCombinationCommand> (
            navigatorCommands, "1.7.2",
            "Applies a graphic override combination, given by id or by name, to the current window. The current window must be a model window (floor plan, section, elevation, 3D, ...). To remove the overrides, apply the project's 'No Overrides' combination. The saved view is not changed (see SetViewSettings for that). Available from Archicad 30."
        );
        err |= RegisterCommand<GetRenovationFiltersCommand> (
            navigatorCommands, "1.8.0",
            "Gets the renovation filters of the project, and the one the current window uses (Document > Renovation > Renovation Filter). A renovationFilterId has the same shape as the renovationFilterGuid of GetViewSettings and SetViewSettings, so a filter listed here can be set on saved views too."
        );
        err |= RegisterCommand<SetActiveRenovationFilterCommand> (
            navigatorCommands, "1.8.0",
            "Sets the renovation filter of the current window (Document > Renovation > Renovation Filter), given by its renovationFilterId from GetRenovationFilters or by its name. Saved views keep their own filter: change that with SetViewSettings."
        );
        err |= RegisterCommand<GetGhostStorySettingsCommand> (
            navigatorCommands, "1.8.0",
            "Gets the Trace & Reference ghost story settings of the floor plan: whether it is on, whether one story, the story above or the one below is shown, which story, its color and the element types shown in it."
        );
        err |= RegisterCommand<SetGhostStorySettingsCommand> (
            navigatorCommands, "1.8.0",
            "Sets the Trace & Reference ghost story settings of the floor plan through its ghost record, which is created when the floor plan has none. Every field is optional; the ones left out keep their value, and without a story choice the current reference is kept."
        );
        err |= RegisterCommand<GetStructureDisplayCommand> (
            navigatorCommands, "1.7.3",
            "Gets the Partial Structure Display (EntireStructure, CoreOnly, WithoutFinishes or StructureOnly) of the current window, and whether it is the 3D window. This is the setting of the window itself, not the one stored in a saved view (see GetViewSettings). Available from Archicad 27."
        );
        err |= RegisterCommand<SetStructureDisplayCommand> (
            navigatorCommands, "1.7.3",
            "Sets the Partial Structure Display (EntireStructure, CoreOnly, WithoutFinishes or StructureOnly) of the current window, e.g. the floor plan, a section or the 3D window. The change cannot be undone. The saved view is not changed (see SetViewSettings for that). Available from Archicad 27."
        );
        err |= RegisterCommand<GetView2DTransformationsCommand> (
            navigatorCommands, "1.1.7",
            "Get zoom and rotation of 2D views"
        );
        err |= RegisterCommand<SetViewRotationCommand> (
            navigatorCommands, "1.1.7",
            "Set the rotation angle of 2D views via their floor plan database."
        );
        err |= RegisterCommand<CloneProjectMapItemToViewMapCommand> (
            navigatorCommands, "1.1.7",
            "Clones Project Map viewpoints into a specified View Map folder."
        );
        err |= RegisterCommand<CreateViewsInViewMapCommand> (
            navigatorCommands, "1.1.7",
            "Creates independent (non-clone) navigator views in a specified View Map folder, by copying database and settings from source items."
        );
        err |= RegisterCommand<CreateViewMapFolderCommand> (
            navigatorCommands, "1.1.7",
            "Creates a new folder in the View Map."
        );
        err |= RegisterCommand<Set3DCutPlanesCommand> (
            navigatorCommands, "1.3.1",
            "Sets the 3D cut planes. An empty cutPlanes array switches the 3D cut off."
        );
        err |= RegisterCommand<Get3DCutPlanesCommand> (
            navigatorCommands, "1.7.3",
            "Gets whether the 3D cut planes are on and the cut planes (pa, pb, pc, pd) of the 3D window, in the form accepted by Set3DCutPlanes."
        );
        err |= RegisterCommand<Get3DStyleDetailsCommand> (
            navigatorCommands, "1.7.3",
            "Gets the settings of 3D styles by name (an empty name or no names means the current 3D style): rendering engine, model type, monochrome and background colors, contours, shadows and hatching. Available from Archicad 27."
        );
        err |= RegisterCommand<Create3DStylesCommand> (
            navigatorCommands, "1.7.3",
            "Creates 3D styles. Each new style starts from the current 3D style and differs from it only in the given fields. Available from Archicad 27."
        );
        err |= RegisterCommand<Change3DStylesCommand> (
            navigatorCommands, "1.7.3",
            "Changes the given fields of existing 3D styles, identified by name. Available from Archicad 27."
        );
        err |= RegisterCommand<Get3DProjectionSettingsCommand> (
            navigatorCommands, "1.7.2",
            "Gets the 3D projection settings of the 3D window: whether it is perspective or axonometric, the camera and target position, view cone, roll angle, distance and azimuth of a perspective, or the projection type, azimuth and transformation matrix of an axonometry, and the sun position of either (date and time or azimuth and altitude)."
        );
        err |= RegisterCommand<Set3DProjectionSettingsCommand> (
            navigatorCommands, "1.7.2",
            "Sets a perspective or an axonometric projection for the 3D window. Every perspective parameter but sun is required: read a known good set with Get3DProjectionSettings and pass it back, with the values to change replaced. An axonometry changes only the values given: the projection type, the azimuth, the transformation matrix and the sun. The sun of either changes only the values given in it and is kept when sun is missing. The 3D window shows the change after a rebuild (RebuildView), e.g. before SaveCurrentWindowAsImage."
        );
        err |= RegisterCommand<GetCamerasCommand> (
            navigatorCommands, "2.0.0",
            "Gets the Camera Sets (camera paths) of the project with their name, whether the set is the active one, its current camera, and its cameras in path order. Each camera has its element id, whether it is active in the path, and its perspective: camera and target position, distance, azimuth, view cone, roll angle, two-point perspective and sun, in the shape Set3DProjectionSettings accepts, so passing a camera's perspective there shows its view in the 3D window. Archicad cameras are always perspective; they have no name of their own."
        );
        err |= RegisterCommand<Get3DStylesCommand> (
            navigatorCommands, "2.0.0",
            "Gets the names of the 3D styles of the project and the name of the current one, which the 3D window uses."
        );
        err |= RegisterCommand<SetCurrent3DStyleCommand> (
            navigatorCommands, "2.0.0",
            "Meant to make the given 3D style, one of the names Get3DStyles returns, the current one. Not available yet: "
            "it checks the name, then refuses, because Archicad's API function for it crashes Archicad."
        );
        err |= RegisterCommand<StoreViewSettingsCommand> (
            navigatorCommands, "2.0.0",
            "Stores the current view settings (action Store) and puts them back later (action Restore), for example before and after a script that changes the view. The two must be called in pairs and cannot be stacked. Restoring may rebuild the view."
        );
        err |= RegisterCommand<ShowElementsIn3DCommand> (
            navigatorCommands, "2.0.0",
            "Opens the 3D window on the given elements, as Show Selection in 3D does: they become the selection and only they are shown. Without elements it shows every element, as Show All in 3D does."
        );
        err |= RegisterCommand<GetDocumentFrom3DDefaultsCommand> (
            navigatorCommands, "1.8.0",
            "Gets the defaults a newly created 3D Document starts with: name, reference ID, vectorial hatching and sun shadows, transparency, connection to the model, surfaces, boundary contours and the uniform cut, uncut, shadow and hidden edge attributes."
        );
        err |= RegisterCommand<SetDocumentFrom3DDefaultsCommand> (
            navigatorCommands, "1.8.0",
            "Sets the defaults a newly created 3D Document starts with. Only the given fields are changed; existing 3D Documents are not affected. The fields are the ones returned by GetDocumentFrom3DDefaults."
        );
        err |= RegisterCommand<Get3DWindowSettingsCommand> (
            navigatorCommands, "1.7.3",
            "Gets the 3D window settings: the pixel size of the 3D window and the zoom (scale and displacement) of its projection."
        );
        err |= RegisterCommand<Set3DWindowSettingsCommand> (
            navigatorCommands, "1.7.3",
            "Changes the pixel size of the 3D window and/or the zoom (scale and displacement) of its projection; the values not given are kept. Returns the settings read back after the change. Archicad may need a rebuild of the 3D window (RebuildView) before it shows the change, e.g. before SaveCurrentWindowAsImage."
        );
        err |= RegisterCommand<Get3DDocumentSettingsCommand> (
            navigatorCommands, "2.0.0",
            "Gets the settings of a 3D Document, given by its navigator item: the element and story filter, the 3D cut, the projection, the window and zoom, the uniform cut and uncut surface attributes, the source and the boundary contours, with the field names of API_DocumentFrom3DType."
        );
        err |= RegisterCommand<Set3DDocumentSettingsCommand> (
            navigatorCommands, "2.0.0",
            "Changes the given settings of a 3D Document, given by its navigator item, in the form Get3DDocumentSettings returns; the settings not given are kept. Returns the settings read back after the change."
        );
        err |= RegisterCommand<Get3DFilterAndCutSettingsCommand> (
            navigatorCommands, "1.8.0",
            "Gets the 'Filter and Cut Elements in 3D' settings of the 3D window: the stories shown, the filter mode and which element types are shown. Available from Archicad 27."
        );
        err |= RegisterCommand<Set3DFilterAndCutSettingsCommand> (
            navigatorCommands, "1.8.0",
            "Changes the 'Filter and Cut Elements in 3D' settings of the 3D window; only the given fields and the listed element types change. Returns the settings read back after the change. Available from Archicad 27."
        );
        err |= RegisterCommand<GetGridSettingsCommand> (
            navigatorCommands, "1.8.0",
            "Gets the grid and snap grid settings of the database of the current window, and which database that is. Each database has its own grid settings. Available from Archicad 27."
        );
        err |= RegisterCommand<SetGridSettingsCommand> (
            navigatorCommands, "1.8.0",
            "Changes the grid and snap grid settings of the database of the current window; only the given fields change. Returns the settings read back after the change. Available from Archicad 27."
        );
        err |= RegisterCommand<FitInWindowCommand> (
            navigatorCommands, "1.3.1",
            "Zooms to the given elements or fits everything in the window."
        );
        err |= RegisterCommand<ZoomToElementsCommand> (
            navigatorCommands, "1.8.0",
            "Zooms the current window, 2D or 3D, to the given elements: selects them, zooms to the selection and restores the previous selection unless keepSelection is false. Fails when none of the elements could be selected, e.g. all of them are on another story or on a hidden layer."
        );
        err |= RegisterCommand<SetZoomCommand> (
            navigatorCommands, "1.7.3",
            "Zooms the current 2D window to the given box in model coordinates."
        );
        err |= RegisterCommand<ResetZoomCommand> (
            navigatorCommands, "1.7.3",
            "Steps back in the zoom history of the current window, like Archicad's Zoom Back."
        );
        err |= RegisterCommand<CreateSectionsCommand> (
            navigatorCommands, "1.5.0",
            "Creates Section elements on the floor plan. A section looks to the left of its start to end direction, with its depth line that far on that side. GetDetailsOfElements reads the geometry of existing sections back. The floor plan must be the active window: creation fails while a Section or Elevation window is active."
        );
        err |= RegisterCommand<ModifySectionsCommand> (
            navigatorCommands, "2.0.0",
            "Changes the name and the reference ID of Section and Elevation elements, which the Project Map and the markers show. GetDetailsOfElements reads those of a Section back as name and idStr."
        );
        err |= RegisterCommand<CreateInteriorElevationsCommand> (
            navigatorCommands, "1.5.8",
            "Creates Interior Elevation elements on the floor plan. Every consecutive pair of the given points becomes one segment, each with its own viewpoint - feed it the corner points of a room to get an interior elevation of that room."
        );
        err |= RegisterCommand<CreateElevationsCommand> (
            navigatorCommands, "2.0.0",
            "Creates Elevation elements on the floor plan. The floor plan must be the active window: creation fails while a Section or Elevation window is active."
        );
        err |= RegisterCommand<GetSectionsAndElevationsDetailsCommand> (
            navigatorCommands, "2.0.0",
            "Gets every configurable setting of the given Section and Elevation elements, with their name and reference ID."
        );
        err |= RegisterCommand<SetSectionsAndElevationsDetailsCommand> (
            navigatorCommands, "2.0.0",
            "Sets settings of the given Section and Elevation elements: pens, line types, horizontal and vertical range, model effects, story handles and markers. The name and the reference ID are changed with ModifySections."
        );
        err |= RegisterCommand<MoveNavigatorItemCommand> (
            navigatorCommands, "1.1.7",
            "Moves a navigator item to a new parent in the navigator tree, or to a new position within its current parent. Fails when Archicad does not put the item where it was asked to."
        );
        err |= RegisterCommand<RenameNavigatorItemCommand> (
            navigatorCommands, "1.1.7",
            "Renames a navigator item or changes its ID. Renaming the Layout Book item of a placed Drawing also sets the custom name of the Drawing. newId sets the custom layout number of a layout and the custom ID of any other item. For a Project Map section, elevation, detail or worksheet the name and ID of the element behind the viewpoint are changed, for an interior elevation those of the segment the item stands for; the change is undone when the item does not show the new name. The name and ID of other Project Map items (stories, schedules, ...) cannot be changed. Fails when Archicad refuses the change or does not apply the new name or ID. With customName false the item drops its custom name and follows its Project Map source again."
        );
        err |= RegisterCommand<DeleteNavigatorItemsCommand> (
            navigatorCommands, "1.1.7",
            "Deletes navigator items from the navigator tree: views and folders of the View Map, layouts and master layouts of the Layout Book, and independent worksheets and details of the Project Map. Layout subsets, View Map folders that are not empty, the other Project Map items and marker-linked worksheets and details cannot be deleted; the failed execution result says which case applies. An item that Archicad refuses to delete (in a Teamwork project: it is not reserved or the user has no right to delete it) is reported as a failed execution result. dryRun reports the result of the checks made before deleting (item not found, layout subset, View Map folder that is not empty) without deleting anything."
        );
        err |= RegisterCommand<GetNavigatorItemTreeCommand> (
            navigatorCommands, "1.1.7",
            "Returns the full navigator item tree for the specified map. With includeChildCount true every item also reports the number of its direct children."
        );
        err |= RegisterCommand<GetSchedulesCommand> (
            navigatorCommands, "2.0.0",
            "Returns the Interactive Schedules of the Project Map: navigator item id, name, ID and the folders they are in. Archicad's API does not give the rows of a schedule nor export it; to export one, put it in a publisher set with a spreadsheet format in Archicad and run PublishPublisherSet."
        );
        err |= RegisterCommand<GetNavigatorParentItemsCommand> (
            navigatorCommands, "1.7.3",
            "Returns the parent of each given navigator item: the folder, subset or layout book item it is in. The root of a map has no parent and gets an error."
        );
        err |= RegisterCommand<SetMasterLayoutContextCommand> (
            navigatorCommands, "1.8.0",
            "Sets which layout's context the master layout of the current window is drawn in: its autotexts then show the values of the given layout. The current window must be a master layout, otherwise an error is returned. It does not assign a master layout to a layout. Available from Archicad 27."
        );
        err |= RegisterCommand<GetAddOnViewpointRootsCommand> (
            navigatorCommands, "1.8.0",
            "Returns the roots of the add-on viewpoints: the Project Map items created through the Archicad API by an add-on, as Archicad lists them to Tapir. Available from Archicad 27."
        );
        err |= RegisterCommand<GetAddOnViewpointChildrenCommand> (
            navigatorCommands, "1.8.0",
            "Returns the direct children of an add-on viewpoint root or group. Available from Archicad 27."
        );
        err |= RegisterCommand<GetAddOnViewpointsCommand> (
            navigatorCommands, "1.8.0",
            "Returns the details of the given add-on viewpoint items: their parent, type (Root, Group or Leaf), ID, name, view settings flags and attached data (base64 encoded). The output is aligned with the input array; an item that cannot be read gets an error. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateAddOnViewpointsCommand> (
            navigatorCommands, "1.8.0",
            "Creates add-on viewpoint roots, groups and leaf nodes in the Project Map, with their ID, name, view settings flags (leaf nodes only) and attached data (base64 encoded). A group or a leaf node is created in the given parent root or group. Returns the identifier of each new item, or an error for an item that could not be created. Roots and groups can't be created in a Teamwork project. Available from Archicad 27."
        );
        err |= RegisterCommand<ChangeAddOnViewpointsCommand> (
            navigatorCommands, "1.8.0",
            "Changes the ID, name, view settings flags or attached data (base64 encoded) of the given add-on viewpoint items; only the given fields are changed. Roots and groups can't be changed in a Teamwork project. Available from Archicad 27."
        );
        err |= RegisterCommand<ReorderNavigatorItemsCommand> (
            navigatorCommands, "2.0.0",
            "Reorders the children of a navigator item to match the given order. Works around MoveNavigatorItem being unable to reposition an item within its current parent, by bouncing it through a sibling."
        );
        AddCommandGroup (navigatorCommands);
    }

    { // Graphic Override Commands
        CommandGroup graphicOverrideCommands ("Graphic Override Commands");
        err |= RegisterCommand<CreateGraphicalOverrideRuleCommand> (
            graphicOverrideCommands, "1.7.2",
            "Creates or overwrites a graphic override rule in a rule group: name, criterion XML, surface color and line pen. Needs Archicad 27 or later."
        );
        err |= RegisterCommand<GetGraphicalOverrideRulesCommand> (
            graphicOverrideCommands, "1.7.2",
            "Returns every graphic override rule with its criterion XML. Needs Archicad 27 or later."
        );
        err |= RegisterCommand<CreateGraphicalOverrideCombinationCommand> (
            graphicOverrideCommands, "1.7.2",
            "Creates a graphic override combination from rule names, or replaces the rules of an existing one. Needs Archicad 27 or later."
        );
        err |= RegisterCommand<GetGraphicalOverrideStyleOfElementsCommand> (
            graphicOverrideCommands, "1.8.0",
            "Returns the graphic override style each element is drawn with in the current window: for every overridable member (line type, pens, fill types, surfaces, ...) whether the active graphic override combination replaces it, and with what. Needs Archicad 27 or later."
        );
        AddCommandGroup (graphicOverrideCommands);
    }

    { // Issue Management Commands
        CommandGroup issueCommands ("Issue Management Commands");
        err |= RegisterCommand<CreateIssueCommand> (
            issueCommands, "1.0.2",
            "Creates a new issue."
        );
        err |= RegisterCommand<DeleteIssueCommand> (
            issueCommands, "1.0.2",
            "Deletes the specified issue. With dryRun true nothing is deleted and no undo step is created: the response has dryRun set to true, and wouldDelete when the issue exists."
        );
        err |= RegisterCommand<GetIssuesCommand> (
            issueCommands, "1.0.2",
            "Retrieves information about existing issues."
        );
        err |= RegisterCommand<AddCommentToIssueCommand> (
            issueCommands, "1.0.6",
            "Adds a new comment to the specified issue."
        );
        err |= RegisterCommand<GetCommentsFromIssueCommand> (
            issueCommands, "1.0.6",
            "Retrieves comments information from the specified issue."
        );
        err |= RegisterCommand<AttachElementsToIssueCommand> (
            issueCommands, "1.0.6",
            "Attaches elements to the specified issue."
        );
        err |= RegisterCommand<DetachElementsFromIssueCommand> (
            issueCommands, "1.0.6",
            "Detaches elements from the specified issue."
        );
        err |= RegisterCommand<GetElementsAttachedToIssueCommand> (
            issueCommands, "1.0.6",
            "Retrieves attached elements of the specified issue, filtered by attachment type."
        );
        err |= RegisterCommand<ExportIssuesToBCFCommand> (
            issueCommands, "1.0.6",
            "Exports specified issues to a BCF file. With overwrite false it refuses to replace an existing file."
        );
        err |= RegisterCommand<ImportIssuesFromBCFCommand> (
            issueCommands, "1.0.6",
            "Imports issues from the specified BCF file."
        );
        err |= RegisterCommand<ImportMarkupsFromPDFCommand> (
            issueCommands, "1.7.3",
            "Imports the markups of the specified PDF file as issues, like Issue Manager > Import PDF markups, without any dialog and without opening the Issue Manager palette, and reports the issues it created. Available from Archicad 29."
        );
        AddCommandGroup (issueCommands);
    }

    { // Revision Management Commands
        CommandGroup issueCommands ("Revision Management Commands");
        err |= RegisterCommand<GetRevisionIssuesCommand> (
            issueCommands, "1.1.9",
            "Retrieves all issues."
        );
        err |= RegisterCommand<ModifyRevisionIssuesCommand> (
            issueCommands, "1.8.0",
            "Modifies the ID, the short description and the custom scheme data of the given revision issues. Only the given fields are changed. Issued revision issues may refuse the change."
        );
        err |= RegisterCommand<GetRevisionChangesCommand> (
            issueCommands, "1.1.9",
            "Retrieves all changes."
        );
        err |= RegisterCommand<GetRevisionChangeCustomSchemeCommand> (
            issueCommands, "1.7.3",
            "Retrieves the custom fields of changes (the change custom scheme): the key and the name of each field. The keys are the customSchemeKey values in the customSchemeData of changes."
        );
        err |= RegisterCommand<GetDocumentRevisionsCommand> (
            issueCommands, "1.1.9",
            "Retrieves all document revisions."
        );
        err |= RegisterCommand<GetCurrentRevisionChangesOfLayoutsCommand> (
            issueCommands, "1.1.9",
            "Retrieves all changes belong to the last revision of the given layouts."
        );
        err |= RegisterCommand<GetRevisionChangesOfElementsCommand> (
            issueCommands, "1.1.9",
            "Retrieves the changes belong to the given elements."
        );
        AddCommandGroup (issueCommands);
    }

    { // Design Options Commands
        CommandGroup designOptionsCommands ("Design Options Commands");
        err |= RegisterCommand<GetDesignOptionsCommand> (
            designOptionsCommands, "1.2.9",
            "Retrieves information about existing design options. Available from Archicad 29."
        );
        err |= RegisterCommand<GetDesignOptionSetsCommand> (
            designOptionsCommands, "1.2.9",
            "Retrieves information about existing design option sets, and in Teamwork projects the Teamwork identifier of the design options. Available from Archicad 29."
        );
        err |= RegisterCommand<GetDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.2.9",
            "Retrieves information about existing design option combinations."
        );
        err |= RegisterCommand<GetElementsOfDesignOptionsCommand> (
            designOptionsCommands, "1.5.1",
            "Retrieves the elements associated with the given design options. Available from Archicad 29."
        );
        err |= RegisterCommand<GetDesignOptionForElementsCommand> (
            designOptionsCommands, "1.5.1",
            "Retrieves the design option association for the specified elements. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateDesignOptionSetsCommand> (
            designOptionsCommands, "1.5.1",
            "Creates new design option sets with the given names. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateDesignOptionsCommand> (
            designOptionsCommands, "1.5.1",
            "Creates new design options with the given parameters. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.5.1",
            "Creates new design option combinations with the given parameters. Available from Archicad 29."
        );
        err |= RegisterCommand<SetActiveDesignOptionsInCombinationsCommand> (
            designOptionsCommands, "1.5.1",
            "Sets active design options in the given combinations. Available from Archicad 29."
        );
        err |= RegisterCommand<ActivateDesignOptionsInCombinationsCommand> (
            designOptionsCommands, "1.8.0",
            "Activates one design option in each of the given combinations, without listing all active design options of the combination. Available from Archicad 29."
        );
        err |= RegisterCommand<DeactivateDesignOptionsInCombinationsCommand> (
            designOptionsCommands, "1.7.3",
            "Turns off the given design options in the given combinations, leaving their other options as they are. Available from Archicad 29."
        );
        err |= RegisterCommand<MoveElementsToDesignOptionsCommand> (
            designOptionsCommands, "1.5.1",
            "Moves the given elements into the given design options. Use NULLGuid for design option to remove the element from any design options and move it to the main model. Available from Archicad 29."
        );
        err |= RegisterCommand<MoveDesignOptionsToAnotherSetCommand> (
            designOptionsCommands, "1.5.1",
            "Moves the given design options to another sets. Available from Archicad 29."
        );
        err |= RegisterCommand<RenameDesignOptionsCommand> (
            designOptionsCommands, "1.7.2",
            "Changes the name and/or the string id of the given design options. Available from Archicad 29."
        );
        err |= RegisterCommand<RenameDesignOptionSetsCommand> (
            designOptionsCommands, "1.7.2",
            "Changes the name of the given design option sets. Available from Archicad 29."
        );
        err |= RegisterCommand<RenameDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.7.2",
            "Changes the name of the given design option combinations. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteDesignOptionsCommand> (
            designOptionsCommands, "1.7.2",
            "Deletes the given design options. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteDesignOptionSetsCommand> (
            designOptionsCommands, "1.7.2",
            "Deletes the given design option sets. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.7.2",
            "Deletes the given design option combinations. Available from Archicad 29."
        );
        err |= RegisterCommand<GetHotlinkDesignOptionTablesCommand> (
            designOptionsCommands, "2.0.0",
            "Retrieves the design option table of the given hotlink nodes and placed hotlink instances: the filter mode (main model only, by options or by combination), the design option sets, options and combinations of the hotlinked module and which of them are active. Available from Archicad 29."
        );
        err |= RegisterCommand<SetHotlinkDesignOptionTablesCommand> (
            designOptionsCommands, "2.0.0",
            "Sets the filter mode, the active combination and the active option of sets in the design option table of the given placed hotlink instances. Available from Archicad 29."
        );
        err |= RegisterCommand<SetDefaultDesignOptionCommand> (
            designOptionsCommands, "1.8.0",
            "Sets the given design option as the default, or the Main Model when no design option is given. Available from Archicad 29."
        );
        err |= RegisterCommand<ReorderDesignOptionSetsCommand> (
            designOptionsCommands, "1.8.0",
            "Moves the given design option sets before another set or to the end of the list. Available from Archicad 29."
        );
        err |= RegisterCommand<ReorderDesignOptionsInSetCommand> (
            designOptionsCommands, "1.8.0",
            "Moves the given design options before another design option of the same set or to the end of their set. Available from Archicad 29."
        );
        err |= RegisterCommand<CanMoveElementsToDesignOptionsCommand> (
            designOptionsCommands, "1.8.0",
            "Checks whether the given elements can be moved to another design option or to the main model, with the reason when they cannot. Available from Archicad 29."
        );
        err |= RegisterCommand<DuplicateDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.7.3",
            "Duplicates the given design option combinations and returns the identifiers of the new combinations. Available from Archicad 29."
        );
        err |= RegisterCommand<GetDesignOptionCombinationOfViewsCommand> (
            designOptionsCommands, "2.0.0",
            "Retrieves which design option combination the given views show: only the main model, a combination, a custom set of design options or a deleted combination. Available from Archicad 27."
        );
        err |= RegisterCommand<SetDesignOptionCombinationOfViewsCommand> (
            designOptionsCommands, "2.0.0",
            "Sets the design option combination the given views show, or makes them show only the main model. Available from Archicad 27."
        );
        err |= RegisterCommand<GetHotlinkDesignOptionTableDetailsCommand> (
            designOptionsCommands, "1.8.0",
            "Reads the design option table of hotlink nodes: the design option sets, options and combinations of the hotlinked source file, and which of them the node shows. Available from Archicad 29."
        );
        AddCommandGroup (designOptionsCommands);
    }

    { // Keynote Commands
        CommandGroup keynoteCommands ("Keynote Commands");
        err |= RegisterCommand<GetKeynoteTreeCommand> (
            keynoteCommands, "1.5.6",
            "Retrieves the whole keynote folder and item hierarchy. The technical root folder is not included in the output; the top-level folders and items are returned directly. Available from Archicad 28."
        );
        err |= RegisterCommand<GetKeynoteAutoTextsCommand> (
            keynoteCommands, "1.5.6",
            "Retrieves the autotext tokens of the given keynote items. The tokens can be used as label text content to reference the fields of a keynote item. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateKeynoteFoldersCommand> (
            keynoteCommands, "1.5.6",
            "Creates keynote folders under the given parent folders (or under the root folder). Available from Archicad 28."
        );
        err |= RegisterCommand<CreateKeynoteItemsCommand> (
            keynoteCommands, "1.5.6",
            "Creates keynote items in the given parent folders (or in the root folder). Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyKeynoteFoldersCommand> (
            keynoteCommands, "1.5.6",
            "Modifies the key, title or reference of the given keynote folders. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyKeynoteItemsCommand> (
            keynoteCommands, "1.5.6",
            "Modifies the key, title, description or reference of the given keynote items. Available from Archicad 28."
        );
        err |= RegisterCommand<DeleteKeynoteFoldersCommand> (
            keynoteCommands, "1.5.6",
            "Deletes the given keynote folders including their content. Available from Archicad 28."
        );
        err |= RegisterCommand<DeleteKeynoteItemsCommand> (
            keynoteCommands, "1.5.6",
            "Deletes the given keynote items. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateKeynoteLabelsCommand> (
            keynoteCommands, "1.5.6",
            "Creates Label elements that reference the given keynote items via autotext. Available from Archicad 28."
        );
        AddCommandGroup (keynoteCommands);
    }

    { // Project Data Commands
        CommandGroup projectDataCommands ("Project Data Commands");
        err |= RegisterCommand<GetProjectDataKeysCommand> (
            projectDataCommands, "1.7.3",
            "Lists the project data stored by Tapir in the project: the names, the identifiers of the add-on objects that store them, whether they are kept on this client only and the Teamwork user who has reserved them. Project data is text (for example JSON) saved in the project file by scripts and Grasshopper definitions, such as their settings or the mapping of elements to an external system."
        );
        err |= RegisterCommand<GetProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Gets the content of the project data with the given names."
        );
        err |= RegisterCommand<SetProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Creates or overwrites project data: text (for example JSON) saved in the project under a name. New project data can be kept on this client only, or created as unique so that in Teamwork only one can exist with its name. Prefix the names with the name of your tool (for example 'my-tool/settings') so scripts do not overwrite each other's data."
        );
        err |= RegisterCommand<DeleteProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Deletes the project data with the given names."
        );
        err |= RegisterCommand<ReserveProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Reserves the project data with the given names in Teamwork, so that other users cannot change it. Returns the users who have already reserved some of it."
        );
        err |= RegisterCommand<ReleaseProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Releases the project data with the given names in Teamwork."
        );
        err |= RegisterCommand<RequestProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Requests the project data with the given names in Teamwork from the users who have reserved it, with an optional message."
        );
        err |= RegisterCommand<GrantProjectDataCommand> (
            projectDataCommands, "1.7.3",
            "Grants the project data with the given names, reserved by the current user, to another Teamwork user."
        );
        err |= RegisterCommand<GetModulDataCommand> (
            projectDataCommands, "2.0.0",
            "Gets the module data Tapir stored in the project with StoreModulData: the given names, or all of them, with their data version and, unless includeContent is false, their content. Module data are the custom data sections of Archicad's ModulData Manager: text saved in the project file under a name, kept apart from project data (SetProjectData)."
        );
        err |= RegisterCommand<StoreModulDataCommand> (
            projectDataCommands, "2.0.0",
            "Creates or overwrites module data: text (for example JSON) saved in the project under a name, with an optional data version. Prefix the names with the name of your tool (for example 'my-tool/settings') so scripts do not overwrite each other's data. Storing module data is not an undoable step."
        );
        err |= RegisterCommand<DeleteModulDataCommand> (
            projectDataCommands, "2.0.0",
            "Deletes the module data with the given names. Deleting module data is not an undoable step."
        );
        AddCommandGroup (projectDataCommands);
    }

    { // MEP Commands
        CommandGroup mepCommands ("MEP Commands");
        err |= RegisterCommand<GetMEPElementsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the MEP (Mechanical, Electrical, Plumbing) elements of the project (every element the MEP API lists, and from Archicad 30 the electrical panels and devices), optionally filtered by type and domain. MEP elements are ordinary elements, so the generic element commands work on them as well (for example they can be deleted with the DeleteElements command). Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPRoutingElementsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the details of the given MEP routing elements: domain, MEP system, offset from the home story, branch preference table, the ports logically connected at the route's beginning and end, the floor plan display settings (cut plane position, floor plan display option, show projection and on-story visibility, from Archicad 29), route polyline, segments with cross section data (shape, size, preference table and reference id), begin and end nodes, anchor, reference line offsets, wall thickness, flange size, connection style, insulation thickness and rigid segments, and nodes with their incoming and outgoing segments, preferred transition placement, elbows and transitions. Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPPortsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the ports of the given MEP elements including position, shape, size, connection status (Archicad 27: whether the port is connected, from Archicad 28: whether it is physically connected) and connection style (from Archicad 30), the flow direction of piping and ventilation ports (from Archicad 28), the nominal size of piping ports (from Archicad 30), and the outer diameter, wall thickness and flange size (Archicad 30) of piping and ventilation ports. Available from Archicad 27."
        );
        err |= RegisterCommand<GetMEPDistributionSystemsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the MEP distribution systems of the project with their domain, MEP system attribute, member elements, physical systems (from Archicad 29) and optionally their tree: the elements from the root element on, each with its previous and next elements and the connecting ports. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateMEPRoutingElementsCommand> (
            mepCommands, "1.5.6",
            "Creates MEP routing elements (duct, pipe or cable carrier routes) along the given polylines with optional cross section data (shape, size, preference table and reference id), MEP system and layer (also set on the segments, elbows and transitions), and optional elbow and transition defaults (preference tables and elbow radius factor, from Archicad 29) that apply only to what is placed. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateMEPElementsCommand> (
            mepCommands, "1.5.6",
            "Creates MEP elements (Terminal, Accessory, Equipment or Fitting) at the given positions, optionally with another library part, size (from Archicad 30), MEP system, port sizes and shapes and a changed insulation, which apply only to what is placed. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyMEPRoutingElementsCommand> (
            mepCommands, "1.5.6",
            "Modifies the given MEP routing elements: MEP system, branch preference table, floor plan display settings (cut plane position, floor plan display option, show projection and on-story visibility, from Archicad 29), cross section data (shape, size, preference table and reference id), anchor, reference line offsets, wall thickness, flange size and connection style of all segments, preferred transition placement of all nodes, insulation thickness of the whole route (segments, elbows and transitions), and node positions. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyMEPPortsCommand> (
            mepCommands, "1.8.0",
            "Modifies the given MEP ports (as returned by GetMEPPorts): shape, connection style (from Archicad 30), the flow direction of piping and ventilation ports, and the outer diameter and nominal size (from Archicad 30) of piping ports. Only the given fields are changed. Available from Archicad 28."
        );
        err |= RegisterCommand<ConnectMEPElementsCommand> (
            mepCommands, "1.5.6",
            "Connects MEP routing elements to other MEP elements or routes. Merges routes, splits routes or creates branch elements as needed. The first or last node of the 'connectToId' route must lie exactly on the 'routingElementId' route, so for a tee pass the main route as 'routingElementId' and the branch as 'connectToId'. A failed connection says which condition is not met. Archicad 27 does not report the merged, split or created elements. Available from Archicad 27."
        );
        err |= RegisterCommand<CheckMEPPortConnectabilityCommand> (
            mepCommands, "1.8.0",
            "Checks whether MEP ports can be connected to other ports, for the given pairs of ports. Archicad 28 and later check the physical connection. Available from Archicad 27."
        );
        err |= RegisterCommand<GetMEPPreferenceTablesCommand> (
            mepCommands, "1.5.7",
            "Gets the circular cross section preference tables (referenceId, diameter, wall thickness, description) and the reference set of the Piping or Ventilation domain, optionally with the elbow, branch and transition preference tables, plus the rectangular duct tables of the Ventilation domain or the segment tables of the CableCarrier domain. Each table has its name and whether it is editable; the segment tables also have their segmentation lengths from Archicad 29. The rows of the circular, elbow, branch and transition tables say whether they are valid, and the reference set and the angle set say which of their reference ids and angles are defined. The angle set and the transition tables are available from Archicad 29. Every row has its index in the table, as ModifyMEPPreferenceTables expects it. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyMEPPreferenceTablesCommand> (
            mepCommands, "1.8.0",
            "Modifies the MEP preference tables of a domain, in the shape GetMEPPreferenceTables returns them: adds tables, changes their names, segmentation lengths (from Archicad 29) and row values and descriptions, deletes or empties rows, adds rows to the rectangular duct and cable carrier tables, deletes tables, deletes reference ids from the reference set, and adds and deletes angles of the angle set (from Archicad 29). Each table is changed in one undoable step. Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPSystemGroupsCommand> (
            mepCommands, "1.7.2",
            "Gets the MEP system groups of the project with their physical systems: name, editable flag, root element and MEP system. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateMEPSystemGroupsCommand> (
            mepCommands, "1.7.2",
            "Creates MEP system groups. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateMEPPhysicalSystemsCommand> (
            mepCommands, "1.7.2",
            "Creates physical systems in MEP system groups, optionally with a root element and an MEP system as their category. Available from Archicad 29."
        );
        err |= RegisterCommand<ModifyMEPSystemGroupsCommand> (
            mepCommands, "1.7.3",
            "Renames MEP system groups. Available from Archicad 29."
        );
        err |= RegisterCommand<ModifyMEPPhysicalSystemsCommand> (
            mepCommands, "1.7.3",
            "Modifies MEP physical systems: name, root element and the MEP system used as their category. Only the given fields are changed. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteMEPPhysicalSystemsCommand> (
            mepCommands, "1.7.3",
            "Removes MEP physical systems from their system groups. Their member elements are not deleted. Available from Archicad 29."
        );
        err |= RegisterCommand<GetMEPToolDefaultsCommand> (
            mepCommands, "2.0.0",
            "Retrieves the current settings of the MEP tools per domain, without changing them: the routing tool (MEP system, branch preference table, floor plan display settings from Archicad 29, and its segment, node, elbow (Archicad 27 and from Archicad 29) and transition defaults), its rigid segment and branch defaults, the flexible segment tool, the library part, insulation and port settings of the terminal, accessory, fitting and equipment tools, and from Archicad 30 the electrical panel tool with its number of circuit defaults. Archicad 27 gives fewer settings, and only the shape of the ports. Available from Archicad 27."
        );
        err |= RegisterCommand<TransferMEPSettingsCommand> (
            mepCommands, "2.0.0",
            "Picks up the settings of an MEP element and injects them into other MEP elements of the same type and domain, like Archicad's pick up and inject parameters. Works for routing elements, terminals, accessories, equipment, fittings, flexible segments, branches, take-offs and (from Archicad 30) electrical panels and devices. Each target is changed in its own undoable step. Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPPhysicalSystemsOfElementsCommand> (
            mepCommands, "2.0.0",
            "Returns the physical systems each of the given MEP elements is part of. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateMEPFlexibleSegmentsCommand> (
            mepCommands, "2.0.0",
            "Creates MEP flexible segments through the given control points, or along a polyline of alternating line and spline parts (Piping), with the current settings of the Flexible Segment tool of their domain, optionally with the orientation (control points) or direction (polyline) at both ends. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateMEPTakeOffsCommand> (
            mepCommands, "2.0.0",
            "Creates MEP take-offs on the given routing segments, at a distance from the beginning of the segment (not more than its length) and optionally rotated around its axis by a multiple of 90 degrees, with the current settings of the Take-Off tool of their domain (by default the domain of the segment's routing element), optionally with their length and without insulation, which apply only to what is created. Archicad only has Ventilation take-offs. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyMEPElementsCommand> (
            mepCommands, "2.0.0",
            "Modifies placed MEP elements: the anchor point, orientation and library part of terminals, accessories, equipment, fittings and flexible segments, the control points, polyline (Piping, from Archicad 28), and width and height of flexible segments. Only the given fields are changed, in one undo step with one execution result per element; an item with a field that does not fit its element changes nothing. Archicad reports a 'libraryPartId' it does not find only after the item's other changes were made, so that item's error says which changes stayed. Available from Archicad 27."
        );
        err |= RegisterCommand<GetMEPRoutingParametersCommand> (
            mepCommands, "1.8.0",
            "Gets the routing parameters of placed MEP elements: of a routing element its routing node parameters (preferred transition placement, elbow from Archicad 29, transition) and routing segment parameters (cross section, preference table, anchor, reference line offsets, wall thickness, flange size, connection style); of a routing node its elbow (from Archicad 29) and transition parameters; of a routing segment its rigid segment parameters (Archicad 28 only). Available from Archicad 28."
        );
        err |= RegisterCommand<SetMEPRoutingParametersCommand> (
            mepCommands, "1.8.0",
            "Sets the routing parameters of placed MEP elements: the routing segment parameters (cross section and preference table) and, from Archicad 29, the routing node parameters (elbow radius factor and preference table, transition preference table) of routing elements, and the elbow and transition parameters of routing nodes (from Archicad 29). Only the given keys are changed, with one execution result per element. A changed preference table is also set on each of the route's segments and nodes and injected into them, because Archicad 29 can otherwise keep the old one; the item fails if they still report the old table. Available from Archicad 28."
        );
        err |= RegisterCommand<GetElectricalPanelsCommand> (
            mepCommands, "1.7.2",
            "Gets the electrical panels (distribution boards) of the project or the given ones: name, circuit naming, circuits, parent circuit, placement and the connected and demand loads in total and per phase. Available from Archicad 30."
        );
        err |= RegisterCommand<GetElectricalCircuitsCommand> (
            mepCommands, "1.7.2",
            "Gets the given electrical circuits, the circuits of the given panels, or every circuit of the project: name, description, owner panel, connected devices and panels, voltage, phases, wiring type, loads, voltage drop and estimated cable length. Available from Archicad 30."
        );
        err |= RegisterCommand<GetElectricalDevicesCommand> (
            mepCommands, "1.7.2",
            "Gets the electrical devices (lights, sockets and other consumers) of the project or the given ones: parent circuit, placement, apparent power, load classification, demand load and directly connected MEP elements. Available from Archicad 30."
        );
        err |= RegisterCommand<GetElectricalProjectSettingsCommand> (
            mepCommands, "1.7.2",
            "Gets the electrical project settings: the default voltage, the wiring types with their cross-sectional area and the load classifications with their demand factor. Available from Archicad 30."
        );
        err |= RegisterCommand<CreateElectricalPanelsCommand> (
            mepCommands, "1.7.2",
            "Places electrical panels with the default settings of the Electrical Panel tool, optionally with the given library part, size, rotation, mirroring, panel name and circuit naming. Available from Archicad 30."
        );
        err |= RegisterCommand<CreateElectricalDevicesCommand> (
            mepCommands, "1.7.2",
            "Places electrical devices with the default settings of the Electrical Device tool, optionally with the given library part, size, rotation, mirroring, apparent power and load classification, and connects them to a circuit. Available from Archicad 30."
        );
        err |= RegisterCommand<ModifyElectricalPanelsCommand> (
            mepCommands, "1.7.2",
            "Modifies the name, circuit naming and placement (anchor point, orientation, rotation, mirroring, library part, size) of electrical panels. Available from Archicad 30."
        );
        err |= RegisterCommand<CreateElectricalCircuitsCommand> (
            mepCommands, "1.7.2",
            "Adds new circuits to electrical panels, optionally with the given description, voltage, wiring type and used phases. Available from Archicad 30."
        );
        err |= RegisterCommand<DeleteElectricalCircuitsCommand> (
            mepCommands, "1.7.2",
            "Removes electrical circuits from their panels. Available from Archicad 30."
        );
        err |= RegisterCommand<ModifyElectricalCircuitsCommand> (
            mepCommands, "1.7.2",
            "Modifies the description, voltage, wiring type and used phases of electrical circuits. Available from Archicad 30."
        );
        err |= RegisterCommand<ModifyElectricalDevicesCommand> (
            mepCommands, "1.7.2",
            "Modifies the apparent power, load classification and placement (anchor point, orientation, rotation, mirroring, library part, size) of electrical devices. Available from Archicad 30."
        );
        err |= RegisterCommand<ConnectToElectricalCircuitsCommand> (
            mepCommands, "1.7.2",
            "Connects electrical devices, or panels as sub-panels, to electrical circuits. Available from Archicad 30."
        );
        err |= RegisterCommand<ModifyElectricalProjectSettingsCommand> (
            mepCommands, "1.7.2",
            "Modifies the electrical project settings: sets the default voltage and clears, adds or removes wiring types and load classifications. Available from Archicad 30."
        );
        err |= RegisterCommand<GetMEPElementDetailsCommand> (
            mepCommands, "1.8.0",
            "Retrieves the type-specific details of the given MEP elements: insulation thickness and routing segment of rigid segments, control points and polyline of flexible segments, radius factor, insulation thickness, preference table and routing node of elbows (from Archicad 29), angle, length and insulation thickness of branches, and offsets, wider and narrower ports, insulation thickness, routing node and (from Archicad 29) preference table of transitions. Available from Archicad 28."
        );
        AddCommandGroup (mepCommands);
    }

    { // MEP Commands (extended)
        CommandGroup mepExtendedCommands ("MEP Commands (extended)");
        err |= RegisterCommand<GetMEPPreferenceTableRowsCommand> (
            mepExtendedCommands, "1.8.0",
            "Gets rows of a segment, branch, elbow or (from Archicad 29) transition preference table by their key (the reference ids and, for branch and elbow rows from Archicad 29, the angle): whether the row is valid and its values (diameters, wall thickness, radius, length, angle, branch offset, connection length, description), and whether the table exists. Rectangular duct and cable carrier segment tables only give whether they exist, as their rows have no key. On Archicad 27, which has one kind of preference table, a row is found by its shape and index and has a value and a description; the table's name and number of rows per shape are returned too. Available from Archicad 27."
        );
        err |= RegisterCommand<SetMEPPreferenceTableRowsCommand> (
            mepExtendedCommands, "1.8.0",
            "Sets the given values of rows of a segment, branch, elbow or (from Archicad 29) transition preference table, found by their key (the reference ids and, for branch and elbow rows from Archicad 29, the angle). Only the given values are changed, in one undo step. On Archicad 27 a row is found by its shape and index, its value and description are set, and its index after the call is returned; the keys give the indices before the call. Available from Archicad 27."
        );
        err |= RegisterCommand<AddMEPPreferenceTableRowsCommand> (
            mepExtendedCommands, "1.8.0",
            "Adds rows with the given keys and optionally values to a branch, elbow or transition preference table and returns their index, in one undo step. Branch tables from Archicad 28, elbow and transition tables from Archicad 29; segment tables have a row for each reference id. On Archicad 27 a row is added to the list of the given shape of any preference table, and the index of each new row after the call is returned. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteMEPPreferenceTableRowsCommand> (
            mepExtendedCommands, "1.8.0",
            "Deletes rows of a segment, branch, elbow or (from Archicad 29) transition preference table by their key (the reference ids and, for branch and elbow rows from Archicad 29, the angle), in one undo step. The rows of segment tables, and of the elbow tables of Archicad 28, are emptied instead. On Archicad 27 a row is found by its shape and index. Available from Archicad 27."
        );
        err |= RegisterCommand<GetMEPPreferenceTableContainersCommand> (
            mepExtendedCommands, "1.8.0",
            "Lists the preference tables of the Piping, Ventilation and Cable Carrier preference table containers of Archicad 27: their id, name, whether they exist and their number of rows per shape, and whether the container exists. Only for Archicad 27; from Archicad 28 use GetMEPPreferenceTables."
        );
        err |= RegisterCommand<CreateMEPPreferenceTablesCommand> (
            mepExtendedCommands, "1.8.0",
            "Adds empty preference tables with the given names to the preference table container of a domain of Archicad 27 and returns their id, in one undo step. Only for Archicad 27."
        );
        AddCommandGroup (mepExtendedCommands);
    }

    { // Analytical Commands
        CommandGroup analyticalCommands ("Analytical Commands");
        err |= RegisterCommand<GetCurrentAnalyticalModelCommand> (
            analyticalCommands, "1.7.3",
            "Retrieves the Structural Analytical Model created for the current view. Available from Archicad 27."
        );
        err |= RegisterCommand<UpdateAnalyticalModelCommand> (
            analyticalCommands, "1.7.3",
            "Triggers an update of the given Structural Analytical Model, or of the model of the current view when no model is given. Available from Archicad 27."
        );
        err |= RegisterCommand<SetAnalyticalGenerationSettingsToNoRuleCommand> (
            analyticalCommands, "1.7.3",
            "Sets the generation settings of the current Structural Analytical Model to no rules. Available from Archicad 27."
        );
        err |= RegisterCommand<GetAnalyticalModelCheckTypeIdCommand> (
            analyticalCommands, "1.7.3",
            "Retrieves the type identifier of the Structural Analytical Model check. Available from Archicad 29."
        );
        err |= RegisterCommand<GetAnalyticalModelVariationCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the variation of the given Structural Analytical Model, or of the model of the current view when no model is given (the model is required before Archicad 27). The fields of the variation depend on the Archicad version."
        );
        err |= RegisterCommand<GetAnalyticalModelCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the Structural Analytical Model of the given variation, as returned by GetAnalyticalModelVariation."
        );
        err |= RegisterCommand<GetAnalyticalVisibilitySettingsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the visibility settings of the Structural Analytical Model. Available from Archicad 26."
        );
        err |= RegisterCommand<SetAnalyticalVisibilitySettingsCommand> (
            analyticalCommands, "1.8.0",
            "Changes the given fields of the Structural Analytical Model visibility settings; the others keep their value. Available from Archicad 26."
        );
        err |= RegisterCommand<GetAnalyticalMemberTypesOfElementsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the analytical member type (Beam, Column, Roof, Slab, Wall or Unknown) of the given elements."
        );
        err |= RegisterCommand<GetAnalyticalLoadGroupsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the structural analysis load groups of the project with all their fields."
        );
        err |= RegisterCommand<GetAnalyticalLoadCasesCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the structural analysis load cases of the given load group, or of the project when no load group is given, with all their fields."
        );
        err |= RegisterCommand<GetAnalyticalLoadCombinationsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the structural analysis load combinations of the project with all their fields and load factors."
        );
        err |= RegisterCommand<CreateAnalyticalLoadGroupsCommand> (
            analyticalCommands, "1.8.0",
            "Creates structural analysis load groups in one undo step and returns their identifiers."
        );
        err |= RegisterCommand<CreateAnalyticalLoadCasesCommand> (
            analyticalCommands, "1.8.0",
            "Creates structural analysis load cases in the given load groups in one undo step and returns their identifiers."
        );
        err |= RegisterCommand<CreateAnalyticalLoadCombinationsCommand> (
            analyticalCommands, "1.8.0",
            "Creates structural analysis load combinations, optionally with their load factors, in one undo step and returns their identifiers. The combination type is available from Archicad 27."
        );
        err |= RegisterCommand<DeleteAnalyticalLoadGroupsCommand> (
            analyticalCommands, "1.8.0",
            "Deletes the given structural analysis load groups in one undo step."
        );
        err |= RegisterCommand<DeleteAnalyticalLoadCasesCommand> (
            analyticalCommands, "1.8.0",
            "Deletes the given structural analysis load cases in one undo step."
        );
        err |= RegisterCommand<DeleteAnalyticalLoadCombinationsCommand> (
            analyticalCommands, "1.8.0",
            "Deletes the given structural analysis load combinations in one undo step."
        );
        err |= RegisterCommand<AddLoadCasesToLoadCombinationCommand> (
            analyticalCommands, "1.8.0",
            "Adds load cases with their factors to the given structural analysis load combination in one undo step."
        );
        err |= RegisterCommand<GetAnalyticalMembersCommand> (
            analyticalCommands, "1.8.0",
            "Lists the elements of the curve and surface members of the given Structural Analytical Model, or of the model of the current view when no model is given (the model is required before Archicad 27)."
        );
        err |= RegisterCommand<GetAnalyticalMemberDetailsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the curve, surface or segmented surface member of the given elements in a Structural Analytical Model, with their connections. Cross sections, curves, nodes and surfaces are returned with every field the Archicad version has."
        );
        err |= RegisterCommand<GetAnalyticalReleasesCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the releases of the given analytical members. Available from Archicad 26."
        );
        err |= RegisterCommand<SetAnalyticalReleasesCommand> (
            analyticalCommands, "1.8.0",
            "Replaces the releases of the given analytical members, in one undo step. Available from Archicad 26."
        );
        err |= RegisterCommand<GetCustomCurveMemberStretchCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the lengths added to the start and end of the given curve members. Available from Archicad 29."
        );
        err |= RegisterCommand<SetCustomCurveMemberStretchCommand> (
            analyticalCommands, "1.8.0",
            "Sets the lengths added to the start and end of the given curve members, in one undo step. Available from Archicad 29."
        );
        err |= RegisterCommand<GetLCSOfAnalyticalMembersCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the local coordinate system of the given analytical member parts, optionally in their intersection with another member part. Available from Archicad 26."
        );
        err |= RegisterCommand<GetAnalyticalMemberPartGeometriesCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the node of the given analytical member vertices and the curve of the given member edges. Available from Archicad 26."
        );
        err |= RegisterCommand<GetAnalyticalSupportsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the settings, the supported member part and the geometry of the given analytical supports. Available from Archicad 26."
        );
        err |= RegisterCommand<GetAnalyticalLinksCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the settings, the connected member parts and the geometry of the given analytical links. Available from Archicad 26."
        );
        err |= RegisterCommand<CreateAnalyticalSupportsCommand> (
            analyticalCommands, "1.8.0",
            "Creates analytical supports from their settings and supported member part, as GetAnalyticalSupports returns them, in one undo step. Available from Archicad 26."
        );
        err |= RegisterCommand<CreateAnalyticalLinksCommand> (
            analyticalCommands, "1.8.0",
            "Creates analytical links from their settings and connected member parts, as GetAnalyticalLinks returns them, in one undo step. Available from Archicad 26."
        );
        err |= RegisterCommand<ConvertAnalyticalLinksToNonShortestCommand> (
            analyticalCommands, "1.8.0",
            "Converts the given shortest analytical links (associative to a curve member or a surface member edge) to links connected to a fixed coordinate, in one undo step, and returns the converted links. Available from Archicad 26."
        );
        err |= RegisterCommand<GetAnalyticalLoadsCommand> (
            analyticalCommands, "1.8.0",
            "Retrieves the settings, the loaded member part, the custom data (the curve of an edge load, the contour of a surface load) and the geometry of the given analytical point, edge and surface loads. Available from Archicad 26."
        );
        err |= RegisterCommand<CreateAnalyticalLoadsCommand> (
            analyticalCommands, "1.8.0",
            "Creates analytical point, edge and surface loads from their settings, loaded member part and custom data, as GetAnalyticalLoads returns them, in one undo step. Available from Archicad 26."
        );

        err |= RegisterCommand<CreateElementsFromAnalyticalMembersCommand> (
            analyticalCommands, "2.0.0",
            "Creates elements (beams, columns, walls, slabs, roofs) from the given curve, surface or segmented surface members, in the form GetAnalyticalMemberDetails returns them, optionally with the settings of a favorite, in one undo step. Returns the new elements and, from Archicad 26, the mapping of their edges and vertices to member parts."
        );
        AddCommandGroup (analyticalCommands);
    }

    { // Solid Element Operation Commands
        CommandGroup solidElementOperationCommands ("Solid Element Operation Commands");
        err |= RegisterCommand<CreateSolidElementLinksCommand> (
            solidElementOperationCommands, "1.5.4",
            "Creates solid element operation links between target and operator elements."
        );
        err |= RegisterCommand<RemoveSolidElementLinksCommand> (
            solidElementOperationCommands, "1.5.4",
            "Removes solid element operation links between target and operator elements. With dryRun true nothing is removed and no undo step is created: the response has dryRun set to true, and each link that exists gets wouldDelete."
        );
        err |= RegisterCommand<GetSolidElementLinksCommand> (
            solidElementOperationCommands, "1.5.4",
            "Returns solid element operation links for each queried element, grouped by role (target or operator)."
        );
        err |= RegisterCommand<CreateSolidOperationsCommand> (
            solidElementOperationCommands, "1.8.0",
            "Runs a solid operation (subtraction, intersection, addition) on two morphs and returns the resulting morphs, one item per operation. Unlike CreateSolidElementLinks this is not a link: the target and the operator morph are replaced by the result. Morphs only; the operations run in one undo step."
        );
        err |= RegisterCommand<TrimElementsCommand> (
            solidElementOperationCommands, "1.5.9",
            "Trims construction elements with a roof or shell: the roofs and shells in the list, or one given trimming element with a trim type. With several roofs or shells in the list Archicad decides which of them trims which element (they may also trim each other), so not every element is necessarily trimmed by each; to trim each element with each roof or shell, call it once per roof or shell with trimmingElement, and check the result with GetElementTrims."
        );
        err |= RegisterCommand<RemoveElementTrimsCommand> (
            solidElementOperationCommands, "1.5.9",
            "Removes the trim between an element and the roof or shell trimming it. The Archicad API removes the trimming connection between the two elements, so when they trim each other (two roofs or shells) both directions are removed; trim the other direction again with TrimElements to keep it. With dryRun true nothing is removed and no undo step is created: the response has dryRun set to true, and each pair whose element is trimmed by its trimming element gets wouldDelete."
        );
        err |= RegisterCommand<GetElementTrimsCommand> (
            solidElementOperationCommands, "1.5.9",
            "Which roofs and shells trim each queried element, with the trim type, and which elements it trims."
        );
        err |= RegisterCommand<MergeElementsCommand> (
            solidElementOperationCommands, "1.8.0",
            "Merges the given construction elements (the Merge command of Archicad, which cleans up the junctions of slabs, beams, walls and similar elements). Undoable. The element types are not filtered: Archicad reports the ones it cannot merge. The merge is read back: when Archicad records no merge between the given elements (it decides which elements and type combinations it merges), the result is a failure, not a success."
        );
        err |= RegisterCommand<UnmergeElementsCommand> (
            solidElementOperationCommands, "1.8.0",
            "Removes the merge between the two elements of each given pair. Undoable; one execution result per pair."
        );
        err |= RegisterCommand<GetMergedElementsCommand> (
            solidElementOperationCommands, "1.8.0",
            "Gets the elements each queried element is merged with, one item per queried element; an unknown element is an error item."
        );
        AddCommandGroup (solidElementOperationCommands);
    }

    { // Script UI Commands
        CommandGroup scriptUICommands ("Script UI Commands");
        err |= RegisterCommand<ShowScriptUICommand> (
            scriptUICommands, "1.5.4",
            "Shows the given HTML content in a native Archicad palette (a tkinter alternative for Python scripts) and returns the uiId of its window. Each script can have its own window, picked by uiId or scriptId; without them the first window is used."
        );
        err |= RegisterCommand<GetScriptUIResultCommand> (
            scriptUICommands, "1.5.4",
            "Retrieves and clears the result last submitted from a Script UI window's page (via window.ACAPI.SubmitResult), if any, and reports whether the window is still open, so a waiting script can stop when the user closes it. The window is picked by uiId or scriptId; without them the first window is used."
        );
        err |= RegisterCommand<OpenSettingsDialogCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's settings dialog of the selected elements or the current tool (Archicad 28 and later), or the default settings dialog of the given element type, waits for the user and reports whether the dialog was closed with OK or Cancel."
        );
        err |= RegisterCommand<PickLibraryPartCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's Object, Lamp, Window or Door Settings dialog, optionally limited to a library part subtype, lets the user pick a library part and set its parameters, and returns the chosen library part with its GDL parameters, or that the user cancelled. Places nothing."
        );
        err |= RegisterCommand<PickLibraryPartFileCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's Open Library Part dialog, waits for the user and returns the location of the chosen library part, or reports that the user cancelled."
        );
        err |= RegisterCommand<PickPictureFileCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's Open Picture dialog (the one of the texture settings), waits for the user and returns the location of the chosen picture, or reports that the user cancelled. Archicad adds the chosen picture to the embedded library when it is not in it yet."
        );
        err |= RegisterCommand<PickZoneStampOrLabelFileCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's dialog for opening a zone stamp or label library part, waits for the user and returns the location of the chosen file, or reports that the user cancelled."
        );
        err |= RegisterCommand<PickLibraryPartSaveLocationCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's Save Library Part dialog (object, door or window), waits for the user and returns the location chosen to save the library part to, or reports that the user cancelled. Nothing is saved."
        );
        err |= RegisterCommand<PickLibraryFolderCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's dialog for choosing or creating a library folder, waits for the user and returns the location of the folder, or reports that the user cancelled. The folder is not added to the libraries of the project."
        );
        err |= RegisterCommand<ShowIFCExportSettingsCommand> (
            scriptUICommands, "2.0.0",
            "Opens Archicad's IFC export settings dialog, where the user picks a view and an IFC export translator, waits for the user and returns the chosen view and translator, or cancelled. Available from Archicad 28."
        );
        err |= RegisterCommand<PickAttributeCommand> (
            scriptUICommands, "2.0.0",
            "Opens a small dialog with Archicad's attribute picker for the given attribute type (layer, building material, composite, profile or surface; fill and line from Archicad 27), waits for the user and returns the chosen attribute, or cancelled. Available from Archicad 26."
        );
        err |= RegisterCommand<PickAttributeFolderCommand> (
            scriptUICommands, "2.0.0",
            "Opens a small dialog with Archicad's attribute folder picker for the given attribute type, waits for the user and returns the chosen folder as its path from the root folder, or cancelled. Available from Archicad 26."
        );
        AddCommandGroup (scriptUICommands);
    }

    { // Developer Commands
        CommandGroup developerCommands ("Developer Commands");
        err |= RegisterCommand<GenerateDocumentationCommand> (
            developerCommands, "1.0.7",
            "Generates files for the documentation. Used by Tapir developers only."
        );
        AddCommandGroup (developerCommands);
    }

    // The latest release is looked up on a background thread, so Archicad does not wait for GitHub while it
    // starts or opens a project (#798). Once the result is in, the palette shows the "run with update" icon and
    // the "update available" icon with "Update to <version>" as its tooltip on its update button, and the menu item
    // reads "Update Tapir to <version>...".
    VersionChecker::StartCheck ([] () {
        TapirPalette::ApplyUpdateMenuItemState ();
        if (TapirPalette::HasInstance ()) {
            TapirPalette::Instance ().SetRunButtonIcon ();
            TapirPalette::Instance ().SetUpdateButtonIcon ();
        }
    });

    // Loading the palette singleton applies the custom shortcut menu labels and the enabled state
    // of the shortcut menu items at startup, rather than only when the user first opens the
    // palette or triggers a shortcut. It deliberately runs LAST and swallows every failure:
    // constructing this DG::Palette from Initialize() aborted the rest of Initialize() on some
    // configurations, which left every JSON command unregistered ("Archicad does not have the
    // registered Add-On command", error 4010) while the add-on still looked healthy in the
    // Add-On Manager and its menu worked - see #516. Registering the commands first makes that
    // failure mode impossible; if the palette cannot be created this early, the shortcut labels
    // are applied later, the first time the palette is actually used.
    try {
        TapirPalette::Instance ();
    } catch (...) {
        // Intentionally ignored: the menu labels are cosmetic, the commands above are not.
    }

    return err;
}

GSErrCode FreeData (void)
{
    // A palette script still running at quit kept its output reader thread alive after the add-on was unloaded,
    // and Archicad crashed in that thread (GS::Process::GetStandardErrorChannel on the destroyed palette).
    TapirPalette::StopRunningScriptOnQuit ();
    // The version lookup may still wait for GitHub when Archicad quits right after starting.
    VersionChecker::StopOnQuit ();
    // Must run here, not left to the static GS::Ref's destructor at DLL/dylib unload time - see
    // ScriptUIPalette::Release for why (real macOS crash on quit, EXC_BAD_ACCESS tearing down the
    // browser control after Archicad's own environment had already begun shutting down).
    ScriptUIPalette::Release ();
    TapirPalette::Release ();
    // Deletes the files of the states CaptureElementState still holds.
    ReleaseAllElementStates ();
    return NoError;
}
