#include "ElementFieldCommands.hpp"
#include "ElementFieldTable.hpp"
#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <string>
#include <vector>

using namespace ElementFields;

namespace {

const char* NotAvailableMessage = "The element field table is not available in this Archicad version (it is generated from the Archicad 29 and 30 DevKits).";

const char* GetKindName (FieldKind kind)
{
    switch (kind) {
        case FieldKind::Real:                   return "Real";
        case FieldKind::Bool:                   return "Bool";
        case FieldKind::SignedInt:              return "SignedInt";
        case FieldKind::UnsignedInt:            return "UnsignedInt";
        case FieldKind::CString:                return "CString";
        case FieldKind::UString:                return "UString";
        case FieldKind::AttributeIndex:         return "AttributeIndex";
        case FieldKind::Guid:                   return "Guid";
        case FieldKind::OverriddenPen:          return "OverriddenPen";
        case FieldKind::OverriddenAttribute:    return "OverriddenAttribute";
        case FieldKind::UniStringPointer:       return "UniStringPointer";
    }
    return "";
}

// The JSON type of a field's value.
const char* GetJsonTypeName (FieldKind kind)
{
    switch (kind) {
        case FieldKind::Real:                   return "number";
        case FieldKind::Bool:                   return "boolean";
        case FieldKind::SignedInt:
        case FieldKind::UnsignedInt:
        case FieldKind::AttributeIndex:         return "integer";
        case FieldKind::CString:
        case FieldKind::UString:
        case FieldKind::Guid:
        case FieldKind::UniStringPointer:       return "string";
        case FieldKind::OverriddenPen:
        case FieldKind::OverriddenAttribute:    return "object";
    }
    return "";
}

// The strings the GS::UniString* fields of elements (FieldKind::UniStringPointer, e.g.
// note.contentUStr) point to. Archicad fills such a string in ACAPI_Element_Get and reads it in
// ACAPI_Element_Change only through the pointer the caller sets, as the DevKit's Element_Test does
// with the text of dimension notes, and never owns it. The store owns the strings, at addresses that
// stay valid as long as it lives (std::deque never moves its items when one is added at its end), so
// a store must outlive every element that points to its strings.
class UniStringStore
{
public:
    GS::UniString* Add (const GS::UniString& value)
    {
        strings.push_back (value);
        return &strings.back ();
    }

private:
    std::deque<GS::UniString> strings;
};

const GS::UniString* GetUniStringPointer (const char* p)
{
    const GS::UniString* s = nullptr;
    std::memcpy (&s, p, sizeof (s));
    return s;
}

void SetUniStringPointer (char* p, const GS::UniString* s)
{
    std::memcpy (p, &s, sizeof (s));
}

// The text a UniStringPointer field points to, empty for nullptr. The pointer is always one the
// commands set to a string of a UniStringStore, or nullptr (see ReadUniStrings).
GS::UniString ReadUniStringPointer (const char* p)
{
    const GS::UniString* s = GetUniStringPointer (p);
    return s != nullptr ? *s : GS::UniString ();
}

// The members of an OverriddenPen or OverriddenAttribute field: an APIOptional of the API (from the
// Archicad 27 API; the tables are only generated for Archicad 29 and 30).
struct OptionalValue
{
    bool    hasValue = false;
    Int64   value = 0;
};

OptionalValue ReadOptionalValue (const char* p, FieldKind kind)
{
    OptionalValue result;
#if defined (ServerMainVers_2900)
    if (kind == FieldKind::OverriddenPen) {
        API_OverriddenPen pen = {};
        std::memcpy (&pen, p, sizeof (pen));
        result.hasValue = pen.hasValue;
        result.value = pen.value;
    } else {
        API_OverriddenAttribute attribute = {};
        std::memcpy (&attribute, p, sizeof (attribute));
        result.hasValue = attribute.hasValue;
        result.value = GetAttributeIndex (attribute.value);
    }
#else
    (void) p;
    (void) kind;
#endif
    return result;
}

void WriteOptionalValue (char* p, FieldKind kind, const OptionalValue& value)
{
#if defined (ServerMainVers_2900)
    if (kind == FieldKind::OverriddenPen) {
        API_OverriddenPen pen = {};
        std::memcpy (&pen, p, sizeof (pen));
        pen.hasValue = value.hasValue;
        pen.value = static_cast<API_PenIndex> (value.value);
        std::memcpy (p, &pen, sizeof (pen));
    } else {
        API_OverriddenAttribute attribute = {};
        std::memcpy (&attribute, p, sizeof (attribute));
        attribute.hasValue = value.hasValue;
        attribute.value = ACAPI_CreateAttributeIndex (static_cast<Int32> (value.value));
        std::memcpy (p, &attribute, sizeof (attribute));
    }
#else
    (void) p;
    (void) kind;
    (void) value;
#endif
}

GS::ObjectState OptionalValueToObjectState (const OptionalValue& value)
{
    return GS::ObjectState ("hasValue", value.hasValue, "value", value.value);
}

#if defined (ServerMainVers_2900)
// "structure[1][2].data.monolith" (a union member of a stair's default structures) -> role 1, side 2
// and "data.monolith"; nullptr for another text.
const char* ParseStairStructureVariant (const char* variant, UInt32& role, UInt32& side)
{
    const char* prefix = "structure[";
    const size_t prefixLength = std::strlen (prefix);
    if (std::strncmp (variant, prefix, prefixLength) != 0) {
        return nullptr;
    }
    const char* c = variant + prefixLength;
    const auto parseIndex = [&c] (UInt32& index) -> bool {
        if (*c < '0' || *c > '9') {
            return false;
        }
        index = 0;
        while (*c >= '0' && *c <= '9') {
            index = index * 10 + static_cast<UInt32> (*c - '0');
            ++c;
        }
        return true;
    };
    if (!parseIndex (role) || std::strncmp (c, "][", 2) != 0) {
        return nullptr;
    }
    c += 2;
    if (!parseIndex (side) || std::strncmp (c, "].", 2) != 0) {
        return nullptr;
    }
    return c + 2;
}
#endif

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
        case API_StairID: {
            // The default structures of a stair, structure[role][side], each with its own structType.
            UInt32 role = 0;
            UInt32 side = 0;
            const char* member = ParseStairStructureVariant (variant, role, side);
            if (member == nullptr || role >= static_cast<UInt32> (API_StairPartRoleNum) || side >= static_cast<UInt32> (API_StairStructureSideNum)) {
                return false;
            }
            const API_StairStructureTypeID structType = element.stair.structure[role][side].structType;
            return (std::strcmp (member, "data.sideStair") == 0 && structType == APIST_Side) ||
                   (std::strcmp (member, "data.monolith") == 0 && structType == APIST_Monolith);
        }
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
        case FieldKind::OverriddenPen:
        case FieldKind::OverriddenAttribute: os.Add (field.path, OptionalValueToObjectState (ReadOptionalValue (p, field.kind))); break;
        case FieldKind::UniStringPointer: os.Add (field.path, ReadUniStringPointer (p)); break;
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
        case FieldKind::OverriddenPen:
        case FieldKind::OverriddenAttribute: {
            const OptionalValue value = ReadOptionalValue (p, field.kind);
            return GS::UniString ("{\"hasValue\": ") + (value.hasValue ? "true" : "false") + GS::UniString::Printf (", \"value\": %lld}", static_cast<long long> (value.value));
        }
        case FieldKind::UniStringPointer: return GS::UniString ("\"") + ReadUniStringPointer (p) + "\"";
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
        case FieldKind::OverriddenPen:
        case FieldKind::OverriddenAttribute: {
            // The value means nothing while hasValue is false.
            const OptionalValue va = ReadOptionalValue (pa, field.kind);
            const OptionalValue vb = ReadOptionalValue (pb, field.kind);
            return va.hasValue == vb.hasValue && (!va.hasValue || va.value == vb.value);
        }
        case FieldKind::UniStringPointer:
            return ReadUniStringPointer (pa) == ReadUniStringPointer (pb);
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

// Writes the value given for a field into element and sets the field in mask. A UniStringPointer field
// is pointed to a new string of store, which keeps the text until Archicad has read it; the element it
// pointed to before (a copy of element made before) keeps its own string. Returns an error text, or nothing.
GS::Optional<GS::UniString> WriteFieldValue (const GS::ObjectState& fields, const GS::String& key, const FieldInfo& field, API_Element& element, API_Element& mask,
                                             UniStringStore& store)
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
        case FieldKind::OverriddenPen:
        case FieldKind::OverriddenAttribute: {
            const bool isPen = field.kind == FieldKind::OverriddenPen;
            const GS::UniString shape = name + " must be an object with hasValue (true or false) and value (" + (isPen ? "a pen index" : "an attribute index") +
                                        "), e.g. {\"hasValue\": true, \"value\": 3}; a member left out keeps its value.";
            GS::ObjectState given;
            if (!fields.Get (key, given)) {
                return shape;
            }
            bool unknownMember = false;
            given.EnumerateFields ([&] (const GS::String& member) {
                if (!(member == "hasValue") && !(member == "value")) {
                    unknownMember = true;
                }
            });
            const bool givesHasValue = given.Contains ("hasValue");
            const bool givesValue = given.Contains ("value");
            if (unknownMember || (!givesHasValue && !givesValue)) {
                return shape;
            }
            OptionalValue value = ReadOptionalValue (p, field.kind);
            if (givesHasValue && !given.Get ("hasValue", value.hasValue)) {
                return shape;
            }
            if (givesValue) {
                Int64 v = 0;
                if (!GetWholeNumber (given, "value", v)) {
                    return shape;
                }
                const Int64 minValue = isPen ? std::numeric_limits<Int16>::min () : std::numeric_limits<Int32>::min ();
                const Int64 maxValue = isPen ? std::numeric_limits<Int16>::max () : std::numeric_limits<Int32>::max ();
                if (v < minValue || v > maxValue) {
                    return name + GS::UniString::Printf (".value must be between %lld and %lld.", static_cast<long long> (minValue), static_cast<long long> (maxValue));
                }
                value.value = v;
                // As assigning a value to an APIOptional in C++: a value given alone is used.
                if (!givesHasValue) {
                    value.hasValue = true;
                }
            }
            WriteOptionalValue (p, field.kind, value);
            break;
        }
        case FieldKind::UniStringPointer: {
            GS::UniString v;
            if (!fields.Get (key, v)) {
                return name + " must be a string.";
            }
            SetUniStringPointer (p, store.Add (v));
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

bool HasUniStringPointers (const FieldTable& table)
{
    for (UInt32 i = 0; i < table.count; ++i) {
        if (table.fields[i].kind == FieldKind::UniStringPointer) {
            return true;
        }
    }
    return false;
}

// Points the GS::UniString* fields of element (read before, with them nullptr) to the texts Archicad
// gives through them: a second read, with read (ACAPI_Element_Get or ACAPI_Element_GetDefaults), of
// a copy of request (the element's guid or the defaults' type) whose GS::UniString* fields point to
// new strings of store, as the DevKit's Element_Test reads the text of a dimension note. Every other
// field of element stays as the first read gave it. The generator only makes these fields outside
// unions, so setting them overwrites no other member. A pointer Archicad did not keep is left
// nullptr in element: the add-on never reads or frees memory it did not allocate.
template <typename ReadFunction>
GSErrCode ReadUniStrings (const FieldTable& table, const API_Element& request, API_Element& element, UniStringStore& store, const ReadFunction& read)
{
    API_Element withStrings = request;
    char* bytes = reinterpret_cast<char*> (&withStrings);
    GS::Array<const GS::UniString*> ownStrings;
    for (UInt32 i = 0; i < table.count; ++i) {
        if (table.fields[i].kind == FieldKind::UniStringPointer) {
            GS::UniString* s = store.Add (GS::UniString ());
            SetUniStringPointer (bytes + table.fields[i].offset, s);
            ownStrings.Push (s);
        }
    }
    const GSErrCode err = read (withStrings);
    UIndex k = 0;
    for (UInt32 i = 0; i < table.count; ++i) {
        if (table.fields[i].kind == FieldKind::UniStringPointer) {
            const bool kept = err == NoError && GetUniStringPointer (bytes + table.fields[i].offset) == ownStrings[k];
            SetUniStringPointer (reinterpret_cast<char*> (&element) + table.fields[i].offset, kept ? ownStrings[k] : nullptr);
            ++k;
        }
    }
    return err;
}

// Sets to nullptr the GS::UniString* fields of element that were not written, as they are when an
// element is read without them: the DevKit's Element_Test changes the text of a note through
// contentUStr with only note.content in the mask, so a pointer left to the text that was read could
// override a change of note.content.
void ClearUnwrittenUniStrings (const FieldTable& table, const GS::Array<const FieldInfo*>& written, API_Element& element)
{
    for (UInt32 i = 0; i < table.count; ++i) {
        const FieldInfo& field = table.fields[i];
        if (field.kind != FieldKind::UniStringPointer) {
            continue;
        }
        bool isWritten = false;
        for (const FieldInfo* w : written) {
            isWritten = isWritten || w == &field;
        }
        if (!isWritten) {
            SetUniStringPointer (reinterpret_cast<char*> (&element) + field.offset, nullptr);
        }
    }
}

// Reads an element. When its type has GS::UniString* fields (the text of dimension notes), they are
// read a second time, pointing to strings of store (see ReadUniStrings): the type, and so the fields,
// is only known after the first read.
GS::Optional<API_ElemTypeID> GetElementTypeId (const API_Guid& guid, API_Element& element, UniStringStore& store, GSErrCode& err)
{
    element = {};
    element.header.guid = guid;
    err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return GS::NoValue;
    }
    const API_ElemTypeID typeId = GetElemTypeId (element.header);
    const FieldTable* table = GetFieldTable (typeId);
    if (table != nullptr && HasUniStringPointers (*table)) {
        API_Element request = {};
        request.header.guid = guid;
        err = ReadUniStrings (*table, request, element, store, [] (API_Element& e) { return ACAPI_Element_Get (&e); });
        if (err != NoError) {
            return GS::NoValue;
        }
    }
    return typeId;
}

// The components of a railing, whose tool defaults Archicad 29 and 30 do not change.
bool IsRailingComponentType (API_ElemTypeID typeId)
{
    switch (typeId) {
        case API_RailingToprailID:
        case API_RailingHandrailID:
        case API_RailingRailID:
        case API_RailingPostID:
        case API_RailingInnerPostID:
        case API_RailingBalusterID:
        case API_RailingPanelID:
        case API_RailingSegmentID:
        case API_RailingNodeID:
        case API_RailingBalusterSetID:
        case API_RailingPatternID:
        case API_RailingToprailEndID:
        case API_RailingHandrailEndID:
        case API_RailingRailEndID:
        case API_RailingToprailConnectionID:
        case API_RailingHandrailConnectionID:
        case API_RailingRailConnectionID:
        case API_RailingEndFinishID:
            return true;
        default:
            return false;
    }
}

// Reads the tool defaults of an element type (its generic variation), without the memo, and the
// texts of its GS::UniString* fields into strings of store (see ReadUniStrings).
GSErrCode GetDefaultsOfType (API_ElemTypeID typeId, API_Element& defaults, UniStringStore& store)
{
    defaults = {};
#ifdef ServerMainVers_2600
    defaults.header.type = typeId;
#else
    defaults.header.typeID = typeId;
#endif
    const API_Element request = defaults;
    const GSErrCode err = ACAPI_Element_GetDefaults (&defaults, nullptr);
    const FieldTable* table = GetFieldTable (typeId);
    if (err != NoError || table == nullptr || !HasUniStringPointers (*table)) {
        return err;
    }
    return ReadUniStrings (*table, request, defaults, store, [] (API_Element& e) { return ACAPI_Element_GetDefaults (&e, nullptr); });
}

// Adds to result the fields of element (every field, or only the wanted paths), with the asked paths
// the element type does not have (missingFields) and those in a union member it does not use
// (inactiveFields).
void AddFieldsOf (GS::ObjectState& result, const FieldTable& table, API_ElemTypeID typeId, const API_Element& element, const GS::Array<GS::UniString>* wanted)
{
    GS::ObjectState values;
    GS::Array<GS::UniString> missing;
    GS::Array<GS::UniString> inactive;
    if (wanted != nullptr) {
        for (const GS::UniString& path : *wanted) {
            const FieldInfo* field = FindField (table, GS::String (path.ToCStr ().Get ()));
            if (field == nullptr) {
                missing.Push (path);
            } else if (!IsVariantActive (typeId, element, field->variant)) {
                inactive.Push (path);
            } else {
                AddFieldValue (values, *field, element);
            }
        }
    } else {
        for (UInt32 i = 0; i < table.count; ++i) {
            if (IsVariantActive (typeId, element, table.fields[i].variant)) {
                AddFieldValue (values, table.fields[i], element);
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
}

const char* GetReadOnlyReason (API_ElemTypeID typeId, const FieldInfo& field, bool ofElement);

// Writes the given values into wanted and sets their bytes in mask; lists the fields written in
// written, with their values before and after in oldValues and newValues. Returns an error text when
// a path is unknown or read-only, a value does not fit its field, or a field is in a union member
// the element does not use; nothing otherwise. ofElement is false for the tool defaults of a type.
GS::UniString WriteFieldValues (const GS::ObjectState& fields, const FieldTable& table, API_ElemTypeID typeId, API_Element& wanted, API_Element& mask,
                                GS::Array<const FieldInfo*>& written, GS::ObjectState& oldValues, GS::ObjectState& newValues, UniStringStore& store,
                                bool ofElement)
{
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    GS::UniString error;
    const API_Element before = wanted;
    fields.EnumerateFields ([&] (const GS::String& key) {
        if (!error.IsEmpty ()) {
            return;
        }
        const FieldInfo* field = FindField (table, key);
        if (field == nullptr) {
            error = GS::UniString ("The element type ") + GetElementTypeNonLocalizedName (typeId) + " has no field " + GS::UniString (key.ToCStr ()) + " (see GetElementFieldDefinitions).";
            return;
        }
        const char* readOnlyReason = GetReadOnlyReason (typeId, *field, ofElement);
        if (readOnlyReason != nullptr) {
            error = GS::UniString (key.ToCStr ()) + " is read-only: " + readOnlyReason + " (see GetElementFieldDefinitions).";
            return;
        }
        GS::Optional<GS::UniString> writeError = WriteFieldValue (fields, key, *field, wanted, mask, store);
        if (writeError.HasValue ()) {
            error = writeError.Get ();
            return;
        }
        written.Push (field);
        AddFieldValue (oldValues, *field, before);
        AddFieldValue (newValues, *field, wanted);
    });
    // A field of a union member the element does not use (after the given values, which may change
    // the roofClass, shellClass, labelClass or structType) would overwrite the member it uses.
    for (const FieldInfo* field : written) {
        if (error.IsEmpty () && !IsVariantActive (typeId, wanted, field->variant)) {
            error = GS::UniString (field->path) + " is in " + field->variant + ", which this element does not use: writing it would overwrite the data of the member it uses.";
        }
    }
    return error;
}

// The written fields whose value in got is not the one in wanted, as " path is value." texts;
// empty when Archicad kept every value.
GS::UniString DescribeNotKept (const GS::Array<const FieldInfo*>& written, const API_Element& wanted, const API_Element& got)
{
    GS::UniString notKept;
    for (const FieldInfo* field : written) {
        if (!IsSameFieldValue (*field, wanted, got)) {
            notKept += GS::UniString (" ") + field->path + " is " + FieldValueToString (*field, got) + ".";
        }
    }
    return notKept;
}

const char* TableReadOnlyReason = "Archicad computes it, or it chooses the shape of the element";

const char* LibraryPartReadOnlyReason = "it is the library part of the element, which Archicad only changes together with the parameters of the new library part, and SetElementFields writes no parameters: use ChangeLibraryPartOfElements";

const char* NoteLeaderLineReadOnlyReason = "Archicad gives a leader line only to the notes of angle dimensions, floor plan dimensions and hatches (API_NoteType), and refuses the leader line fields of the notes of a level dimension";

bool EndsWith (const std::string& text, const char* suffix)
{
    const size_t n = std::strlen (suffix);
    return text.size () >= n && text.compare (text.size () - n, n, suffix) == 0;
}

bool BeginsWith (const std::string& text, const char* prefix)
{
    return text.compare (0, std::strlen (prefix), prefix) == 0;
}

// Whether path is a leader line field of the note at prefix (e.g. "note1."): useLeaderLine,
// anchorAtTextEnd and the coordinates of the leader line.
bool IsNoteLeaderLineField (const std::string& path, const char* prefix)
{
    for (const char* leaf : { "useLeaderLine", "anchorAtTextEnd", "begC.x", "begC.y", "midC.x", "midC.y", "endC.x", "endC.y" }) {
        if (path == std::string (prefix) + leaf) {
            return true;
        }
    }
    return false;
}

// Why a field of an element type cannot be written, or nullptr when it can. Besides the fields the
// generated table marks read-only (owners, counts and the fields that choose the shape of an
// element), an element (ofElement; not the tool defaults, which were not checked) has fields Archicad
// refuses whatever their value, even the value GetElementFields just read (#3054, Archicad 30): the
// library part of the elements ChangeLibraryPartOfElements changes, and the leader lines of the notes
// of a level dimension.
const char* GetReadOnlyReason (API_ElemTypeID typeId, const FieldInfo& field, bool ofElement)
{
    if (field.readOnly) {
        return TableReadOnlyReason;
    }
    if (!ofElement) {
        return nullptr;
    }
    const std::string path (field.path);
    switch (typeId) {
        case API_ObjectID:
        case API_LampID:
        case API_ZoneID:
            return path == "libInd" ? LibraryPartReadOnlyReason : nullptr;
        case API_WindowID:
        case API_DoorID:
        case API_SkylightID:
            return path == "openingBase.libInd" ? LibraryPartReadOnlyReason : nullptr;
        case API_LabelID:
            return path == "u.symbol.libInd" ? LibraryPartReadOnlyReason : nullptr;
        case API_LevelDimensionID:
            return IsNoteLeaderLineField (path, "note1.") || IsNoteLeaderLineField (path, "note2.") ? NoteLeaderLineReadOnlyReason : nullptr;
        default:
            return nullptr;
    }
}

// The railing components Archicad generates from the post model of their node (defPost) or the
// pattern of their segment (defPattern), and so does not take a change to.
bool IsGeneratedRailingComponentType (API_ElemTypeID typeId)
{
    switch (typeId) {
        case API_RailingPostID:
        case API_RailingInnerPostID:
        case API_RailingPanelID:
        case API_RailingBalusterSetID:
        case API_RailingBalusterID:
        case API_RailingPatternID:
            return true;
        default:
            return false;
    }
}

bool ReadBoolField (const FieldTable& table, const char* path, const API_Element& element, bool& value)
{
    const FieldInfo* field = FindField (table, GS::String (path));
    if (field == nullptr || field->kind != FieldKind::Bool) {
        return false;
    }
    value = *(reinterpret_cast<const char*> (&element) + field->offset) != 0;
    return true;
}

// The isAutoOnStoryVisibility field that decides a story visibility field, e.g. "showContour.showOnHome"
// -> "isAutoOnStoryVisibility", "floorPlanLayout.visibility.showRelAbove" -> "floorPlanLayout.isAutoOnStoryVisibility",
// and an isAutoOnStoryVisibility field itself; empty for the other fields.
std::string GetAutoStoryVisibilityPath (const std::string& path)
{
    if (path == "isAutoOnStoryVisibility" || EndsWith (path, ".isAutoOnStoryVisibility")) {
        return path;
    }
    bool isVisibilityLeaf = false;
    for (const char* leaf : { ".showOnHome", ".showAllAbove", ".showAllBelow", ".showRelAbove", ".showRelBelow" }) {
        isVisibilityLeaf = isVisibilityLeaf || EndsWith (path, leaf);
    }
    if (!isVisibilityLeaf) {
        return std::string ();
    }
    // Drop the leaf and the story visibility member: what is left is the struct both are in.
    const std::string member = path.substr (0, path.rfind ('.'));
    const size_t dot = member.rfind ('.');
    const std::string prefix = dot == std::string::npos ? std::string () : member.substr (0, dot + 1);
    return prefix + "isAutoOnStoryVisibility";
}

// The conditions known to make Archicad refuse, or not keep, the given fields, as sentences to add to
// the message; empty when none applies. written is the element as it was given to Archicad, state the
// element as Archicad kept it (written again when Archicad refused the change).
GS::UniString DescribeConditions (API_ElemTypeID typeId, const FieldTable& table, const GS::Array<const FieldInfo*>& fields, const API_Element& written, const API_Element& state)
{
    const API_Element& element = written;
    GS::UniString conditions;
    if (IsGeneratedRailingComponentType (typeId)) {
        // Confirmed on Archicad 29 for posts, inner posts, panels, baluster sets and balusters (a
        // refused change), and in #3054 on Archicad 30 for panels and patterns (values not kept).
        conditions += " Archicad generates this railing component from the post model of its node (defPost of the Railing Node) or the pattern of its segment (defPattern of the Railing Segment), and does not take a change to it: change those instead (ModifyRailings).";
    }

    // While the automatic story visibility is on, Archicad recomputes the visibility from the
    // element's vertical extent and drops the values given for it (as ModifyRailings, ModifySlabs and
    // the other Modify commands of elements with a story visibility find).
    std::vector<std::string> autoPaths;
    bool lineArrowsGiven = false;
    for (const FieldInfo* field : fields) {
        const std::string path (field->path);
        const std::string autoPath = GetAutoStoryVisibilityPath (path);
        if (!autoPath.empty () && std::find (autoPaths.begin (), autoPaths.end (), autoPath) == autoPaths.end ()) {
            autoPaths.push_back (autoPath);
        }

        const char* p = reinterpret_cast<const char*> (&element) + field->offset;
        if (field->kind == FieldKind::Real && (EndsWith (path, "ArrowData.arrowSize") || path == "arrowData.arrowSize") && ReadReal (p, field->size) <= 0.0) {
            conditions += GS::UniString (" ") + field->path + " is " + FieldValueToString (*field, element) + ": Archicad takes an arrowhead size greater than 0 (an end without an arrowhead may read 0), so leave it out or give a size.";
        }
        if (field->kind == FieldKind::SignedInt && (EndsWith (path, "ArrowData.arrowPen") || path == "arrowData.arrowPen")) {
            const Int64 pen = ReadSigned (p, field->size);
            if (pen < 1 || pen > 255) {
                conditions += GS::UniString (" ") + field->path + " is " + FieldValueToString (*field, element) + ": Archicad takes an arrowhead pen from 1 to 255, so leave it out or give a pen.";
            }
        }
        lineArrowsGiven = lineArrowsGiven || BeginsWith (path, "startArrowData.arrow") || BeginsWith (path, "endArrowData.arrow") || path == "linkedArrowAttributes";
    }
    for (const std::string& autoPath : autoPaths) {
        bool isAuto = false;
        if (!ReadBoolField (table, autoPath.c_str (), state, isAuto) || !isAuto) {
            continue;
        }
        conditions += GS::UniString (" ") + autoPath.c_str () + " is true: Archicad recomputes the story visibility from the element's vertical extent and drops the values given for it";
        if (typeId == API_RailingID) {
            // ModifyRailings, confirmed on Archicad 29.
            conditions += ", and does not turn the automatic story visibility of a railing off through the API.";
        } else {
            conditions += GS::UniString (", so give ") + autoPath.c_str () + " false in the same call.";
        }
    }

    // Linked arrowheads share one size and pen (linkedArrowAttributes of lines, arcs, polylines and
    // splines from Archicad 29).
    bool linked = false;
    if (lineArrowsGiven && ReadBoolField (table, "linkedArrowAttributes", element, linked) && linked) {
        const FieldInfo* startSize = FindField (table, GS::String ("startArrowData.arrowSize"));
        const FieldInfo* endSize = FindField (table, GS::String ("endArrowData.arrowSize"));
        const FieldInfo* startPen = FindField (table, GS::String ("startArrowData.arrowPen"));
        const FieldInfo* endPen = FindField (table, GS::String ("endArrowData.arrowPen"));
        const char* bytes = reinterpret_cast<const char*> (&element);
        const bool sizesDiffer = startSize != nullptr && endSize != nullptr && startSize->kind == FieldKind::Real && endSize->kind == FieldKind::Real &&
            std::abs (ReadReal (bytes + startSize->offset, startSize->size) - ReadReal (bytes + endSize->offset, endSize->size)) > 1e-9;
        const bool pensDiffer = startPen != nullptr && endPen != nullptr && startPen->kind == FieldKind::SignedInt && endPen->kind == FieldKind::SignedInt &&
            ReadSigned (bytes + startPen->offset, startPen->size) != ReadSigned (bytes + endPen->offset, endPen->size);
        if (sizesDiffer || pensDiffer) {
            conditions += " linkedArrowAttributes is true: both ends share one arrowhead size and pen, so give startArrowData and endArrowData the same arrowSize and arrowPen, or linkedArrowAttributes false in the same call.";
        }
    }
    return conditions;
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
                            "enum": [ "number", "boolean", "integer", "string", "object" ],
                            "description": "The JSON type of the field's value: object for an OverriddenPen or OverriddenAttribute (see ElementFieldOptionalValue)."
                        },
                        "kind": {
                            "type": "string",
                            "enum": [ "Real", "Bool", "SignedInt", "UnsignedInt", "CString", "UString", "AttributeIndex", "Guid", "OverriddenPen", "OverriddenAttribute", "UniStringPointer" ],
                            "description": "The C++ kind of the field: an enum is a SignedInt, an AttributeIndex is the index of an attribute (pen tables excepted, pens are plain integers). OverriddenPen and OverriddenAttribute are the optional pens and attributes of the API (API_OverriddenPen, API_OverriddenAttribute, e.g. cutFillPen and refMat of a wall), read and written as an object with hasValue and value. A UniStringPointer is a GS::UniString* through which Archicad passes a text (e.g. note.contentUStr, the text of a dimension note): it is read and written as a string, the commands pointing it to a string of their own."
                        },
                        "size": {
                            "type": "integer",
                            "description": "The size of the field in bytes (for a string, the room in bytes including the closing zero; for a UniStringPointer, the size of the pointer)."
                        },
                        "readOnly": {
                            "type": "boolean",
                            "description": "True for the owners and counts that Archicad computes, and for the fields that choose the shape of an element (a roof's roofClass, a shell's shellClass, a label's labelClass, a stair structure's structType, also in the default structures of a stair such as structure[0][0].structType, a wall's type: changing one without the data of the other shape made Archicad 29 quit), and for the fields of placed elements Archicad refuses whatever their value: the library part (libInd) of Objects, Lamps, Windows, Doors, Skylights, Zones and symbol Labels, which ChangeLibraryPartOfElements changes, and the leader line fields of the notes of a Level Dimension (note1/note2 useLeaderLine, anchorAtTextEnd, begC, midC, endC). SetElementFields refuses them, giving readOnlyReason; SetFieldsOfElementDefaults refuses only the first group (owners, counts and shape selectors)."
                        },
                        "readOnlyReason": {
                            "type": "string",
                            "description": "Why the field is read-only; only given for a read-only field."
                        },
                        "variant": {
                            "type": "string",
                            "description": "For a field in a union, the member it is in, e.g. u.revolvedShell, u.planeRoof or structure[0][1].data.monolith. Only the member the element uses holds values (a roof's roofClass, a shell's shellClass, a label's labelClass, a stair structure's structType, the structType of that default structure of a stair): GetElementFields leaves the others out and SetElementFields refuses them."
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
        const char* readOnlyReason = GetReadOnlyReason (typeId, field, true);
        GS::ObjectState definition (
            "path", field.path,
            "type", GetJsonTypeName (field.kind),
            "kind", GetKindName (field.kind),
            "size", static_cast<Int32> (field.size),
            "readOnly", readOnlyReason != nullptr);
        if (readOnlyReason != nullptr) {
            definition.Add ("readOnlyReason", GS::UniString (readOnlyReason));
        }
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
                                        "$ref": "#/ElementFieldValue"
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

    // The texts of the GS::UniString* fields: they must outlive the elements read.
    UniStringStore strings;
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
        const GS::Optional<API_ElemTypeID> typeId = GetElementTypeId (GetGuidFromObjectState (*elementId), element, strings, err);
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
        AddFieldsOf (result, *table, *typeId, element, hasWanted ? &wanted : nullptr);
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
                            "description": "The new value of each field, by its path (see GetElementFieldDefinitions), e.g. {\"note.noteFont\": 3, \"note.frame\": true, \"note.contentUStr\": \"A\"}, or {\"refMat\": {\"hasValue\": true, \"value\": 12}} for a wall. Only these fields change: they are written with ACAPI_Element_Change and a mask of exactly their bytes, then read back.",
                            "additionalProperties": {
                                "$ref": "#/ElementFieldValue"
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

    // Everything is checked and prepared before the undoable command. The texts of the GS::UniString*
    // fields are in strings, which outlives the copies of the elements in prepared and the change.
    struct Prepared {
        bool                            ok = false;
        GS::ObjectState                 result;
        API_Element                     wanted = {};
        API_Element                     mask = {};
        GS::Array<const FieldInfo*>     fields;
        const FieldTable*               table = nullptr;
    };
    UniStringStore strings;
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
        const GS::Optional<API_ElemTypeID> typeId = GetElementTypeId (GetGuidFromObjectState (*elementId), p.wanted, strings, err);
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
        GS::ObjectState oldValues;
        GS::ObjectState newValues;
        const GS::UniString error = WriteFieldValues (fields, *table, *typeId, p.wanted, p.mask, p.fields, oldValues, newValues, strings, true);
        if (!error.IsEmpty ()) {
            p.result = CreateFailedExecutionResult (APIERR_BADPARS, error);
            prepared.Push (p);
            continue;
        }
        ClearUnwrittenUniStrings (*table, p.fields, p.wanted);
        p.table = table;
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
                // The conditions known to make Archicad refuse the given values (#3054), checked on the
                // element as it was to be written; not for a refusal whose reason is already known (a
                // locked layer, a Teamwork reservation).
                if (refusal.IsEmpty ()) {
                    message += DescribeConditions (GetElemTypeId (p.wanted.header), *p.table, p.fields, p.wanted, p.wanted);
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
                GSErrCode getErr = NoError;
                if (!GetElementTypeId (p.wanted.header.guid, got, strings, getErr).HasValue ()) {
                    p.result = CreateFailedExecutionResult (APIERR_GENERAL, "The element was changed but could not be read back.");
                } else {
                    const GS::UniString notApplied = DescribeNotKept (p.fields, p.wanted, got);
                    if (notApplied.IsEmpty ()) {
                        p.result = CreateSuccessfulExecutionResult ();
                    } else {
                        // The conditions known to make Archicad drop the values it did not keep (#3054),
                        // checked on the element as Archicad kept it.
                        GS::Array<const FieldInfo*> notKept;
                        for (const FieldInfo* field : p.fields) {
                            if (!IsSameFieldValue (*field, p.wanted, got)) {
                                notKept.Push (field);
                            }
                        }
                        p.result = CreateFailedExecutionResult (APIERR_REFUSEDPAR, GS::UniString ("Archicad did not keep every value:") + notApplied +
                            DescribeConditions (GetElemTypeId (got.header), *p.table, notKept, p.wanted, got) + " The other given fields were changed.");
                    }
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

// ---------------------------------------------------------------------------------------------
// GetFieldsOfElementDefaults

GetFieldsOfElementDefaultsCommand::GetFieldsOfElementDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFieldsOfElementDefaultsCommand::GetName () const
{
    return "GetFieldsOfElementDefaults";
}

GS::Optional<GS::UniString> GetFieldsOfElementDefaultsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementTypes": {
                "type": "array",
                "description": "The element types whose tool defaults are read.",
                "items": {
                    "$ref": "#/ElementType"
                }
            },
            "fields": {
                "type": "array",
                "description": "The paths of the fields to read (see GetElementFieldDefinitions), e.g. [\"thickness\", \"height\"]. Every field of the defaults is read when it is left out. A path the element type does not have is listed in missingFields.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementTypes"
        ]
    })";
}

GS::Optional<GS::UniString> GetFieldsOfElementDefaultsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fieldsOfElementDefaults": {
                "type": "array",
                "description": "The fields of the tool defaults of each element type, in the input order, or an error when the type has no fields in the table or no tool defaults.",
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
                                    "description": "The C++ struct of the element type in the Archicad API."
                                },
                                "fields": {
                                    "type": "object",
                                    "description": "The value of each field of the tool defaults, by its path.",
                                    "additionalProperties": {
                                        "$ref": "#/ElementFieldValue"
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
                                    "description": "The asked paths in a member of a union the defaults do not use (see variant in GetElementFieldDefinitions). Without the fields input, these are simply left out.",
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
            "fieldsOfElementDefaults"
        ]
    })";
}

GS::ObjectState GetFieldsOfElementDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    if (!IsAvailable ()) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, NotAvailableMessage);
    }
    GS::Array<GS::UniString> elementTypes;
    parameters.Get ("elementTypes", elementTypes);
    GS::Array<GS::UniString> wanted;
    const bool hasWanted = parameters.Get ("fields", wanted);

    // The texts of the GS::UniString* fields: they must outlive the defaults read.
    UniStringStore strings;
    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("fieldsOfElementDefaults");
    for (const GS::UniString& typeName : elementTypes) {
        const API_ElemTypeID typeId = GetElementTypeFromNonLocalizedName (typeName);
        const FieldTable* table = GetFieldTable (typeId);
        if (table == nullptr) {
            results (CreateErrorResponse (APIERR_BADPARS, GS::UniString ("No fields are known for the element type ") + typeName + "."));
            continue;
        }
        API_Element defaults = {};
        const GSErrCode err = GetDefaultsOfType (typeId, defaults, strings);
        if (err != NoError) {
            results (CreateErrorResponse (err, GS::UniString ("Failed to read the tool defaults of ") + typeName + "."));
            continue;
        }
        GS::ObjectState result;
        result.Add ("elementType", typeName);
        result.Add ("structName", table->structName);
        AddFieldsOf (result, *table, typeId, defaults, hasWanted ? &wanted : nullptr);
        results (result);
    }
    return response;
}

