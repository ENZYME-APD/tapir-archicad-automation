#include "CommandBase.hpp"

constexpr const char* CommandNamespace = "TapirCommand";

GS::String CommandBase::GetNamespace () const
{
    return CommandNamespace;
}

GS::Optional<GS::UniString> CommandBase::GetSchemaDefinitions () const
{
    return {};
}

GS::Optional<GS::UniString> CommandBase::GetInputParametersSchema () const
{
    return {};
}

GS::Optional<GS::UniString> CommandBase::GetResponseSchema () const
{
    return {};
}

void CommandBase::OnResponseValidationFailed (const GS::ObjectState& /*response*/) const
{

}

API_AddOnCommandExecutionPolicy CommandBase::GetExecutionPolicy () const
{
    return API_AddOnCommandExecutionPolicy::ScheduleForExecutionOnMainThread;
}

#ifdef ServerMainVers_2600
bool CommandBase::IsProcessWindowVisible () const
{
    return false;
}
#endif

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const char* errorMessage)
{
    GS::ObjectState error;
    error.Add ("code", errorCode);
    error.Add ("message", errorMessage);
    return GS::ObjectState ("error", error);
}
