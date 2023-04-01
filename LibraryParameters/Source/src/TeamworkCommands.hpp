#pragma once

#include "CommandBase.hpp"

class TeamworkSendCommand : public CommandBase
{
public:
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class TeamworkReceiveCommand : public CommandBase
{
public:
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
