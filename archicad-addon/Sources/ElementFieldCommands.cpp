#include "ElementFieldCommands.hpp"
#include "ElementFieldTable.hpp"
#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

using namespace ElementFields;

namespace {

const char* NotAvailableMessage = "The element field table is not available in this Archicad version (it is generated from the Archicad 29 and 30 DevKits).";

const char* GetKindName (FieldKind kind)
{
    switch (kind) {
        case FieldKind::Real:           return "Real";
        case FieldKind::Bool:           return "Bool";
        case FieldKind::SignedInt:      return "SignedInt";
        case FieldKind::UnsignedInt:    return "UnsignedInt";
        case FieldKind::CString:        return "CString";
        case FieldKind::UString:        return "UString";
        case FieldKind::AttributeIndex: return "AttributeIndex";
        case FieldKind::Guid:           return "Guid";
    }
    return "";
}

// The JSON type of a field's value.
const char* GetJsonTypeName (FieldKind kind)
{
    switch (kind) {
        case FieldKind::Real:           return "number";
        case FieldKind::Bool:           return "boolean";
        case FieldKind::SignedInt:
        case FieldKind::UnsignedInt:
        case FieldKind::AttributeIndex: return "integer";
        case FieldKind::CString:
        case FieldKind::UString:
        case FieldKind::Guid:           return "string";
    }
    return "";
}

// Whether the union member a field is in (FieldInfo::variant) is the one the element uses. The
// members of a union share their bytes: reading another one gives meaningless values, and writing
// one overwrites the element's real data (writing a revolved shell's field into an extruded shell
// made Archicad 29 quit). A field outside any union is always in use.
bool IsVariantActive (API_ElemTypeID typeId, const API_Element& element, const char* variant)
{
    if (variant == nullptr) {
        return true;
    }
#if defined (ServerMainVers_2900)
    // The union members and their selectors are the same in the Archicad 29 and 30 DevKits, the
    // versions ElementFieldTable has tables for; other versions have no table, so no variant.
    const GS::String v (variant);
    switch (typeId) {
        case API_RoofID:
            return (v == "u.planeRoof" && element.roof.roofClass == API_PlaneRoofID) ||
                   (v == "u.polyRoof" && element.roof.roofClass == API_PolyRoofID);
        case API_ShellID:
            return (v == "u.extrudedShell" && element.shell.shellClass == API_ExtrudedShellID) ||
                   (v == "u.revolvedShell" && element.shell.shellClass == API_RevolvedShellID) ||
                   (v == "u.ruledShell" && element.shell.shellClass == API_RuledShellID);
        case API_LabelID:
            return (v == "u.text" && element.label.labelClass == APILblClass_Text) ||
                   (v == "u.symbol" && element.label.labelClass == APILblClass_Symbol);
        case API_StairStructureID:
            return (v == "data.sideStair" && element.stairStructure.structType == APIST_Side) ||
                   (v == "data.monolith" && element.stairStructure.structType == APIST_Monolith);
        default:
            return false;
    }
#else
    (void) typeId;
    (void) element;
    return false;
#endif
}

const FieldInfo* FindField (const FieldTable& table, const GS::String& path)
{
    for (UInt32 i = 0; i < table.count; ++i) {
        if (path == table.fields[i].path) {
            return &table.fields[i];
        }
    }
    return nullptr;
}

Int64 ReadSigned (const char* p, UInt32 size)
{
    switch (size) {
        case 1: { Int8 v;  std::memcpy (&v, p, 1); return v; }
        case 2: { Int16 v; std::memcpy (&v, p, 2); return v; }
        case 4: { Int32 v; std::memcpy (&v, p, 4); return v; }
        case 8: { Int64 v; std::memcpy (&v, p, 8); return v; }
    }
    return 0;
}

UInt64 ReadUnsigned (const char* p, UInt32 size)
{
    switch (size) {
        case 1: { UInt8 v;  std::memcpy (&v, p, 1); return v; }
        case 2: { UInt16 v; std::memcpy (&v, p, 2); return v; }
        case 4: { UInt32 v; std::memcpy (&v, p, 4); return v; }
        case 8: { UInt64 v; std::memcpy (&v, p, 8); return v; }
    }
    return 0;
}

double ReadReal (const char* p, UInt32 size)
{
    if (size == sizeof (float)) {
        float v;
        std::memcpy (&v, p, sizeof (v));
        return v;
    }
    double v;
    std::memcpy (&v, p, sizeof (v));
    return v;
}

GS::UniString ReadCString (const char* p, UInt32 size)
{
    GS::Array<char> buffer;
    buffer.SetSize (size + 1);
    std::memcpy (buffer.GetContent (), p, size);
    buffer[size] = '\0';
    return GS::UniString (buffer.GetContent (), CC_UTF8);
}

GS::UniString ReadUString (const char* p, UInt32 size)
{
    const UInt32 n = size / sizeof (GS::uchar_t);
    GS::Array<GS::uchar_t> buffer;
    buffer.SetSize (n + 1);
    std::memcpy (buffer.GetContent (), p, n * sizeof (GS::uchar_t));
    buffer[n] = 0;
    return GS::UniString (buffer.GetContent ());
}

// Adds the value of a field of an element to os, under the field's path.
void AddFieldValue (GS::ObjectState& os, const FieldInfo& field, const API_Element& element)
{
    const char* p = reinterpret_cast<const char*> (&element) + field.offset;
    switch (field.kind) {
        case FieldKind::Real:           os.Add (field.path, ReadReal (p, field.size)); break;
        case FieldKind::Bool:           os.Add (field.path, *reinterpret_cast<const bool*> (p)); break;
        case FieldKind::SignedInt:      os.Add (field.path, ReadSigned (p, field.size)); break;
        case FieldKind::UnsignedInt:    os.Add (field.path, static_cast<Int64> (ReadUnsigned (p, field.size))); break;
        case FieldKind::CString:        os.Add (field.path, ReadCString (p, field.size)); break;
        case FieldKind::UString:        os.Add (field.path, ReadUString (p, field.size)); break;
        case FieldKind::AttributeIndex: os.Add (field.path, static_cast<Int64> (GetAttributeIndex (*reinterpret_cast<const API_AttributeIndex*> (p)))); break;
        case FieldKind::Guid:           os.Add (field.path, APIGuidToString (*reinterpret_cast<const API_Guid*> (p))); break;
    }
}

// The value of a field as text, for the messages that say what Archicad kept.
GS::UniString FieldValueToString (const FieldInfo& field, const API_Element& element)
{
    const char* p = reinterpret_cast<const char*> (&element) + field.offset;
    switch (field.kind) {
        case FieldKind::Real:           return GS::UniString::Printf ("%.6g", ReadReal (p, field.size));
        case FieldKind::Bool:           return *reinterpret_cast<const bool*> (p) ? "true" : "false";
        case FieldKind::SignedInt:      return GS::UniString::Printf ("%lld", static_cast<long long> (ReadSigned (p, field.size)));
        case FieldKind::UnsignedInt:    return GS::UniString::Printf ("%llu", static_cast<unsigned long long> (ReadUnsigned (p, field.size)));
        case FieldKind::CString:        return GS::UniString ("\"") + ReadCString (p, field.size) + "\"";
        case FieldKind::UString:        return GS::UniString ("\"") + ReadUString (p, field.size) + "\"";
        case FieldKind::AttributeIndex: return GS::UniString::Printf ("%d", static_cast<int> (GetAttributeIndex (*reinterpret_cast<const API_AttributeIndex*> (p))));
        case FieldKind::Guid:           return APIGuidToString (*reinterpret_cast<const API_Guid*> (p));
    }
    return GS::UniString ();
}

// Whether a field has the same value in two elements (reals within a small tolerance).
bool IsSameFieldValue (const FieldInfo& field, const API_Element& a, const API_Element& b)
{
    const char* pa = reinterpret_cast<const char*> (&a) + field.offset;
    const char* pb = reinterpret_cast<const char*> (&b) + field.offset;
    switch (field.kind) {
        case FieldKind::Real: {
            const double va = ReadReal (pa, field.size);
            const double vb = ReadReal (pb, field.size);
            return std::abs (va - vb) <= 1e-6 * std::max (1.0, std::max (std::abs (va), std::abs (vb)));
        }
        case FieldKind::Bool:
            return (*reinterpret_cast<const bool*> (pa)) == (*reinterpret_cast<const bool*> (pb));
        case FieldKind::CString:
            return ReadCString (pa, field.size) == ReadCString (pb, field.size);
        case FieldKind::UString:
            return ReadUString (pa, field.size) == ReadUString (pb, field.size);
        case FieldKind::AttributeIndex:
            return GetAttributeIndex (*reinterpret_cast<const API_AttributeIndex*> (pa)) == GetAttributeIndex (*reinterpret_cast<const API_AttributeIndex*> (pb));
        default:
            return std::memcmp (pa, pb, field.size) == 0;
    }
}

// Reads a whole number from a fields object: JSON integers may come as integers or as doubles.
bool GetWholeNumber (const GS::ObjectState& os, const GS::String& key, Int64& value)
{
    Int64 i = 0;
    if (os.Get (key, i)) {
        value = i;
        return true;
    }
    double d = 0.0;
    if (os.Get (key, d) && std::floor (d) == d && std::abs (d) < 9.0e15) {
        value = static_cast<Int64> (d);
        return true;
    }
    return false;
}

// Writes the value given for a field into element and sets the field in mask. Returns an error text, or nothing.
GS::Optional<GS::UniString> WriteFieldValue (const GS::ObjectState& fields, const GS::String& key, const FieldInfo& field, API_Element& element, API_Element& mask)
{
    char* p = reinterpret_cast<char*> (&element) + field.offset;
    const GS::UniString name (key.ToCStr ());
    switch (field.kind) {
        case FieldKind::Real: {
            double v = 0.0;
            Int64 i = 0;
            if (!fields.Get (key, v)) {
                if (!GetWholeNumber (fields, key, i)) {
                    return name + " must be a number.";
                }
                v = static_cast<double> (i);
            }
            if (!std::isfinite (v)) {
                return name + " must be a finite number.";
            }
            if (field.size == sizeof (float)) {
                const float f = static_cast<float> (v);
                std::memcpy (p, &f, sizeof (f));
            } else {
                std::memcpy (p, &v, sizeof (v));
            }
            break;
        }
        case FieldKind::Bool: {
            bool v = false;
            if (!fields.Get (key, v)) {
                return name + " must be true or false.";
            }
            *reinterpret_cast<bool*> (p) = v;
            break;
        }
        case FieldKind::SignedInt: {
            Int64 v = 0;
            if (!GetWholeNumber (fields, key, v)) {
                return name + " must be an integer.";
            }
            const Int64 maxValue = field.size >= 8 ? std::numeric_limits<Int64>::max () : (Int64 (1) << (field.size * 8 - 1)) - 1;
            const Int64 minValue = field.size >= 8 ? std::numeric_limits<Int64>::min () : -(Int64 (1) << (field.size * 8 - 1));
            if (v < minValue || v > maxValue) {
                return name + GS::UniString::Printf (" must be between %lld and %lld.", static_cast<long long> (minValue), static_cast<long long> (maxValue));
            }
            switch (field.size) {
                case 1: { const Int8 x = static_cast<Int8> (v);   std::memcpy (p, &x, 1); break; }
                case 2: { const Int16 x = static_cast<Int16> (v); std::memcpy (p, &x, 2); break; }
                case 4: { const Int32 x = static_cast<Int32> (v); std::memcpy (p, &x, 4); break; }
                default: std::memcpy (p, &v, 8); break;
            }
            break;
        }
        case FieldKind::UnsignedInt: {
            Int64 v = 0;
            if (!GetWholeNumber (fields, key, v)) {
                return name + " must be an integer.";
            }
            // -1 is accepted for the fields Archicad reads as "the last one" (e.g. topRailIndex), stored as all bits set.
            const UInt64 maxValue = field.size >= 8 ? std::numeric_limits<UInt64>::max () : (UInt64 (1) << (field.size * 8)) - 1;
            if (v < -1 || (v >= 0 && static_cast<UInt64> (v) > maxValue)) {
                return name + GS::UniString::Printf (" must be between 0 and %llu (or -1).", static_cast<unsigned long long> (maxValue));
            }
            const UInt64 u = v < 0 ? maxValue : static_cast<UInt64> (v);
            switch (field.size) {
                case 1: { const UInt8 x = static_cast<UInt8> (u);   std::memcpy (p, &x, 1); break; }
                case 2: { const UInt16 x = static_cast<UInt16> (u); std::memcpy (p, &x, 2); break; }
                case 4: { const UInt32 x = static_cast<UInt32> (u); std::memcpy (p, &x, 4); break; }
                default: std::memcpy (p, &u, 8); break;
            }
            break;
        }
        case FieldKind::CString: {
            GS::UniString v;
            if (!fields.Get (key, v)) {
                return name + " must be a string.";
            }
            const auto utf8String = v.ToCStr (0, MaxUSize, CC_UTF8);
            const char* utf8 = utf8String.Get ();
            const size_t length = std::strlen (utf8);
            if (length >= field.size) {
                return name + GS::UniString::Printf (" is too long: at most %u bytes in UTF-8.", static_cast<unsigned> (field.size - 1));
            }
            std::memset (p, 0, field.size);
            std::memcpy (p, utf8, length);
            break;
        }
        case FieldKind::UString: {
            GS::UniString v;
            if (!fields.Get (key, v)) {
                return name + " must be a string.";
            }
            const UInt32 capacity = field.size / sizeof (GS::uchar_t);
            if (v.GetLength () >= capacity) {
                return name + GS::UniString::Printf (" is too long: at most %u characters.", static_cast<unsigned> (capacity - 1));
            }
            std::memset (p, 0, field.size);
            std::memcpy (p, v.ToUStr ().Get (), v.GetLength () * sizeof (GS::uchar_t));
            break;
        }
        case FieldKind::AttributeIndex: {
            Int64 v = 0;
            if (!GetWholeNumber (fields, key, v) || v < std::numeric_limits<Int32>::min () || v > std::numeric_limits<Int32>::max ()) {
                return name + " must be an attribute index (an integer).";
            }
            *reinterpret_cast<API_AttributeIndex*> (p) = ACAPI_CreateAttributeIndex (static_cast<Int32> (v));
            break;
        }
        case FieldKind::Guid: {
            GS::UniString v;
            if (!fields.Get (key, v)) {
                return name + " must be a guid string.";
            }
            const API_Guid guid = APIGuidFromString (v.ToCStr ().Get ());
            if (guid == APINULLGuid && v != "00000000-0000-0000-0000-000000000000") {
                return name + " must be a guid string, e.g. 00000000-0000-0000-0000-000000000000.";
            }
            *reinterpret_cast<API_Guid*> (p) = guid;
            break;
        }
    }
    char* maskBytes = reinterpret_cast<char*> (&mask);
    std::memset (maskBytes + field.offset, 0xFF, field.size);
    // Archicad reads the mask of a member on its first byte only (ACAPI_ELEMENT_MASK_CHECK): the "y"
    // of begC is only taken when the first byte of begC is set too.
    for (UInt32 i = 0; i < field.ancestorCount; ++i) {
        maskBytes[field.ancestors[i]] = static_cast<char> (0xFF);
    }
    return GS::NoValue;
}

GS::Optional<API_ElemTypeID> GetElementTypeId (const API_Guid& guid, API_Element& element, GSErrCode& err)
{
    element = {};
    element.header.guid = guid;
    err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return GS::NoValue;
    }
    return GetElemTypeId (element.header);
}

}

