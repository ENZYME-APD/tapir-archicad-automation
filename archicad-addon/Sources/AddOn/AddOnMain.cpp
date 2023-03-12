#include "APIEnvir.h"
#include "ACAPinc.h"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "ApplicationCommands.hpp"
#include "ProjectCommands.hpp"
#include "ElementCommands.hpp"
#include "AttributeCommands.hpp"
#include "TeamworkCommands.hpp"
#include "LibraryCommands.hpp"
#include "Commands.hpp"

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

    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetAddOnVersionCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetArchicadLocationCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<QuitArchicadCommand> ());

    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetProjectInfoCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetProjectInfoFieldsCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<SetProjectInfoFieldCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetHotlinksCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<PublishPublisherSetCommand> ());

    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetSelectedElementsCommand> ());

    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<GetBuildingMaterialPhysicalPropertiesCommand> ());

    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<TeamworkReceiveCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<ReloadLibrariesCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<MoveElementsCommand> ());
    err |= ACAPI_Install_AddOnCommandHandler (GS::NewOwned<CreateColumnsCommand> ());

    return err;
}

GSErrCode __ACENV_CALL FreeData (void)
{
    return NoError;
}
