#include "APIEnvir.h"
#include "ACAPinc.h"

#include "DGModule.hpp"
#include "DGFolderDialog.hpp"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "DeveloperTools.hpp"

#include "ApplicationCommands.hpp"
#include "ProjectCommands.hpp"
#include "ElementCommands.hpp"
#include "ElementCreationCommands.hpp"
#include "AttributeCommands.hpp"
#include "TeamworkCommands.hpp"
#include "IssueCommands.hpp"
#include "LibraryCommands.hpp"
#include "PropertyCommands.hpp"
#include "ClassificationCommands.hpp"
#include "MigrationHelper.hpp"

static std::vector<CommandGroup> gCommandGroups;

template <typename CommandType>
GSErrCode RegisterCommand (CommandGroup& group, const GS::UniString& version, const GS::UniString& description)
{
    GS::Owner command = GS::NewOwned<CommandType> ();
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


static void GenerateDocumentation ()
{
    DG::FolderDialog folderPicker;
    if (folderPicker.Invoke ()) {
        GenerateDocumentation (folderPicker.GetFolder (), gCommandGroups);
    }
}

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
    switch (menuParams->menuItemRef.menuResID) {
        case ID_ADDON_MENU:
            switch (menuParams->menuItemRef.itemIndex) {
                case ID_ADDON_MENU_GENERATE_DOC:
                    GenerateDocumentation ();
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
    return APIAddon_Preload;
}

GSErrCode RegisterInterface (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU, 0, MenuCode_UserDef, MenuFlag_Default);

    return err;
}

GSErrCode Initialize (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU, MenuCommandHandler);

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
        gCommandGroups.push_back (applicationCommands);
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
        err |= RegisterCommand<GetStoryInfoCommand> (
            projectCommands, "1.0.2",
            "Retrieves information about the story sructure of the currently loaded project."
        );
        err |= RegisterCommand<GetHotlinksCommand> (
            projectCommands, "0.1.0",
            "Gets the file system locations (path) of the hotlink modules. The hotlinks can have tree hierarchy in the project."
        );
        err |= RegisterCommand<PublishPublisherSetCommand> (
            projectCommands, "0.1.0",
            "Performs a publish operation on the currently opened project. Only the given publisher set will be published."
        );
        gCommandGroups.push_back (projectCommands);
    }

    { // Element Commands
        CommandGroup elementCommands ("Element Commands");
        err |= RegisterCommand<GetSelectedElementsCommand> (
            elementCommands, "0.1.0",
            "Gets the list of the currently selected elements."
        );
        err |= RegisterCommand<GetDetailsOfElementsCommand> (
            elementCommands, "1.0.7",
            "Gets the details of the given elements (geometry parameters etc)."
        );
        err |= RegisterCommand<GetSubelementsOfHierarchicalElementsCommand> (
            elementCommands, "1.0.6",
            "Gets the subelements of the given hierarchical elements."
        );
#ifdef ServerMainVers_2600
        err |= RegisterCommand<HighlightElementsCommand> (
            elementCommands, "1.0.3",
            "Highlights the elements given in the elements array. In case of empty elements array removes all previously set highlights."
        );
#endif
        err |= RegisterCommand<MoveElementsCommand> (
            elementCommands, "1.0.2",
            "Moves elements with a given vector."
        );
        err |= RegisterCommand<GetGDLParametersOfElementsCommand> (
            elementCommands, "1.0.2",
            "Gets all the GDL parameters (name, type, value) of the given elements."
        );
        err |= RegisterCommand<SetGDLParametersOfElementsCommand> (
            elementCommands, "1.0.2",
            "Sets the given GDL parameters of the given elements."
        );
        err |= RegisterCommand<GetPropertyValuesOfElementsCommand> (
            elementCommands, "1.0.6",
            "Returns the property values of the elements for the given property. It works for subelements of hierarchal elements also."
        );
        err |= RegisterCommand<SetPropertyValuesOfElementsCommand> (
            elementCommands, "1.0.6",
            "Sets the property values of elements. It works for subelements of hierarchal elements also."
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
        err |= RegisterCommand<CreateObjectsCommand> (
            elementCommands, "1.0.3",
            "Creates Object elements based on the given parameters."
        );
        gCommandGroups.push_back (elementCommands);
    }

    { // Attribute Commands
        CommandGroup attributeCommands ("Attribute Commands");
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
        gCommandGroups.push_back (attributeCommands);
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
        gCommandGroups.push_back (libraryCommands);
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
        gCommandGroups.push_back (teamworkCommands);
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
        gCommandGroups.push_back (issueCommands);
    }

    return err;
}

GSErrCode FreeData (void)
{
    return NoError;
}
