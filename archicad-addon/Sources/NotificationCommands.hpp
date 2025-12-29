#pragma once

#include "CommandBase.hpp"

class SetElementNotificationClientCommand : public CommandBase
{
public:
    SetElementNotificationClientCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;

private:
    static void SendMessageToNotificationClient (const GS::ObjectState& os);
    static GSErrCode ElementEventHandlerProc (const API_NotifyElementType *elemType);

private:
    static std::vector<std::pair<GS::UniString, Int32>> clientConnections;
};
