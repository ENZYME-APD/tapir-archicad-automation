#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"

// The fields of the element structs of the API_Element union, generated from the DevKit headers by
// tools/generate_element_field_table.py into ElementFieldTable.cpp. GetElementFields,
// SetElementFields and GetElementFieldDefinitions read and write an element's fields by these paths.
namespace ElementFields {

enum class FieldKind
{
    Real,           // float or double, by size
    Bool,
    SignedInt,      // signed integers and enums, by size
    UnsignedInt,
    CString,        // a char array
    UString,        // a GS::uchar_t array
    AttributeIndex,
    Guid
};

struct FieldInfo
{
    const char* path;           // as written in C++ from the element struct, e.g. "note.noteFont" or "u.planeRoof.angle"
    UInt32      offset;         // from the start of the API_Element union
    UInt32      size;
    FieldKind   kind;
    bool        readOnly;       // owners and counts that Archicad computes from the memo
    // The member of a union the field is in (e.g. "u.revolvedShell"), or nullptr. The members of a
    // union share their bytes: only the one the element uses (its roofClass, shellClass, labelClass
    // or structType) holds its values, and writing another one overwrites them.
    const char* variant;
    // The offsets of the members the field is in, outermost first ("begC" for "begC.y"). Archicad
    // checks a change mask on the first byte of a member (ACAPI_ELEMENT_MASK_CHECK), so the first
    // byte of each of them is set in the mask too.
    UInt32      ancestorCount;
    UInt32      ancestors[12];
};

struct FieldTable
{
    const char*         structName;     // e.g. "API_AngleDimensionType"
    const char*         unionMember;    // e.g. "angleDimension"
    const FieldInfo*    fields;
    UInt32              count;
};

// The fields of an element type, or nullptr when the type has none in the table or the table was not
// generated for this Archicad version.
const FieldTable* GetFieldTable (API_ElemTypeID typeID);

// Whether the table was generated for the Archicad version the add-on is built for.
bool IsAvailable ();

}