// ---------------------------------------------------------------------------------------------
// SetFieldsOfElementDefaults

SetFieldsOfElementDefaultsCommand::SetFieldsOfElementDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetFieldsOfElementDefaultsCommand::GetName () const
{
    return "SetFieldsOfElementDefaults";
}

GS::Optional<GS::UniString> SetFieldsOfElementDefaultsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fieldsOfElementDefaults": {
                "type": "array",
                "description": "The element types whose tool defaults change and the fields to write to each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementType": {
                            "$ref": "#/ElementType"
                        },
                        "fields": {
                            "type": "object",
                            "description": "The new value of each field, by its path (see GetElementFieldDefinitions), e.g. {\"thickness\": 0.25, \"height\": 2.7, \"cutFillPen\": {\"hasValue\": true, \"value\": 5}}. Only these fields change: they are written with ACAPI_Element_ChangeDefaults and a mask of exactly their bytes, then read back.",
                            "additionalProperties": {
                                "$ref": "#/ElementFieldValue"
                            },
                            "minProperties": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementType",
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
            "fieldsOfElementDefaults"
        ]
    })";
}

GS::Optional<GS::UniString> SetFieldsOfElementDefaultsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per item, in the input order. Tool defaults whose fields Archicad did not all keep fail with the fields and the values Archicad kept; the other fields are changed. On a dry run, a successful result has wouldChange with the oldValue and newValue of the given fields."
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

