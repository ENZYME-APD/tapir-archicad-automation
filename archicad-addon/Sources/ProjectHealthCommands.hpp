#pragma once

#include "CommandBase.hpp"

// GetProjectHealth: one read-only summary of what a project cleanup looks at. It runs GetUnusedAttributes and
// GetUnusedLibraryParts and adds the missing library parts, the hotlinks whose source is not available, the layers
// with duplicate names and the stories without elements. What the Archicad API does not give is reported with
// known: false, never guessed.
class GetProjectHealthCommand : public CommandBase
{
public:
    GetProjectHealthCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
