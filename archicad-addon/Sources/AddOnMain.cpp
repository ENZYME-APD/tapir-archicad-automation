#include "APIEnvir.h"
#include "ACAPinc.h"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
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

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, ID_ADDON_INFO_NAME, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, ID_ADDON_INFO_DESC, ACAPI_GetOwnResModule ());
    envir->addOnInfo.description += GS::UniString (" ") + ADDON_VERSION;
    return APIAddon_Preload;
}

GSErrCode RegisterInterface (void)
{
    return NoError;
}

GSErrCode Initialize (void)
{
    GSErrCode err = NoError;

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetAddOnVersionCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetArchicadLocationCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<QuitArchicadCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetProjectInfoCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetProjectInfoFieldsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<SetProjectInfoFieldCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetStoryInfoCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetHotlinksCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<PublishPublisherSetCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetDetailsOfElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetSelectedElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetSubelementsOfHierarchicalElementsCommand> ());
#ifdef ServerMainVers_2600
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<HighlightElementsCommand> ());
#endif
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<MoveElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetGDLParametersOfElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<SetGDLParametersOfElementsCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateColumnsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateSlabsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateObjectsCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateBuildingMaterialsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateLayersCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateCompositesCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetBuildingMaterialPhysicalPropertiesCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<TeamworkReceiveCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetLibrariesCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ReloadLibrariesCommand> ());

    // Issue management
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DeleteIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetIssuesCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<AddCommentToIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetCommentsFromIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<AttachElementsToIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DetachElementsFromIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetElementsAttachedToIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ExportIssuesToBCFCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ImportIssuesFromBCFCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetPropertyValuesOfElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<SetPropertyValuesOfElementsCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetClassificationsOfElementsCommand> ());

    return err;
}

GSErrCode FreeData (void)
{
    return NoError;
}
