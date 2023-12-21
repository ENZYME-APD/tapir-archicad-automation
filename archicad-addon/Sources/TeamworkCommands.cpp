#include "TeamworkCommands.hpp"
#include "MigrationHelper.hpp"

TeamworkSendCommand::TeamworkSendCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String TeamworkSendCommand::GetName () const
{
    return "TeamworkSend";
}

GS::ObjectState TeamworkSendCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_Teamwork_SendChanges ();

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to send changes.");
    }

    return {};
}

TeamworkReceiveCommand::TeamworkReceiveCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String TeamworkReceiveCommand::GetName () const
{
    return "TeamworkReceive";
}

GS::ObjectState TeamworkReceiveCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_Teamwork_ReceiveChanges ();

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to receive changes.");
    }

    return {};
}
