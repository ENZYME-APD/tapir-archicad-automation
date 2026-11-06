#pragma once

#include "CommandBase.hpp"

#ifdef ServerMainVers_2700

// Builds the criterion XML of a graphic override rule from the structured "criterion" object
// (schema: GraphicOverrideCriterion). Returns false with outError set on an invalid criterion.
bool GraphicOverrideCriterionToXML (const GS::ObjectState& criterionOS, GS::UniString& outXML, GS::UniString& outError);

#endif
