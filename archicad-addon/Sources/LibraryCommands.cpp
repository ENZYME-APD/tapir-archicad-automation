#include "LibraryCommands.hpp"
#include "ObjectState.hpp"

ReloadLibrariesCommand::ReloadLibrariesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReloadLibrariesCommand::GetName () const
{
    return "ReloadLibraries";
}

GS::ObjectState ReloadLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_Automate (APIDo_ReloadLibrariesID);

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to reload libraries.");
    }

    return {};
}
