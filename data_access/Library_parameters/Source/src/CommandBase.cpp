#include "CommandBase.hpp"

#include "SchemaDefinitions.hpp"

constexpr const char* CommandNamespace = "TapirCommand";

GS::String CommandBase::GetNamespace () const
{
    return CommandNamespace;
}

API_AddOnCommandExecutionPolicy CommandBase::GetExecutionPolicy () const
{
    return API_AddOnCommandExecutionPolicy::ScheduleForExecutionOnMainThread;
}

void CommandBase::OnResponseValidationFailed (const GS::ObjectState& /*response*/) const
{

}

#ifdef ServerMainVers_2600
bool CommandBase::IsProcessWindowVisible () const
{
    return false;
}
#endif

GS::Optional<GS::UniString> CommandBase::GetSchemaDefinitions () const
{
    if (IsUsingCommonSchemaDefinitions ()) {
        return GetCommonSchemaDefinitions ();
    } else {
        return {};
    }
}

bool CommandBase::IsUsingCommonSchemaDefinitions () const
{
    return false;
}

GS::Optional<GS::UniString> CommandBase::GetInputParametersSchema () const
{
    return {};
}

GS::Optional<GS::UniString> CommandBase::GetResponseSchema () const
{
    return {};
}

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const char* errorMessage)
{
    GS::ObjectState error;
    error.Add ("code", errorCode);
    error.Add ("message", errorMessage);
    return GS::ObjectState ("error", error);
}