// ---------------------------------------------------------------------------------------------
// GetElementFieldDefinitions

GetElementFieldDefinitionsCommand::GetElementFieldDefinitionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementFieldDefinitionsCommand::GetName () const
{
    return "GetElementFieldDefinitions";
}

GS::Optional<GS::UniString> GetElementFieldDefinitionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType",
                "description": "The element type whose fields are listed."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementType"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementFieldDefinitionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "structName": {
                "type": "string",
                "description": "The C++ struct of the element type in the Archicad API, e.g. API_AngleDimensionType."
            },
            "fields": {
                "type": "array",
                "description": "Every field GetElementFields reads and SetElementFields writes for this element type.",
                "items": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "The field's path in the struct, as in C++: e.g. note.noteFont, u.planeRoof.angle or defHandrailEnd[0].x."
                        },
                        "type": {
                            "type": "string",
                            "enum": [ "number", "boolean", "integer", "string" ],
                            "description": "The JSON type of the field's value."
                        },
                        "kind": {
                            "type": "string",
                            "enum": [ "Real", "Bool", "SignedInt", "UnsignedInt", "CString", "UString", "AttributeIndex", "Guid" ],
                            "description": "The C++ kind of the field: an enum is a SignedInt, an AttributeIndex is the index of an attribute (pen tables excepted, pens are plain integers)."
                        },
                        "size": {
                            "type": "integer",
                            "description": "The size of the field in bytes (for a string, the room in bytes including the closing zero)."
                        },
                        "readOnly": {
                            "type": "boolean",
                            "description": "True for the owners and counts that Archicad computes, and for the fields that choose the shape of an element (a roof's roofClass, a shell's shellClass, a label's labelClass, a stair structure's structType, a wall's type: changing one without the data of the other shape made Archicad 29 quit): SetElementFields refuses them."
                        },
                        "variant": {
                            "type": "string",
                            "description": "For a field in a union, the member it is in, e.g. u.revolvedShell or u.planeRoof. Only the member the element uses holds values (a roof's roofClass, a shell's shellClass, a label's labelClass, a stair structure's structType): GetElementFields leaves the others out and SetElementFields refuses them."
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "path", "type", "kind", "size", "readOnly" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "structName",
            "fields"
        ]
    })";
}

