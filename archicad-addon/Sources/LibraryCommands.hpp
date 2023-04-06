#pragma once

#include "CommandBase.hpp"

class ReloadLibrariesCommand : public CommandBase
{
public:
    ReloadLibrariesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
