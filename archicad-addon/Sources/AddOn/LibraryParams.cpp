#include "LibraryParams.hpp"
#include "ObjectState.hpp"

GS::String LibraryParamsCommand::GetName() const
{
    return "ReloadLibraries";
}

GS::ObjectState LibraryParamsCommand::Execute(const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = {};
    err = ACAPI_Automate(APIDo_ReloadLibrariesID);
    
    if (err != NoError) {
        return CreateErrorResponse(err, "Failed to reload libraries.");
    }

    return {};
}