GS::ObjectState GetElementFieldDefinitionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    if (!IsAvailable ()) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, NotAvailableMessage);
    }
    GS::UniString typeName;
    parameters.Get ("elementType", typeName);
    const API_ElemTypeID typeId = GetElementTypeFromNonLocalizedName (typeName);
    const FieldTable* table = GetFieldTable (typeId);
    if (table == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("No fields are known for the element type ") + typeName + ".");
    }
    GS::ObjectState response;
    response.Add ("structName", table->structName);
    const auto& fields = response.AddList<GS::ObjectState> ("fields");
    for (UInt32 i = 0; i < table->count; ++i) {
        const FieldInfo& field = table->fields[i];
        GS::ObjectState definition (
            "path", field.path,
            "type", GetJsonTypeName (field.kind),
            "kind", GetKindName (field.kind),
            "size", static_cast<Int32> (field.size),
            "readOnly", field.readOnly);
        if (field.variant != nullptr) {
            definition.Add ("variant", field.variant);
        }
        fields (definition);
    }
    return response;
}

// ---------------------------------------------------------------------------------------------
// GetElementFields

GetElementFieldsCommand::GetElementFieldsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementFieldsCommand::GetName () const
{
    return "GetElementFields";
}

GS::Optional<GS::UniString> GetElementFieldsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "fields": {
                "type": "array",
                "description": "The paths of the fields to read (see GetElementFieldDefinitions), e.g. [\"note.noteFont\", \"note.frame\"]. Every field of the element is read when it is left out. A path the element type does not have is listed in the element's missingFields.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementFieldsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementFields": {
                "type": "array",
                "description": "The fields of each element, aligned with the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementType": {
                                    "$ref": "#/ElementType"
                                },
                                "structName": {
                                    "type": "string",
                                    "description": "The C++ struct of the element in the Archicad API."
                                },
                                "fields": {
                                    "type": "object",
                                    "description": "The value of each field, by its path.",
                                    "additionalProperties": {
                                        "type": [ "number", "integer", "boolean", "string" ]
                                    }
                                },
                                "missingFields": {
                                    "type": "array",
                                    "description": "The asked paths this element type does not have.",
                                    "items": {
                                        "type": "string"
                                    }
                                },
                                "inactiveFields": {
                                    "type": "array",
                                    "description": "The asked paths in a member of a union this element does not use (see variant in GetElementFieldDefinitions), e.g. u.revolvedShell fields of an extruded shell: their bytes hold another member's values, so they are not read. Without the fields input, these are simply left out.",
                                    "items": {
                                        "type": "string"
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [ "elementType", "structName", "fields" ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementFields"
        ]
    })";
}

GS::ObjectState GetElementFieldsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    if (!IsAvailable ()) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, NotAvailableMessage);
    }
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);
    GS::Array<GS::UniString> wanted;
    const bool hasWanted = parameters.Get ("fields", wanted);

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("elementFields");
    for (const GS::ObjectState& item : elements) {
        const GS::ObjectState* elementId = item.Get ("elementId");
        if (elementId == nullptr) {
            results (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }
        API_Element element = {};
        GSErrCode err = NoError;
        const GS::Optional<API_ElemTypeID> typeId = GetElementTypeId (GetGuidFromObjectState (*elementId), element, err);
        if (!typeId.HasValue ()) {
            results (CreateErrorResponse (err, "Failed to get the element."));
            continue;
        }
        const FieldTable* table = GetFieldTable (*typeId);
        if (table == nullptr) {
            results (CreateErrorResponse (APIERR_BADPARS, GS::UniString ("No fields are known for elements of type ") + GetElementTypeNonLocalizedName (*typeId) + "."));
            continue;
        }
        GS::ObjectState result;
        result.Add ("elementType", GetElementTypeNonLocalizedName (*typeId));
        result.Add ("structName", table->structName);
        GS::ObjectState values;
        GS::Array<GS::UniString> missing;
        GS::Array<GS::UniString> inactive;
        if (hasWanted) {
            for (const GS::UniString& path : wanted) {
                const FieldInfo* field = FindField (*table, GS::String (path.ToCStr ().Get ()));
                if (field == nullptr) {
                    missing.Push (path);
                } else if (!IsVariantActive (*typeId, element, field->variant)) {
                    inactive.Push (path);
                } else {
                    AddFieldValue (values, *field, element);
                }
            }
        } else {
            for (UInt32 i = 0; i < table->count; ++i) {
                if (IsVariantActive (*typeId, element, table->fields[i].variant)) {
                    AddFieldValue (values, table->fields[i], element);
                }
            }
        }
        result.Add ("fields", values);
        if (!missing.IsEmpty ()) {
            result.Add ("missingFields", missing);
        }
        if (!inactive.IsEmpty ()) {
            result.Add ("inactiveFields", inactive);
        }
        results (result);
    }
    return response;
}

