#pragma once

#include "CommandBase.hpp"

class GetSelectedElementsCommand : public CommandBase
{
public:
    GetSelectedElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
