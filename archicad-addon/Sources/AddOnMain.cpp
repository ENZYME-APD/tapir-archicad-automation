#include "APIEnvir.h"
#include "ACAPinc.h"

#include "DGModule.hpp"
#include "DGFolderDialog.hpp"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "DeveloperTools.hpp"

#include "AboutDialog.hpp"
#include "TapirPalette.hpp"
#include "VersionChecker.hpp"

#include "ApplicationCommands.hpp"
#include "ProjectCommands.hpp"
#include "ElementCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "ElementCreationCommands.hpp"
#include "AttributeCommands.hpp"
#include "TeamworkCommands.hpp"
#include "IssueCommands.hpp"
#include "LibraryCommands.hpp"
#include "PropertyCommands.hpp"
#include "ClassificationCommands.hpp"
#include "FavoritesCommands.hpp"
#include "MigrationHelper.hpp"
#include "NavigatorCommands.hpp"
#include "RevisionCommands.hpp"

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

    return err;
}

GSErrCode Initialize (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_FOR_PALETTE, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU_FOR_UPDATE, MenuCommandHandler);
    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU, MenuCommandHandler);
	err |= TapirPalette::RegisterPaletteControlCallBack ();

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
        err |= RegisterCommand<GetGeoLocationCommand> (
            projectCommands, "1.1.6",
            "Gets the project location details."
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
        err |= RegisterCommand<HighlightElementsCommand> (
            elementCommands, "1.0.3",
            "Highlights the elements given in the elements array. In case of empty elements array removes all previously set highlights."
        );
        err |= RegisterCommand<MoveElementsCommand> (
            elementCommands, "1.0.2",
            "Moves elements with a given vector."
        );
        err |= RegisterCommand<DeleteElementsCommand> (
            elementCommands, "1.2.1",
            "Deletes elements."
        );
        err |= RegisterCommand<GetGDLParametersOfElementsCommand> (
            elementCommands, "1.0.8",
            "Gets all the GDL parameters (name, type, value) of the given elements."
        );
        err |= RegisterCommand<SetGDLParametersOfElementsCommand> (
            elementCommands, "1.0.8",
            "Sets the given GDL parameters of the given elements."
        );
        err |= RegisterCommand<GetClassificationsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Returns the classification of the given elements in the given classification systems. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<SetClassificationsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Sets the classifications of elements. In order to set the classification of an element to unclassified, omit the classificationItemId field. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<CreateColumnsCommand> (
            elementCommands, "1.0.3",
            "Creates Column elements based on the given parameters."
        );
        err |= RegisterCommand<CreateSlabsCommand> (
            elementCommands, "1.0.3",
            "Creates Slab elements based on the given parameters."
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
        err |= RegisterCommand<CreateMeshesCommand> (
            elementCommands, "1.1.9",
            "Creates Mesh elements based on the given parameters."
        );
        AddCommandGroup (elementCommands);
    }

    { // Favorites Commands
        CommandGroup favoritesCommands ("Favorites Commands");
        err |= RegisterCommand<ApplyFavoritesToElementDefaultsCommand> (
            favoritesCommands, "1.1.2",
            "Apply the given favorites to element defaults."
        );
        err |= RegisterCommand<CreateFavoritesFromElementsCommand> (
            favoritesCommands, "1.1.2",
            "Create favorites from the given elements."
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
        AddCommandGroup (propertyCommands);
    }

    { // Attribute Commands
        CommandGroup attributeCommands ("Attribute Commands");
        err |= RegisterCommand<GetAttributesByTypeCommand> (
            attributeCommands, "1.1.3",
            "Returns the details of every attribute of the given type."
        );
        err |= RegisterCommand<CreateLayersCommand> (
            attributeCommands, "1.0.3",
            "Creates Layer attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateBuildingMaterialsCommand> (
            attributeCommands, "1.0.1",
            "Creates Building Material attributes based on the given parameters."
        );
        err |= RegisterCommand<CreateCompositesCommand> (
            attributeCommands, "1.0.2",
            "Creates Composite attributes based on the given parameters."
        );
        err |= RegisterCommand<GetBuildingMaterialPhysicalPropertiesCommand> (
            attributeCommands, "0.1.3",
            "Retrieves the physical properties of the given Building Materials."
        );
        AddCommandGroup (attributeCommands);
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
