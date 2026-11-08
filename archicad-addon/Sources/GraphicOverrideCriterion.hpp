#pragma once

#include "CommandBase.hpp"

#ifdef ServerMainVers_2700

// Builds the criterion XML of a graphic override rule from the structured "criterion" object
// (schema: GraphicOverrideCriterion). Returns false with outError set on an invalid criterion.
bool GraphicOverrideCriterionToXML (const GS::ObjectState& criterionOS, GS::UniString& outXML, GS::UniString& outError);

// Reads the criterion XML of a graphic override rule back into the structured "criterion" object
// that GraphicOverrideCriterionToXML takes. A condition it does not decode comes back as a raw
// {"criterionXML": "..."} leaf, which GraphicOverrideCriterionToXML writes back unchanged. Returns
// false when the XML is not in the format Archicad keeps rules in.
bool GraphicOverrideCriterionFromXML (const GS::UniString& criterionXML, GS::ObjectState& outCriterion);

#endif