GS::ObjectState SetFieldsOfElementDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    if (!IsAvailable ()) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, NotAvailableMessage);
    }
    const bool dryRun = IsDryRun (parameters);
    GS::Array<GS::ObjectState> items;
    parameters.Get ("fieldsOfElementDefaults", items);

    // The texts of the GS::UniString* fields: they must outlive the defaults and their change.
    UniStringStore strings;
    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& item : items) {
        GS::UniString typeName;
        GS::ObjectState fields;
        if (!item.Get ("elementType", typeName) || !item.Get ("fields", fields)) {
            results (CreateFailedExecutionResult (APIERR_BADPARS, "elementType and fields are needed."));
            continue;
        }
        const API_ElemTypeID typeId = GetElementTypeFromNonLocalizedName (typeName);
        const FieldTable* table = GetFieldTable (typeId);
        if (table == nullptr) {
            results (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("No fields are known for the element type ") + typeName + "."));
            continue;
        }
        API_Element wanted = {};
        GSErrCode err = GetDefaultsOfType (typeId, wanted, strings);
        if (err != NoError) {
            results (CreateFailedExecutionResult (err, GS::UniString ("Failed to read the tool defaults of ") + typeName + "."));
            continue;
        }
        const API_Element before = wanted;
        API_Element mask = {};
        GS::Array<const FieldInfo*> written;
        GS::ObjectState oldValues;
        GS::ObjectState newValues;
        const GS::UniString error = WriteFieldValues (fields, *table, typeId, wanted, mask, written, oldValues, newValues, strings, false);
        if (!error.IsEmpty ()) {
            results (CreateFailedExecutionResult (APIERR_BADPARS, error));
            continue;
        }
        if (dryRun) {
            results (CreateWouldChangeExecutionResult (GS::ObjectState ("oldValue", oldValues, "newValue", newValues)));
            continue;
        }
        // The tool defaults are not part of the undo history, as in the settings dialogs of the tools.
        ClearUnwrittenUniStrings (*table, written, wanted);
        const GSErrCode changeErr = ACAPI_Element_ChangeDefaults (&wanted, nullptr, &mask);
        API_Element got = {};
        err = GetDefaultsOfType (typeId, got, strings);
        if (changeErr != NoError) {
            GS::UniString message = GS::UniString ("Archicad refused the change of the tool defaults of ") + typeName + ".";
            // Seen on Archicad 30: a refused change can still have written the other given fields.
            if (err == NoError) {
                GS::UniString changedAnyway;
                for (const FieldInfo* field : written) {
                    if (IsSameFieldValue (*field, wanted, got) && !IsSameFieldValue (*field, before, got)) {
                        changedAnyway += GS::UniString (" ") + field->path;
                    }
                }
                message += changedAnyway.IsEmpty () ? GS::UniString (" None of the given fields was changed.") :
                    GS::UniString (" It changed these given fields all the same:") + changedAnyway + ".";
            }
            for (const FieldInfo* field : written) {
                const size_t length = std::strlen (field->path);
                if (length >= 6 && std::strcmp (field->path + length - 6, "libInd") == 0) {
                    // Seen on Archicad 29 and 30 for doors, windows, skylights, objects, lamps and zones, even with the same value.
                    message += " Archicad does not take libInd (the library part) in the tool defaults, even unchanged: leave it out, or give the tool another library part by applying a favorite (ApplyFavoriteToToolDefault).";
                    break;
                }
            }
            results (CreateFailedExecutionResult (changeErr, message));
            continue;
        }
        if (err != NoError) {
            results (CreateFailedExecutionResult (err, GS::UniString ("The tool defaults of ") + typeName + " were changed but could not be read back."));
            continue;
        }
        GS::UniString notKept = DescribeNotKept (written, wanted, got);
        if (!notKept.IsEmpty () && IsRailingComponentType (typeId)) {
            notKept += " Archicad 29 and 30 keep no change in the tool defaults of railing components: the Railing defaults hold the segment and node models (defSegment, defNode).";
        }
        results (notKept.IsEmpty () ? CreateSuccessfulExecutionResult () :
            CreateFailedExecutionResult (APIERR_REFUSEDPAR, GS::UniString ("Archicad did not keep every value:") + notKept + " The other given fields were changed."));
    }
    AddDryRunField (response, dryRun);
    return response;
}
