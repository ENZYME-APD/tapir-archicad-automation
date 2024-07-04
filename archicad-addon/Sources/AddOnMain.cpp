#include "APIEnvir.h"
#include "ACAPinc.h"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "ApplicationCommands.hpp"
#include "ProjectCommands.hpp"
#include "ElementCommands.hpp"
#include "AttributeCommands.hpp"
#include "TeamworkCommands.hpp"
#include "IssueCommands.hpp"
#include "LibraryCommands.hpp"
#include "MigrationHelper.hpp"

API_AddonType __ACDLL_CALL CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, ID_ADDON_INFO_NAME, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, ID_ADDON_INFO_DESC, ACAPI_GetOwnResModule ());
    envir->addOnInfo.description += GS::UniString (" ") + ADDON_VERSION;
    return APIAddon_Preload;
}

GSErrCode __ACDLL_CALL RegisterInterface (void)
{
    return NoError;
}

GSErrCode __ACENV_CALL Initialize (void)
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

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetSelectedElementsCommand> ());
#ifdef ServerMainVers_2600
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<HighlightElementsCommand> ());
#endif

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateBuildingMaterialsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateCompositesCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetBuildingMaterialPhysicalPropertiesCommand> ());

    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<TeamworkReceiveCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetLibrariesCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ReloadLibrariesCommand> ());

    // Issue management
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CreateIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DeleteIssueCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetIssuesCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<AddCommentCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetCommentsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<AttachElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<DetachElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<GetAttachedElementsCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ExportToBCFCommand> ());
    err |= ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<ImportFromBCFCommand> ());

    return err;
}

GSErrCode __ACENV_CALL FreeData (void)
{
    return NoError;
}
