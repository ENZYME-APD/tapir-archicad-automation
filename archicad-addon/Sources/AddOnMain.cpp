#include "APIEnvir.h"
#include "ACAPinc.h"

#include "DGModule.hpp"
#include "DGFolderDialog.hpp"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "DeveloperTools.hpp"

#include "3DCutPlaneCommands.hpp"



#include "AboutDialog.hpp"
#include "TapirPalette.hpp"
#include "VersionChecker.hpp"

#include "ApplicationCommands.hpp"
#include "ProjectCommands.hpp"
#include "ElementCommands.hpp"
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

template <typename CommandType>
GSErrCode RegisterCommand (CommandGroup& group, const GS::UniString& version, const GS::UniString& description)
{
    GS::Owner<CommandType> command = GS::NewOwned<CommandType> ();
    group.commands.push_back (CommandInfo (
        command->GetName (),
        description,
        version,
        command->GetInputParametersSchema (),
        command->GetResponseSchema ())
    );

    GSErrCode err = ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (command.Pass ());
    if (err != NoError) {
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
                        if (!VersionChecker::IsUsingLatestVersion ()) {
                            TapirPalette::Instance ().UpdateAddOn ();
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
    VersionChecker::CreateInstance (envir->serverInfo.mainVersion);
    return APIAddon_Preload;
}

GSErrCode RegisterInterface (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_FOR_PALETTE, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU_FOR_UPDATE, 0, MenuCode_UserDef, MenuFlag_Default);
    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU, 0, MenuCode_UserDef, MenuFlag_Default);
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
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_1, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_2, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_3, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_4, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_5, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_RUNSCRIPT_6, MenuCommandHandler);
    err |= TapirPalette::RegisterPaletteControlCallBack ();

    // Forces the palette singleton (and its saved shortcut-slot preferences) to load immediately,
    // so custom shortcut menu labels are applied at startup rather than only after the user first
    // opens the palette or triggers a shortcut.
    TapirPalette::Instance ();

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
        err |= RegisterCommand<QuitArchicadCommand> (
            applicationCommands, "0.1.0",
            "Performs a quit operation on the currently running Archicad instance."
        );
        err |= RegisterCommand<GetCurrentWindowTypeCommand> (
            applicationCommands, "1.0.7",
            "Returns the type of the current (active) window."
        );
        err |= RegisterCommand<ChangeWindowCommand> (
            applicationCommands, "1.3.1",
            "Changes the current (active) window to the given window."
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
            "Retrieves information about the story sructure of the currently loaded project."
        );
        err |= RegisterCommand<SetStoriesCommand> (
            projectCommands, "1.1.5",
            "Sets the story sructure of the currently loaded project."
        );
        err |= RegisterCommand<GetHotlinksCommand> (
            projectCommands, "0.1.0",
            "Gets the file system locations (path) of the hotlink modules. The hotlinks can have tree hierarchy in the project."
        );
        err |= RegisterCommand<OpenProjectCommand> (
            projectCommands, "1.0.7",
            "Opens the given project."
        );
        err |= RegisterCommand<CloseProjectCommand> (
            projectCommands, "1.3.1",
            "Closes the currently opened project."
        );
        err |= RegisterCommand<SaveProjectCommand> (
            projectCommands, "1.3.1",
            "Saves the currently opened project."
        );
        err |= RegisterCommand<GetCalculationUnitsCommand> (
            projectCommands, "1.4.0",
            "Gets the project calculation units."
        );
        err |= RegisterCommand<GetGeoLocationCommand> (
            projectCommands, "1.1.6",
            "Gets the project location details."
        );
        err |= RegisterCommand<SetGeoLocationCommand> (
            projectCommands, "1.2.9",
            "Sets the project location details."
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
        err |= RegisterCommand<ChangeSelectionOfElementsCommand> (
            elementCommands, "1.0.7",
            "Adds/removes a number of elements to/from the current selection."
        );
        err |= RegisterCommand<FilterElementsCommand> (
            elementCommands, "1.0.7",
            "Tests an elements by the given criterias."
        );
        err |= RegisterCommand<GetDetailsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Gets the details of the given elements (geometry parameters etc)."
        );
        err |= RegisterCommand<SetDetailsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Sets the details of the given elements (floor, layer, order etc)."
        );
        err |= RegisterCommand<Get3DBoundingBoxesCommand> (
            elementCommands, "1.1.2",
            "Get the 3D bounding box of elements. The bounding box is calculated from the global origin in the 3D view. The output is the array of the bounding boxes respective to the input array of elements."
        );
        err |= RegisterCommand<GetSubelementsOfHierarchicalElementsCommand> (
            elementCommands, "1.0.6",
            "Gets the subelements of the given hierarchical elements."
        );
        err |= RegisterCommand<GetConnectedElementsCommand> (
            elementCommands, "1.1.4",
            "Gets connected elements of the given elements."
        );
        err |= RegisterCommand<GetZoneBoundariesCommand> (
            elementCommands, "1.2.3",
            "Gets the boundaries of the given Zone (connected elements, neighbour zones, etc.)."
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
            "Moves elements with a given vector."
        );
        err |= RegisterCommand<RotateElementsCommand> (
            elementCommands, "1.5.3",
            "Rotates elements around a reference point."
        );
        err |= RegisterCommand<DeleteElementsCommand> (
            elementCommands, "1.2.1",
            "Deletes elements."
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
            elementCommands, "1.0.8",
            "Gets all the GDL parameters (name, type, value) of the given elements."
        );
        err |= RegisterCommand<SetGDLParametersOfElementsCommand> (
            elementCommands, "1.0.8",
            "Sets the given GDL parameters of the given elements."
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
        err |= RegisterCommand<CreateMorphsCommand> (
            elementCommands, "1.4.0",
            "Creates Morph elements from simple box definitions."
        );
        err |= RegisterCommand<CreateRoofsCommand> (
            elementCommands, "1.4.0",
            "Creates multi-plane Roof elements based on footprint, level and roof profile data."
        );
        err |= RegisterCommand<CreateAssociativeDimensionsCommand> (
            elementCommands, "1.4.0",
            "Creates associative linear dimensions from explicit witness point references."
        );
        err |= RegisterCommand<CreateAssociativeDimensionsOnSectionCommand> (
            elementCommands, "1.4.0",
            "Creates associative linear dimensions on section elements using common wall, slab, beam, column and opening presets."
        );
        err |= RegisterCommand<CreateWallThicknessDimensionsCommand> (
            elementCommands, "1.4.0",
            "Creates associative wall thickness dimensions for the given walls."
        );
        err |= RegisterCommand<GetDimensionDataCommand> (
            elementCommands, "1.5.0",
            "Gets witness point data (coordinates, measured values) from existing dimension chains."
        );
        err |= RegisterCommand<CreateZonesCommand> (
            elementCommands, "1.1.8",
            "Creates Zone elements based on the given parameters."
        );
        err |= RegisterCommand<CreatePolylinesCommand> (
            elementCommands, "1.1.5",
            "Creates Polyline elements based on the given parameters."
        );
        err |= RegisterCommand<CreateObjectsCommand> (
            elementCommands, "1.0.3",
            "Creates Object elements based on the given parameters."
        );
        err |= RegisterCommand<CreateLampsCommand> (
            elementCommands, "1.5.0",
            "Creates Lamp elements based on the given parameters."
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
        AddCommandGroup (elementCommands);
    }

    { // Element grouping Commands
        CommandGroup elementGroupingCommands ("Element grouping Commands");
        err |= RegisterCommand<CreateGroupsCommand> (
            elementGroupingCommands, "1.4.0",
            "Creates groups of the passed elements"
        );
        AddCommandGroup (elementGroupingCommands);
    }

    { // Favorites Commands
        CommandGroup favoritesCommands ("Favorites Commands");
        err |= RegisterCommand<GetFavoritesByTypeCommand> (
            favoritesCommands, "1.2.2",
            "Returns a list of the names of all favorites with the given element type"
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
            "Export the project's Favorites to a .prefs file or folder."
        );
        err |= RegisterCommand<ApplyFavoritesToElementsCommand> (
            favoritesCommands, "1.5.4",
            "Apply the given favorites to existing elements. Only settings-type parameters are changed - "
            "geometry (position, floor, and dimensions such as a Wall's height) is left untouched, so applying "
            "a Favorite never moves or resizes the target element. By default settings, classifications, "
            "categories and properties are all applied; each can be opted out of individually."
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
            "Returns the property values of the elements for the given property. It works for subelements of hierarchal elements also."
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
            "Updates the expression(s) of existing expression-based Custom Property Definitions."
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
        AddCommandGroup (classificationCommands);
    }

    { // Attribute Commands
        CommandGroup attributeCommands ("Attribute Commands");
        err |= RegisterCommand<GetAttributesByTypeCommand> (
            attributeCommands, "1.1.3",
            "Returns the details of every attribute of the given type."
        );
        err |= RegisterCommand<DeleteAttributesCommand> (
            attributeCommands, "1.5.4",
            "Deletes the given attributes."
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
            "Creates or overwrites MEP System attributes based on the given parameters."
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
            "Returns the details of the given MEP System attributes."
        );
        err |= RegisterCommand<GetPenTablesCommand> (
            attributeCommands, "1.5.4",
            "Returns the details of the given Pen Table attributes."
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
        AddCommandGroup (attributeCommands);
    }

    { // IFC Commands
        CommandGroup ifcCommands ("IFC Commands");
        err |= RegisterCommand<IFCFileOperationCommand> (
            ifcCommands, "1.2.6",
            "Executes an IFC file operation."
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
            "Adds the given files into the embedded library."
        );
        err |= RegisterCommand<GetAvailableLibraryPartsCommand> (
            libraryCommands, "1.5.0",
            "Lists library parts currently available to the project. Filter by typeId (e.g. 'Door', 'Window', 'Object', 'Lamp')."
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
        err |= RegisterCommand<GetDatabaseIdFromNavigatorItemIdCommand> (
            navigatorCommands, "1.1.4",
            "Gets the ID of the database associated with the supplied navigator item id"
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
            "Creates Layouts and their backing master layouts."
        );
        err |= RegisterCommand<CreateLayoutSubsetCommand> (
            navigatorCommands, "1.4.0",
            "Creates Layout Book subsets."
        );
        err |= RegisterCommand<CreateDrawingsCommand> (
            navigatorCommands, "1.4.0",
            "Creates Drawing elements on the specified or active layout from navigator items."
        );
        err |= RegisterCommand<ChangeDrawingLinkCommand> (
            navigatorCommands, "1.5.4",
            "Relinks a Drawing to a different source navigator item. Archicad has no in-place relink "
            "API, so this recreates the Drawing against the new source and deletes the original - the "
            "returned elementId is a NEW guid, not the input one. The Drawing Title marker's own "
            "position is not preserved (undocumented Archicad limitation)."
        );
        err |= RegisterCommand<GetLayoutSettingsCommand> (
            navigatorCommands, "1.1.7",
            "Gets settings of layouts, including Layout Info Panel custom data fields."
        );
        err |= RegisterCommand<SetLayoutSettingsCommand> (
            navigatorCommands, "1.1.7",
            "Sets settings of layouts, including Layout Info Panel custom data fields."
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
            "Clones Project Map viewpoints into the View Map, optionally into a specified folder."
        );
        err |= RegisterCommand<CreateViewsInViewMapCommand> (
            navigatorCommands, "1.1.7",
            "Creates independent (non-clone) navigator views in the View Map by copying database and settings from source items."
        );
        err |= RegisterCommand<CreateViewMapFolderCommand> (
            navigatorCommands, "1.1.7",
            "Creates a new folder in the View Map."
        );
        err |= RegisterCommand<Set3DCutPlanesCommand> (
            navigatorCommands, "1.3.1",
            "Sets the 3D cut planes."
        );
        err |= RegisterCommand<FitInWindowCommand> (
            navigatorCommands, "1.3.1",
            "Zooms to the given elements or fits everything in the window."
        );
        err |= RegisterCommand<CreateSectionsCommand> (
            navigatorCommands, "1.5.0",
            "Creates Section elements on the floor plan."
        );
        err |= RegisterCommand<MoveNavigatorItemCommand> (
            navigatorCommands, "1.1.7",
            "Moves a navigator item to a new parent in the navigator tree."
        );
        err |= RegisterCommand<RenameNavigatorItemCommand> (
            navigatorCommands, "1.1.7",
            "Renames a navigator item or changes its ID."
        );
        err |= RegisterCommand<DeleteNavigatorItemsCommand> (
            navigatorCommands, "1.1.7",
            "Deletes navigator items from the navigator tree."
        );
        err |= RegisterCommand<GetNavigatorItemTreeCommand> (
            navigatorCommands, "1.1.7",
            "Returns the full navigator item tree for the specified map."
        );
        AddCommandGroup (navigatorCommands);
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
            "Retrieves information about existing design option sets. Available from Archicad 29."
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
        err |= RegisterCommand<MoveElementsToDesignOptionsCommand> (
            designOptionsCommands, "1.5.1",
            "Moves the given elements into the given design options. Use NULLGuid for design option to remove the element from any design options and move it to the main model. Available from Archicad 29."
        );
        err |= RegisterCommand<MoveDesignOptionsToAnotherSetCommand> (
            designOptionsCommands, "1.5.1",
            "Moves the given design options to another sets. Available from Archicad 29."
        );
        AddCommandGroup (designOptionsCommands);
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
        AddCommandGroup (solidElementOperationCommands);
    }

    { // Developer Commands
        CommandGroup developerCommands ("Developer Commands");
        err |= RegisterCommand<GenerateDocumentationCommand> (
            developerCommands, "1.0.7",
            "Generates files for the documentation. Used by Tapir developers only."
        );
        AddCommandGroup (developerCommands);
    }

    return err;
}

GSErrCode FreeData (void)
{
    return NoError;
}
