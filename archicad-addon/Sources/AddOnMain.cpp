#include "APIEnvir.h"
#include "ACAPinc.h"

#include "DGModule.hpp"
#include "DGFolderDialog.hpp"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "DeveloperTools.hpp"

#include "3DCutPlaneCommands.hpp"
#include "3DProjectionCommands.hpp"
#include "3DStyleCommands.hpp"



#include "AboutDialog.hpp"
#include "TapirPalette.hpp"
#include "ScriptUIPalette.hpp"
#include "VersionChecker.hpp"

#include "ApplicationCommands.hpp"
#include "ScriptUICommands.hpp"
#include "ProjectCommands.hpp"
#include "ElementCommands.hpp"
#include "ElementCategoryCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "ElementCreationCommands.hpp"
#include "ExtendedElementCommands.hpp"
#include "ElementGroupingCommands.hpp"
#include "AttributeCommands.hpp"
#include "TeamworkCommands.hpp"
#include "IssueCommands.hpp"
#include "LibraryCommands.hpp"
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
#include "AnalyticalCommands.hpp"
#include "KeynoteCommands.hpp"
#include "ProjectDataCommands.hpp"
#include "GraphicOverrideCommands.hpp"
#include "GridElementCommands.hpp"
#include "GraphicalOverrideCommands.hpp"
#include "PolygonCommands.hpp"

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
        ACAPI_WriteReport ("Tapir: the %s command could not be registered (error %d).", false, commandName.ToCStr (), static_cast<int> (err));
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
                    {
                        if (!VersionChecker::IsCheckCompleted ()) {
                            DGAlert (DG_INFORMATION, "Tapir Update",
                                "Tapir is still checking GitHub for the latest release.",
                                "Please try again in a moment.",
                                "OK");
                        } else if (!VersionChecker::IsUsingLatestVersion ()) {
                            TapirPalette::Instance ().UpdateAddOn ();
                        } else if (VersionChecker::IsNewerVersionWithoutAddOn ()) {
                            DGAlert (DG_INFORMATION, "Tapir Update",
                                GS::UniString::Printf ("Tapir %T is available, but not yet for Archicad %d.",
                                    VersionChecker::LatestVersion ().ToPrintf (), static_cast<int> (VersionChecker::ArchicadMainVersion ())),
                                GS::UniString::Printf ("You are using Tapir %s.", ADDON_VERSION),
                                "OK");
                        } else {
                            DGAlert (DG_INFORMATION,
                                RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_LATESTVERSION_ALERT_TITLE, ACAPI_GetOwnResModule ()),
                                GS::UniString::Printf (RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_LATESTVERSION_ALERT_TEXT, ACAPI_GetOwnResModule ()), ADDON_VERSION),
                                GS::EmptyUniString,
                                RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_LATESTVERSION_ALERT_BUTTON, ACAPI_GetOwnResModule ()));
                        }
                    }
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

    { // Application Commands
        CommandGroup applicationCommands ("Application Commands");
        err |= RegisterCommand<GetAddOnVersionCommand> (
            applicationCommands, "0.1.0",
            "Retrieves the version of the Tapir Additional JSON Commands Add-On."
        );
        err |= RegisterCommand<GetArchicadLocationCommand> (
            applicationCommands, "0.1.0",
            "Retrieves the location of the currently running Archicad executable."
        );
        err |= RegisterCommand<GetArchicadReleaseInfoCommand> (
            applicationCommands, "1.7.3",
            "Retrieves the main version, release version and build number of the running Archicad, with the caption of its window."
        );
        err |= RegisterCommand<GetPrinterInfoCommand> (
            applicationCommands, "1.7.3",
            "Retrieves Archicad's current print settings: the print area, grid, fixed text size and scale (the settings PrintView takes)."
        );
        err |= RegisterCommand<QuitArchicadCommand> (
            applicationCommands, "0.1.0",
            "Performs a quit operation on the currently running Archicad instance."
        );
        err |= RegisterCommand<GetPointFromUserCommand> (
            applicationCommands, "1.5.9",
            "Asks the designer to click a point in the current window and returns it. Archicad waits for the click or for Escape, and every other JSON command queues behind this one until then; the call fails when the input is cancelled."
        );
        err |= RegisterCommand<GetElementFromUserCommand> (
            applicationCommands, "1.7.3",
            "Asks the designer to click an element in the current window and returns its id, its type and the clicked point. A click on empty space or on an element of a type not asked for asks again. Archicad waits for the click or for Escape, and every other JSON command queues behind this one until then; cancelling returns cancelled."
        );
        err |= RegisterCommand<GetCurrentWindowTypeCommand> (
            applicationCommands, "1.0.7",
            "Returns the type of the current (active) window."
        );
        err |= RegisterCommand<GetCurrentWindowDrawingScaleCommand> (
            applicationCommands, "1.7.2",
            "Returns the drawing scale of the current (active) window, the scale shown in the bottom bar, as its denominator: 100.0 for 1:100, 20.0 for 1:20. Unlike GetViewSettings it needs no saved view. It reads the scale of the current database: the floor plan, a section, elevation, interior elevation, detail, worksheet or 3D document window, each of which has its own. Layouts always return 1.0."
        );
        err |= RegisterCommand<SetCurrentWindowDrawingScaleCommand> (
            applicationCommands, "1.7.2",
            "Changes the drawing scale of the current (active) window, the scale shown in the bottom bar, to the given denominator: 100 for 1:100. It changes the scale of the current database: the floor plan, a section, elevation, interior elevation, detail, worksheet or 3D document window, each of which has its own. Layouts are always 1:1 and are rejected. The scale must be between 1 and 30000. The change is undoable."
        );
        err |= RegisterCommand<ChangeWindowCommand> (
            applicationCommands, "1.3.1",
            "Changes the current (active) window to the given window. A databaseId on its own is enough; the window type is taken from the database. With windowType 'FloorPlan' and a storyIndex it also activates that story on the floor plan."
        );
        err |= RegisterCommand<GetContainingDatabasesCommand> (
            applicationCommands, "1.7.2",
            "Returns the database (window type, database id and name) that contains each of the given elements, from any database, not only the current one. Pass the windowType and databaseId to ChangeWindow to open it, for example to find an element placed on a layout."
        );
        err |= RegisterCommand<GetUserGSIDCommand> (
            applicationCommands, "1.5.6",
            "Get the current registered User-GSID and OrganizationsID. Requires Archicad 27 or later."
        );
        err |= RegisterCommand<ShowAlertCommand>(
            applicationCommands,"1.5.6",
            "Display a dialog with up to three buttons."
        );
        err |= RegisterCommand<GetSpecialFoldersCommand> (
            applicationCommands, "1.5.6",
            "Retrieves the filesystem paths of the special folders of the running Archicad (preferences, cache, data, temporary, application, defaults, templates, help, embedded project library, etc.)."
        );
        err |= RegisterCommand<GetProductVersionInfoCommand> (
            applicationCommands, "1.7.3",
            "Retrieves the version information of the running Archicad: build number, language code, product family and version text, plus the product name, product type and main/update/hotfix versions from Archicad 29. Requires Archicad 28 or later."
        );
        err |= RegisterCommand<Get3DCutawayStatusCommand> (
            applicationCommands, "1.7.3",
            "Returns whether 3D Cutaway is on and whether the cutting planes are shown in the 3D window. In Archicad 25-27 the 3D window may need to be open."
        );
        err |= RegisterCommand<Set3DCutawayStatusCommand> (
            applicationCommands, "1.7.3",
            "Turns 3D Cutaway on or off and shows or hides the cutting planes in the 3D window. In Archicad 25-27 the 3D window may need to be open."
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
            "Creates one or more custom project info fields."
        );
        err |= RegisterCommand<DeleteProjectInfoFieldsCommand> (
            projectCommands, "1.5.4",
            "Deletes one or more custom project info fields. Hardcoded fields cannot be deleted."
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
            "Retrieves the available autotext keys (name and embeddable key), optionally for a specific element. Embed a key in a Text or Label content by surrounding it with '<' and '>'."
        );
        err |= RegisterCommand<GetAutoTextNameCommand> (
            projectCommands, "1.5.9",
            "Retrieves the display names of one or more autotext keys (as returned inside a '<...>' embedded key), with a direct guid lookup for property-based keys instead of enumerating every property definition."
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
        err |= RegisterCommand<GetHotlinkProxyElementTableCommand> (
            projectCommands, "1.7.3",
            "Maps the proxy elements of placed hotlink instances to the elements of the source file they were made from, so an element seen in a hotlinked module can be traced back to its source element."
        );
        err |= RegisterCommand<OpenProjectCommand> (
            projectCommands, "1.0.7",
            "Opens the given project."
        );
        err |= RegisterCommand<NewProjectCommand> (
            projectCommands, "1.7.2",
            "Starts a new, untitled project from the template set in Archicad's New Project settings, as File > New does; with newAndReset also the working environment is reset, as New and Reset All does. The unsaved changes of the open project are discarded, as with OpenProject."
        );
        err |= RegisterCommand<CloseProjectCommand> (
            projectCommands, "1.3.1",
            "Closes the currently opened project."
        );
        err |= RegisterCommand<SaveProjectCommand> (
            projectCommands, "1.3.1",
            "Saves the currently opened project."
        );
        err |= RegisterCommand<SaveAsModuleFileCommand> (
            projectCommands, "1.5.9",
            "Saves the given elements, or the current selection, as a hotlink module (.mod) file."
        );
        err |= RegisterCommand<SaveProjectAsArchiveCommand> (
            projectCommands, "1.6.0",
            "Saves the open project as an archive (.pla) file, with the library parts it uses inside."
        );
        err |= RegisterCommand<SaveCurrentWindowAsImageCommand> (
            projectCommands, "1.7.1",
            "Saves what the current window shows as an image file (png, jpg, bmp, tiff or gif), as File > Save as does with a picture format."
        );
        err |= RegisterCommand<GetCurrentViewImageCommand> (
            projectCommands, "1.7.2",
            "Returns what the 3D window, or the PhotoRendering window, shows as a base64 encoded png or jpg image with its size in pixels, without writing a file. Available from Archicad 30."
        );
        err |= RegisterCommand<GetCalculationUnitsCommand> (
            projectCommands, "1.4.0",
            "Gets the project calculation units."
        );
        err |= RegisterCommand<GetProjectPreferencesCommand> (
            projectCommands, "1.7.2",
            "Gets pages of the project preferences (Options > Project Preferences): the working units, the dimensions and the calculation units. Further pages may follow."
        );
        err |= RegisterCommand<SetProjectPreferencesCommand> (
            projectCommands, "1.7.2",
            "Changes pages of the project preferences (Options > Project Preferences): the working units, the dimensions and the calculation units. Only the given fields change, the rest of each page keeps its value. In Teamwork the PreferencesDialog lockable object set has to be reserved first (ReserveLockableObjectSet). Available from Archicad 27."
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
        err |= RegisterCommand<GetProjectNotesCommand> (
            projectCommands, "1.7.3",
            "Gets the Project Notes of the project (File > Info > Project Notes): client, project name, company, address, architect, draftsmen, project status, date of issue, keywords and notes."
        );
        err |= RegisterCommand<SetProjectNotesCommand> (
            projectCommands, "1.7.3",
            "Changes the Project Notes of the project (File > Info > Project Notes). Only the given fields are changed; texts longer than the field Archicad stores are truncated."
        );
        err |= RegisterCommand<PrintViewCommand> (
            projectCommands, "1.3.1",
            "Prints from the current view."
        );
        err |= RegisterCommand<RebuildViewCommand> (
            projectCommands, "1.5.0",
            "Rebuilds the current view."
        );
        AddCommandGroup (projectCommands);
    }

    { // Element Commands
        CommandGroup elementCommands ("Element Commands");
        err |= RegisterCommand<GetSelectedElementsCommand> (
            elementCommands, "0.1.0",
            "Gets the list of the currently selected elements."
        );
        err |= RegisterCommand<GetElementsByTypeCommand> (
            elementCommands, "1.0.7",
            "Returns the identifier of every element of the given type on the plan. It works for any type. Use the optional filter parameter for filtering."
        );
        err |= RegisterCommand<GetAllElementsCommand> (
            elementCommands, "1.0.7",
            "Returns the identifier of all elements on the plan. Use the optional filter parameter for filtering."
        );
        err |= RegisterCommand<GetElementTypeNamesCommand> (
            elementCommands, "1.7.3",
            "Gets the localized names of the given element types, as the running Archicad shows them (for example 'Wand' for Wall in German Archicad). The output is aligned with the input array of element types. Available from Archicad 27."
        );
        err |= RegisterCommand<ChangeSelectionOfElementsCommand> (
            elementCommands, "1.0.7",
            "Adds/removes a number of elements to/from the current selection."
        );
        err |= RegisterCommand<FilterElementsCommand> (
            elementCommands, "1.0.7",
            "Keeps the elements that match all the given criteria: Archicad's flag filters (filters), layer (layerIndices), home story (floorIndices) and element type (elementTypes). Within one criterion any of the listed values matches; an omitted criterion does not filter."
        );
        err |= RegisterCommand<GetDetailsOfElementsCommand> (
            elementCommands, "1.5.7",
            "Gets the details of the given elements (geometry parameters etc). Use the optional fields parameter to return only the fields you need and skip the computation of the others (for example floorPlanPolygons)."
        );
        err |= RegisterCommand<SetDetailsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Sets the details of the given elements (floor, layer, order, Element ID etc)."
        );
        err |= RegisterCommand<GetElementCategoriesCommand> (
            elementCommands, "1.7.2",
            "Lists the element categories of the project (Structural Function, Position, Renovation Status, Renovation Filter and, in the German version, BRI) with their possible values. The names of the categories and values are localized: identify a category by its type and a value by its categoryValueId, never by name."
        );
        err |= RegisterCommand<GetCategoryValuesOfElementsCommand> (
            elementCommands, "1.7.2",
            "Gets the category values (Structural Function, Position, Renovation Status ...) of the given elements, optionally only for the given category types. An element that has none of the requested categories (an annotation, for example) gets an error item. The names are localized: use the type and the categoryValueId in scripts."
        );
        err |= RegisterCommand<SetCategoryValuesOfElementsCommand> (
            elementCommands, "1.7.2",
            "Sets the category values (Structural Function, Position, Renovation Status ...) of the given elements, one element and one category per item, in a single undoable step. The value must belong to the given category, as GetElementCategories lists them; an element without that category gets a failed execution result. The names are localized: use the type and the categoryValueId in scripts."
        );
        err |= RegisterCommand<Get3DBoundingBoxesCommand> (
            elementCommands, "1.1.2",
            "Get the 3D bounding box of elements. The bounding box is calculated from the global origin in the 3D view. The output is the array of the bounding boxes respective to the input array of elements."
        );
        err |= RegisterCommand<ConvertRevealOffsetsCommand> (
            elementCommands, "1.7.3",
            "Converts the reveal depth offset of windows and doors placed in walls between the two references Archicad uses: measured from the core skin of a composite wall, or from the wall side. Give exactly one of coreOffset or sideOffset per opening; both values are returned. For walls with a basic structure the two values are equal. The output is aligned with the input array of openings. Available from Archicad 27."
        );
        err |= RegisterCommand<Get3DGeometryOfElementsCommand> (
            elementCommands, "1.7.2",
            "Gets the 3D geometry of elements as the solid bodies of their 3D model: a vertex list in world coordinates and the planar faces (with their holes) referencing it, each face with its surface attribute when that is a project attribute. The bodies of the subelements of a Stair, Railing or Curtain Wall, and of the segments of a Column or Beam, are included. The output is aligned with the input array of elements. The 3D model of the elements is generated as needed, so many elements can take a while."
        );
        err |= RegisterCommand<GetElementAtCoordinateCommand> (
            elementCommands, "1.7.2",
            "Finds the element of the given polygonal type (for example Slab, Roof, Mesh, Zone or Hatch) at a coordinate of the floor plan of the current database, the way a click there would. Use the optional filters parameter (the values of FilterElements) to consider only, for example, the elements visible by layer, and the optional z parameter to give the elevation of the point. Returns found: false when there is no such element. Useful together with GetPointFromUser."
        );
        err |= RegisterCommand<GetQuantitiesOfElementsCommand> (
            elementCommands, "1.7.2",
            "Gets the quantities of the given elements calculated by Archicad (volumes, surfaces, lengths etc.), together with the volume and projected area per building material (one item per skin of composite and complex profile elements, one item for basic elements). Supported types: Wall, Column, Beam, Slab, Roof, Shell, Morph, Zone, ColumnSegment, BeamSegment. Use the optional minOpeningSize parameter to control the conditional values, and calculateInOneCall to calculate all the elements in one Archicad call."
        );
        err |= RegisterCommand<TriangulatePolygonsOfElementsCommand> (
            elementCommands, "1.7.3",
            "Triangulates the plan polygon of the given elements, holes excluded, as Archicad does it: the outline of a Slab, Zone, Hatch or Mesh and the polygon of a Roof. Each triangle is given by the 2D coordinates of its three vertices. The output is aligned with the input array of elements; an element of another type gets an error item."
        );
        err |= RegisterCommand<GetSurfaceQuantitiesOfElementsCommand> (
            elementCommands, "1.7.2",
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
        err |= RegisterCommand<GetConnectedElementsCommand> (
            elementCommands, "1.1.4",
            "Gets the elements hosted by (connected to) the given owner elements, filtered to the given element type: for example the Windows or Doors of a Wall, the frames, panels, junctions and accessories of a Curtain Wall, the risers, treads and structures of a Stair, or the posts, rails and panels of a Railing."
        );
        err |= RegisterCommand<GetElementsInDrawOrderCommand> (
            elementCommands, "1.7.2",
            "Returns the identifier of every element of the current database, or of the given databases, in Archicad's drawing order (the order set by Bring to Front and Send to Back), as ACAPI_DrawOrder_DrawOrderGetNext walks it. Use the optional elementType and filters parameters for filtering."
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
            elementCommands, "1.7.2",
            "Creates persistent links between pairs of elements, each from one element to another, with an optional integer flags value per link. Scripts can record their own relations between elements this way (for example which label documents which column) instead of storing GUIDs in properties. The links are stored in the project and have no effect on geometry; they are independent from groups, hotlinks and solid element operations. A link is directional: it is returned by GetLinksOfElements for the element it starts from only, and removed by UnlinkElements. Undoable."
        );
        err |= RegisterCommand<UnlinkElementsCommand> (
            elementCommands, "1.7.2",
            "Removes the links created by LinkElements between the given pairs of elements. The direction matters: the link starting from fromElementId and pointing to toElementId is removed. Undoable."
        );
        err |= RegisterCommand<GetLinksOfElementsCommand> (
            elementCommands, "1.7.2",
            "Gets the links created by LinkElements that start from the given elements: the linked element and the flags of each link. Links pointing to an element are not returned; query the elements they start from. Returns an error item for an element that does not exist."
        );
        err |= RegisterCommand<GetZoneBoundariesCommand> (
            elementCommands, "1.2.3",
            "Gets the boundaries of the given Zones (connected elements, neighbour zones, etc.). Accepts either a single zoneElementId or a list of zones. Prefer the list: the expensive boundary recalculation runs once per call, so querying many Zones in one call is much faster than calling the command once per Zone."
        );
        err |= RegisterCommand<GetZoneReductionsCommand> (
            elementCommands, "1.7.2",
            "Gets the area reductions of the given Zones, as the Zone Stamp's area calculation uses them: the polygons of the Zone covered by Walls, Columns, reducing Fills, sloping Roofs and low ceilings, each with its type, the percentage it is reduced by, its area and its outline with holes. Area calculations by local standards need these parts, which GetQuantitiesOfElements only gives as totals. The output is aligned with the input array of elements."
        );
        err |= RegisterCommand<UpdateZonesCommand> (
            elementCommands, "1.5.4",
            "Updates all Zones (recalculates their geometry, updates their Zone Stamps and the connected elements)."
        );
        err |= RegisterCommand<SetZoneLabelingSettingsCommand> (
            elementCommands, "1.7.3",
            "Turns the automatic labeling of Zones on or off. Available from Archicad 28."
        );
        err |= RegisterCommand<GetCollisionsCommand> (
            elementCommands, "1.2.2",
            "Detect collisions between the given two groups of elements."
        );
        err |= RegisterCommand<HighlightElementsCommand> (
            elementCommands, "1.0.3",
            "Highlights the elements given in the elements array. In case of empty elements array removes all previously set highlights."
        );
        err |= RegisterCommand<MoveElementsCommand> (
            elementCommands, "1.0.2",
            "Moves elements with a given vector. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<RotateElementsCommand> (
            elementCommands, "1.5.3",
            "Rotates elements around a reference point. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<MirrorElementsCommand> (
            elementCommands, "1.7.2",
            "Mirrors elements across an axis given by two points on the floor plan, optionally as a copy. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently."
        );
        err |= RegisterCommand<DeleteElementsCommand> (
            elementCommands, "1.2.1",
            "Deletes elements. The zones of the call are deleted before all the other elements, so an automatic zone deleted together with its bounding walls is gone before its boundary is; deleting the boundary of an automatic zone without the zone itself can open Archicad's modal 'No closed polygon found' dialog, which blocks the command until someone clicks it in Archicad. Returns an execution result for each input element: an element that could not be deleted (for example because its layer is locked) gets a failed execution result instead of being skipped silently."
        );
        err |= RegisterCommand<DeleteSelectedElementsCommand> (
            elementCommands, "1.7.3",
            "Deletes the currently selected elements in one undo step, the same as pressing the Delete key."
        );
        err |= RegisterCommand<LockElementsCommand> (
            elementCommands, "1.5.2",
            "Locks the given elements. Manual lock, not teamwork!"
        );
        err |= RegisterCommand<UnlockElementsCommand> (
            elementCommands, "1.5.2",
            "Unlocks the given elements. Manual lock, not teamwork!"
        );
        err |= RegisterCommand<GetGDLParametersOfElementsCommand> (
            elementCommands, "1.5.7",
            "Gets all the GDL parameters (name, type, value) of the given elements."
        );
        err |= RegisterCommand<SetGDLParametersOfElementsCommand> (
            elementCommands, "1.5.7",
            "Sets the given GDL parameters of the given elements. An element Archicad does not let the user edit (its layer locked, the element locked, or not reserved by the user in Teamwork) gets a failed execution result naming that reason, whatever the parameter."
        );
        err |= RegisterCommand<GetMarkerParametersOfElementsCommand> (
            elementCommands, "1.7.1",
            "Gets the marker library part and the marker's GDL parameters (name, type, value) of the given Window and Door elements. The marker is the stamp drawn next to the opening on the floor plan; its parameters are separate from the opening's own GDL parameters, which GetGDLParametersOfElements returns."
        );
        err |= RegisterCommand<SetMarkerParametersOfElementsCommand> (
            elementCommands, "1.7.1",
            "Sets the given GDL parameters of the marker of the given Window and Door elements. The opening's own settings and the other marker parameters are left as they are."
        );
        err |= RegisterCommand<RunParameterScriptOfElementsCommand> (
            elementCommands, "1.7.3",
            "Runs the parameter script of the library part of the given elements (objects, lamps, windows, doors, etc.), so that parameter values that depend on other parameters are updated, e.g. after SetGDLParametersOfElements."
        );
        err |= RegisterCommand<CreateColumnsCommand> (
            elementCommands, "1.0.3",
            "Creates Column elements based on the given parameters."
        );
        err |= RegisterCommand<CreateWallsCommand> (
            elementCommands, "1.4.0",
            "Creates Wall elements based on the given parameters."
        );
        err |= RegisterCommand<CreateBeamsCommand> (
            elementCommands, "1.4.0",
            "Creates Beam elements based on the given parameters."
        );
        err |= RegisterCommand<CreateStairsCommand> (
            elementCommands, "1.5.0",
            "Creates Stair elements based on the given baseline and parameters."
        );
        err |= RegisterCommand<SolveStairBaselineCommand> (
            elementCommands, "1.7.3",
            "Runs the Archicad stair solver on the given Stair baselines and parameters without creating any element. "
            "Returns for each one whether the baseline was already correct, could be solved or could not be solved, "
            "with the baseline and the main stair settings after solving. Available from Archicad 27."
        );
        err |= RegisterCommand<CreateSlabsCommand> (
            elementCommands, "1.0.3",
            "Creates Slab elements based on the given parameters."
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
        err |= RegisterCommand<GetOpeningFloorPlanParametersCommand> (
            elementCommands, "1.7.3",
            "Returns the floor plan parameters of Opening elements: the reference axis, the cut surface and outline lines, the cover fills and the cut and uncut symbols. Available from Archicad 29."
        );
        err |= RegisterCommand<SetOpeningFloorPlanParametersCommand> (
            elementCommands, "1.7.3",
            "Changes the floor plan parameters of Opening elements returned by GetOpeningFloorPlanParameters, and sets or removes their cut and uncut symbols. Available from Archicad 29."
        );
        err |= RegisterCommand<GetOpeningDetailsCommand> (
            elementCommands, "1.7.3",
            "Returns the details of Opening elements: the parent element, the cut and connected elements, the geometry (anchor point, extrusion direction, axes, polygon) and the extrusion parameters (size, anchor, offsets, surface overrides). Available from Archicad 29."
        );
        err |= RegisterCommand<SetOpeningDetailsCommand> (
            elementCommands, "1.7.3",
            "Changes the details of Opening elements: the parent element, the cut elements, the polygon, a transformation of the geometry and the extrusion parameters (size, anchor, offsets, surface overrides) returned by GetOpeningDetails. Available from Archicad 29."
        );
        err |= RegisterCommand<CreateMorphsCommand> (
            elementCommands, "1.4.0",
            "Creates Morph elements from simple box definitions."
        );
        err |= RegisterCommand<CreateRoofsCommand> (
            elementCommands, "1.4.0",
            "Creates Roof elements based on footprint, level and roof profile data. Creates a multi-plane roof by default; pass 'pivotLine' (and optionally 'angle') to create a single-plane roof instead."
        );
        err |= RegisterCommand<CreateCurtainWallsCommand> (
            elementCommands, "1.7.2",
            "Creates Curtain Wall elements along a reference polyline with a column (primary) and row (secondary) pattern. The frame and panel classes come from the favorite or from the Curtain Wall tool defaults; the pattern cells can assign them to the grid."
        );
        err |= RegisterCommand<CreateAssociativeDimensionsCommand> (
            elementCommands, "1.4.0",
            "Creates associative linear dimensions from explicit witness point references. A witness point can also be static, given by its coordinate instead of an element."
        );
        err |= RegisterCommand<CreateAssociativeDimensionsOnSectionCommand> (
            elementCommands, "1.4.0",
            "Creates associative linear dimensions on section elements using common wall, slab, beam, column and opening presets. The preset points of multiple section elements can be merged into one continuous dimension chain via sectionElementIds."
        );
        err |= RegisterCommand<CreateWallThicknessDimensionsCommand> (
            elementCommands, "1.4.0",
            "Creates associative wall thickness dimensions for the given walls."
        );
        err |= RegisterCommand<GetDimensionDataCommand> (
            elementCommands, "1.5.0",
            "Gets witness point data (coordinates, measured values) from existing dimension chains."
        );
        err |= RegisterCommand<GetOpeningTransformationsCommand> (
            elementCommands, "1.7.3",
            "Gets the placement of Opening elements without changing them: the global-to-local transformation matrix Archicad returns, its inverse, and the origin and axes of the opening in world coordinates derived from the inverse. Mirrored openings give a left-handed matrix (isMirrored). Needs Archicad 27 or later."
        );
        err |= RegisterCommand<CreateZonesCommand> (
            elementCommands, "1.1.8",
            "Creates Zone elements based on the given parameters."
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
            "Creates Hatch elements based on the given parameters."
        );
        err |= RegisterCommand<CreateSplinesCommand> (
            elementCommands, "1.5.6",
            "Creates Spline elements based on the given parameters."
        );
        err |= RegisterCommand<CreateObjectsCommand> (
            elementCommands, "1.5.7",
            "Creates Object elements based on the given parameters."
        );
        err |= RegisterCommand<CreateLampsCommand> (
            elementCommands, "1.5.7",
            "Creates Lamp elements based on the given parameters."
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
        err |= RegisterCommand<ModifyWallsCommand> (
            elementCommands, "1.4.0",
            "Modifies Wall elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyBeamsCommand> (
            elementCommands, "1.4.0",
            "Modifies Beam elements based on the given parameters."
        );
        err |= RegisterCommand<ModifySlabsCommand> (
            elementCommands, "1.4.0",
            "Modifies Slab elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyColumnsCommand> (
            elementCommands, "1.4.0",
            "Modifies Column elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyWindowsCommand> (
            elementCommands, "1.4.0",
            "Modifies Window elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyDoorsCommand> (
            elementCommands, "1.4.0",
            "Modifies Door elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyMorphsCommand> (
            elementCommands, "1.4.0",
            "Modifies Morph elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyRoofsCommand> (
            elementCommands, "1.4.0",
            "Modifies multi-plane Roof elements based on the given parameters."
        );
        err |= RegisterCommand<ModifyMeshesCommand> (
            elementCommands, "1.5.4",
            "Modifies the attributes of Mesh elements based on the given parameters."
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
        err |= RegisterCommand<MeasureTextsCommand> (
            elementCommands, "1.7.2",
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
        err |= RegisterCommand<AddElementNotificationClientCommand> (
            elementCommands, "1.2.8",
            "Sets up a new notification client to receive element events."
        );
        err |= RegisterCommand<RemoveElementNotificationClientCommand> (
            elementCommands, "1.2.8",
            "Removes an element notification client."
        );
        err |= RegisterCommand<RegularizePolygonsCommand> (
            elementCommands, "1.7.2",
            "Regularizes polygons with Archicad's own polygon regularization, before they are used to create slabs, zones or fills: removes duplicate points, fixes the orientation, resolves self-intersections and touching holes. Each input polygon gives 0, 1 or more valid polygons in the format of CreateSlabs, optionally with the input vertex and edge each result vertex and edge came from. Changes nothing in the project."
        );
        err |= RegisterCommand<RegularizePolylinesCommand> (
            elementCommands, "1.7.2",
            "Regularizes polylines with Archicad's own polyline regularization: removes repeated consecutive points. The result is in the format of CreatePolylines and may be empty, optionally with the input point and segment each result point and segment came from. Changes nothing in the project."
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
            elementGroupingCommands, "1.7.2",
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
        err |= RegisterCommand<GetUserDataOfGroupsCommand> (
            elementGroupingCommands, "1.7.2",
            "Gets the user data an add-on attached to each given group: its version, platform sign, flags and bytes (base64 encoded). Returns an error for groups that have no user data."
        );
        err |= RegisterCommand<GetSuspendGroupsModeCommand> (
            elementGroupingCommands, "1.5.6",
            "Gets the current state of the Suspend Groups mode."
        );
        err |= RegisterCommand<SetSuspendGroupsModeCommand> (
            elementGroupingCommands, "1.5.6",
            "Turns the Suspend Groups mode on or off. Suspend groups to perform operations on elements that are part of a group; remember to restore the previous state afterwards."
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
            "Import Favorites from a .prefs file or folder into the current project."
        );
        err |= RegisterCommand<ExportFavoritesCommand> (
            favoritesCommands, "1.5.0",
            "Export the project's Favorites to a .prefs file or folder. From Archicad 29 they can also be "
            "exported to a folder of Favorite files (xml format) or to a legacy .prf file (legacyPRF format)."
        );
        err |= RegisterCommand<ApplyFavoritesToElementsCommand> (
            favoritesCommands, "1.5.4",
            "Apply the given favorites to existing elements. Only settings-type parameters are changed - "
            "geometry (position, floor, and dimensions such as a Wall's height) is left untouched, so applying "
            "a Favorite never moves or resizes the target element. For the hierarchical types (Stair, "
            "Railing, Curtain Wall) the settings of the sub-elements are not applied, because they are "
            "inseparable from the Favorite's own geometry. By default settings, classifications, "
            "categories and properties are all applied; each can be opted out of individually. "
            "keepFields lists fields of the target (its ID, layer, an opening's width, height and sill "
            "height, a Wall's reference line location, height and top link) that are copied back after "
            "the Favorite has been applied, so that e.g. only the library part or the wall type changes."
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
        err |= RegisterCommand<GetFavoriteSettingsCommand> (
            favoritesCommands, "1.7.3",
            "Reads the stored settings of the given favorites without applying them or placing an element: "
            "the element type, the folder, the library part and GDL parameters, the marker (library part and "
            "parameters) of Window and Door favorites, and the sub-elements. A favorite without a marker has no "
            "marker field, which is not an error. The other settings of the element (pens, layer, structure, etc.) "
            "are not returned."
        );
        AddCommandGroup (favoritesCommands);
    }

    { // Property Commands
        CommandGroup propertyCommands ("Property Commands");
        err |= RegisterCommand<GetAllPropertiesCommand> (
            propertyCommands, "1.1.3",
            "Returns all user defined and built-in properties."
        );
        err |= RegisterCommand<GetPropertyValuesOfElementsCommand> (
            propertyCommands, "1.0.6",
            "Returns the property values of the elements for the given property. It works for subelements of hierarchal elements also. With includeAvailability it also tells whether each property is available for the element."
        );
        err |= RegisterCommand<SetPropertyValuesOfElementsCommand> (
            propertyCommands, "1.0.6",
            "Sets the property values of elements. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<GetPropertyValuesOfAttributesCommand> (
            propertyCommands, "1.1.8",
            "Returns the property values of the attributes for the given property."
        );
        err |= RegisterCommand<SetPropertyValuesOfAttributesCommand> (
            propertyCommands, "1.1.8",
            "Sets the property values of attributes."
        );
        err |= RegisterCommand<GetPropertyDefinitionsOfAttributesCommand> (
            propertyCommands, "1.7.3",
            "Returns the property definitions of the given attributes (for example Building Materials): the user defined, the fundamental built-in, or all of them."
        );
        err |= RegisterCommand<AddPropertyToAttributesCommand> (
            propertyCommands, "1.7.3",
            "Makes an existing custom property available for the given attributes (for example Building Materials), in one undo step. Available in Archicad 25, 26 and 27 only."
        );
        err |= RegisterCommand<RemovePropertyFromAttributesCommand> (
            propertyCommands, "1.7.3",
            "Makes a custom property unavailable for the given attributes (for example Building Materials), in one undo step. Available in Archicad 25, 26 and 27 only."
        );
        err |= RegisterCommand<CreatePropertyGroupsCommand> (
            propertyCommands, "1.0.7",
            "Creates Property Groups based on the given parameters."
        );
        err |= RegisterCommand<DeletePropertyGroupsCommand> (
            propertyCommands, "1.0.9",
            "Deletes the given Custom Property Groups."
        );
        err |= RegisterCommand<CreatePropertyDefinitionsCommand> (
            propertyCommands, "1.0.9",
            "Creates Custom Property Definitions based on the given parameters."
        );
        err |= RegisterCommand<DeletePropertyDefinitionsCommand> (
            propertyCommands, "1.0.9",
            "Deletes the given Custom Property Definitions."
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
        err |= RegisterCommand<MakePropertyAvailableForElementsCommand> (
            propertyCommands, "1.7.2",
            "Makes a property definition available for all the given elements. Available only in Archicad 27."
        );
        err |= RegisterCommand<MakePropertyUnavailableForElementsCommand> (
            propertyCommands, "1.7.2",
            "Makes a property definition unavailable for all the given elements. Available only in Archicad 27."
        );
        AddCommandGroup (propertyCommands);
    }


    { // Classification Commands
        CommandGroup classificationCommands ("Classification Commands");
        err |= RegisterCommand<GetClassificationsOfElementsCommand> (
            classificationCommands, "1.0.7",
            "Returns the classification of the given elements in the given classification systems. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<SetClassificationsOfElementsCommand> (
            classificationCommands, "1.0.7",
            "Sets the classifications of elements. In order to set the classification of an element to unclassified, omit the classificationItemId field. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<GetClassificationsOfElementDefaultsCommand> (
            classificationCommands, "1.7.3",
            "Returns the classification of the tool defaults (the settings new elements get) of the given element types in the given classification systems."
        );
        err |= RegisterCommand<SetClassificationsOfElementDefaultsCommand> (
            classificationCommands, "1.7.3",
            "Sets the classifications of the tool defaults (the settings new elements get) of the given element types. In order to set a default to unclassified, omit the classificationItemId field."
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
            "Deletes the given Classification Systems."
        );
        err |= RegisterCommand<DeleteClassificationItemsCommand> (
            classificationCommands, "1.5.2",
            "Deletes the given Classification Items."
        );
        err |= RegisterCommand<UpdateClassificationSystemsCommand> (
            classificationCommands, "1.6.0",
            "Updates the name, description, source, version and/or date of existing Classification Systems, keeping their guid."
        );
        err |= RegisterCommand<UpdateClassificationItemsCommand> (
            classificationCommands, "1.6.0",
            "Updates the id (code), name and/or description of existing Classification Items, keeping their guid and so the elements classified with them. Items cannot be moved to another parent."
        );
        err |= RegisterCommand<ImportClassificationsXmlCommand> (
            classificationCommands, "1.6.0",
            "Imports a Classification Manager XML export, with the given policies for systems and items that already exist. Returns the systems and items it created and removed."
        );
        AddCommandGroup (classificationCommands);
    }

    { // Attribute Commands
        CommandGroup attributeCommands ("Attribute Commands");
        err |= RegisterCommand<GetAttributesByTypeCommand> (
            attributeCommands, "1.1.3",
            "Returns the details of every attribute of the given type. From Archicad 30 an attribute that Archicad keeps a creation record of also carries creationInfo: how (method), from where (source), by whom and when it was created."
        );
        err |= RegisterCommand<GetAttributesFromProjectCommand> (
            attributeCommands, "1.7.3",
            "Reads the attributes of the given types from another project or template file without opening it, for example to compare them with the attributes of the open project. Nothing is imported into the open project. The names are the legacy 8-bit names, so non-ASCII characters may not come through. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteAttributesCommand> (
            attributeCommands, "1.5.4",
            "Deletes the given attributes."
        );
        err |= RegisterCommand<CreateAttributeFoldersCommand> (
            attributeCommands, "1.7.2",
            "Creates attribute folders, each given by attribute type and path (the folder names from the root folder down). Missing parent folders are created too; a folder that already exists is kept and reported as success. Available from Archicad 26."
        );
        err |= RegisterCommand<DeleteAttributeFoldersCommand> (
            attributeCommands, "1.7.2",
            "Deletes attribute folders, each given by attribute type and path, together with all the subfolders and deletable attributes they contain (a folder holding an attribute that cannot be deleted is kept). The root folder cannot be deleted. Available from Archicad 26."
        );
        err |= RegisterCommand<RenameAttributeFoldersCommand> (
            attributeCommands, "1.7.2",
            "Renames attribute folders, each given by attribute type and path, to the given new names. The root folder cannot be renamed. Available from Archicad 26."
        );
        err |= RegisterCommand<MoveAttributesAndFoldersCommand> (
            attributeCommands, "1.7.2",
            "Moves attributes and attribute folders (with their contents) into the target folder, which must be of the same attribute type; an empty target path is the root folder. Returns one execution result per attribute and per folder. Available from Archicad 26."
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
            attributeCommands, "1.7.2",
            "Returns the layer combination in effect in the current window. No layer combination is returned when the current state of the layers matches none."
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
            attributeCommands, "1.7.2",
            "Finds loaded fonts by name through Archicad's font search and returns their index (as Text and Label fields reference fonts in fontIndex) and name, or an error for a name no loaded font has."
        );
        err |= RegisterCommand<GetGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.2",
            "Gets all graphic override rules of the project, with their rule group, criterion and style. Available from Archicad 27."
        );
        err |= RegisterCommand<GetGraphicOverrideRulesByNameCommand> (
            attributeCommands, "1.7.2",
            "Gets graphic override rules by name, with their rule group, criterion and style. The rule group narrows the search; without it the rule is searched in every rule group. Available from Archicad 27."
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
        err |= RegisterCommand<DeleteGraphicOverrideCombinationsCommand> (
            attributeCommands, "1.7.2",
            "Deletes graphic override combinations. Their rules are kept. Available from Archicad 27."
        );
        err |= RegisterCommand<GetGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.7.2",
            "Gets all graphic override rule groups of the project with the rules they contain, empty groups included. Available from Archicad 27."
        );
        err |= RegisterCommand<RenameGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.7.3",
            "Renames graphic override rule groups, found by name. Their rules are kept. Available from Archicad 27."
        );
        err |= RegisterCommand<DeleteGraphicOverrideRuleGroupsCommand> (
            attributeCommands, "1.7.2",
            "Deletes graphic override rule groups, found by name. A group that still contains rules is kept unless its rules are moved to another group or deleted with it. Available from Archicad 27."
        );
        err |= RegisterCommand<MoveGraphicOverrideRulesCommand> (
            attributeCommands, "1.7.3",
            "Moves graphic override rules to another rule group, or to another place in their own group. Available from Archicad 27."
        );
        AddCommandGroup (attributeCommands);
    }

    { // IFC Commands
        CommandGroup ifcCommands ("IFC Commands");
        err |= RegisterCommand<IFCFileOperationCommand> (
            ifcCommands, "1.2.6",
            "Executes an IFC file operation: opens or merges an IFC file, or saves the project as an IFC file. A save can name the export translator to use."
        );
        err |= RegisterCommand<GetIFCExportTranslatorsCommand> (
            ifcCommands, "1.6.0",
            "Lists the IFC export translators of the project, the preview translator first. Pass one of the names to IFCFileOperation as translatorName."
        );
        err |= RegisterCommand<GetElementsByIFCIdsCommand> (
            ifcCommands, "1.5.1",
            "Retrieves the elements by the given IFC identifiers."
        );
        err |= RegisterCommand<GetIFCIdsOfElementsCommand> (
            ifcCommands, "1.5.1",
            "Retrieves the IFC identifiers of the given elements."
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
        err |= RegisterCommand<RemoveIFCPropertiesOfElementsCommand> (
            ifcCommands, "1.7.3",
            "Removes local IFC properties, given by property set and property name, from elements. Available only in Archicad 25, 26 and 27 for now."
        );
        AddCommandGroup (ifcCommands);
    }

    { // Library Commands
        CommandGroup libraryCommands ("Library Commands");
        err |= RegisterCommand<GetLibrariesCommand> (
            libraryCommands, "1.0.1",
            "Gets the list of loaded libraries."
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
        err |= RegisterCommand<IsUsableGDLStringCommand> (
            libraryCommands, "1.7.3",
            "Checks whether each of the given strings is usable in GDL, e.g. before writing it into a GDL parameter or script. Returns one boolean for each string."
        err |= RegisterCommand<SetLibraryPartScriptsCommand> (
            libraryCommands, "1.7.3",
            "Sets the master, parameter, 2D, 3D and properties scripts of loaded library parts. Only the given scripts are changed; an empty script makes the section empty. A library part that is not editable (e.g. a part of a library package or a read-only library) is refused and left unchanged. Available from Archicad 28."
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
        err |= RegisterCommand<ReserveElementsCommand> (
            teamworkCommands, "1.1.4",
            "Reserves elements in Teamwork mode."
        );
        err |= RegisterCommand<ReleaseElementsCommand> (
            teamworkCommands, "1.1.4",
            "Releases elements in Teamwork mode."
        );
        err |= RegisterCommand<GetReservationStatusOfElementsCommand> (
            teamworkCommands, "1.7.2",
            "Returns the Teamwork reservation status of elements without reserving them: whether each element is free, reserved by the current user or reserved by another user, with the reserving user and the element's owner."
        );
        err |= RegisterCommand<GrantElementsCommand> (
            teamworkCommands, "1.7.2",
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
        AddCommandGroup (teamworkCommands);
    }

    { // Navigator Commands
        CommandGroup navigatorCommands ("Navigator Commands");
        err |= RegisterCommand<PublishPublisherSetCommand> (
            navigatorCommands, "0.1.0",
            "Performs a publish operation on the currently opened project. Only the given publisher set will be published."
        );
        err |= RegisterCommand<UpdateDrawingsCommand> (
            navigatorCommands, "1.1.4",
            "Performs a drawing update on the given elements."
        );
        err |= RegisterCommand<GetDrawingContentBoxesCommand> (
            navigatorCommands, "1.7.2",
            "Returns the full content box of the given Drawing elements, independent of their frames, as ACAPI_Drawing_GetFullDrawingContentBox reports it. Available from Archicad 27."
        );
        err |= RegisterCommand<GetDatabaseIdFromNavigatorItemIdCommand> (
            navigatorCommands, "1.1.4",
            "Gets the ID of the database associated with the supplied navigator item id"
        );
        err |= RegisterCommand<GetDatabasesCommand> (
            navigatorCommands, "1.7.2",
            "Lists every database of the given types (sections, elevations, interior elevations, details, worksheets, 3D documents, layouts, master layouts) "
            "with its database ID, name and reference ID, without walking the navigator trees."
        );
        err |= RegisterCommand<CreateDetailsCommand> (
            navigatorCommands, "1.4.0",
            "Creates independent Detail databases."
        );
        err |= RegisterCommand<CreateWorksheetsCommand> (
            navigatorCommands, "1.4.0",
            "Creates independent Worksheet databases."
        );
        err |= RegisterCommand<CreateLayoutCommand> (
            navigatorCommands, "1.4.0",
            "Creates Layouts and their backing master layouts. Archicad inserts a new layout at the first "
            "place of its subset, renumbering the sheets after it; previousNavigatorItemId or position "
            "'last' places it after an existing sheet or at the end instead."
        );
        err |= RegisterCommand<CreateLayoutSubsetCommand> (
            navigatorCommands, "1.4.0",
            "Creates Layout Book subsets."
        );
        err |= RegisterCommand<GetLayoutSubsetSettingsCommand> (
            navigatorCommands, "1.7.2",
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
            "Sets settings of layouts, including their master layout and Layout Info Panel custom data fields."
        );
        err |= RegisterCommand<GetLayoutCustomSchemeCommand> (
            navigatorCommands, "1.1.7",
            "Gets the Layout Info Panel custom field definitions (name and key) from Book Settings."
        );
        err |= RegisterCommand<GetModelViewOptionsCommand> (
            navigatorCommands, "1.1.4",
            "Gets all model view options"
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
        err |= RegisterCommand<GetStructureDisplayCommand> (
            navigatorCommands, "1.7.2",
            "Gets the Partial Structure Display (EntireStructure, CoreOnly, WithoutFinishes or StructureOnly) of the current window, and whether it is the 3D window. This is the setting of the window itself, not the one stored in a saved view (see GetViewSettings). Available from Archicad 27."
        );
        err |= RegisterCommand<SetStructureDisplayCommand> (
            navigatorCommands, "1.7.2",
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
            navigatorCommands, "1.7.2",
            "Gets whether the 3D cut planes are on and the cut planes (pa, pb, pc, pd) of the 3D window, in the form accepted by Set3DCutPlanes."
        );
        err |= RegisterCommand<Get3DStyleDetailsCommand> (
            navigatorCommands, "1.7.2",
            "Gets the settings of 3D styles by name (an empty name or no names means the current 3D style): rendering engine, model type, monochrome and background colors, contours, shadows and hatching. Available from Archicad 27."
        );
        err |= RegisterCommand<Create3DStylesCommand> (
            navigatorCommands, "1.7.2",
            "Creates 3D styles. Each new style starts from the current 3D style and differs from it only in the given fields. Available from Archicad 27."
        );
        err |= RegisterCommand<Change3DStylesCommand> (
            navigatorCommands, "1.7.2",
            "Changes the given fields of existing 3D styles, identified by name. Available from Archicad 27."
        );
        err |= RegisterCommand<Get3DProjectionSettingsCommand> (
            navigatorCommands, "1.7.1",
            "Gets the 3D projection settings of the 3D window: whether it is perspective or axonometric, the camera and target position, view cone, roll angle, distance and azimuth of a perspective, or the transformation matrix of an axonometry."
        );
        err |= RegisterCommand<Set3DProjectionSettingsCommand> (
            navigatorCommands, "1.7.1",
            "Sets a perspective projection for the 3D window. Every perspective parameter is required: read a known good set with Get3DProjectionSettings and pass it back, with the values to change replaced."
        );
        err |= RegisterCommand<Get3DWindowSettingsCommand> (
            navigatorCommands, "1.7.3",
            "Gets the 3D window settings: the pixel size of the 3D window and the zoom (scale and displacement) of its projection."
        );
        err |= RegisterCommand<Set3DWindowSettingsCommand> (
            navigatorCommands, "1.7.3",
            "Changes the pixel size of the 3D window and/or the zoom (scale and displacement) of its projection; the values not given are kept. Returns the settings read back after the change. Archicad may need a rebuild of the 3D window (RebuildView) before it shows the change, e.g. before SaveCurrentWindowAsImage."
        );
        err |= RegisterCommand<FitInWindowCommand> (
            navigatorCommands, "1.3.1",
            "Zooms to the given elements or fits everything in the window."
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
            "Creates Section elements on the floor plan."
        );
        err |= RegisterCommand<CreateInteriorElevationsCommand> (
            navigatorCommands, "1.5.8",
            "Creates Interior Elevation elements on the floor plan. Every consecutive pair of the given points becomes one segment, each with its own viewpoint - feed it the corner points of a room to get an interior elevation of that room."
        );
        err |= RegisterCommand<MoveNavigatorItemCommand> (
            navigatorCommands, "1.1.7",
            "Moves a navigator item to a new parent in the navigator tree."
        );
        err |= RegisterCommand<RenameNavigatorItemCommand> (
            navigatorCommands, "1.1.7",
            "Renames a navigator item or changes its ID. newId sets the custom layout number of a layout and the custom ID of any other item. Fails when Archicad refuses the change or does not apply the new name or ID (e.g. Project Map items, whose schedule items are named by their scheme). With customName false the item drops its custom name and follows its Project Map source again."
        );
        err |= RegisterCommand<DeleteNavigatorItemsCommand> (
            navigatorCommands, "1.1.7",
            "Deletes navigator items from the navigator tree: views and folders of the View Map, layouts and master layouts of the Layout Book, and independent worksheets and details of the Project Map. Layout subsets and marker-linked worksheets and details cannot be deleted."
        );
        err |= RegisterCommand<GetNavigatorItemTreeCommand> (
            navigatorCommands, "1.1.7",
            "Returns the full navigator item tree for the specified map. With includeChildCount true every item also reports the number of its direct children."
        );
        err |= RegisterCommand<GetNavigatorParentItemsCommand> (
            navigatorCommands, "1.7.3",
            "Returns the parent of each given navigator item: the folder, subset or layout book item it is in. The root of a map has no parent and gets an error."
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
            "Deletes the specified issue."
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
            "Exports specified issues to a BCF file."
        );
        err |= RegisterCommand<ImportIssuesFromBCFCommand> (
            issueCommands, "1.0.6",
            "Imports issues from the specified BCF file."
        );
        err |= RegisterCommand<ImportMarkupsFromPDFCommand> (
            issueCommands, "1.7.2",
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
            designOptionsCommands, "1.7.1",
            "Changes the name and/or the string id of the given design options. Available from Archicad 29."
        );
        err |= RegisterCommand<RenameDesignOptionSetsCommand> (
            designOptionsCommands, "1.7.1",
            "Changes the name of the given design option sets. Available from Archicad 29."
        );
        err |= RegisterCommand<RenameDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.7.1",
            "Changes the name of the given design option combinations. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteDesignOptionsCommand> (
            designOptionsCommands, "1.7.1",
            "Deletes the given design options. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteDesignOptionSetsCommand> (
            designOptionsCommands, "1.7.1",
            "Deletes the given design option sets. Available from Archicad 29."
        );
        err |= RegisterCommand<DeleteDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.7.1",
            "Deletes the given design option combinations. Available from Archicad 29."
        );
        err |= RegisterCommand<DuplicateDesignOptionCombinationsCommand> (
            designOptionsCommands, "1.7.3",
            "Duplicates the given design option combinations and returns the identifiers of the new combinations. Available from Archicad 29."
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
        AddCommandGroup (projectDataCommands);
    }

    { // MEP Commands
        CommandGroup mepCommands ("MEP Commands");
        err |= RegisterCommand<GetMEPElementsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the MEP (Mechanical, Electrical, Plumbing) elements of the project, optionally filtered by type and domain. MEP elements are ordinary elements, so the generic element commands work on them as well (for example they can be deleted with the DeleteElements command). Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPRoutingElementsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the details of the given MEP routing elements: domain, MEP system, offset from the home story, branch preference table, the ports logically connected at the route's beginning and end, route polyline, segments with cross section data (shape, size, preference table and reference id), begin and end nodes, anchor, reference line offsets, wall thickness, flange size and rigid segments, and nodes with their incoming and outgoing segments, elbows and transitions. Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPPortsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the ports of the given MEP elements including position, shape, size and connection status. Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPDistributionSystemsCommand> (
            mepCommands, "1.5.6",
            "Retrieves the MEP distribution systems of the project with their domain, MEP system attribute and member elements. Available from Archicad 28."
        );
        err |= RegisterCommand<CreateMEPRoutingElementsCommand> (
            mepCommands, "1.5.6",
            "Creates MEP routing elements (duct, pipe or cable carrier routes) along the given polylines with optional cross section data (shape, size, preference table and reference id), MEP system and layer (also set on the segments, elbows and transitions). Available from Archicad 28."
        );
        err |= RegisterCommand<CreateMEPElementsCommand> (
            mepCommands, "1.5.6",
            "Creates MEP elements (Terminal, Accessory, Equipment or Fitting) at the given positions. Available from Archicad 28."
        );
        err |= RegisterCommand<ModifyMEPRoutingElementsCommand> (
            mepCommands, "1.5.6",
            "Modifies the given MEP routing elements: MEP system, branch preference table, cross section data (shape, size, preference table and reference id), anchor, reference line offsets, wall thickness and flange size of all segments, and node positions. Available from Archicad 28."
        );
        err |= RegisterCommand<ConnectMEPElementsCommand> (
            mepCommands, "1.5.6",
            "Connects MEP routing elements to other MEP elements or routes. Merges routes, splits routes or creates branch elements as needed. Available from Archicad 28."
        );
        err |= RegisterCommand<GetMEPPreferenceTablesCommand> (
            mepCommands, "1.5.7",
            "Gets the circular cross section preference tables (referenceId, diameter, description) and the reference set of the Piping or Ventilation domain, optionally with the elbow, branch and transition preference tables. The angle set and the transition tables are available from Archicad 29. Available from Archicad 28."
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
            "Modifies the electrical project settings: sets the default voltage and adds or removes wiring types and load classifications. Available from Archicad 30."
        );
        AddCommandGroup (mepCommands);
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
            "Removes solid element operation links between target and operator elements."
        );
        err |= RegisterCommand<GetSolidElementLinksCommand> (
            solidElementOperationCommands, "1.5.4",
            "Returns solid element operation links for each queried element, grouped by role (target or operator)."
        );
        err |= RegisterCommand<TrimElementsCommand> (
            solidElementOperationCommands, "1.5.9",
            "Trims construction elements with a roof or shell: the roofs and shells in the list, or one given trimming element with a trim type."
        );
        err |= RegisterCommand<RemoveElementTrimsCommand> (
            solidElementOperationCommands, "1.5.9",
            "Removes the trim between an element and the roof or shell trimming it."
        );
        err |= RegisterCommand<GetElementTrimsCommand> (
            solidElementOperationCommands, "1.5.9",
            "Which roofs and shells trim each queried element, with the trim type, and which elements it trims."
        );
        AddCommandGroup (solidElementOperationCommands);
    }

    { // Script UI Commands
        CommandGroup scriptUICommands ("Script UI Commands");
        err |= RegisterCommand<ShowScriptUICommand> (
            scriptUICommands, "1.5.4",
            "Shows the given HTML content in a native Archicad palette (a tkinter alternative for Python scripts)."
        );
        err |= RegisterCommand<GetScriptUIResultCommand> (
            scriptUICommands, "1.5.4",
            "Retrieves and clears the result last submitted from the Script UI palette's page (via window.ACAPI.SubmitResult), if any, and reports whether the palette is still open, so a waiting script can stop when the user closes it."
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
    // starts or opens a project (#798). The palette shows the "run with update" icon once the result is in.
    VersionChecker::StartCheck ([] () {
        if (TapirPalette::HasInstance ()) {
            TapirPalette::Instance ().SetRunButtonIcon ();
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
    // Must run here, not left to the static GS::Ref's destructor at DLL/dylib unload time - see
    // ScriptUIPalette::Release for why (real macOS crash on quit, EXC_BAD_ACCESS tearing down the
    // browser control after Archicad's own environment had already begun shutting down).
    ScriptUIPalette::Release ();
    return NoError;
}
