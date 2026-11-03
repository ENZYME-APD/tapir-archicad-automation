#pragma once

#include "CommandBase.hpp"

// Polygon commands: geometry helpers that work on coordinates sent by the caller, not on
// elements of the project, so they change nothing and need no undo scope.

class RegularizePolygonsCommand : public CommandBase
{
public:
    RegularizePolygonsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class RegularizePolylinesCommand : public CommandBase
{
public:
    RegularizePolylinesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