// ---------------------------------------------------------------------------------------------
// SetElementFields

SetElementFieldsCommand::SetElementFieldsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetElementFieldsCommand::GetName () const
{
    return "SetElementFields";
}

GS::Optional<GS::UniString> SetElementFieldsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithFields": {
                "type": "array",
                "description": "The elements to change and the fields to write to each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "fields": {
                            "type": "object",
                            "description": "The new value of each field, by its path (see GetElementFieldDefinitions), e.g. {\"note.noteFont\": 3, \"note.frame\": true}. Only these fields change: they are written with ACAPI_Element_Change and a mask of exactly their bytes, then read back.",
                            "additionalProperties": {
                                "type": [ "number", "integer", "boolean", "string" ]
                            },
                            "minProperties": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "fields"
                    ]
                }
            },
            "dryRun": {
                "$ref": "#/DryRun"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithFields"
        ]
    })";
}

GS::Optional<GS::UniString> SetElementFieldsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per element, aligned with the input. An element whose fields Archicad did not all keep fails with the fields and the values Archicad kept; the other fields of that element are changed. On a dry run, a successful result has wouldChange with the oldValue and newValue of the given fields."
            },
            "dryRun": {
                "$ref": "#/DryRunFlag"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetElementFieldsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    if (!IsAvailable ()) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, NotAvailableMessage);
    }
    const bool dryRun = IsDryRun (parameters);
    GS::Array<GS::ObjectState> items;
    parameters.Get ("elementsWithFields", items);

    // Everything is checked and prepared before the undoable command.
    struct Prepared {
        bool                            ok = false;
        GS::ObjectState                 result;
        API_Element                     wanted = {};
        API_Element                     mask = {};
        GS::Array<const FieldInfo*>     fields;
    };
    GS::Array<Prepared> prepared;
    for (const GS::ObjectState& item : items) {
        Prepared p;
        const GS::ObjectState* elementId = item.Get ("elementId");
        GS::ObjectState fields;
        if (elementId == nullptr || !item.Get ("fields", fields)) {
            p.result = CreateFailedExecutionResult (APIERR_BADPARS, "elementId and fields are needed.");
            prepared.Push (p);
            continue;
        }
        GSErrCode err = NoError;
        const GS::Optional<API_ElemTypeID> typeId = GetElementTypeId (GetGuidFromObjectState (*elementId), p.wanted, err);
        if (!typeId.HasValue ()) {
            p.result = CreateFailedExecutionResult (err, "Failed to get the element.");
            prepared.Push (p);
            continue;
        }
        const FieldTable* table = GetFieldTable (*typeId);
        if (table == nullptr) {
            p.result = CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("No fields are known for elements of type ") + GetElementTypeNonLocalizedName (*typeId) + ".");
            prepared.Push (p);
            continue;
        }
        ACAPI_ELEMENT_MASK_CLEAR (p.mask);
        GS::UniString error;
        const API_Element before = p.wanted;
        GS::ObjectState oldValues;
        GS::ObjectState newValues;
        fields.EnumerateFields ([&] (const GS::String& key) {
            if (!error.IsEmpty ()) {
                return;
            }
            const FieldInfo* field = FindField (*table, key);
            if (field == nullptr) {
                error = GS::UniString ("The element type ") + GetElementTypeNonLocalizedName (*typeId) + " has no field " + GS::UniString (key.ToCStr ()) + " (see GetElementFieldDefinitions).";
                return;
            }
            if (field->readOnly) {
                error = GS::UniString (key.ToCStr ()) + " is read-only: Archicad computes it, or it chooses the shape of the element (see GetElementFieldDefinitions).";
                return;
            }
            GS::Optional<GS::UniString> writeError = WriteFieldValue (fields, key, *field, p.wanted, p.mask);
            if (writeError.HasValue ()) {
                error = writeError.Get ();
                return;
            }
            p.fields.Push (field);
            AddFieldValue (oldValues, *field, before);
            AddFieldValue (newValues, *field, p.wanted);
        });
        // A field of a union member the element does not use (after the given values, which may change
        // the roofClass, shellClass, labelClass or structType) would overwrite the member it uses.
        for (const FieldInfo* field : p.fields) {
            if (error.IsEmpty () && !IsVariantActive (*typeId, p.wanted, field->variant)) {
                error = GS::UniString (field->path) + " is in " + field->variant + ", which this element does not use: writing it would overwrite the data of the member it uses.";
            }
        }
        if (!error.IsEmpty ()) {
            p.result = CreateFailedExecutionResult (APIERR_BADPARS, error);
            prepared.Push (p);
            continue;
        }
        p.ok = true;
        if (dryRun) {
            p.result = CreateWouldChangeExecutionResult (GS::ObjectState ("oldValue", oldValues, "newValue", newValues));
        }
        prepared.Push (p);
    }

    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (items);
    const GSErrCode err = CallUndoableCommandUnlessDryRun (dryRun, "SetElementFields", [&] () -> GSErrCode {
        for (Prepared& p : prepared) {
            if (!p.ok) {
                continue;
            }
            const GSErrCode changeErr = ACAPI_Element_Change (&p.wanted, &p.mask, nullptr, 0, true);
            if (changeErr != NoError) {
                p.ok = false;
                const GS::UniString refusal = DescribeElementEditRefusal (changeErr, p.wanted.header);
                GS::UniString message = GS::UniString ("Archicad refused the change") + (refusal.IsEmpty () ? GS::UniString (".") : GS::UniString (": ") + refusal + ".");
                const API_ElemTypeID typeId = GetElemTypeId (p.wanted.header);
                if (changeErr == APIERR_BADPARS && (typeId == API_RailingPostID || typeId == API_RailingInnerPostID || typeId == API_RailingPanelID ||
                                                    typeId == API_RailingBalusterSetID || typeId == API_RailingBalusterID)) {
                    // Confirmed on Archicad 29: these components are generated from the post model of their
                    // node (defPost) and from the pattern of their segment (defPattern).
                    message += " Archicad generates this railing component from the post model of its node or the pattern of its segment, and does not take a change to it.";
                }
                p.result = CreateFailedExecutionResult (changeErr, message);
            }
        }
        return NoError;
    });

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("executionResults");
    for (UIndex i = 0; i < prepared.GetSize (); ++i) {
        Prepared& p = prepared[i];
        if (p.ok && !dryRun) {
            if (err != NoError) {
                p.result = CreateFailedExecutionResult (err, "Failed to run the change as an undoable command.");
            } else {
                // Read back after the undoable command, as some values are only updated once it is over.
                API_Element got = {};
                got.header.guid = p.wanted.header.guid;
                if (ACAPI_Element_Get (&got) != NoError) {
                    p.result = CreateFailedExecutionResult (APIERR_GENERAL, "The element was changed but could not be read back.");
                } else {
                    GS::UniString notApplied;
                    for (const FieldInfo* field : p.fields) {
                        if (!IsSameFieldValue (*field, p.wanted, got)) {
                            notApplied += GS::UniString (" ") + field->path + " is " + FieldValueToString (*field, got) + ".";
                        }
                    }
                    p.result = notApplied.IsEmpty () ? CreateSuccessfulExecutionResult () :
                        CreateFailedExecutionResult (APIERR_REFUSEDPAR, GS::UniString ("Archicad did not keep every value:") + notApplied + " The other given fields were changed.");
                    changeReporter.MarkChanged (p.wanted.header.guid);
                }
            }
        }
        results (p.result);
    }
    if (!dryRun) {
        changeReporter.Report ();
    }
    AddDryRunField (response, dryRun);
    return response;
}
