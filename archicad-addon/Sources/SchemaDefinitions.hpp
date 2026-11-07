#pragma once

#include "UniString.hpp"
#include "Array.hpp"
#include "Optional.hpp"

const GS::UniString& GetCommonSchemaDefinitions ();

// The part of the common schema definitions the given schemas need: every definition they reference
// with "$ref": "#/<Name>", and every definition those reference, and so on. No value when they
// reference none. Whenever the references cannot be followed with certainty (a reference that is not
// in the file, one of another form, a schema that cannot be read), it gives the whole file, so a
// command never loses a definition it needs.
GS::Optional<GS::UniString> GetCommonSchemaDefinitionsFor (const GS::Array<GS::UniString>& schemas);
