#pragma once

#include "CommandBase.hpp"

// Drawing primitive commands: read the 2D drawing primitives (lines, arcs, polygons, texts, ...)
// Archicad draws an element from. Read only, they change nothing in the project.

class GetDrawingPrimitivesOfElementsCommand : public CommandBase
{
public:
    GetDrawingPrimitivesOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
