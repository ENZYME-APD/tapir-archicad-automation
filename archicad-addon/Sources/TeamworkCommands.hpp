#pragma once

#include "CommandBase.hpp"

class TeamworkSendCommand : public CommandBase
{
public:
    TeamworkSendCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class TeamworkReceiveCommand : public CommandBase
{
public:
    TeamworkReceiveCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
