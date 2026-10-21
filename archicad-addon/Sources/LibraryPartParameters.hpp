#pragma once

#include "CommandBase.hpp"

// GDL parameter and library part helpers shared by the commands that place library parts
// (CreateObjects, CreateLamps, CreateGridElements). Free of version guards: CreateObjects
// builds for every supported Archicad, the grid commands only from Archicad 26.

bool IsNumericParam (API_AddParID t);

// GDL parameter names are matched without case, as GDL does: a part's own list may spell
// ZZYZX as zzyzx (Bed Double 01 does), while a tool's list spells it ZZYZX.

// The parameter of that name that holds a single number, or nullptr.
API_AddParType* FindParameter (API_AddParType** params, const char* name);
bool GetNumericParameter (API_AddParType** params, const char* name, double& value);
bool SetNumericParameter (API_AddParType** params, const char* name, double value);

// The library part's own A and B; a and b stay as they are when it has none to divide by.
GSErrCode GetLibraryPartOwnSize (Int32 libInd, double& a, double& b);

// Sets one GDL parameter by name. Returns false (and a warning) when the name is unknown
// or the value has the wrong JSON type for the parameter.
bool ApplyParameter (API_AddParType** params, const GS::ObjectState& p, GS::Array<GS::UniString>& warnings);

// Replaces memo.params with the default parameters of the library part element.object.libInd
// names, and sizes the element by the part's own A and B, as the Object Settings dialog does
// when another part is picked.
GSErrCode LoadLibraryPartDefaults (API_Element& element, API_ElementMemo& memo);
