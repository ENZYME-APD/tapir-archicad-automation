#include "GraphicOverrideCriterion.hpp"

#include "MigrationHelper.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#ifdef ServerMainVers_2700

// ---------------------------------------------------------------------------
// Structured criterion -> criterionXML generator
//
// Reverse-engineered from real criterionXML exported by ArchiCAD (there is no official
// documentation of this format beyond "save the rule as XML in ArchiCAD and look at it" - see
// API_OverrideRule::criterionXML's doc comment). ClassGuids and the element type table below were
// captured live: ClassGuids from ArchiCAD's own DevKit sample (Override_Test.cpp) and from rules
// exported by the running project; the element type table (ElemRegistryRefId / tool UnID guids) by
// creating one single-criterion rule per tool in ArchiCAD's own criteria editor and reading back
// its generated criterionXML through GetGraphicOverrideRules.
//
// Structure: <CriterionExpression> always wraps exactly one root CompositeCriterion (even for a
// single bare condition), and every leaf condition (ElemTypeCriterion, PropertyCriterion, ...) is
// itself always wrapped in its own trivial single-child CompositeCriterion before being placed as
// a composite's child - confirmed identical in every real example captured this session. Also
// confirmed (from the DevKit's own "Window or Door" sample, where the two conditions are known to
// be OR-combined): LogicalOperator 1 = OR, 2 = AND - the reverse of what the values might suggest.
// ---------------------------------------------------------------------------

static const char* kCompositeClassGuid   = "C6EBD1BD-7702-46FF-8ED9-9CC37648A7C7";
static const char* kElemTypeClassGuid    = "B4B7B134-EC56-4D40-8D4C-71D7C5A2493A";
static const char* kPropertyClassGuid    = "58E4905F-AD57-45F5-8D26-0100000F60BF";
static const char* kModelElemClassGuid   = "2C7FF58A-DE0F-4B5A-82EE-EA6A943E1951";
static const char* kDrawingElemClassGuid = "FD699EDF-F0A1-4728-87B8-092C09E4DE71";

struct ElemTypeInfo { const char* name; const char* refId; const char* mainGuid; const char* revGuid; };

// Every element type is identified by its ElemRegistryRefId alone, with an all-zero
// ElemCreatorToolUnID. For library part based tools (Door, Window, Object, Lamp, Zone, Label, Grid,
// MEP parts) the criteria editor also writes the tool's library subtype, whose revision differs
// between library versions; the registry code alone still matches every element of the tool, see
// the comment above the library part based entries.
static const ElemTypeInfo kElemTypeTable[] = {
    { "Wall",              "1463897138", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Column",            "1129270357", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Beam",              "1111834957", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Slab",              "1128614220", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Roof",              "1314017094", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Shell",              "1397245260", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Stair",              "1398030665", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Railing",            "1380010316", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CurtainWall",        "1129791820", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Opening",            "1330661703", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Mesh",               "1296388936", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Morph",              "1179800392", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Dimension",          "1145654623", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "LevelDimension",     "1279543629", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "RadialDimension",    "1380206925", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "AngleDimension",     "1094994253", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Text",               "1464816196", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "ChangeMarker",       "1128810317", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Hatch",              "1212240963", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Line",               "1279872581", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Arc",                "1095910239", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PolyLine",           "1347176782", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Spline",             "1397771342", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Hotspot",            "1213158495", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Section",            "1397048148", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Elevation",          "1162626390", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "InteriorElevation",  "1229735254", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Detail",             "1145394241", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Worksheet",          "1146242644", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Camera",             "1128353093", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Figure",             "1347371594", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Drawing",            "1146241367", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "StructuralLink",     "1095584841", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "StructuralSupport",  "1095586645", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "LinearLoad",         "1095583052", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PointLoad",          "1095585868", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "SurfaceLoad",        "1095586636", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctRouting",        "1296323141", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CableCarrierRouting","1296257605", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PipeRouting",        "1297109573", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    // Library part based tools and MEP parts. The criteria editor also writes the tool's library
    // subtype as ElemCreatorToolUnID, but the registry code alone matches every element of the tool
    // (checked live on Archicad 27 and 29 with the override Archicad actually applies to each element:
    // Window, Door, Zone, Label, Object, Lamp, GridElement), and the subtype's revision differs
    // between library versions. Codes: APIVarId_* in APIdefs_Elements.h, the others read from rules
    // made in the criteria editor.
    { "Window",              "1464421956", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Door",                "1146048338", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Zone",                "1380929357", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Label",               "1279345228", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Object",              "1398361410", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Lamp",                "1279870792", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "GridElement",         "1196574028", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Skylight",            "1397446988", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CornerWindow",        "1129469764", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "WallEnd",             "1463895374", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctBranch",          "1296323653", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctTerminal",        "1296323666", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctAccessory",       "1296318787", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctFitting",         "1296320073", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctFlexibleSegment", "1296320083", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CableCarrierBranch",  "1296258117", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CableCarrierFitting", "1296254537", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PipeBranch",          "1297110085", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PipeTerminal",        "1297110098", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PipeAccessory",       "1297105219", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PipeFitting",         "1297106505", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Equipment",           "1162958153", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
};

// Archicad 29 and 30 keep an element type as ElemType/ClassId (VBEF::CritToolType Mv="2"), not as
// the ElemRegistryRefId Tapir writes: Archicad rewrites the type when it stores the rule, and the
// criteria editor writes it that way too. The ClassIds of the tools below were read in Archicad 29
// (build 5101) and 30 (build 3001), both giving the same values, by creating one rule per tool with
// CreateGraphicOverrideRules and reading it back (issue #1816). The library part based tools are not
// listed: their ClassId was not read, so a type of theirs is returned as raw criterionXML.
struct ElemTypeClassIdInfo { const char* name; const char* classId; };

static const char* kAllTypesElemTypeName = "AllTypes";
static const char* kZeroGuid = "00000000-0000-0000-0000-000000000000";

static const ElemTypeClassIdInfo kElemTypeClassIdTable[] = {
    { "Wall",                "4F654582-33AF-4E52-ABFC-C35D46765EC4" },
    { "Column",              "13E8EB85-5A33-47A3-A4D4-1E05296AEC7E" },
    { "Beam",                "A5C2D285-5A7C-4A68-9554-9EDA9D4556CA" },
    { "Slab",                "945C1275-7948-41D4-8744-9D22D72CA486" },
    { "Roof",                "6EE31DEA-1D22-487B-9464-EBEF72C2C497" },
    { "Shell",               "4E697088-D921-4470-B8D3-50425E05EAA5" },
    { "Stair",               "FA0F7F15-29B4-4905-80FA-C6769109847B" },
    { "Railing",             "4C5AE61A-AF17-4B85-93E0-005C987D5F5B" },
    { "CurtainWall",         "C653C817-A855-4446-AEF4-24EFB46271AA" },
    { "Opening",             "6746AA79-7498-4BEA-BAD8-191CDAD31D05" },
    { "Mesh",                "8F2FEE46-6A3F-41AC-A32B-69E44DC6276D" },
    { "Morph",               "E6363C7B-940E-4DEB-AF95-8B3B39E86BD2" },
    { "Dimension",           "965D38E5-0CAD-4BE4-B502-4F5E907815DC" },
    { "LevelDimension",      "690878FA-B11F-46C9-953C-3864D85BDEE6" },
    { "RadialDimension",     "9938D642-ADAF-4471-944D-1801372AFE10" },
    { "AngleDimension",      "2120198C-D317-4CEA-AB4E-C7B6CEB5156E" },
    { "Text",                "E6C07EAE-5A2A-4597-95A9-73E7543AC91E" },
    { "ChangeMarker",        "0A3ED604-99A8-436B-8B4C-039FE67BFA0D" },
    { "Hatch",               "66306134-F76A-410F-991D-C9EE97B541EC" },
    { "Line",                "BDFE7D73-1C78-4ACA-809B-1B25747E3126" },
    { "Arc",                 "F3497B2B-11FC-49AE-8424-D933780BACCE" },
    { "PolyLine",            "B0DC8B22-AD6D-4BF1-9733-5077B6588FC4" },
    { "Spline",              "1E3120A6-36CE-464F-90E2-322CE53E2521" },
    { "Hotspot",             "DAACFF27-AC06-469B-BB7C-B1F15EAEDF70" },
    { "Section",             "B04555DD-A98F-4788-BDB1-C4A0F7DCF754" },
    { "Elevation",           "74AA8C96-4616-4777-BDEB-2F1B056D78CD" },
    { "InteriorElevation",   "D237E1F7-4DBC-4E54-8CCE-9F1C3C7D2A8C" },
    { "Detail",              "5E8AE540-D498-42C2-85FC-0710BD677752" },
    { "Worksheet",           "D75AD808-23AF-4278-969B-FA3ED3A6773E" },
    { "Camera",              "FB1AA3EA-890A-4AFD-A8F8-09B0DF5E502A" },
    { "Figure",              "558759DB-6AEA-4B80-B47F-FCD070B825CD" },
    { "Drawing",             "2DBE8C52-93B3-46B5-A0A9-2492C45C2AB3" },
    { "StructuralLink",      "C758E95E-D1C7-431E-993E-D3A91D0B6D8C" },
    { "StructuralSupport",   "5AE66636-F6CE-4FB7-9AAF-8B9B144DEBB1" },
    { "LinearLoad",          "726D33B4-6C35-4D53-993B-EF9101B12C90" },
    { "PointLoad",           "7E109364-8415-43D9-AE47-CE268AB99BD5" },
    { "SurfaceLoad",         "5417D715-D521-4A4E-8B4A-E409EAD7C4F4" },
    { "DuctRouting",         "FCECE4C5-E4E2-4335-A976-DD7F902466A1" },
    { "CableCarrierRouting", "D5FC1FF7-9DD7-4759-8700-FD78186FCE6C" },
    { "PipeRouting",         "BD6A8805-AD79-461B-AC77-23F0325DB295" },
};

// User strings (values, property and group names, classification item IDs and system names) go
// into the XML as element text: escape the characters XML reserves, or a name like "Doors & Windows"
// makes the XML malformed and Archicad drops the whole criterion without an error.
static GS::UniString XmlEscape (const GS::UniString& text)
{
    GS::UniString escaped = text;
    escaped.ReplaceAll ("&", "&amp;");
    escaped.ReplaceAll ("<", "&lt;");
    escaped.ReplaceAll (">", "&gt;");
    escaped.ReplaceAll ("\"", "&quot;");
    return escaped;
}

// ---------------------------------------------------------------------------
// A small reader for criterionXML (issue #1816). Archicad keeps a criterion as elements,
// attributes and text only, so this reads exactly that, skips comments, the XML declaration and
// processing instructions, and fails on anything else (CDATA, DOCTYPE, mismatched tags) instead of
// guessing. Strings are handled as UTF-8.
// ---------------------------------------------------------------------------

struct CriterionXmlNode {
    std::string                   name;
    std::string                   attributes;   // name="value" pairs as written, separated by one space
    std::string                   text;         // the unescaped text of an element without children
    std::vector<CriterionXmlNode> children;
};

static std::string ToUtf8 (const GS::UniString& text)
{
    return std::string (text.ToCStr (CC_UTF8).Get ());
}

static GS::UniString FromUtf8 (const std::string& text)
{
    return GS::UniString (text.c_str (), CC_UTF8);
}

static std::string XmlEscapeUtf8 (const std::string& text)
{
    std::string escaped;
    for (char c : text) {
        switch (c) {
            case '&':  escaped += "&amp;";  break;
            case '<':  escaped += "&lt;";   break;
            case '>':  escaped += "&gt;";   break;
            case '"':  escaped += "&quot;"; break;
            default:   escaped += c;        break;
        }
    }
    return escaped;
}

static void AppendUtf8 (unsigned long codePoint, std::string& out)
{
    if (codePoint < 0x80) {
        out += static_cast<char> (codePoint);
    } else if (codePoint < 0x800) {
        out += static_cast<char> (0xC0 | (codePoint >> 6));
        out += static_cast<char> (0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        out += static_cast<char> (0xE0 | (codePoint >> 12));
        out += static_cast<char> (0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char> (0x80 | (codePoint & 0x3F));
    } else {
        out += static_cast<char> (0xF0 | (codePoint >> 18));
        out += static_cast<char> (0x80 | ((codePoint >> 12) & 0x3F));
        out += static_cast<char> (0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char> (0x80 | (codePoint & 0x3F));
    }
}

static bool XmlUnescapeUtf8 (const std::string& text, std::string& out)
{
    out.clear ();
    for (size_t i = 0; i < text.size (); ++i) {
        if (text[i] != '&') {
            out += text[i];
            continue;
        }
        const size_t end = text.find (';', i);
        if (end == std::string::npos)
            return false;
        const std::string entity = text.substr (i + 1, end - i - 1);
        if (entity == "amp")        out += '&';
        else if (entity == "lt")    out += '<';
        else if (entity == "gt")    out += '>';
        else if (entity == "quot")  out += '"';
        else if (entity == "apos")  out += '\'';
        else if (entity.size () > 1 && entity[0] == '#') {
            const bool hex = entity[1] == 'x' || entity[1] == 'X';
            const std::string digits = entity.substr (hex ? 2 : 1);
            if (digits.empty ())
                return false;
            char* parsedEnd = nullptr;
            const unsigned long codePoint = std::strtoul (digits.c_str (), &parsedEnd, hex ? 16 : 10);
            if (parsedEnd == nullptr || *parsedEnd != '\0' || codePoint == 0 || codePoint > 0x10FFFF)
                return false;
            AppendUtf8 (codePoint, out);
        } else {
            return false;
        }
        i = end;
    }
    return true;
}

class CriterionXmlReader {
public:
    explicit CriterionXmlReader (const std::string& source) : s (source), pos (0) {}

    // Reads the elements that follow each other up to the end of the source.
    bool ReadSequence (std::vector<CriterionXmlNode>& outNodes)
    {
        while (true) {
            if (!SkipMisc ())
                return false;
            if (pos >= s.size ())
                return true;
            CriterionXmlNode node;
            if (!ReadElement (node))
                return false;
            outNodes.push_back (node);
        }
    }

private:
    std::string s;
    size_t      pos;

    bool StartsWith (const char* prefix) const
    {
        return s.compare (pos, std::strlen (prefix), prefix) == 0;
    }

    static bool IsSpace (char c)
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    void SkipSpaces ()
    {
        while (pos < s.size () && IsSpace (s[pos]))
            ++pos;
    }

    bool SkipPast (const char* terminator)
    {
        const size_t end = s.find (terminator, pos);
        if (end == std::string::npos)
            return false;
        pos = end + std::strlen (terminator);
        return true;
    }

    // White space, comments, the XML declaration and processing instructions between elements.
    bool SkipMisc ()
    {
        while (true) {
            SkipSpaces ();
            if (StartsWith ("<!--")) {
                if (!SkipPast ("-->"))
                    return false;
            } else if (StartsWith ("<?")) {
                if (!SkipPast ("?>"))
                    return false;
            } else {
                return true;
            }
        }
    }

    bool ReadName (std::string& outName)
    {
        const size_t start = pos;
        while (pos < s.size () && !IsSpace (s[pos]) && s[pos] != '/' && s[pos] != '>' && s[pos] != '=')
            ++pos;
        outName = s.substr (start, pos - start);
        return !outName.empty ();
    }

    bool ReadElement (CriterionXmlNode& node)
    {
        if (pos >= s.size () || s[pos] != '<')
            return false;
        ++pos;
        if (!ReadName (node.name) || node.name[0] == '!' || node.name[0] == '?')
            return false;

        while (true) {
            SkipSpaces ();
            if (pos >= s.size ())
                return false;
            if (s[pos] == '/') {
                ++pos;
                if (pos >= s.size () || s[pos] != '>')
                    return false;
                ++pos;
                return true;
            }
            if (s[pos] == '>') {
                ++pos;
                break;
            }
            std::string attributeName;
            if (!ReadName (attributeName))
                return false;
            SkipSpaces ();
            if (pos >= s.size () || s[pos] != '=')
                return false;
            ++pos;
            SkipSpaces ();
            if (pos >= s.size () || (s[pos] != '"' && s[pos] != '\''))
                return false;
            const char quote = s[pos++];
            const size_t valueEnd = s.find (quote, pos);
            if (valueEnd == std::string::npos)
                return false;
            if (!node.attributes.empty ())
                node.attributes += ' ';
            node.attributes += attributeName + "=\"" + s.substr (pos, valueEnd - pos) + "\"";
            pos = valueEnd + 1;
        }

        std::string rawText;
        while (true) {
            if (pos >= s.size ())
                return false;
            if (StartsWith ("<!--")) {
                if (!SkipPast ("-->"))
                    return false;
            } else if (StartsWith ("</")) {
                pos += 2;
                std::string endName;
                if (!ReadName (endName) || endName != node.name)
                    return false;
                SkipSpaces ();
                if (pos >= s.size () || s[pos] != '>')
                    return false;
                ++pos;
                break;
            } else if (s[pos] == '<') {
                if (StartsWith ("<!") || StartsWith ("<?"))
                    return false;
                CriterionXmlNode child;
                if (!ReadElement (child))
                    return false;
                node.children.push_back (child);
            } else {
                rawText += s[pos++];
            }
        }

        if (!node.children.empty ()) {
            for (char c : rawText) {
                if (!IsSpace (c))
                    return false;
            }
            return true;
        }
        return XmlUnescapeUtf8 (rawText, node.text);
    }
};

// One form for comparing and returning XML: no white space between elements, attributes as read,
// text escaped the way the generator escapes it, and empty elements as <Name/>.
static std::string CanonicalXml (const CriterionXmlNode& node)
{
    std::string xml = "<" + node.name;
    if (!node.attributes.empty ())
        xml += " " + node.attributes;
    if (node.children.empty () && node.text.empty ())
        return xml + "/>";
    xml += ">";
    if (node.children.empty ()) {
        xml += XmlEscapeUtf8 (node.text);
    } else {
        for (const CriterionXmlNode& child : node.children)
            xml += CanonicalXml (child);
    }
    return xml + "</" + node.name + ">";
}

static bool ReadCriterionXmlSequence (const GS::UniString& xml, std::vector<CriterionXmlNode>& outNodes)
{
    const std::string source = ToUtf8 (xml);
    CriterionXmlReader reader (source);
    return reader.ReadSequence (outNodes);
}

static const ElemTypeInfo* FindElemTypeInfo (const GS::UniString& name)
{
    for (const ElemTypeInfo& info : kElemTypeTable) {
        if (name == info.name)
            return &info;
    }
    return nullptr;
}

// isNot: the element type criterion's own LogicalOperator is 1 for "is" and 2 for "is not" (read
// from rules made in the criteria editor, issue #1816). mainGuid/revGuid: the library part subtype
// (ElemCreatorToolUnID), all zeros for the whole tool.
static GS::UniString ElemTypeCriterionXML (const ElemTypeInfo& info, bool isNot = false, const GS::UniString& mainGuid = GS::UniString (), const GS::UniString& revGuid = GS::UniString ())
{
    return GS::UniString ("<ClassGuid>") + kElemTypeClassGuid + "</ClassGuid>"
         + "<ElemTypeCriterion Mv=\"2\" Sv=\"3\">"
         +     "<VBEF::CritToolType Mv=\"1\" Sv=\"0\">"
         +         "<ElemRegistryRefId>" + info.refId + "</ElemRegistryRefId>"
         +         "<ElemCreatorToolUnID>"
         +             "<MainGuid>" + (mainGuid.IsEmpty () ? GS::UniString (info.mainGuid) : mainGuid) + "</MainGuid>"
         +             "<RevGuid>"  + (revGuid.IsEmpty () ? GS::UniString (info.revGuid) : revGuid) + "</RevGuid>"
         +         "</ElemCreatorToolUnID>"
         +     "</VBEF::CritToolType>"
         +     "<LogicalOperator>" + GS::UniString (isNot ? "2" : "1") + "</LogicalOperator>"
         + "</ElemTypeCriterion>";
}

// "Element Type is All Types": the all-zero ClassId, written exactly as Archicad keeps it in its own
// renovation rules (issue #1816); it has no ElemRegistryRefId.
static GS::UniString AllTypesElemTypeCriterionXML ()
{
    return GS::UniString ("<ClassGuid>") + kElemTypeClassGuid + "</ClassGuid>"
         + "<ElemTypeCriterion Mv=\"2\" Sv=\"3\">"
         +     "<VBEF::CritToolType Mv=\"2\" Sv=\"0\">"
         +         "<ElemType Mv=\"1\" Sv=\"0\"><ClassId>" + kZeroGuid + "</ClassId><Variation>0</Variation></ElemType>"
         +         "<ElemCreatorToolUnID><MainGuid>" + kZeroGuid + "</MainGuid><RevGuid>" + kZeroGuid + "</RevGuid></ElemCreatorToolUnID>"
         +     "</VBEF::CritToolType>"
         +     "<LogicalOperator>1</LogicalOperator>"
         + "</ElemTypeCriterion>";
}

// CriteriaOperatorEnum values, read in the criteria editor and checked on classified elements:
// 0 is (this item exactly), 1 is not, 10 has a value, 11 has no value (both ignore the item but
// need one), 12 is in the branch of (the item and its descendants, what the built-in
// "Classification - X" rules use), 13 is a direct child of, 14 is not in the branch of, 15 is not a
// direct child of.
//
// The classification SYSTEM is named, not identified by guid (HasNameId, like every custom
// property), and a mismatched name silently discards the whole criterion. The criteria editor
// writes it as "<name> - <edition version>" (read back live: "Classification Archicad - v 2.0"
// for the system named "Classification Archicad" with version "v 2.0"), and the label differs
// between languages and versions, so it is never hardcoded: it is resolved from the systems of the
// project, see ResolveClassificationSystemLabel.
static GS::UniString ClassificationSystemLabel (const API_ClassificationSystem& system)
{
    return system.editionVersion.IsEmpty () ? system.name : system.name + " - " + system.editionVersion;
}

static bool ContainsClassificationItemId (const GS::Array<API_ClassificationItem>& items, const GS::UniString& itemId)
{
    for (const API_ClassificationItem& item : items) {
        if (item.id == itemId)
            return true;
        GS::Array<API_ClassificationItem> children;
        if (ACAPI_Classification_GetClassificationItemChildren (item.guid, children) == NoError && ContainsClassificationItemId (children, itemId))
            return true;
    }
    return false;
}

// requested: a system's name or its "<name> - <version>" label. Not given: the only system of the
// project that contains itemId (or the only system at all when itemId is empty); an error when none
// or several do.
static bool ResolveClassificationSystemLabel (const GS::UniString& itemId, const GS::UniString& requested, GS::UniString& outLabel, GS::UniString& outError)
{
    GS::Array<API_ClassificationSystem> systems;
    ACAPI_Classification_GetClassificationSystems (systems);
    GS::Array<GS::UniString> candidates;
    for (const API_ClassificationSystem& system : systems) {
        const GS::UniString label = ClassificationSystemLabel (system);
        if (!requested.IsEmpty ()) {
            if (requested == label || requested == system.name) {
                outLabel = label;
                return true;
            }
            continue;
        }
        GS::Array<API_ClassificationItem> roots;
        if (itemId.IsEmpty () || (ACAPI_Classification_GetClassificationSystemRootItems (system.guid, roots) == NoError && ContainsClassificationItemId (roots, itemId)))
            candidates.Push (label);
    }
    if (!requested.IsEmpty ()) {
        outError = "Unknown classification system '" + requested + "'.";
        return false;
    }
    if (candidates.GetSize () == 1) {
        outLabel = candidates[0];
        return true;
    }
    if (itemId.IsEmpty ()) {
        outError = candidates.IsEmpty () ? "The project has no classification system." : "The project has several classification systems: give 'classificationSystem'.";
        return false;
    }
    outError = candidates.IsEmpty ()
        ? "No classification system of the project contains the item '" + itemId + "'."
        : "Several classification systems contain the item '" + itemId + "': give 'classificationSystem'.";
    return false;
}

// The ID of the first root item of the system with this label. "has a value" / "has no value" ignore
// the item, but Archicad drops the whole condition when it is empty, like the editor that always
// keeps one.
static GS::UniString FirstClassificationItemId (const GS::UniString& systemLabel)
{
    GS::Array<API_ClassificationSystem> systems;
    ACAPI_Classification_GetClassificationSystems (systems);
    for (const API_ClassificationSystem& system : systems) {
        GS::Array<API_ClassificationItem> roots;
        if (ClassificationSystemLabel (system) == systemLabel && ACAPI_Classification_GetClassificationSystemRootItems (system.guid, roots) == NoError && !roots.IsEmpty ())
            return roots[0].id;
    }
    return GS::UniString ();
}

// op: see the operator comment above ClassificationSystemLabel.
static GS::UniString ClassificationCriterionXML (const GS::UniString& itemId, int op, const GS::UniString& systemName)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\">"
         +     "<VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         +         "<PropertyDefinitionUserId Version=\"2\">"
         +             "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +             "<Guid>00000000-0000-0000-0000-000000000000</Guid>"
         +             "<HasNameId>true</HasNameId><Name>" + XmlEscape (systemName) + "</Name>"
         +             "<PropertyDefinitionGroupUserId Version=\"2\">"
         +                 "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +                 "<Guid>00000000-0000-0000-0000-000000000000</Guid>"
         +                 "<HasNameId>true</HasNameId><Name>ClassificationSystemPropertyDefinitionGroup</Name>"
         +             "</PropertyDefinitionGroupUserId>"
         +         "</PropertyDefinitionUserId>"
         +         "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", op) + "</CriteriaOperatorEnum>"
         +         "<ClassificationValue><ClassificationItemUserID>" + XmlEscape (itemId) + "</ClassificationItemUserID></ClassificationValue>"
         +     "</VBEF::ConditionIO>"
         + "</PropertyCriterion>";
}

// Wraps a leaf's own <ClassGuid>/<Tag> pair in the trivial single-child CompositeCriterion every
// leaf is always nested in, whether it sits at the root or inside an explicit and/or group.
static GS::UniString WrapAsTrivialGroup (const GS::UniString& leafXML)
{
    // LogicalOperator is logically irrelevant here (CriteriaCount=1), but every real single-item
    // wrap captured this session used 1 (OR) - and the criteria editor was confirmed to silently
    // fail to render a classification row as such when 2 was used instead, despite ArchiCAD
    // accepting the rule without error. Match the real value exactly rather than relying on it
    // being a don't-care.
    return GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
         + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>1</LogicalOperator><CriteriaCount>1</CriteriaCount>"
         + leafXML
         + "</CompositeCriterion>";
}

// "Model View" (Vue modele) system properties - reverse-engineered from a rule containing every
// operator of every field in that criteria category, created live in ArchiCAD and read back
// through GetGraphicOverrideRules. Unlike Classification, these are identified purely by GUID
// (HasNameId=false) and share one fixed PropertyDefinitionGroupUserId guid. Confirmed generic
// operator values across property kinds: 0=is, 1=is not, 6=contains, 7=does not contain, 8=starts
// with, 9=ends with (string only). "Layer" and "Layer combination" are attribute-index-valued (the
// UI shows the attribute's name, but the XML stores its index), so their generator functions
// resolve a name to an index via ACAPI_Attribute_GetAttributesByType at generation time.
static const char* kModelViewPropertyGroupGuid = "517BE4C1-0FAF-4CA2-9C89-B631C2DEAFAE";
static const char* kMissingAttributesPropertyGuid = "45D43C12-49F3-4818-A56B-C033DEA90C8B";
static const char* kMissingLibraryPartPropertyGuid = "CF85835A-A655-4B72-9BBC-EE3F4441DB5B";
static const char* kLayerPropertyGuid            = "0F43553E-58E7-4661-8CEB-8B4E5D2ED50A";
static const char* kLayerLockedPropertyGuid      = "194656B7-E624-466F-A527-50804AB99E8D";
static const char* kLayerVisiblePropertyGuid     = "708E442F-1EDF-4E5D-84F9-F97C66D68FE7";
static const char* kLayerCombinationPropertyGuid = "C0FFBC04-E003-4357-A1E4-A6A092BFCF86";
static const char* kLayerNamePropertyGuid        = "812CD483-40FB-4E75-966F-E76C3E68B9F7";

// header.name is the legacy 8-bit name and garbles non-ASCII characters, so the Unicode name is read
// through uniStringNamePtr, the way GetAttributeName in AttributeCommands.cpp does.
static bool FindAttributeIndexByName (API_AttrTypeID typeID, const GS::UniString& name, Int32& outIndex)
{
    GS::Array<API_Attribute> attrs;
    ACAPI_Attribute_GetAttributesByType (typeID, attrs);
    bool found = false;
    for (API_Attribute& attr : attrs) {
        if (!found) {
            GS::UniString attrName;
            API_Attribute named = {};
            named.header.typeID = attr.header.typeID;
            named.header.index = attr.header.index;
            named.header.guid = attr.header.guid;
            named.header.uniStringNamePtr = &attrName;
            if (ACAPI_Attribute_Get (&named) == NoError && attrName == name) {
                outIndex = GetAttributeIndex (attr.header.index);
                found = true;
            }
            DisposeAttribute (named);
        }
        DisposeAttribute (attr);
    }
    return found;
}

// "Construction" criteria category system properties - same reverse-engineering method as "Model
// View" (a rule exercising every field/operator in the category, created live and read back).
static const char* kConstructionPropertyGroupGuid   = "AFBE1CD9-B14D-4AB8-A16F-99686F399F6C";
static const char* kCompositeStructurePropertyGuid  = "17743909-E801-4C87-A1B8-D4168A10D115";
static const char* kCompositeStructureNamePropertyGuid = "704E9212-3E21-4790-BAE4-CF3DE2395481";
static const char* kComplexProfileNamePropertyGuid  = "75F1B979-F6C1-40ED-A478-E950CB729E81";
static const char* kComplexProfilePropertyGuid      = "78781057-3451-4282-8799-558D15BCADAA";
static const char* kRoofConnectedPropertyGuid       = "CC564BEC-6D0B-476E-9D1F-762AFCCA3627";
static const char* kStructureTypePropertyGuid       = "98A26D3B-3BAF-4019-BE7A-09285FFA597C";

static GS::UniString PropertyHeaderXML (const char* propertyGuid, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>0</PrimaryId><HasGuidId>true</HasGuidId>"
         +     "<Guid>" + propertyGuid + "</Guid><HasNameId>false</HasNameId><Name/>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>0</PrimaryId><HasGuidId>true</HasGuidId>"
         +         "<Guid>" + groupGuid + "</Guid><HasNameId>false</HasNameId><Name/>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

// Looks up a custom property by its (name, group) pair and, if it is backed by an option list (a
// "jeu d'options" - API_PropertySingleChoiceEnumerationCollectionType), returns one of its real
// option values. Needed only for the meta-operators (see TryGetMetaOperator): ArchiCAD validates a
// customPropertyString meta-operator's placeholder value against the property's own valid value set
// when it's option-list-backed - confirmed live that ANY placeholder that isn't a real option value
// (including an empty string, or any other string not in the list) causes rule creation to silently
// succeed while ArchiCAD discards the whole criterion internally, with no error reported. A
// genuinely free-text (non-option-list) custom string property has no such constraint.
static bool FindFirstOptionValueForCustomProperty (const GS::UniString& propertyName, const GS::UniString& groupName, GS::UniString& outValue)
{
    GS::Array<API_PropertyGroup> groups;
    if (ACAPI_Property_GetPropertyGroups (groups) != NoError)
        return false;

    for (const API_PropertyGroup& group : groups) {
        if (group.name != groupName)
            continue;
        GS::Array<API_PropertyDefinition> definitions;
        if (ACAPI_Property_GetPropertyDefinitions (group.guid, definitions) != NoError)
            continue;
        for (const API_PropertyDefinition& definition : definitions) {
            if (definition.name != propertyName)
                continue;
            if (definition.collectionType != API_PropertySingleChoiceEnumerationCollectionType || definition.possibleEnumValues.IsEmpty ())
                return false;
            outValue = definition.possibleEnumValues[0].displayVariant.uniStringValue;
            return true;
        }
    }
    return false;
}

// Custom (user-defined) properties, identified purely by name + group name (both HasNameId=true,
// no guid) - reverse-engineered from a "TAPIR8" rule. Confirmed live: EVERY custom property type,
// regardless of its own value type, additionally supports 6 shared "availability" meta-operators on
// top of its normal value-comparison operators (16=has default value, 17=has custom value,
// 18=available, 19=not available, 20=is undefined, 21=is not undefined) - counted live as exactly
// 7 total operators for a boolean property (1 value-op + 6 meta), 8 for a string/option-set
// property (2 value-ops + 6 meta), 12 for a numeric property (6 value-ops + 6 meta).
static GS::UniString CustomPropertyHeaderXML (const GS::UniString& propertyName, const GS::UniString& groupName)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +     "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +     "<Name>" + XmlEscape (propertyName) + "</Name>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>" + XmlEscape (groupName) + "</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

// The 6 shared availability meta-operators (see CustomPropertyHeaderXML's doc). valueVariantXML is
// a placeholder <Variant> matching the property's own type - confirmed live that ArchiCAD still
// requires a syntactically valid (if practically ignored) value here even for these meta-operators.
static bool TryGetMetaOperator (const GS::ObjectState& node, int& outOp)
{
    bool flag = false;
    if (node.Get ("hasDefaultValue", flag) && flag)   { outOp = 16; return true; }
    if (node.Get ("hasCustomValue", flag) && flag)     { outOp = 17; return true; }
    if (node.Get ("isAvailable", flag) && flag)        { outOp = 18; return true; }
    if (node.Get ("isNotAvailable", flag) && flag)     { outOp = 19; return true; }
    if (node.Get ("isUndefined", flag) && flag)        { outOp = 20; return true; }
    if (node.Get ("isNotUndefined", flag) && flag)     { outOp = 21; return true; }
    return false;
}

// bareVariant: confirmed live that a custom String/Option-Set property omits the <Value> wrapper
// entirely (for every operator, including the meta-operators) - unlike Bool/Number custom
// properties, which do wrap. Yet another per-property-type quirk, not inferable from anything else.
static GS::UniString CustomPropertyCriterionXML (const GS::UniString& headerXML, int op, const GS::UniString& valueVariantXML, bool bareVariant = false)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + headerXML
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", op) + "</CriteriaOperatorEnum>"
         + (bareVariant ? valueVariantXML : (GS::UniString ("<Value>") + valueVariantXML + "</Value>"))
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString BoolVariantXML (bool value)
{
    return GS::UniString ("<Variant Type=\"BoolVariant\"><Status>Normal</Status><Value>") + (value ? "true" : "false") + "</Value></Variant>";
}

static GS::UniString StringVariantXML (const GS::UniString& value)
{
    return GS::UniString ("<Variant Type=\"StringVariant\"><Status>Normal</Status><Value>") + XmlEscape (value) + "</Value></Variant>";
}

static GS::UniString NumVariantXML (double value)
{
    return GS::UniString ("<Variant Type=\"NumVariant\"><Status>Normal</Status><Value>") + GS::UniString::Printf ("%.15g", value) + "</Value></Variant>";
}

// Custom properties of Integer valueType (e.g. "Occupants") use a distinct IntVariant, not
// NumVariant - confirmed against a real "TAPIR10" export. Length/Volume/Angle/Area custom
// properties are all just Real valueType with a different display unit and already use NumVariant
// like plain numbers, so they need no separate handling.
static GS::UniString IntVariantXML (int value)
{
    return GS::UniString ("<Variant Type=\"IntVariant\"><Status>Normal</Status><Value>") + GS::UniString::Printf ("%d", value) + "</Value></Variant>";
}

// "Modificateurs de profil" (Profile Modifiers) criteria category, reverse-engineered from a
// "TAPIR7" rule - GEOMETRIE tab. Every field here is numeric with all 6 comparison operators
// (=,!=,<,>,<=,>=). Unlike every other property criterion this session, the value sits inside an
// extra <ProfileParameterValue> wrapper around the usual <Value><Variant>. The 6 built-in
// parameters (Etirement hauteur/largeur, Hauteur nominale/totale, Largeur totale/nominale) are
// identified by a fixed GUID; any other profile modifier (a parameter specific to one Complex
// Profile attribute in the project, e.g. "Debord tole") is identified by its plain name instead -
// both forms share the same by-name "ProfileParameterPropertyDefinitionGroup" group.
static const char* kProfileParameterGroupName = "ProfileParameterPropertyDefinitionGroup";
static const char* kProfileStretchHeightGuid = "B9FFB90F-9C1A-4BC4-8A4E-5EE02A800513";
static const char* kProfileStretchWidthGuid  = "B6FB6EFE-ED97-4F4D-814B-D9FEC39E63FC";
static const char* kProfileNominalHeightGuid = "F7028EBA-7D6D-4475-A9B3-61470DF7FDDD";
static const char* kProfileTotalHeightGuid   = "D1A91234-F6F1-45D9-966F-407A686062FB";
static const char* kProfileTotalWidthGuid    = "6D5B0B90-2266-40B6-8CDE-B2638101F78D";
static const char* kProfileNominalWidthGuid  = "98E721E3-372F-4D9E-85EC-2C763372CEBD";

static GS::UniString ProfileParameterHeaderXML (const char* propertyGuid)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>0</PrimaryId><HasGuidId>true</HasGuidId>"
         +     "<Guid>" + propertyGuid + "</Guid><HasNameId>false</HasNameId><Name/>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>" + kProfileParameterGroupName + "</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

static GS::UniString ProfileParameterHeaderByNameXML (const GS::UniString& propertyName)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +     "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +     "<Name>" + XmlEscape (propertyName) + "</Name>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>" + kProfileParameterGroupName + "</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

// A profile parameter that holds one value per profile component sits in its own by-name group and
// stores a NumListVariant inside <ProfileParameterValueList> (read from a rule made in the editor,
// where only "is" was offered for it).
static GS::UniString ProfileParameterListCriterionXML (const GS::UniString& propertyName, double value)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + "<PropertyDefinitionUserId Version=\"2\">"
         +     "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +     "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +     "<Name>" + XmlEscape (propertyName) + "</Name>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>ListTypeProfileParameterPropertyDefinitionGroup</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>"
         + "<CriteriaOperatorEnum>0</CriteriaOperatorEnum>"
         + "<ProfileParameterValueList><Value><Variant Type=\"NumListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%.15g", value) + "</ValueItem></ValueArray></Variant></Value></ProfileParameterValueList>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// numOp: 0==, 1=not-equal, 2=less-than, 3=greater-than, 4=less-or-equal, 5=greater-or-equal.
static GS::UniString ProfileParameterCriterionXML (const GS::UniString& headerXML, double value, int numOp)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + headerXML
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<ProfileParameterValue><Value><Variant Type=\"NumVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%.15g", value) + "</Value></Variant></Value></ProfileParameterValue>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// Tries each of the 6 numeric-comparison field name suffixes for a fixed-guid profile parameter.
static bool TryEmitProfileParamField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (ProfileParameterCriterionXML (ProfileParameterHeaderXML (propertyGuid), value, kOps[i]));
            return true;
        }
    }
    return false;
}

static GS::UniString BoolModelViewCriterionXML (const char* propertyGuid, bool value, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>0</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"BoolVariant\"><Status>Normal</Status><Value>" + GS::UniString (value ? "true" : "false") + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString AttrIndexEqualsModelViewCriterionXML (const char* propertyGuid, Int32 attrIndex, bool isNot, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "1" : "0") + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"Property::AttributeIndexValue\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%d", attrIndex) + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString AttrIndexContainsModelViewCriterionXML (const char* propertyGuid, Int32 attrIndex, bool isNot, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "7" : "6") + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"AttributeIndexIndexValueListValueVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%d", attrIndex) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// intOp: 0=is, 6=contains, 7=does not contain (only ops confirmed for Font/Pen fields - no "is not"
// was seen for any of them in the captured "TAPIR3" rule, only for the separate Pen Replacement
// field which uses IntEqualsPropertyCriterionXML/op 0-1 instead).
static GS::UniString IntListCriterionXML (const char* propertyGuid, int value, int intOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", intOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"IntListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%d", value) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "Surface et Materiaux" (Surface and Materials) criteria category - reverse-engineered from a
// "TAPIR4" rule. Unlike "Nom Calque" (Model View), its string fields use StringListVariant (a
// <ValueArray> of <ValueItem> even for a single value) rather than a plain StringVariant.
static const char* kSurfaceMaterialPropertyGroupGuid = "6D514CD0-C97B-4363-BEBD-3B0E78192001";
static const char* kConstructionMaterialIdPropertyGuid   = "5E7EAF3C-AD5F-43C5-9C0C-223B0833742D";
static const char* kConstructionMaterialPropertyGuid     = "DE3E93A6-2D3B-4123-BAAE-2F8DABA6A41C";
static const char* kConstructionMaterialNamePropertyGuid = "38B71747-1B59-4486-A06E-A6D491AB5B24";
static const char* kSurfaceNamePropertyGuid              = "716D5B45-15FB-4473-AD28-FE358AC94B70";
static const char* kSurfacePropertyGuid                  = "E800D234-AD63-47BF-897A-99A98B04C76B";

// stringOp: 0=is, 1=is not, 6=contains, 7=does not contain, 8=starts with, 9=ends with.
static GS::UniString StringListCriterionXML (const char* propertyGuid, const GS::UniString& value, int stringOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", stringOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"StringListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + XmlEscape (value) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// stringOp: 0=is, 1=is not, 6=contains, 7=does not contain, 8=starts with, 9=ends with.
// bareVariant: see GuidEqualsPropertyCriterionXML's doc - some property definitions (e.g.
// "Affichage sur filtre de renovation", "Etat de Variante", "Jeu de Variantes") omit the <Value>
// wrapper around their <Variant> entirely, confirmed live; most others (e.g. "Nom", "ID unique")
// don't, hence the default of false here.
static GS::UniString StringModelViewCriterionXML (const char* propertyGuid, const GS::UniString& value, int stringOp, const char* groupGuid = kModelViewPropertyGroupGuid, bool bareVariant = false)
{
    const GS::UniString variantXML = GS::UniString ("<Variant Type=\"StringVariant\"><Status>Normal</Status><Value>") + XmlEscape (value) + "</Value></Variant>";
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", stringOp) + "</CriteriaOperatorEnum>"
         + (bareVariant ? variantXML : (GS::UniString ("<Value>") + variantXML + "</Value>"))
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "ID et Categories" criteria category - reverse-engineered from a "TAPIR5" rule. Several distinct
// sub-groups exist within this one category (each field below lists its own). "ID de l'element",
// "Identifiant principal de Lien" and "Lien et ID d'element" support all 10 operators (is/is not/
// less-than/greater-than/less-or-equal/greater-or-equal/contains/does not contain/starts with/ends
// with) and, notably, still use plain StringVariant (not a numeric type) even for the numeric-
// looking comparison operators - confirmed live, e.g. "less than" stores its value as the string
// "1", not a number. "ID unique" and "Nom" support only the 6 text operators (no numeric ones).
// Only fields with an unambiguous, fully captured value mapping are implemented - "Calcul de Zone"
// and "Etat de renovation" were exercised in the same rule but their exact value-to-label mapping
// could not be disambiguated from the captured data (fewer distinct values were saved than there
// are dropdown options), so they are intentionally left unsupported rather than guessed.
static const char* kIdCategoryGroupGuid1     = "21AFD5A3-5024-457F-BC50-1AC8B973BA81"; // shared with Classification's ModelElement-adjacent group seen earlier this session
static const char* kIdCategoryGroupGuid2     = "7480DAA4-9E45-49BF-AE44-8AD0F57F95B2";
static const char* kIdCategoryGroupGuid3     = "953BBE44-8DD0-4527-85D8-DC6FF12E57D3";
static const char* kIdCategoryGroupGuid4     = "C77496B3-B63A-4272-ABC3-0A8C530E2B45";

static const char* kRenovationFilterPropertyGuid = "3FC77583-1284-478B-BA80-9C81F0A091B1"; // group1
static const char* kStructuralFunctionPropertyGuid = "9B2F38A8-198B-4125-9E73-5646F072C10E"; // group1
static const char* kPositionPropertyGuid     = "53402BE5-29D9-44A5-B6D6-4FA90938DDC9"; // group1
static const char* kVariantStatePropertyGuid = "5C3FB768-C349-49E7-B6E6-7675A26A2AEC"; // group2
static const char* kVariantSetPropertyGuid   = "29914344-AE69-41BE-983F-856EFD41AC33"; // group2
static const char* kElementIdPropertyGuid    = "B1B54D45-C951-42C9-9AF8-898F0BF212AB"; // group3
static const char* kUniqueIdPropertyGuid     = "B028B081-606E-4465-B530-6DC2457E1E78"; // group3
static const char* kLinkMainIdPropertyGuid   = "1E6DD646-48B8-47AB-B4C5-727A7B42DC0C"; // group3
static const char* kLinkedElementIdPropertyGuid = "D6FD38B0-DCD1-4F61-AA7E-3E6FD1D81F90"; // group3
static const char* kNamePropertyGuid         = "7B8277EB-5891-4222-A03B-4B7F432739CC"; // group3
static const char* kLinkSourcePropertyGuid   = "848C0C29-BD43-47D1-8B35-403F64280ADA"; // group3
static const char* kMissingVariantPropertyGuid = "5CC582DF-EC76-4A09-AFB9-B97FADA4F2EC"; // group4

// Same idea as TryEmitStringListField, for the 6-operator plain-StringVariant field family (ID et
// Categories category: "ID unique", "Nom").
static bool TryEmitStringField6Ops (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, const char* groupGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Same idea, for the 10-operator hybrid field family ("ID de l'element", "Identifiant principal de
// Lien", "Lien et ID d'element") - adds LessThan/GreaterThan/LessOrEqual/GreaterOrEqual, still all
// plain StringVariant (see the category comment above).
static bool TryEmitStringField10Ops (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, const char* groupGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       2,          3,             4,             5,                6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 10; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Enum-valued properties identified by a fixed internal GuidVariant value (Fonction structurelle,
// Position, Source de lien) - each option's guid is a hardcoded constant with no name lookup
// possible/needed. bareVariant: some property definitions omit the <Value> wrapper around their
// <Variant> entirely (confirmed live, e.g. Fonction structurelle/Position do, Source de lien
// doesn't) - a per-property quirk of ArchiCAD's own property schema, not tied to type or operator
// count, so it must be passed in explicitly rather than inferred.
static GS::UniString GuidEqualsPropertyCriterionXML (const char* propertyGuid, const char* valueGuid, bool isNot, const char* groupGuid, bool bareVariant = false)
{
    const GS::UniString variantXML = GS::UniString ("<Variant Type=\"GuidVariant\"><Status>Normal</Status><Value>") + valueGuid + "</Value></Variant>";
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "1" : "0") + "</CriteriaOperatorEnum>"
         + (bareVariant ? variantXML : (GS::UniString ("<Value>") + variantXML + "</Value>"))
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "Type de structure" (structureType) uses a bare <Variant> directly under <VBEF::ConditionIO> -
// no <Value> wrapper - unlike every other property criterion captured this session. Confirmed via
// the exact byte structure of a live-captured "TAPIR" rule exercising this field.
static GS::UniString IntEqualsPropertyCriterionXML (const char* propertyGuid, int value, const char* groupGuid, bool isNot = false)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "1" : "0") + "</CriteriaOperatorEnum>"
         + "<Variant Type=\"IntVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%d", value) + "</Value></Variant>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "Positionnement" (Positioning) criteria category. numOp: 0==, 1=not-equal, 2=less-than,
// 3=greater-than, 4=less-or-equal, 5=greater-or-equal - confirmed live via a rule exercising all 6
// operators on a single field ("Altitude a niveau de la mer" in a "TAPIR2" rule).
static const char* kPositioningPropertyGroupGuid   = "C42CED8B-DD12-4782-A0C5-8FF167D55DBB";
static const char* kSeaLevelAltitudePropertyGuid   = "C35800B4-5126-405B-964B-EF0C43B1D2C2";
static const char* kProjectZeroAltitudePropertyGuid = "5294566C-2BB7-4ADC-A6F7-F8E947B1E500";
// "Placement story" - only ever seen live with the "=" operator (a single captured row); unlike
// every other Positioning-group field, no evidence yet of "not-equal" support, so only equality is
// exposed for now. Value is the same 0-based story/floor index used elsewhere in Tapir (floorIndex).
static const char* kPlacementStoryPropertyGuid     = "792002ED-8D64-4DE4-877E-817D5CDF798F";
static const char* kGroundFloorAltitudePropertyGuid = "6EC682F2-34E5-419F-BC0E-4B1020A5DFC5";
static const char* kRoofAltitudePropertyGuid       = "D204DBB0-D697-47F2-9FD6-EA988BBF35DF";

// "MEP" (mechanical/electrical/plumbing element) criteria category - reverse-engineered from a
// "TAPIR1" rule on Archicad 29. Only applicable to MEP elements (ducts, pipes, cable carriers, ...).
// The 12 numeric fields all use the same generic 6-operator NumVariant pattern as Positionnement;
// Description/Nom du materiau use the same 6-operator plain StringVariant pattern as ID et
// Categories' "ID unique"/"Nom" - confirmed live for the "contains" operator on both, the rest of
// the operator set matches by direct analogy with every other field using these two shared helpers.
static const char* kMepPropertyGroupGuid           = "85599253-0EF2-4CA9-8086-A49B507DA053";
static const char* kMepDiameterPropertyGuid        = "8CF9515E-3BE6-4D59-AD21-3534E75AEAA8";
static const char* kMepDiameter2PropertyGuid       = "3B4312F8-C31F-493B-B19F-A2A07E908C3A";
static const char* kMepDNPropertyGuid              = "4A94BDEC-3A8C-47F6-A7D0-BE1866CC8E69";
static const char* kMepDN2PropertyGuid             = "9407DEA8-D393-41ED-91F3-3540F83B935F";
static const char* kMepHeightPropertyGuid          = "F2074948-92A3-41C7-B036-6E8C31EA135F";
static const char* kMepHeight2PropertyGuid         = "AA6A2A58-1AAE-49E7-B14F-080B8F1EADD9";
static const char* kMepWidthPropertyGuid           = "8300E1CC-B910-44AB-AC3A-529D6863836E";
static const char* kMepWidth2PropertyGuid          = "B4DD3D87-3231-4860-A15B-75451254B109";
static const char* kMepAnglePropertyGuid           = "671B771B-2437-4649-BF49-0AFDA391739A";
static const char* kMepCustomAnglePropertyGuid     = "6DA39F69-F58B-42C4-B260-27BD70A5B696";
static const char* kMepInsulationThicknessPropertyGuid = "FE791816-C043-4878-B1A3-E96C0BE01D2F";
static const char* kMepLengthPropertyGuid          = "F4D30EE5-C430-4CF9-BFA4-67DF4A4EE15A";
static const char* kMepDescriptionPropertyGuid     = "DB605867-4DC8-458C-8159-83B12B539A1E";
static const char* kMepMaterialNamePropertyGuid    = "8D4F0D8F-9915-4E89-90F6-340B0CC1BB20";

// Wall-specific criteria - reverse-engineered from a "TESTTAPIR_MUR" rule on Archicad 29. Unlike
// every category so far, these fields are scattered across several different property groups:
// already-known ones (Positionnement, Construction, Plan et Coupe, ID et Categories, Surface et
// Materiaux) plus one new one only ever seen paired with wall-specific fields - kWallGeometryGroupGuid
// ("Geometrie" tab fields specific to walls: thickness/height/width/angles/...).
// Note: the structural/FEM analytical-model fields (offsets, edge releases, member type/position,
// group guid CD27E698-F857-47A6-833B-6F0F558DA5F5) were reverse-engineered and briefly supported here
// too, but were deliberately removed by user decision: too many rules for Column/Beam with no clear
// "not covered" boundary, so analytical-model criteria are entirely out of scope for this generator.
static const char* kWallGeometryGroupGuid       = "554FDF95-B57F-427D-85F5-C00CFBF7406D";

static const char* kWallComplexityPropertyGuid           = "1BDC244B-242B-4DF9-A77C-20B079491495"; // Construction group
static const char* kWallLevelOffsetPropertyGuid          = "015C36FD-F3C5-4AE0-AF60-EBC2935CC5E9"; // Positioning group
static const char* kWallTopOffsetPropertyGuid            = "501D6492-507A-47D8-9158-EEA2F0882B7F"; // Positioning group
static const char* kWallReferenceLinePositionPropertyGuid = "54FEFC6B-6BCA-4535-8311-BBE04203CB07"; // Positioning group
static const char* kWallOutsideAnglePropertyGuid         = "7EF7271A-5EAB-4805-86AB-120869542EAA";
static const char* kWallInsideAnglePropertyGuid          = "654377FC-E480-4C7A-9906-25892080710B";
static const char* kWallThicknessPropertyGuid            = "AC754097-00D3-4301-9E20-328A4780FC72";
static const char* kWallHeightPropertyGuid               = "7CAB47B5-D80B-4E2F-AB2B-71CF0BB57856";
static const char* kWallProfileHeightPropertyGuid        = "7EDA68CA-D646-4180-B840-4327181C3E62";
static const char* kWallHeightReversedPropertyGuid       = "E0B209EB-A5B1-4198-A86A-462F3BB59955";
static const char* kWallWidthPropertyGuid                = "19A99799-5B44-4A6F-9904-F132D7A3F0AC";
static const char* kWallProfileWidthPropertyGuid         = "A69ECF2A-DFEE-4B64-A1CF-C562F57FB503";
static const char* kObjectLengthPropertyGuid             = "DA895241-B437-4897-864C-F001FD704F64"; // Object's "A" (length) parameter, same Geometrie group
static const char* kWallGeometryTypePropertyGuid         = "3FEBB287-6D09-4629-8F48-099D15927732";
static const char* kWallReferenceLineOffsetAttrGuid      = "D625E555-CBB8-443E-BFC5-F29F575472F8"; // RealCriterion/AttributeCriterion pattern
static const char* kWallTopLinePropertyGuid              = "E0FB1BED-9779-4547-839C-666C65B4F961"; // Plan et Coupe group
static const char* kWallCutLinePropertyGuid              = "801FA71E-EAF1-44C5-B6F5-F85671367FB7"; // Plan et Coupe group
static const char* kWallUncutLinePropertyGuid            = "B672C3D2-EC21-4D47-B0FE-FB34899D0CFF"; // Plan et Coupe group
static const char* kWallHatchForegroundPenPropertyGuid   = "B9482485-AF7B-49A9-B012-553C4366EA86"; // Plan et Coupe group
static const char* kWallContourPenPropertyGuid           = "51B1930E-12AB-4A8B-9C25-99BF0997FE4C"; // Plan et Coupe group
static const char* kWallTopLinePenPropertyGuid           = "66B0FA67-6487-4585-9168-AD0F0586A213"; // Plan et Coupe group
static const char* kWallCutHatchBackgroundPenPropertyGuid = "7AF8E5FF-4DC2-4869-9BED-14A2E8D8247D"; // Plan et Coupe group
static const char* kWallHatchBackgroundPenPropertyGuid   = "ED2B9124-DE9C-46D9-9E42-9796ECB73C3F"; // Plan et Coupe group
static const char* kWallUncutLinePenPropertyGuid         = "245B9933-064A-4B59-BE0A-F4517AE79E5D"; // Plan et Coupe group
static const char* kWallOutsideSurfacePropertyGuid       = "CFD1BA5B-19BF-48CB-A007-03C017190DE7"; // Surface et Materiaux group
static const char* kWallInsideSurfacePropertyGuid        = "ED36D500-8AC8-41AD-A316-5178D7786B16"; // Surface et Materiaux group
static const char* kWallSideSurfacePropertyGuid          = "553149E3-242B-4686-8190-68C3BB122B64"; // Surface et Materiaux group
static const char* kWallParentIdPropertyGuid             = "652333FC-B73A-4D25-92A2-ACAD3BCF847E"; // ID et Categories group3
static const char* kWallConnectedOpeningIdsPropertyGuid  = "917EE466-F091-4794-AC4C-1ED044C9F802"; // ID et Categories group3

// "Decalage Ligne de Reference" uses an entirely different mechanism from every other numeric field
// captured this session: a RealCriterion/AttributeCriterion pair (not PropertyCriterion), with its
// own "Type" enum for the operator (1=is, 2=lessThan, 3=greaterThan, 4=isNot, 5=lessOrEqual,
// 6=greaterOrEqual - NOT the usual 0-5 CriteriaOperatorEnum numbering) - confirmed live against all
// 6 rows of a captured rule.
static const char* kRealCriterionClassGuid = "52E38E9F-7E9F-44F1-A295-4873E529D52E";
static const char* kRealConditionClassGuid = "43ACB724-3561-4277-98FA-456D782D6D67";

static GS::UniString RealAttributeCriterionXML (const char* attributeGuid, double value, int realType)
{
    return GS::UniString ("<ClassGuid>") + kRealCriterionClassGuid + "</ClassGuid>"
         + "<RealCriterion Mv=\"2\" Sv=\"0\">"
         +     "<AttributeCriterion Mv=\"1\" Sv=\"0\"><AttributeGuid>" + attributeGuid + "</AttributeGuid></AttributeCriterion>"
         +     "<ClassGuid>" + kRealConditionClassGuid + "</ClassGuid>"
         +     "<RealSimpleCondition Mv=\"1\" Sv=\"0\"><Type>" + GS::UniString::Printf ("%d", realType) + "</Type>"
         +         "<Value>" + GS::UniString::Printf ("%.15g", value) + "</Value></RealSimpleCondition>"
         + "</RealCriterion>";
}

static bool TryEmitRealAttributeField (const GS::ObjectState& node, const char* fieldPrefix, const char* attributeGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kTypes[]    = { 1,   4,     2,          3,             5,             6 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (RealAttributeCriterionXML (attributeGuid, value, kTypes[i]));
            return true;
        }
    }
    return false;
}

// Column-specific criteria - reverse-engineered from a "TESTTAPIR_POTEAU" rule on Archicad 29. Most
// of Column's fields turned out to be the exact same properties already captured for Wall (Height,
// ProfileHeight, HeightReversed, Width, ProfileWidth, the 6 Plan et Coupe pens, TopLine, UncutLine,
// ParentId, ConnectedOpeningIds, wallLevelOffset/wallTopOffset) - reused as-is via their existing
// "wallXxx" JSON field names rather than duplicated under new column-prefixed names, since the
// underlying guid/group/XML shape is byte-identical. Only the genuinely column-specific fields below
// are new. Analytical/structural (FEM) fields are entirely out of scope for this generator - see the
// note above kWallGeometryGroupGuid.
static const char* kColumnCoatingTypePropertyGuid          = "2810579D-4FDE-4021-9A45-8909DC9491F4"; // Construction group
static const char* kColumnInclinationAnglePropertyGuid     = "1A448310-F491-4C72-B457-29ED14337221";
static const char* kColumnCrossSectionTypePropertyGuid     = "CD4B0B73-1D23-4C6C-A2E8-A9F785795ECE";
static const char* kColumnCoreDiameterPropertyGuid         = "B801C842-A167-4C70-A333-521608A2E906";
static const char* kColumnCoatingThicknessPropertyGuid     = "98E22B3A-C663-4249-8D85-BC5C4F32D487";
static const char* kColumnBottomCrossSectionHeightCutPropertyGuid           = "8362B613-A280-448F-9A18-5CFFE7D2F396";
static const char* kColumnBottomCrossSectionHeightPerpendicularPropertyGuid = "44B25D67-1443-48BD-823B-FED0F337EE2A";
static const char* kColumnTopCrossSectionHeightCutPropertyGuid              = "2252E6BF-8732-44A2-BCBD-DEDB4755B401";
static const char* kColumnTopCrossSectionHeightPerpendicularPropertyGuid    = "DA523AF3-DDF3-4CB3-918A-2E351AEDBF28";
static const char* kColumnProfileCoreHeightPropertyGuid                     = "EC45A091-9916-48B0-8A8F-1EF9CD875558";
static const char* kColumnCoreHeightDiameterRatioPropertyGuid               = "C9D4DA15-A7A7-4B5D-8E48-854F3D2A7053";
static const char* kColumnBottomCrossSectionWidthCutPropertyGuid            = "AF929A6D-7987-4D67-85DB-54DCC4108C50";
static const char* kColumnBottomCrossSectionWidthPerpendicularPropertyGuid  = "60229C62-3E47-4465-83E6-EAF08EAA58ED";
static const char* kColumnTopCrossSectionWidthCutPropertyGuid               = "D802D20B-7C6E-4C3B-BD47-DF9B493275F6";
static const char* kColumnTopCrossSectionWidthPerpendicularPropertyGuid     = "AC42946C-D4CF-429E-9B96-5EC771A70F34";
static const char* kColumnCoreWidthPropertyGuid                             = "2E32E96D-6220-4AE2-86C2-FEDB45450DC2";
static const char* kColumnProfileCoreWidthPropertyGuid                      = "98A67AD0-6B13-46CE-B675-7C121375FF89";
static const char* kColumnCoveringHatchPropertyGuid         = "127C75B5-E910-499A-9ADC-D96C177EEE94"; // Plan et Coupe group
static const char* kColumnCoatingHatchPropertyGuid          = "4D8A160B-C77F-457A-90B6-F5DC20DFA3D5"; // Plan et Coupe group
static const char* kColumnCoreForegroundPenPropertyGuid           = "89E86F63-D06B-4C79-8FE9-3AA79367E01E";
static const char* kColumnCoatingForegroundPenPropertyGuid        = "CDD27246-A306-441B-927C-B5B848057978";
static const char* kColumnCoveringHatchForegroundPenPropertyGuid  = "080B21EF-26A0-4549-BA5D-4296CCD9DFAB";
static const char* kColumnCoveringHatchBackgroundPenPropertyGuid  = "8DDAC1BD-539C-4770-A3CF-0AD197858016";
static const char* kColumnCoatingBackgroundPenPropertyGuid        = "E6381AC7-4CBE-4017-9AE9-7B7323FB7901";
static const char* kColumnCoreBackgroundPenPropertyGuid           = "B969A0F1-C258-4552-B231-D470AD8B9697";
static const char* kColumnHiddenLinePenPropertyGuid               = "A707B974-39B9-43F5-8B6A-37157903265B";
static const char* kColumnHiddenLineTypePropertyGuid        = "2DFDC696-E0E3-4122-A796-A872CB861ED2"; // Plan et Coupe group
static const char* kColumnCoreSurfacePropertyGuid           = "782C3212-829A-4BBE-B0F3-278CFD3FD3C0"; // Surface et Materiaux group
static const char* kColumnCoatingSurfacePropertyGuid        = "277C094D-89F6-4B93-BA37-82C7D3C619EC"; // Surface et Materiaux group
static const char* kColumnExtrusionSurfacePropertyGuid      = "45CD5C6B-92AB-4872-B774-B6CC8E0920E0"; // Surface et Materiaux group

// Beam-specific criteria - reverse-engineered from a "TESTTAPIR_POUTRE" rule on Archicad 29. Almost
// every field turned out to be shared with Wall/Column (same guids, reused via their existing
// "wallXxx"/"columnXxx" JSON field names) - only these 7 are genuinely new to Beam.
static const char* kBeamAxisPenPropertyGuid          = "4A2025B4-8CDA-44F4-B4D1-0DA9FE50E8F4"; // Plan et Coupe group
static const char* kBeamAxisLineTypePropertyGuid     = "E0E205E4-F84D-45A4-B9F4-90C61E87CFF3"; // Plan et Coupe group
static const char* kBeamEndSurfacePropertyGuid       = "BA800584-43FD-458E-B644-A848E4B28268"; // Surface et Materiaux group
static const char* kBeamRightSurfacePropertyGuid     = "F99C4755-8DD4-4740-AEE9-D6DF185BA2A7"; // Surface et Materiaux group
static const char* kBeamLeftSurfacePropertyGuid      = "EC6BEC35-B802-4D08-96D1-5D40FB4754E3"; // Surface et Materiaux group
static const char* kBeamBottomSurfacePropertyGuid    = "9592AFF2-C655-4E1A-8434-D5CC9409D609"; // Surface et Materiaux group
static const char* kBeamTopSurfacePropertyGuid       = "8B571B29-384C-4D2C-A87F-594E60A6686F"; // Surface et Materiaux group

// Slab-specific criteria - reverse-engineered from a "TESTTAPIR_DALLE" rule on Archicad 29. Again,
// most fields are shared with Wall/Column/Beam (reused via their existing JSON field names) - only
// these 5 are genuinely new to Slab.
static const char* kSlabReferencePlaneLocationPropertyGuid = "EF2304F7-4AD4-4D29-AD23-904A92244982"; // Positionnement group
static const char* kSlabEdgeAnglePropertyGuid               = "FF81335A-A237-44A5-9E22-02B07585DFE3"; // Geometrie group
static const char* kSlabDefaultEdgeAnglePropertyGuid         = "B00BB8A0-2F90-4B15-B6EA-4FEFA4DE2DE1"; // Geometrie group
static const char* kSlabCutLinePenPropertyGuid               = "34408308-7AE6-4A11-B4C4-DF041DB2C0DC"; // Plan et Coupe group
static const char* kSlabConstructionMaterialFillPropertyGuid = "A9B69EE4-6AEE-4F29-B9E7-F8C4BA7ADB01"; // Plan et Coupe group

// Roof-specific criteria - reverse-engineered from a "TESTTAPIR_TOIT" rule on Archicad 29. Again,
// most fields are shared with Wall/Column/Beam/Slab (reused via their existing JSON field names,
// including structureType, which Roof also exposes) - only these 7 are genuinely new to Roof.
static const char* kRoofCutBodyTypePropertyGuid       = "7F100F87-6D8F-43AA-97B5-8194C600B3FE"; // Construction group
static const char* kRoofLevelNumberPropertyGuid       = "599C4BCA-CC94-46F8-BF4B-EA787984CC4E"; // Construction group
static const char* kRoofEaveOverhangTypePropertyGuid  = "00A1F4A4-B63C-4226-BB80-D4FFCEBDDC2E"; // Construction group
static const char* kRoofEdgeAnglePropertyGuid         = "C06EA0C1-93EB-49A4-A3A0-AC0D842C778B"; // Geometrie group
static const char* kRoofEaveOverhangPropertyGuid      = "7513211D-FB9D-45DE-9C1B-CFC9BBFD8E49"; // Geometrie group
static const char* kRoofGeometryTypePropertyGuid      = "0E58BDFD-8B5F-443E-AD3F-15F92A53F65A"; // Geometrie group
static const char* kRoofPitchPropertyGuid             = "96CA70EA-53AE-4966-9679-AE6699CD78E3"; // Geometrie group

static GS::UniString NumListCriterionXML (const char* propertyGuid, double value, int numOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"NumListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%.15g", value) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static bool TryEmitNumListField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (NumListCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Shell-specific criteria - reverse-engineered from a "TESTTAPIR_COQUE" rule on Archicad 29. Again,
// most fields are shared with Wall/Column/Beam/Slab/Roof (reused via their existing JSON field
// names) - only these 3 are genuinely new to Shell. shellGeometryType's 3 values (Extrude/Revolved/
// Ruled) were all confirmed live, re-captured after the user set each row to a distinct value.
static const char* kShellCutBodyTypePropertyGuid  = "626C831E-F093-4983-A3C8-3723CE5D24D8"; // Construction group
static const char* kShellTiltAnglePropertyGuid     = "1E6BCA80-2880-4D63-9DA2-CF8B66CA28A5"; // Geometrie group
static const char* kShellGeometryTypePropertyGuid  = "5D862E7D-3DD8-4FEB-BC3F-090B3D96B27B"; // Geometrie group

// Door/Window/RoofWindow/Opening/CurtainWall/Stair-specific criteria - reverse-engineered from
// "TESTTAPIR_PORTE", "TESTTAPIR_FENETRES", "TESTTAPIR_OUVERTURE DE TOIT", "TESTTAPIR_PERCEMENT",
// "TESTTAPIR_MUR RIDEAU" and "TESTTAPIR_ESCALIER" rules on Archicad 29. Most fields turned out to
// already be covered by existing generic properties (missingAttributes, missingLibraryPart,
// roofConnected, altitudes, floorIndex, surface/surfaceName, hatchFill, lineAttribute) - only the
// genuinely new fields below are added. "Garde-corps" (Railing) and "Zone" contributed nothing new
// beyond linkedUpperStoryAbsolute/Relative (shared with Stair).
// NOT YET IMPLEMENTED (deliberately, needs live disambiguation before guessing): "Etat de
// renovation" (GuidVariant referencing a built-in renovation status item, unlike the existing
// String-based renovationFilterIs/renovationFilterIsNot) and Opening's "Forme d'ouverture"/"Limites
// d'ouverture"/"Orientation de l'Ouverture" (IntVariant enums whose captured value order does not
// reliably map to their on-screen row order - see project memory before adding these).
static const char* kLibraryPartNamePropertyGuid    = "432FA53A-B71E-404B-A9D5-F1964237A3EB"; // ID et Categories group3
static const char* kSubTypeNamePropertyGuid        = "ADAB9A93-9B0C-4534-888F-154637E85B2F"; // ID et Categories group3
static const char* kSillHeightPropertyGuid         = "5348DD2B-980F-4846-BB56-82F63E29D2D3"; // Positioning group
static const char* kLintelHeightPropertyGuid       = "5E3A5950-FB8D-4C6B-90CC-4A0F800E4F3F"; // Positioning group
static const char* kScreedThicknessPropertyGuid    = "B10B1591-80BC-4594-8EAD-67108D3BC681"; // Positioning group, 6 ops confirmed live
static const char* kLinkedUpperStoryAbsolutePropertyGuid = "D7794904-7C0F-4DBF-B605-97E7ADF31398"; // Positioning group, equals only (captured value -31001 = "Non lie")
static const char* kLinkedUpperStoryRelativePropertyGuid = "6AE32B74-2F68-4156-8A3C-526E3B0D0E5E"; // Positioning group, equals only
static const char* kJambThickness1PropertyGuid     = "8B474B84-BBD0-42CE-A257-14AA9B799BB7"; // Geometrie group (Door/Window "Epaisseur Montant1")
static const char* kJambThickness2PropertyGuid     = "97D11F04-F9EF-475D-A818-EF9597079C1D"; // Geometrie group ("Epaisseur Montant2")
static const char* kOpeningEndSurfacePropertyGuid       = "BD0EB07B-0C2F-4982-8275-9B6F2EB85E12"; // Surface et Materiaux group
static const char* kOpeningExtrusionSurfacePropertyGuid = "5FCAF13E-6CCF-4064-95FB-B0095EFE78C1"; // Surface et Materiaux group
static const char* kCurtainWallPanelOffsetPropertyGuid   = "A18DF336-50A5-4F12-8636-5449D676ED55"; // Geometrie group
static const char* kCurtainWallFrameCutLinePropertyGuid  = "9555ADAE-C62F-4D9D-B4E6-AAC60A6F5606"; // Plan et Coupe group
static const char* kCurtainWallPanelCutLinePropertyGuid  = "5692D710-471B-4589-9270-D4498E91E74D"; // Plan et Coupe group
static const char* kCurtainWallFrameCutLinePenPropertyGuid = "3FA0E6A3-EB0F-4339-B197-AAE3B51A1638"; // Plan et Coupe group
static const char* kCurtainWallPanelCutLinePenPropertyGuid = "22EA110D-AE6C-4777-988D-C9AE77CE1CFA"; // Plan et Coupe group
static const char* kStairRiserInclineAnglePropertyGuid = "4EAD33D3-191A-43AF-8840-BCD9DB58E50A"; // Geometrie group
static const char* kStairMinHeadroomPropertyGuid       = "2F893F86-7AAE-45A2-8831-63277D8A0CE6"; // Geometrie group
static const char* kStairInvalidHeightPropertyGuid     = "99822D1B-DB8F-43A4-A0E7-AD151C790608"; // Geometrie group
static const char* kStairDefaultWidthPropertyGuid      = "DBD410FF-55E3-45CD-BDF6-73903377B406"; // Geometrie group
static const char* kStairSlopePropertyGuid             = "01FBABBB-5B2B-41EE-9782-E675AA3A0858"; // Geometrie group
// "Regles et normes" (Rules and Standards) group, first seen on Stair. The criteria editor shows
// all 6 comparison operators (=, !=, <, >, <=, >=) for its numeric fields; the per-flight count is a
// list value and keeps "is" only.
static const char* kStairRulesGroupGuid          = "9056DEE6-79CD-479E-B272-DC3AF25C784A";
static const char* kStairMaxRiserHeightPropertyGuid = "A9630A1F-96EB-4EA6-AC20-118159E87402";
static const char* kStairMinRiserHeightPropertyGuid = "FDEA5248-5DDE-4C40-8EA0-F426483E4C87";
static const char* kStairMaxGoingPropertyGuid       = "C3D20956-7F11-4335-B0EE-C07C913DEA8B";
static const char* kStairMinGoingPropertyGuid       = "E9A114C9-F1A7-461D-8D5B-6F174E34D033";
static const char* kStairRiserCountPropertyGuid     = "84F5ADA8-A634-4FDE-8FEE-B1FCC01265AB";
static const char* kStairStepCountPropertyGuid      = "DA0BC1B5-CB21-49B6-B4BE-E558D6B8C8FD";
static const char* kStairStepCountPerFlightPropertyGuid = "645C36B4-4191-4AD2-B9AB-A5AFDDDC1573";

// "Plan et Coupe" (Plan and Section) criteria category - reverse-engineered from a "TAPIR3" rule.
// Hachure/Type Ligne reference a real Fill/Line attribute (contains/does not contain only, like
// layerCombination). Police/Stylo/Stylo Texte reference a font/pen by its plain numeric ID (no
// attribute name lookup - pens in particular are always plain 1-255 numbers in ArchiCAD, not
// separate named attributes), each confirmed to support only the operator subset actually shown in
// the criteria editor for that field (Stylo: contains/does not contain only, no "is"; Stylo Texte
// and Police: is/contains/does not contain, no "is not"). Remplacement stylo (Pen Replacement) is
// a plain int equals/not-equals like structureType.
static const char* kPlanSectionPropertyGroupGuid = "50C777CB-6E83-4E01-911D-F45ECAA1A454";
static const char* kHatchFillPropertyGuid        = "AF2AC4AD-2176-4D0F-A20F-2CD2A6214288";
static const char* kFontPropertyGuid             = "06C5E0BC-B83A-4415-AD9A-3155EAB92CBF";
static const char* kPenReplacementPropertyGuid   = "2E2D1387-3642-4BAC-AE05-C16C68EA9A71";
static const char* kLinePenPropertyGuid          = "528B2B52-F122-4B1D-A0CA-5F3FFCE26BB2";
static const char* kTextPenPropertyGuid          = "06FC1DC4-A694-49F5-A0F3-6A5BF1C3CCC0";
static const char* kLineTypeAttrPropertyGuid     = "E8523625-0F5C-45DE-8A64-FD941F92F27F";
// "Stylo Contour Hachure" (hatch contour pen) - unlike the generic "Stylo" field above (contains/does
// not contain only), this one is is/is-not (numOp 0/1), like the wallXxx pen fields.
static const char* kHatchContourPenPropertyGuid  = "58AA86FB-238C-4406-BDBF-D51345B68000";

// Fields read from rules made in the criteria editor of Archicad 29 (one rule per tool, one row per
// criterion, then one rule per value to read each label). Only the operators the editor offered in
// those rules are exposed. Archicad 27 drops the two MEP fields and profileParameterList: they come
// from later versions. The MEP system field ("Systeme MEP") is not offered: on Archicad 29 the value
// the editor stores is not the MEP system's attribute index.
static const char* kMeshEdgePenPropertyGuid            = "2F9EEA27-1EC6-4A97-842A-316711A9EBC9"; // Plan et Coupe
static const char* kMeshSkirtSurfacePropertyGuid       = "6E408207-3694-45F1-BA3C-47CF1908D829"; // Surface et Materiaux
static const char* kMorphBodyTypePropertyGuid          = "F3660383-BA08-4D4C-B980-52151726CF08"; // Construction
static const char* kBeamCrossSectionTypePropertyGuid   = "1672FA80-BA17-47E5-9534-5F03667A085F"; // Geometrie
static const char* kTextAlignmentPropertyGuid          = "65447A7E-F3ED-4745-AF2B-69F4FDBEF3A0"; // Plan et Coupe
static const char* kTextStylePropertyGuid              = "62EB6A8A-BE4E-44AE-9D82-955DEA25D201"; // Plan et Coupe
static const char* kTextLineSpacingPropertyGuid        = "E0086C44-E61D-42A4-96E4-C26D0D10FED2"; // Plan et Coupe
static const char* kDimensionMarkerSizePropertyGuid    = "0E052983-4375-4EDA-88D1-7353072BC80E"; // Geometrie
static const char* kDimensionMarkerPenPropertyGuid     = "F82DF163-F43F-43AF-9B33-4B87CD7A7A74"; // Plan et Coupe
static const char* kDimensionMarkerPenListPropertyGuid = "5BD213D1-669A-4553-80C8-E86C2956F781"; // Plan et Coupe
static const char* kDimensionPointerPenPropertyGuid    = "6FE1182D-D8B7-4324-9BB6-C767EA812512"; // Plan et Coupe
static const char* kDimensionPointerLineTypePropertyGuid = "927D1625-305C-43A4-9B43-3FED6E109E45"; // Plan et Coupe
static const char* kHatchCategoryPropertyGuid          = "1114CD0A-6BF8-4A4D-8386-B0D5ECDEB35B"; // Plan et Coupe
static const char* kStairRisersPerFlightPropertyGuid   = "D1323E5E-20FA-4919-BA73-319444F388FB"; // Regles et normes
static const char* kOpeningShapePropertyGuid           = "CD42AD05-5E0B-461D-928A-31DD2FBEB0E7"; // Geometrie
static const char* kOpeningLimitsPropertyGuid          = "9CAEF363-1C79-44AC-A7D3-AAFC667F1DF0"; // Geometrie
static const char* kOpeningOrientationPropertyGuid     = "486C8DA7-A3AA-4E0C-898C-625BA0CF2B71"; // Geometrie
static const char* kRenovationStatusPropertyGuid       = "9CF14061-6E9C-45BE-BD8F-D3BCF59D218E"; // ID et Categories group1
static const char* kVariantSetNamePropertyGuid         = "59634BB2-6D8C-4A4B-B349-172F4157974B"; // ID et Categories group4
static const char* kVariantNamePropertyGuid            = "114FC56B-160D-4CCF-91F1-EAD505051C9A"; // ID et Categories group4
static const char* kMepCommonPropertyGroupGuid         = "85599253-0EF2-4CA9-8086-A49B507DA053";
static const char* kMepOpenConnectionPropertyGuid      = "51C349F2-F113-4B50-9FB5-D9610C14B12F";
// Renovation status values are built-in status guids, identical in every project.
static const char* kRenovationExistingGuid      = "615A6BBB-1B83-FF46-57B3-683CFDD57EBF";
static const char* kRenovationToBeDemolishedGuid = "57E66097-9C38-8012-D9C7-0CA01AA5EB1E";
static const char* kRenovationNewGuid           = "913BE5A9-C24C-DD41-AE08-079792A58CF6";

struct EnumValue {
    const char* name;
    int         value;
};

// Maps an enum name of the schema to the raw value Archicad stores; false with outError when unknown.
static bool FindEnumValue (const char* fieldName, const GS::UniString& name, const EnumValue* values, int count, int& outValue, GS::UniString& outError)
{
    GS::UniString allowed;
    for (int i = 0; i < count; ++i) {
        if (name == values[i].name) {
            outValue = values[i].value;
            return true;
        }
        allowed += GS::UniString (i == 0 ? "" : ", ") + values[i].name;
    }
    outError = GS::UniString ("Invalid ") + fieldName + " '" + name + "'. Must be one of: " + allowed + ".";
    return false;
}

static const EnumValue kMorphBodyTypes[]  = { { "NonSolid", 0 }, { "Solid", 1 } };
// The values the structureType condition writes, see EmitCriterionNode.
static const EnumValue kStructureTypes[] = { { "Basic", 1 }, { "Composite", 2 }, { "ComplexProfile", 3 } };
// The values of the Plan and Section enum conditions, see EmitCriterionNode.
static const EnumValue kPenReplacementValues[] = { { "None", 1 }, { "ForegroundOnly", 2 }, { "BackgroundOnly", 3 }, { "Both", 4 } };
static const EnumValue kTextAlignments[]       = { { "Left", 0 }, { "Center", 1 }, { "Right", -1 }, { "Justified", 20 } };
static const EnumValue kTextStyles[]           = { { "Bold", 1 }, { "Italic", 2 }, { "Underline", 4 }, { "Strikethrough", 256 }, { "Superscript", 512 }, { "Subscript", 1024 } };
static const EnumValue kHatchCategories[]      = { { "Drafting", 1 }, { "Cover", 2 }, { "Cut", 3 }, { "BuildingMaterialCut", 4 }, { "SurfaceColor", 5 }, { "SurfaceCover", 6 }, { "SurfaceTexture", 7 } };
// The values of the tool enum conditions, see EmitCriterionNode (the reader of part 5 of #1816 names
// them with these tables, so keep them the same as the writer's).
static const EnumValue kBeamCrossSections[]    = { { "Rectangular", 0 }, { "Profiled", 1 }, { "Circular", 2 } };
static const EnumValue kOpeningShapes[]        = { { "Square", 0 }, { "Rectangle", 1 }, { "Circle", 2 }, { "Ellipse", 3 }, { "Polygonal", 4 } };
static const EnumValue kOpeningLimits[]        = { { "Infinite", 0 }, { "BothSides", 1 }, { "OneSide", 2 } };
static const EnumValue kOpeningOrientations[]  = { { "Vertical", 0 }, { "Horizontal", 1 }, { "Aligned", 2 }, { "Custom", 3 } };
static const EnumValue kWallComplexities[]     = { { "Straight", 0 }, { "Slanted", 3 }, { "DoubleSlanted", 5 } };
static const EnumValue kWallReferenceLinePositions[] = { { "Outside", 0 }, { "Center", 1 }, { "Inside", 2 }, { "CoreOutside", 3 }, { "CoreCenter", 4 }, { "CoreInside", 5 } };
static const EnumValue kWallGeometryTypes[]    = { { "Uniform", 0 }, { "Trapezoid", 1 }, { "Polygonal", 3 } };
static const EnumValue kColumnCoatingTypes[]   = { { "Core", 0 }, { "Finish", 1 }, { "Other", 2 } };
static const EnumValue kColumnCrossSectionTypes[] = { { "Rectangular", 2 }, { "Circular", 1 }, { "Profiled", 3 } };
static const EnumValue kSlabReferencePlaneLocations[] = { { "Top", 0 }, { "CoreTop", 1 }, { "CoreBottom", 2 }, { "Bottom", 3 } };
static const EnumValue kSlabEdgeAngles[]       = { { "Vertical", 0 }, { "Custom", 3 } };
static const EnumValue kRoofCutBodyTypes[]     = { { "ContoursDown", 2 }, { "ReferenceLineDown", 3 } };
static const EnumValue kRoofEaveOverhangTypes[] = { { "Offset", 1 }, { "Manual", 2 } };
static const EnumValue kRoofEdgeAngles[]       = { { "Vertical", 0 }, { "Perpendicular", 1 }, { "Custom", 3 } };
static const EnumValue kRoofGeometryTypes[]    = { { "SinglePanel", 1 }, { "MultiplePanels", 2 } };
static const EnumValue kShellCutBodyTypes[]    = { { "Editable", 1 }, { "ExtrudeUp", 4 }, { "ExtrudeDown", 5 } };
static const EnumValue kShellGeometryTypes[]   = { { "Extrude", 1 }, { "Revolved", 2 }, { "Ruled", 3 } };

// An enum stored as a GuidVariant: each name's fixed built-in value guid.
struct GuidEnumValue {
    const char* name;
    const char* guid;
};

static const GuidEnumValue kStructuralFunctionValues[] = {
    { "LoadBearing",    "C852BD7D-1198-2802-7E37-E4925B997B8B" },
    { "NonLoadBearing", "539EA1F2-E319-7D11-7CAC-CCAA638FAD6E" },
    { "Undefined",      "CCCC2E1F-54E6-8534-EEA0-F571386498E1" }
};
static const GuidEnumValue kPositionValues[] = {
    { "Exterior",  "6F76AEB9-1C41-7E76-5254-57399CBF985B" },
    { "Interior",  "512CBDAC-06A3-A73D-ED46-1701A8BCE536" },
    { "Undefined", "677677E5-E514-E73F-39B9-E7AFC2031846" }
};
static const GuidEnumValue kRenovationStatusValues[] = {
    { "Existing",       kRenovationExistingGuid },
    { "ToBeDemolished", kRenovationToBeDemolishedGuid },
    { "New",            kRenovationNewGuid }
};
// "Source de lien": the one value the editor offered, "Toute source de lien" (any link source).
static const GuidEnumValue kLinkSourceValues[] = { { "Any", "4BD63BB7-0A82-4680-B6A7-708DC0AF0EC2" } };

static const char* FindGuidEnumValue (const GS::UniString& name, const GuidEnumValue* values, int count)
{
    for (int i = 0; i < count; ++i) {
        if (name == values[i].name)
            return values[i].guid;
    }
    return nullptr;
}

static GS::UniString NumPropertyCriterionXML (const char* propertyGuid, double value, int numOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"NumVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%.15g", value) + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString IntPropertyCriterionXML (const char* propertyGuid, int value, int numOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"IntVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%d", value) + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString ModelElementLeafXML ()
{
    return GS::UniString ("<ClassGuid>") + kModelElemClassGuid + "</ClassGuid>"
         + "<ModelElementCriterion Mv=\"2\" Sv=\"0\"><LogicalOperator>1</LogicalOperator></ModelElementCriterion>";
}

static GS::UniString DrawingElementLeafXML ()
{
    return GS::UniString ("<ClassGuid>") + kDrawingElemClassGuid + "</ClassGuid>"
         + "<DrawingElementCriteiron Mv=\"1\" Sv=\"0\"><LogicalOperator>1</LogicalOperator></DrawingElementCriteiron>";
}

// Tries each of the 6 numeric-comparison field name suffixes ("", "Not", "LessThan",
// "GreaterThan", "LessOrEqual", "GreaterOrEqual") for fieldPrefix against node; if one is present,
// emits the corresponding bare NumVariant PropertyCriterion into outXML and returns true (matched).
static bool TryEmitNumField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid = kPositioningPropertyGroupGuid)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (NumPropertyCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Same idea as TryEmitNumField, but for fields that store an IntVariant (DN/2eme DN in the MEP
// category) rather than a NumVariant - confirmed live these fail creation silently (rule accepted,
// criterion discarded) when sent as NumVariant, unlike every other numeric MEP field.
static bool TryEmitIntField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    int value = 0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Same idea as TryEmitNumField, for the 6-suffix string-comparison field family emitted as
// StringListVariant (Surface et Materiaux category).
static bool TryEmitStringListField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid = kSurfaceMaterialPropertyGroupGuid)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringListCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// A raw {"criterionXML": ...} leaf (see GraphicOverrideCriterionFromXML) that is an "is" element
// type, model element or drawing element condition, e.g. the type of a library part based tool
// whose ClassId is not known: it establishes the element type like elementType does.
static bool IsTypeEstablishingRawLeaf (const GS::UniString& fragmentXML)
{
    std::vector<CriterionXmlNode> nodes;
    if (!ReadCriterionXmlSequence (fragmentXML, nodes) || nodes.size () != 2 || nodes[0].name != "ClassGuid")
        return false;
    const CriterionXmlNode& leaf = nodes[1];
    if (leaf.name != "ElemTypeCriterion" && leaf.name != "ModelElementCriterion" && leaf.name != "DrawingElementCriteiron")
        return false;
    for (const CriterionXmlNode& child : leaf.children) {
        if (child.name == "LogicalOperator")
            return child.text == "1";
    }
    return false;
}

// A shallow check for whether a node directly establishes an element type/model-element-ness
// (elementType, all3DTypes, all2DTypes, or a raw leaf of that kind). elementTypeNot does not: it
// excludes a type without naming the ones that match.
static bool IsTypeEstablishingNode (const GS::ObjectState& node)
{
    GS::UniString dummyStr;
    bool dummyBool = false;
    return node.Get ("elementType", dummyStr) ||
           (node.Get ("all3DTypes", dummyBool) && dummyBool) ||
           (node.Get ("all2DTypes", dummyBool) && dummyBool) ||
           (node.Get ("criterionXML", dummyStr) && IsTypeEstablishingRawLeaf (dummyStr));
}

// Checks whether a type-establishing node is reachable from the given node by following ONLY
// "and" edges (never descending into "or"). This intentionally does NOT mean "is there a
// type-establishing node anywhere in the tree": one sitting inside a single "or" branch does not
// make a shared anchor available to that branch's siblings (each still needs its own, since only
// one of the OR's branches is guaranteed to hold) - it only helps when it's on every path a
// candidate element must satisfy simultaneously, i.e. reachable purely through "and". Confirmed
// live: a rule combining "classificationNot" OR "elementType" failed to create at all when the
// generator (wrongly, in an earlier version) treated the elementType inside the "or" as covering
// the whole rule and skipped injecting a shared anchor.
// An "or" all of whose branches hold an anchor holds one too: every element it matches has one of
// the types. This is the editor's usual "Element Type is Slab or Roof or Stair" row (issue #1816),
// which would otherwise get a second, needless "3D types" row when read back and written again.
static bool HasAndReachableAnchor (const GS::ObjectState& node)
{
    if (IsTypeEstablishingNode (node))
        return true;
    GS::Array<GS::ObjectState> andChildren, orChildren;
    if (node.Get ("and", andChildren)) {
        for (const GS::ObjectState& child : andChildren) {
            if (HasAndReachableAnchor (child))
                return true;
        }
    } else if (node.Get ("or", orChildren) && !orChildren.IsEmpty ()) {
        for (const GS::ObjectState& child : orChildren) {
            if (!HasAndReachableAnchor (child))
                return false;
        }
        return true;
    }
    return false;
}

// Stylo Texte and Police are 2D annotation (Text/Label) properties, not applicable to 3D model
// elements - confirmed live that pairing them with the ModelElementCriterion ("Type d'element est
// Types 3D") anchor used for every other bare property leaf produces a semantically invalid rule
// (a Text-pen-only rule silently accepted "Types 3D AND Stylo Texte" without erroring, but that
// combination can never usefully match anything real, per direct user confirmation). Police is
// stricter still: ArchiCAD outright REJECTS creating a rule pairing it with Types 3D (confirmed
// live, APIERR-level failure) - it needs the "Types 2D" (DrawingElementCriteiron) anchor instead
// when otherwise unanchored, whereas Stylo Texte needs no anchor at all.
static bool IsTextPenNode (const GS::ObjectState& node)
{
    int dummyInt = 0;
    return node.Get ("textPenIs", dummyInt) || node.Get ("textPenContains", dummyInt) || node.Get ("textPenNotContains", dummyInt);
}

static bool IsFontNode (const GS::ObjectState& node)
{
    GS::UniString dummyStr;
    return node.Get ("fontIs", dummyStr) || node.Get ("fontContains", dummyStr) || node.Get ("fontNotContains", dummyStr);
}

// Fields of text, dimension and hatch elements: like Stylo Texte and Police, they only apply to 2D
// drawing elements, so the default 3D-model-element anchor would make the rule match nothing.
static bool IsDrawingOnlyFieldNode (const GS::ObjectState& node)
{
    static const char* kDrawingOnlyFields[] = {
        "textAlignment", "textStyle", "textLineSpacing", "hatchCategory",
        "dimensionMarkerSize", "dimensionMarkerSizeNot", "dimensionMarkerSizeLessThan", "dimensionMarkerSizeGreaterThan",
        "dimensionMarkerSizeLessOrEqual", "dimensionMarkerSizeGreaterOrEqual",
        "dimensionMarkerPen", "dimensionMarkerPenContains", "dimensionPointerPenContains", "dimensionPointerLineTypeContains"
    };
    for (const char* field : kDrawingOnlyFields) {
        if (node.Contains (field))
            return true;
    }
    return false;
}

// True only if EVERY leaf reachable from node (through any and/or nesting) is a 2D-only field
// (Stylo Texte, Police and the fields of IsDrawingOnlyFieldNode) - the only kinds of leaf that
// don't need the default 3D-model-element anchor.
static bool AllLeavesAreTextPenOrFont (const GS::ObjectState& node)
{
    GS::Array<GS::ObjectState> children;
    if (node.Get ("and", children) || node.Get ("or", children)) {
        for (const GS::ObjectState& child : children) {
            if (!AllLeavesAreTextPenOrFont (child))
                return false;
        }
        return true;
    }
    return IsTextPenNode (node) || IsFontNode (node) || IsDrawingOnlyFieldNode (node);
}

// Every leaf is emitted wrapped in its trivial group (WrapAsTrivialGroup), which is what a row of
// the criteria editor's table holds when it has one value. Inside a row with several values, the
// editor keeps the values bare (issue #1816), so the and/or of a row takes the wrapping off again.
static bool UnwrapTrivialGroup (GS::UniString& xml)
{
    const std::string prefix = ToUtf8 (GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
                                       + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>1</LogicalOperator><CriteriaCount>1</CriteriaCount>");
    const std::string suffix = "</CompositeCriterion>";
    const std::string text = ToUtf8 (xml);
    if (text.size () < prefix.size () + suffix.size () || text.compare (0, prefix.size (), prefix) != 0 || text.compare (text.size () - suffix.size (), suffix.size (), suffix) != 0)
        return false;
    xml = FromUtf8 (text.substr (prefix.size (), text.size () - prefix.size () - suffix.size ()));
    return true;
}

static bool IsCompositeFragment (const GS::UniString& fragmentXML)
{
    std::vector<CriterionXmlNode> nodes;
    return ReadCriterionXmlSequence (fragmentXML, nodes) && nodes.size () == 2 && nodes[0].name == "ClassGuid" && nodes[1].name == "CompositeCriterion";
}

// outIsSelfRooting is set true only when outXML already represents a complete, real multi-item
// CompositeCriterion suitable to be CriterionExpression's root directly - see the comment on
// CriterionToXML for why this distinction is needed.
static bool EmitCriterionNode (const GS::ObjectState& node, GS::UniString& outXML, GS::UniString& outError, bool& outIsSelfRooting, int depth = 0)
{
    outIsSelfRooting = false;
    GS::Array<GS::ObjectState> andChildren, orChildren;
    GS::UniString elementType, elementTypeNot, classification, classificationNot;
    bool all3DTypes = false, all2DTypes = false;

    const bool hasAnd = node.Get ("and", andChildren);
    const bool hasOr  = !hasAnd && node.Get ("or", orChildren);
    if (hasAnd || hasOr) {
        // The nesting follows the criteria editor's table, as Archicad keeps it (read from rules
        // made in the editor, issue #1816): the top and/or holds the rows, a row is an and/or of its
        // values, and values joined with "and" inside an "or" row are one more group. A row's
        // values are bare leaves, not wrapped in their own trivial group (see UnwrapTrivialGroup).
        // An earlier version wrapped them, and the editor then showed the rule wrongly, which is
        // why nesting was rejected before. Nothing deeper than these three levels is written.
        if (depth > 2) {
            outError = "'and'/'or' can be nested at most three levels deep, like the rows of the criteria "
                       "editor: the rows, the values of a row, and values joined with 'and' within a row.";
            return false;
        }
        const bool isAnd = hasAnd;
        const GS::Array<GS::ObjectState>& children = isAnd ? andChildren : orChildren;
        if (children.IsEmpty ()) {
            outError = "'and'/'or' must contain at least one criterion.";
            return false;
        }
        GS::UniString childrenXML;
        for (const GS::ObjectState& child : children) {
            GS::UniString childXML;
            bool childIsSelfRooting = false;
            if (!EmitCriterionNode (child, childXML, outError, childIsSelfRooting, depth + 1))
                return false;
            const bool childIsGroup = child.Contains ("and") || child.Contains ("or");
            if (depth > 0 && !childIsGroup)
                UnwrapTrivialGroup (childXML);
            childrenXML += childXML;
        }
        outXML = GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
               + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>" + GS::UniString (isAnd ? "2" : "1") + "</LogicalOperator>"
               + "<CriteriaCount>" + GS::UniString::Printf ("%d", (int) children.GetSize ()) + "</CriteriaCount>"
               + childrenXML
               + "</CompositeCriterion>";
        // An explicit "and"/"or" already represents the full top-level criteria table by itself
        // (confirmed live: hand-correcting "ElemType AND property" produced a root CompositeCriterion
        // whose own CriteriaCount is 2 directly, not a root of 1 wrapping a nested group of 2) - so,
        // like the ModelElement-paired leaves, it needs no further wrapping when used as the root.
        outIsSelfRooting = true;
        return true;
    }

    // A condition the reader does not decode, written back as it was read (issue #1816).
    GS::UniString rawCriterionXML;
    if (node.Get ("criterionXML", rawCriterionXML)) {
        std::vector<CriterionXmlNode> nodes;
        if (!ReadCriterionXmlSequence (rawCriterionXML, nodes) || nodes.size () != 2 || nodes[0].name != "ClassGuid") {
            outError = "'criterionXML' in a criterion must be one condition as GetGraphicOverrideRules returns it: a <ClassGuid> followed by one element.";
            return false;
        }
        const GS::UniString fragmentXML = FromUtf8 (CanonicalXml (nodes[0]) + CanonicalXml (nodes[1]));
        outXML = IsCompositeFragment (fragmentXML) ? fragmentXML : WrapAsTrivialGroup (fragmentXML);
        return true;
    }

    const bool hasElementType = node.Get ("elementType", elementType);
    if (hasElementType || node.Get ("elementTypeNot", elementTypeNot)) {
        const GS::UniString& typeName = hasElementType ? elementType : elementTypeNot;
        GS::UniString mainGuid, revGuid;
        const GS::ObjectState* subtype = node.Get ("libraryPartSubtype");
        if (subtype != nullptr && (!subtype->Get ("mainGuid", mainGuid) || !subtype->Get ("revGuid", revGuid) || mainGuid.IsEmpty () || revGuid.IsEmpty ())) {
            outError = "'libraryPartSubtype' requires 'mainGuid' and 'revGuid'.";
            return false;
        }
        if (typeName == kAllTypesElemTypeName) {
            if (!hasElementType || subtype != nullptr) {
                outError = "'AllTypes' can only be used with 'elementType', without 'libraryPartSubtype'.";
                return false;
            }
            outXML = WrapAsTrivialGroup (AllTypesElemTypeCriterionXML ());
            return true;
        }
        const ElemTypeInfo* info = FindElemTypeInfo (typeName);
        if (info == nullptr) {
            outError = "Unknown or unsupported element type '" + typeName + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (ElemTypeCriterionXML (*info, !hasElementType, mainGuid, revGuid));
        return true;
    }

    {
        static const char* kClassificationFields[] = { "classification", "classificationNot", "classificationIs", "classificationIsNot", "classificationChildOf", "classificationNotChildOf" };
        static const int   kClassificationOps[]    = { 12,               14,                  0,                  1,                     13,                      15 };
        for (int i = 0; i < 6; ++i) {
            GS::UniString itemId;
            if (!node.Get (kClassificationFields[i], itemId)) {
                continue;
            }
            if (itemId.IsEmpty ()) {
                outError = GS::UniString ("'") + kClassificationFields[i] + "' needs a Classification System item ID. Use classificationHasValue or classificationHasNoValue to test any item of a system.";
                return false;
            }
            GS::UniString requestedSystem, classificationSystem;
            node.Get ("classificationSystem", requestedSystem);
            if (!ResolveClassificationSystemLabel (itemId, requestedSystem, classificationSystem, outError))
                return false;
            outXML = WrapAsTrivialGroup (ClassificationCriterionXML (itemId, kClassificationOps[i], classificationSystem));
            return true;
        }
        // "has a value" / "has no value" in a system: Archicad ignores the item for these operators.
        static const char* kValueFields[] = { "classificationHasValue", "classificationHasNoValue" };
        static const int   kValueOps[]    = { 10,                       11 };
        for (int i = 0; i < 2; ++i) {
            bool flag = false;
            if (!node.Get (kValueFields[i], flag)) {
                continue;
            }
            GS::UniString requestedSystem, classificationSystem;
            node.Get ("classificationSystem", requestedSystem);
            if (!ResolveClassificationSystemLabel (GS::UniString (), requestedSystem, classificationSystem, outError))
                return false;
            const GS::UniString anyItemId = FirstClassificationItemId (classificationSystem);
            if (anyItemId.IsEmpty ()) {
                outError = "The classification system '" + classificationSystem + "' has no item.";
                return false;
            }
            const int op = flag ? kValueOps[i] : kValueOps[1 - i];
            outXML = WrapAsTrivialGroup (ClassificationCriterionXML (anyItemId, op, classificationSystem));
            return true;
        }
    }

    if (node.Get ("all3DTypes", all3DTypes) && all3DTypes) {
        outXML = WrapAsTrivialGroup (ModelElementLeafXML ());
        return true;
    }
    if (node.Get ("all2DTypes", all2DTypes) && all2DTypes) {
        outXML = WrapAsTrivialGroup (DrawingElementLeafXML ());
        return true;
    }

    bool missingAttributes = false, missingLibraryPart = false, layerLocked = false, layerVisible = false;
    GS::UniString layer, layerNot, layerCombination, layerCombinationNot;
    GS::UniString layerNameIs, layerNameIsNot, layerNameContains, layerNameNotContains, layerNameStartsWith, layerNameEndsWith;

    if (node.Get ("missingAttributes", missingAttributes)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kMissingAttributesPropertyGuid, missingAttributes));
        return true;
    }
    if (node.Get ("missingLibraryPart", missingLibraryPart)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kMissingLibraryPartPropertyGuid, missingLibraryPart));
        return true;
    }
    if (node.Get ("layerLocked", layerLocked)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kLayerLockedPropertyGuid, layerLocked));
        return true;
    }
    if (node.Get ("layerVisible", layerVisible)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kLayerVisiblePropertyGuid, layerVisible));
        return true;
    }
    if (node.Get ("layer", layer) || node.Get ("layerNot", layerNot)) {
        const bool isNot = !layerNot.IsEmpty ();
        const GS::UniString& name = isNot ? layerNot : layer;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LayerID, name, idx)) {
            outError = "Unknown layer '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kLayerPropertyGuid, idx, isNot));
        return true;
    }
    if (node.Get ("layerCombination", layerCombination) || node.Get ("layerCombinationNot", layerCombinationNot)) {
        const bool isNot = !layerCombinationNot.IsEmpty ();
        const GS::UniString& name = isNot ? layerCombinationNot : layerCombination;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LayerCombID, name, idx)) {
            outError = "Unknown layer combination '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kLayerCombinationPropertyGuid, idx, isNot));
        return true;
    }
    if (node.Get ("layerNameIs", layerNameIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameIs, 0));
        return true;
    }
    if (node.Get ("layerNameIsNot", layerNameIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameIsNot, 1));
        return true;
    }
    if (node.Get ("layerNameContains", layerNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameContains, 6));
        return true;
    }
    if (node.Get ("layerNameNotContains", layerNameNotContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameNotContains, 7));
        return true;
    }
    if (node.Get ("layerNameStartsWith", layerNameStartsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameStartsWith, 8));
        return true;
    }
    if (node.Get ("layerNameEndsWith", layerNameEndsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameEndsWith, 9));
        return true;
    }

    GS::UniString compositeStructure, compositeStructureNot, complexProfile, complexProfileNot;
    GS::UniString compositeStructureNameIs, compositeStructureNameIsNot, compositeStructureNameContains, compositeStructureNameNotContains, compositeStructureNameStartsWith, compositeStructureNameEndsWith;
    GS::UniString complexProfileNameIs, complexProfileNameIsNot, complexProfileNameContains, complexProfileNameNotContains, complexProfileNameStartsWith, complexProfileNameEndsWith;
    bool roofConnected = false;
    GS::UniString structureType;

    if (node.Get ("compositeStructure", compositeStructure) || node.Get ("compositeStructureNot", compositeStructureNot)) {
        const bool isNot = !compositeStructureNot.IsEmpty ();
        const GS::UniString& name = isNot ? compositeStructureNot : compositeStructure;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_CompWallID, name, idx)) {
            outError = "Unknown composite structure '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kCompositeStructurePropertyGuid, idx, isNot, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfile", complexProfile) || node.Get ("complexProfileNot", complexProfileNot)) {
        const bool isNot = !complexProfileNot.IsEmpty ();
        const GS::UniString& name = isNot ? complexProfileNot : complexProfile;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_ProfileID, name, idx)) {
            outError = "Unknown complex profile '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kComplexProfilePropertyGuid, idx, isNot, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameIs", compositeStructureNameIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameIs, 0, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameIsNot", compositeStructureNameIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameIsNot, 1, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameContains", compositeStructureNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameContains, 6, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameNotContains", compositeStructureNameNotContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameNotContains, 7, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameStartsWith", compositeStructureNameStartsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameStartsWith, 8, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameEndsWith", compositeStructureNameEndsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameEndsWith, 9, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameIs", complexProfileNameIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameIs, 0, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameIsNot", complexProfileNameIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameIsNot, 1, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameContains", complexProfileNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameContains, 6, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameNotContains", complexProfileNameNotContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameNotContains, 7, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameStartsWith", complexProfileNameStartsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameStartsWith, 8, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameEndsWith", complexProfileNameEndsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameEndsWith, 9, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("roofConnected", roofConnected)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kRoofConnectedPropertyGuid, roofConnected, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("structureType", structureType)) {
        int value = 0;
        if (structureType == "Basic")               value = 1;
        else if (structureType == "Composite")       value = 2;
        else if (structureType == "ComplexProfile")  value = 3;
        else {
            outError = "Invalid structureType '" + structureType + "'. Must be 'Basic', 'Composite', or 'ComplexProfile'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kStructureTypePropertyGuid, value, kConstructionPropertyGroupGuid));
        return true;
    }

    if (TryEmitNumField (node, "seaLevelAltitude", kSeaLevelAltitudePropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "projectZeroAltitude", kProjectZeroAltitudePropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "groundFloorAltitude", kGroundFloorAltitudePropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "roofAltitude", kRoofAltitudePropertyGuid, outXML))
        return true;
    int floorIndex = 0;
    if (node.Get ("floorIndex", floorIndex)) {
        // Uses IntPropertyCriterionXML (Value-wrapped Variant), not IntEqualsPropertyCriterionXML -
        // confirmed live that ArchiCAD rejects (silently reduces to a match-all rule) the unwrapped
        // form for this particular field, unlike every other IntEqualsPropertyCriterionXML user.
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kPlacementStoryPropertyGuid, floorIndex, 0, kPositioningPropertyGroupGuid));
        return true;
    }

    if (TryEmitNumField (node, "mepDiameter", kMepDiameterPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepDiameter2", kMepDiameter2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitIntField (node, "mepDN", kMepDNPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitIntField (node, "mepDN2", kMepDN2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepHeight", kMepHeightPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepHeight2", kMepHeight2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepWidth", kMepWidthPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepWidth2", kMepWidth2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepAngle", kMepAnglePropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepCustomAngle", kMepCustomAnglePropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepInsulationThickness", kMepInsulationThicknessPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepLength", kMepLengthPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitStringField6Ops (node, "mepDescription", kMepDescriptionPropertyGuid, kMepPropertyGroupGuid, outXML))
        return true;
    if (TryEmitStringField6Ops (node, "mepMaterialName", kMepMaterialNamePropertyGuid, kMepPropertyGroupGuid, outXML))
        return true;

    GS::UniString wallComplexity, wallComplexityNot;
    if (node.Get ("wallComplexity", wallComplexity) || node.Get ("wallComplexityNot", wallComplexityNot)) {
        const bool isNot = !wallComplexityNot.IsEmpty ();
        const GS::UniString& s = isNot ? wallComplexityNot : wallComplexity;
        int value = 0;
        if (s == "Straight")            value = 0;
        else if (s == "Slanted")        value = 3;
        else if (s == "DoubleSlanted")  value = 5;
        else {
            outError = "Invalid wallComplexity/wallComplexityNot '" + s + "'. Must be 'Straight', 'Slanted', or 'DoubleSlanted'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kWallComplexityPropertyGuid, value, kConstructionPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "wallLevelOffset", kWallLevelOffsetPropertyGuid, outXML, kPositioningPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallTopOffset", kWallTopOffsetPropertyGuid, outXML, kPositioningPropertyGroupGuid))
        return true;
    GS::UniString wallRefLinePos, wallRefLinePosNot;
    if (node.Get ("wallReferenceLinePosition", wallRefLinePos) || node.Get ("wallReferenceLinePositionNot", wallRefLinePosNot)) {
        const bool isNot = !wallRefLinePosNot.IsEmpty ();
        const GS::UniString& s = isNot ? wallRefLinePosNot : wallRefLinePos;
        int value = 0;
        if (s == "Outside")            value = 0;
        else if (s == "Center")        value = 1;
        else if (s == "Inside")        value = 2;
        else if (s == "CoreOutside")   value = 3;
        else if (s == "CoreCenter")    value = 4;
        else if (s == "CoreInside")    value = 5;
        else {
            outError = "Invalid wallReferenceLinePosition/wallReferenceLinePositionNot '" + s + "'. Must be 'Outside', 'Center', 'Inside', 'CoreOutside', 'CoreCenter', or 'CoreInside'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kWallReferenceLinePositionPropertyGuid, value, kPositioningPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "wallOutsideAngle", kWallOutsideAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallInsideAngle", kWallInsideAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallThickness", kWallThicknessPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallHeight", kWallHeightPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallProfileHeight", kWallProfileHeightPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    bool wallHeightReversed = false;
    if (node.Get ("wallHeightReversed", wallHeightReversed)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kWallHeightReversedPropertyGuid, wallHeightReversed, kWallGeometryGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "wallWidth", kWallWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallProfileWidth", kWallProfileWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "objectLength", kObjectLengthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString wallGeometryType, wallGeometryTypeNot;
    if (node.Get ("wallGeometryType", wallGeometryType) || node.Get ("wallGeometryTypeNot", wallGeometryTypeNot)) {
        const bool isNot = !wallGeometryTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? wallGeometryTypeNot : wallGeometryType;
        int value = 0;
        if (s == "Uniform")         value = 0;
        else if (s == "Trapezoid")  value = 1;
        else if (s == "Polygonal")  value = 3;
        else {
            outError = "Invalid wallGeometryType/wallGeometryTypeNot '" + s + "'. Must be 'Uniform', 'Trapezoid', or 'Polygonal'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kWallGeometryTypePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitRealAttributeField (node, "wallReferenceLineOffset", kWallReferenceLineOffsetAttrGuid, outXML))
        return true;

    GS::UniString wallTopLine, wallTopLineNot, wallCutLine, wallCutLineNot, wallUncutLine, wallUncutLineNot;
    if (node.Get ("wallTopLine", wallTopLine) || node.Get ("wallTopLineNot", wallTopLineNot)) {
        const bool isNot = !wallTopLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallTopLineNot : wallTopLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallTopLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallCutLine", wallCutLine) || node.Get ("wallCutLineNot", wallCutLineNot)) {
        const bool isNot = !wallCutLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallCutLineNot : wallCutLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallCutLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallUncutLine", wallUncutLine) || node.Get ("wallUncutLineNot", wallUncutLineNot)) {
        const bool isNot = !wallUncutLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallUncutLineNot : wallUncutLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallUncutLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallHatchForegroundPen = 0, wallHatchForegroundPenNot = 0;
    if (node.Get ("wallHatchForegroundPen", wallHatchForegroundPen) || node.Get ("wallHatchForegroundPenNot", wallHatchForegroundPenNot)) {
        const bool isNot = node.Get ("wallHatchForegroundPenNot", wallHatchForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallHatchForegroundPenPropertyGuid, isNot ? wallHatchForegroundPenNot : wallHatchForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallContourPen = 0, wallContourPenNot = 0;
    if (node.Get ("wallContourPen", wallContourPen) || node.Get ("wallContourPenNot", wallContourPenNot)) {
        const bool isNot = node.Get ("wallContourPenNot", wallContourPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallContourPenPropertyGuid, isNot ? wallContourPenNot : wallContourPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallTopLinePen = 0, wallTopLinePenNot = 0;
    if (node.Get ("wallTopLinePen", wallTopLinePen) || node.Get ("wallTopLinePenNot", wallTopLinePenNot)) {
        const bool isNot = node.Get ("wallTopLinePenNot", wallTopLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallTopLinePenPropertyGuid, isNot ? wallTopLinePenNot : wallTopLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallCutHatchBackgroundPen = 0, wallCutHatchBackgroundPenNot = 0;
    if (node.Get ("wallCutHatchBackgroundPen", wallCutHatchBackgroundPen) || node.Get ("wallCutHatchBackgroundPenNot", wallCutHatchBackgroundPenNot)) {
        const bool isNot = node.Get ("wallCutHatchBackgroundPenNot", wallCutHatchBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallCutHatchBackgroundPenPropertyGuid, isNot ? wallCutHatchBackgroundPenNot : wallCutHatchBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallHatchBackgroundPen = 0, wallHatchBackgroundPenNot = 0;
    if (node.Get ("wallHatchBackgroundPen", wallHatchBackgroundPen) || node.Get ("wallHatchBackgroundPenNot", wallHatchBackgroundPenNot)) {
        const bool isNot = node.Get ("wallHatchBackgroundPenNot", wallHatchBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallHatchBackgroundPenPropertyGuid, isNot ? wallHatchBackgroundPenNot : wallHatchBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallUncutLinePen = 0, wallUncutLinePenNot = 0;
    if (node.Get ("wallUncutLinePen", wallUncutLinePen) || node.Get ("wallUncutLinePenNot", wallUncutLinePenNot)) {
        const bool isNot = node.Get ("wallUncutLinePenNot", wallUncutLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallUncutLinePenPropertyGuid, isNot ? wallUncutLinePenNot : wallUncutLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString wallOutsideSurface, wallOutsideSurfaceNot, wallInsideSurface, wallInsideSurfaceNot, wallSideSurface, wallSideSurfaceNot;
    if (node.Get ("wallOutsideSurface", wallOutsideSurface) || node.Get ("wallOutsideSurfaceNot", wallOutsideSurfaceNot)) {
        const bool isNot = !wallOutsideSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallOutsideSurfaceNot : wallOutsideSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallOutsideSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallInsideSurface", wallInsideSurface) || node.Get ("wallInsideSurfaceNot", wallInsideSurfaceNot)) {
        const bool isNot = !wallInsideSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallInsideSurfaceNot : wallInsideSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallInsideSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallSideSurface", wallSideSurface) || node.Get ("wallSideSurfaceNot", wallSideSurfaceNot)) {
        const bool isNot = !wallSideSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallSideSurfaceNot : wallSideSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallSideSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (TryEmitStringField6Ops (node, "wallParentId", kWallParentIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringListField (node, "wallConnectedOpeningIds", kWallConnectedOpeningIdsPropertyGuid, outXML, kIdCategoryGroupGuid3))
        return true;
    GS::UniString columnCoatingType, columnCoatingTypeNot;
    if (node.Get ("columnCoatingType", columnCoatingType) || node.Get ("columnCoatingTypeNot", columnCoatingTypeNot)) {
        const bool isNot = !columnCoatingTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? columnCoatingTypeNot : columnCoatingType;
        int value = 0;
        if (s == "Core")         value = 0;
        else if (s == "Finish")  value = 1;
        else if (s == "Other")   value = 2;
        else {
            outError = "Invalid columnCoatingType/columnCoatingTypeNot '" + s + "'. Must be 'Core', 'Finish', or 'Other'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kColumnCoatingTypePropertyGuid, value, kConstructionPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "columnInclinationAngle", kColumnInclinationAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString columnCrossSectionType;
    if (node.Get ("columnCrossSectionType", columnCrossSectionType)) {
        int value = 0;
        if (columnCrossSectionType == "Rectangular")     value = 2;
        else if (columnCrossSectionType == "Circular")   value = 1;
        else if (columnCrossSectionType == "Profiled")   value = 3;
        else {
            outError = "Invalid columnCrossSectionType '" + columnCrossSectionType + "'. Must be 'Rectangular', 'Circular', or 'Profiled'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kColumnCrossSectionTypePropertyGuid, value, kWallGeometryGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "columnCoreDiameter", kColumnCoreDiameterPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnCoatingThickness", kColumnCoatingThicknessPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionHeightCut", kColumnBottomCrossSectionHeightCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionHeightPerpendicular", kColumnBottomCrossSectionHeightPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionHeightCut", kColumnTopCrossSectionHeightCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionHeightPerpendicular", kColumnTopCrossSectionHeightPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnProfileCoreHeight", kColumnProfileCoreHeightPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnCoreHeightDiameterRatio", kColumnCoreHeightDiameterRatioPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionWidthCut", kColumnBottomCrossSectionWidthCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionWidthPerpendicular", kColumnBottomCrossSectionWidthPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionWidthCut", kColumnTopCrossSectionWidthCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionWidthPerpendicular", kColumnTopCrossSectionWidthPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnCoreWidth", kColumnCoreWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnProfileCoreWidth", kColumnProfileCoreWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString columnCoveringHatch, columnCoveringHatchNot, columnCoatingHatch, columnCoatingHatchNot;
    if (node.Get ("columnCoveringHatch", columnCoveringHatch) || node.Get ("columnCoveringHatchNot", columnCoveringHatchNot)) {
        const bool isNot = !columnCoveringHatchNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoveringHatchNot : columnCoveringHatch;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoveringHatchPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("columnCoatingHatch", columnCoatingHatch) || node.Get ("columnCoatingHatchNot", columnCoatingHatchNot)) {
        const bool isNot = !columnCoatingHatchNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoatingHatchNot : columnCoatingHatch;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoatingHatchPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoreForegroundPen = 0, columnCoreForegroundPenNot = 0;
    if (node.Get ("columnCoreForegroundPen", columnCoreForegroundPen) || node.Get ("columnCoreForegroundPenNot", columnCoreForegroundPenNot)) {
        const bool isNot = node.Get ("columnCoreForegroundPenNot", columnCoreForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoreForegroundPenPropertyGuid, isNot ? columnCoreForegroundPenNot : columnCoreForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoatingForegroundPen = 0, columnCoatingForegroundPenNot = 0;
    if (node.Get ("columnCoatingForegroundPen", columnCoatingForegroundPen) || node.Get ("columnCoatingForegroundPenNot", columnCoatingForegroundPenNot)) {
        const bool isNot = node.Get ("columnCoatingForegroundPenNot", columnCoatingForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoatingForegroundPenPropertyGuid, isNot ? columnCoatingForegroundPenNot : columnCoatingForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoveringHatchForegroundPen = 0, columnCoveringHatchForegroundPenNot = 0;
    if (node.Get ("columnCoveringHatchForegroundPen", columnCoveringHatchForegroundPen) || node.Get ("columnCoveringHatchForegroundPenNot", columnCoveringHatchForegroundPenNot)) {
        const bool isNot = node.Get ("columnCoveringHatchForegroundPenNot", columnCoveringHatchForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoveringHatchForegroundPenPropertyGuid, isNot ? columnCoveringHatchForegroundPenNot : columnCoveringHatchForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoveringHatchBackgroundPen = 0, columnCoveringHatchBackgroundPenNot = 0;
    if (node.Get ("columnCoveringHatchBackgroundPen", columnCoveringHatchBackgroundPen) || node.Get ("columnCoveringHatchBackgroundPenNot", columnCoveringHatchBackgroundPenNot)) {
        const bool isNot = node.Get ("columnCoveringHatchBackgroundPenNot", columnCoveringHatchBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoveringHatchBackgroundPenPropertyGuid, isNot ? columnCoveringHatchBackgroundPenNot : columnCoveringHatchBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoatingBackgroundPen = 0, columnCoatingBackgroundPenNot = 0;
    if (node.Get ("columnCoatingBackgroundPen", columnCoatingBackgroundPen) || node.Get ("columnCoatingBackgroundPenNot", columnCoatingBackgroundPenNot)) {
        const bool isNot = node.Get ("columnCoatingBackgroundPenNot", columnCoatingBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoatingBackgroundPenPropertyGuid, isNot ? columnCoatingBackgroundPenNot : columnCoatingBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoreBackgroundPen = 0, columnCoreBackgroundPenNot = 0;
    if (node.Get ("columnCoreBackgroundPen", columnCoreBackgroundPen) || node.Get ("columnCoreBackgroundPenNot", columnCoreBackgroundPenNot)) {
        const bool isNot = node.Get ("columnCoreBackgroundPenNot", columnCoreBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoreBackgroundPenPropertyGuid, isNot ? columnCoreBackgroundPenNot : columnCoreBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnHiddenLinePen = 0, columnHiddenLinePenNot = 0;
    if (node.Get ("columnHiddenLinePen", columnHiddenLinePen) || node.Get ("columnHiddenLinePenNot", columnHiddenLinePenNot)) {
        const bool isNot = node.Get ("columnHiddenLinePenNot", columnHiddenLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnHiddenLinePenPropertyGuid, isNot ? columnHiddenLinePenNot : columnHiddenLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString columnHiddenLineType, columnHiddenLineTypeNot;
    if (node.Get ("columnHiddenLineType", columnHiddenLineType) || node.Get ("columnHiddenLineTypeNot", columnHiddenLineTypeNot)) {
        const bool isNot = !columnHiddenLineTypeNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnHiddenLineTypeNot : columnHiddenLineType;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnHiddenLineTypePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString columnCoreSurface, columnCoreSurfaceNot, columnCoatingSurface, columnCoatingSurfaceNot, columnExtrusionSurface, columnExtrusionSurfaceNot;
    if (node.Get ("columnCoreSurface", columnCoreSurface) || node.Get ("columnCoreSurfaceNot", columnCoreSurfaceNot)) {
        const bool isNot = !columnCoreSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoreSurfaceNot : columnCoreSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoreSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("columnCoatingSurface", columnCoatingSurface) || node.Get ("columnCoatingSurfaceNot", columnCoatingSurfaceNot)) {
        const bool isNot = !columnCoatingSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoatingSurfaceNot : columnCoatingSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoatingSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("columnExtrusionSurface", columnExtrusionSurface) || node.Get ("columnExtrusionSurfaceNot", columnExtrusionSurfaceNot)) {
        const bool isNot = !columnExtrusionSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnExtrusionSurfaceNot : columnExtrusionSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnExtrusionSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }

    int beamAxisPen = 0, beamAxisPenNot = 0;
    if (node.Get ("beamAxisPen", beamAxisPen) || node.Get ("beamAxisPenNot", beamAxisPenNot)) {
        const bool isNot = node.Get ("beamAxisPenNot", beamAxisPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kBeamAxisPenPropertyGuid, isNot ? beamAxisPenNot : beamAxisPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString beamAxisLineType, beamAxisLineTypeNot;
    if (node.Get ("beamAxisLineType", beamAxisLineType) || node.Get ("beamAxisLineTypeNot", beamAxisLineTypeNot)) {
        const bool isNot = !beamAxisLineTypeNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamAxisLineTypeNot : beamAxisLineType;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamAxisLineTypePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString beamEndSurface, beamEndSurfaceNot, beamRightSurface, beamRightSurfaceNot, beamLeftSurface, beamLeftSurfaceNot;
    GS::UniString beamBottomSurface, beamBottomSurfaceNot, beamTopSurface, beamTopSurfaceNot;
    if (node.Get ("beamEndSurface", beamEndSurface) || node.Get ("beamEndSurfaceNot", beamEndSurfaceNot)) {
        const bool isNot = !beamEndSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamEndSurfaceNot : beamEndSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamEndSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamRightSurface", beamRightSurface) || node.Get ("beamRightSurfaceNot", beamRightSurfaceNot)) {
        const bool isNot = !beamRightSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamRightSurfaceNot : beamRightSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamRightSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamLeftSurface", beamLeftSurface) || node.Get ("beamLeftSurfaceNot", beamLeftSurfaceNot)) {
        const bool isNot = !beamLeftSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamLeftSurfaceNot : beamLeftSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamLeftSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamBottomSurface", beamBottomSurface) || node.Get ("beamBottomSurfaceNot", beamBottomSurfaceNot)) {
        const bool isNot = !beamBottomSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamBottomSurfaceNot : beamBottomSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamBottomSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamTopSurface", beamTopSurface) || node.Get ("beamTopSurfaceNot", beamTopSurfaceNot)) {
        const bool isNot = !beamTopSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamTopSurfaceNot : beamTopSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamTopSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }

    GS::UniString slabRefPlaneLoc, slabRefPlaneLocNot;
    if (node.Get ("slabReferencePlaneLocation", slabRefPlaneLoc) || node.Get ("slabReferencePlaneLocationNot", slabRefPlaneLocNot)) {
        const bool isNot = !slabRefPlaneLocNot.IsEmpty ();
        const GS::UniString& s = isNot ? slabRefPlaneLocNot : slabRefPlaneLoc;
        int value = 0;
        if (s == "Top")             value = 0;
        else if (s == "CoreTop")    value = 1;
        else if (s == "CoreBottom") value = 2;
        else if (s == "Bottom")     value = 3;
        else {
            outError = "Invalid slabReferencePlaneLocation/slabReferencePlaneLocationNot '" + s + "'. Must be 'Top', 'CoreTop', 'CoreBottom', or 'Bottom'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kSlabReferencePlaneLocationPropertyGuid, value, kPositioningPropertyGroupGuid, isNot));
        return true;
    }
    GS::UniString slabEdgeAngle, slabEdgeAngleNot;
    if (node.Get ("slabEdgeAngle", slabEdgeAngle) || node.Get ("slabEdgeAngleNot", slabEdgeAngleNot)) {
        const bool isNot = !slabEdgeAngleNot.IsEmpty ();
        const GS::UniString& s = isNot ? slabEdgeAngleNot : slabEdgeAngle;
        int value = 0;
        if (s == "Vertical")     value = 0;
        else if (s == "Custom")  value = 3;
        else {
            outError = "Invalid slabEdgeAngle/slabEdgeAngleNot '" + s + "'. Must be 'Vertical' or 'Custom'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kSlabEdgeAnglePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "slabDefaultEdgeAngle", kSlabDefaultEdgeAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    int slabCutLinePen = 0, slabCutLinePenNot = 0;
    if (node.Get ("slabCutLinePen", slabCutLinePen) || node.Get ("slabCutLinePenNot", slabCutLinePenNot)) {
        const bool isNot = node.Get ("slabCutLinePenNot", slabCutLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kSlabCutLinePenPropertyGuid, isNot ? slabCutLinePenNot : slabCutLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString slabConstructionMaterialFill, slabConstructionMaterialFillNot;
    if (node.Get ("slabConstructionMaterialFill", slabConstructionMaterialFill) || node.Get ("slabConstructionMaterialFillNot", slabConstructionMaterialFillNot)) {
        const bool isNot = !slabConstructionMaterialFillNot.IsEmpty ();
        const GS::UniString& name = isNot ? slabConstructionMaterialFillNot : slabConstructionMaterialFill;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kSlabConstructionMaterialFillPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }

    GS::UniString roofCutBodyType, roofCutBodyTypeNot;
    if (node.Get ("roofCutBodyType", roofCutBodyType) || node.Get ("roofCutBodyTypeNot", roofCutBodyTypeNot)) {
        const bool isNot = !roofCutBodyTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? roofCutBodyTypeNot : roofCutBodyType;
        int value = 0;
        if (s == "ContoursDown")            value = 2;
        else if (s == "ReferenceLineDown")  value = 3;
        else {
            outError = "Invalid roofCutBodyType/roofCutBodyTypeNot '" + s + "'. Must be 'ContoursDown' or 'ReferenceLineDown'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofCutBodyTypePropertyGuid, value, kConstructionPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitIntField (node, "roofLevelNumber", kRoofLevelNumberPropertyGuid, outXML, kConstructionPropertyGroupGuid))
        return true;
    GS::UniString roofEaveOverhangType;
    if (node.Get ("roofEaveOverhangType", roofEaveOverhangType)) {
        int value = 0;
        if (roofEaveOverhangType == "Offset")      value = 1;
        else if (roofEaveOverhangType == "Manual") value = 2;
        else {
            outError = "Invalid roofEaveOverhangType '" + roofEaveOverhangType + "'. Must be 'Offset' or 'Manual'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofEaveOverhangTypePropertyGuid, value, kConstructionPropertyGroupGuid));
        return true;
    }
    GS::UniString roofEdgeAngle, roofEdgeAngleNot;
    if (node.Get ("roofEdgeAngle", roofEdgeAngle) || node.Get ("roofEdgeAngleNot", roofEdgeAngleNot)) {
        const bool isNot = !roofEdgeAngleNot.IsEmpty ();
        const GS::UniString& s = isNot ? roofEdgeAngleNot : roofEdgeAngle;
        int value = 0;
        if (s == "Vertical")           value = 0;
        else if (s == "Perpendicular") value = 1;
        else if (s == "Custom")        value = 3;
        else {
            outError = "Invalid roofEdgeAngle/roofEdgeAngleNot '" + s + "'. Must be 'Vertical', 'Perpendicular', or 'Custom'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofEdgeAnglePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "roofEaveOverhang", kRoofEaveOverhangPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString roofGeometryType, roofGeometryTypeNot;
    if (node.Get ("roofGeometryType", roofGeometryType) || node.Get ("roofGeometryTypeNot", roofGeometryTypeNot)) {
        const bool isNot = !roofGeometryTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? roofGeometryTypeNot : roofGeometryType;
        int value = 0;
        if (s == "SinglePanel")          value = 1;
        else if (s == "MultiplePanels")  value = 2;
        else {
            outError = "Invalid roofGeometryType/roofGeometryTypeNot '" + s + "'. Must be 'SinglePanel' or 'MultiplePanels'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofGeometryTypePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumListField (node, "roofPitch", kRoofPitchPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;

    GS::UniString shellCutBodyType;
    if (node.Get ("shellCutBodyType", shellCutBodyType)) {
        int value = 0;
        if (shellCutBodyType == "Editable")           value = 1;
        else if (shellCutBodyType == "ExtrudeUp")     value = 4;
        else if (shellCutBodyType == "ExtrudeDown")   value = 5;
        else {
            outError = "Invalid shellCutBodyType '" + shellCutBodyType + "'. Must be 'Editable', 'ExtrudeUp', or 'ExtrudeDown'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kShellCutBodyTypePropertyGuid, value, kConstructionPropertyGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "shellTiltAngle", kShellTiltAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString shellGeometryType;
    if (node.Get ("shellGeometryType", shellGeometryType)) {
        int value = 0;
        if (shellGeometryType == "Extrude")       value = 1;
        else if (shellGeometryType == "Revolved") value = 2;
        else if (shellGeometryType == "Ruled")    value = 3;
        else {
            outError = "Invalid shellGeometryType '" + shellGeometryType + "'. Must be 'Extrude', 'Revolved', or 'Ruled'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kShellGeometryTypePropertyGuid, value, kWallGeometryGroupGuid));
        return true;
    }

    GS::UniString hatchFill, hatchFillNot, lineAttribute, lineAttributeNot;
    GS::UniString fontIs, fontContains, fontNotContains;
    GS::UniString penReplacement, penReplacementNot;
    int linePenContains = 0, linePenNotContains = 0;
    int textPenIs = 0, textPenContains = 0, textPenNotContains = 0;

    if (node.Get ("hatchFill", hatchFill) || node.Get ("hatchFillNot", hatchFillNot)) {
        const bool isNot = !hatchFillNot.IsEmpty ();
        const GS::UniString& name = isNot ? hatchFillNot : hatchFill;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kHatchFillPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("lineAttribute", lineAttribute) || node.Get ("lineAttributeNot", lineAttributeNot)) {
        const bool isNot = !lineAttributeNot.IsEmpty ();
        const GS::UniString& name = isNot ? lineAttributeNot : lineAttribute;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kLineTypeAttrPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("fontIs", fontIs)) {
        const GS::Optional<short> idx = GetFontIndexFromName (fontIs);
        if (!idx.HasValue ()) {
            outError = "Unknown font '" + fontIs + "'. Give the name GetDetailsOfElements reports in fontName (e.g. 'Arial Western'), or a name GetFontsByName finds.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kFontPropertyGuid, *idx, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("fontContains", fontContains)) {
        const GS::Optional<short> idx = GetFontIndexFromName (fontContains);
        if (!idx.HasValue ()) {
            outError = "Unknown font '" + fontContains + "'. Give the name GetDetailsOfElements reports in fontName (e.g. 'Arial Western'), or a name GetFontsByName finds.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kFontPropertyGuid, *idx, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("fontNotContains", fontNotContains)) {
        const GS::Optional<short> idx = GetFontIndexFromName (fontNotContains);
        if (!idx.HasValue ()) {
            outError = "Unknown font '" + fontNotContains + "'. Give the name GetDetailsOfElements reports in fontName (e.g. 'Arial Western'), or a name GetFontsByName finds.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kFontPropertyGuid, *idx, 7, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("penReplacement", penReplacement) || node.Get ("penReplacementNot", penReplacementNot)) {
        const bool isNot = !penReplacementNot.IsEmpty ();
        const GS::UniString& name = isNot ? penReplacementNot : penReplacement;
        int value = 0;
        GS::UniString enumError;
        if (!FindEnumValue ("penReplacement", name, kPenReplacementValues, 4, value, enumError)) {
            outError = "Invalid penReplacement/penReplacementNot '" + name + "'. Must be 'None', 'ForegroundOnly', 'BackgroundOnly', or 'Both'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kPenReplacementPropertyGuid, value, kPlanSectionPropertyGroupGuid, isNot));
        return true;
    }
    if (node.Get ("linePenContains", linePenContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kLinePenPropertyGuid, linePenContains, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("linePenNotContains", linePenNotContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kLinePenPropertyGuid, linePenNotContains, 7, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("textPenIs", textPenIs)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextPenPropertyGuid, textPenIs, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("textPenContains", textPenContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextPenPropertyGuid, textPenContains, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("textPenNotContains", textPenNotContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextPenPropertyGuid, textPenNotContains, 7, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int hatchContourPen = 0, hatchContourPenNot = 0;
    if (node.Get ("hatchContourPen", hatchContourPen) || node.Get ("hatchContourPenNot", hatchContourPenNot)) {
        const bool isNot = node.Get ("hatchContourPenNot", hatchContourPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kHatchContourPenPropertyGuid, isNot ? hatchContourPenNot : hatchContourPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }

    GS::UniString constructionMaterial, constructionMaterialNot, surface, surfaceNot;

    if (node.Get ("constructionMaterial", constructionMaterial) || node.Get ("constructionMaterialNot", constructionMaterialNot)) {
        const bool isNot = !constructionMaterialNot.IsEmpty ();
        const GS::UniString& name = isNot ? constructionMaterialNot : constructionMaterial;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_BuildingMaterialID, name, idx)) {
            outError = "Unknown construction material '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kConstructionMaterialPropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("surface", surface) || node.Get ("surfaceNot", surfaceNot)) {
        const bool isNot = !surfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? surfaceNot : surface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (TryEmitStringListField (node, "constructionMaterialId", kConstructionMaterialIdPropertyGuid, outXML))
        return true;
    if (TryEmitStringListField (node, "constructionMaterialName", kConstructionMaterialNamePropertyGuid, outXML))
        return true;
    if (TryEmitStringListField (node, "surfaceName", kSurfaceNamePropertyGuid, outXML))
        return true;

    GS::UniString renovationFilterIs, renovationFilterIsNot;
    GS::UniString structuralFunction, structuralFunctionNot, position, positionNot;
    GS::UniString variantStateIs, variantStateIsNot, variantSetIs, variantSetIsNot;
    bool hasLinkSource = false, hasLinkSourceNot = false, missingVariant = false;

    if (node.Get ("renovationFilterIs", renovationFilterIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kRenovationFilterPropertyGuid, renovationFilterIs, 0, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("renovationFilterIsNot", renovationFilterIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kRenovationFilterPropertyGuid, renovationFilterIsNot, 1, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("structuralFunction", structuralFunction) || node.Get ("structuralFunctionNot", structuralFunctionNot)) {
        const bool isNot = !structuralFunctionNot.IsEmpty ();
        const GS::UniString& val = isNot ? structuralFunctionNot : structuralFunction;
        const char* valueGuid = FindGuidEnumValue (val, kStructuralFunctionValues, 3);
        if (valueGuid == nullptr) {
            outError = "Invalid structuralFunction/structuralFunctionNot '" + val + "'. Must be 'LoadBearing', 'NonLoadBearing', or 'Undefined'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kStructuralFunctionPropertyGuid, valueGuid, isNot, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("position", position) || node.Get ("positionNot", positionNot)) {
        const bool isNot = !positionNot.IsEmpty ();
        const GS::UniString& val = isNot ? positionNot : position;
        const char* valueGuid = FindGuidEnumValue (val, kPositionValues, 3);
        if (valueGuid == nullptr) {
            outError = "Invalid position/positionNot '" + val + "'. Must be 'Exterior', 'Interior', or 'Undefined'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kPositionPropertyGuid, valueGuid, isNot, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("variantStateIs", variantStateIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantStatePropertyGuid, variantStateIs, 0, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (node.Get ("variantStateIsNot", variantStateIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantStatePropertyGuid, variantStateIsNot, 1, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (node.Get ("variantSetIs", variantSetIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantSetPropertyGuid, variantSetIs, 0, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (node.Get ("variantSetIsNot", variantSetIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantSetPropertyGuid, variantSetIsNot, 1, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (TryEmitStringField10Ops (node, "elementId", kElementIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField6Ops (node, "uniqueId", kUniqueIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField10Ops (node, "linkMainId", kLinkMainIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField10Ops (node, "linkedElementId", kLinkedElementIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField6Ops (node, "name", kNamePropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    // "Source de lien" only ever showed a single fixed value ("Toute source de lien") in the
    // captured example - exposed as two presence-triggered booleans (matching all3DTypes/
    // all2DTypes' convention) around that one constant, rather than a real enum.
    if (node.Get ("hasLinkSource", hasLinkSource) && hasLinkSource) {
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kLinkSourcePropertyGuid, kLinkSourceValues[0].guid, false, kIdCategoryGroupGuid3));
        return true;
    }
    if (node.Get ("hasLinkSourceNot", hasLinkSourceNot) && hasLinkSourceNot) {
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kLinkSourcePropertyGuid, kLinkSourceValues[0].guid, true, kIdCategoryGroupGuid3));
        return true;
    }
    if (node.Get ("missingVariant", missingVariant)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kMissingVariantPropertyGuid, missingVariant, kIdCategoryGroupGuid4));
        return true;
    }

    if (TryEmitProfileParamField (node, "profileStretchHeight", kProfileStretchHeightGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileStretchWidth", kProfileStretchWidthGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileNominalHeight", kProfileNominalHeightGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileTotalHeight", kProfileTotalHeightGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileTotalWidth", kProfileTotalWidthGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileNominalWidth", kProfileNominalWidthGuid, outXML))
        return true;

    GS::ObjectState profileParameter;
    if (node.Get ("profileParameter", profileParameter)) {
        GS::UniString paramName;
        if (!profileParameter.Get ("name", paramName) || paramName.IsEmpty ()) {
            outError = "'profileParameter' requires a non-empty 'name'.";
            return false;
        }
        static const char* kSuffixes[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
        static const int   kOps[]      = { 0,         1,           2,          3,             4,             5 };
        double value = 0.0;
        bool matched = false;
        for (int i = 0; i < 6 && !matched; ++i) {
            if (profileParameter.Get (kSuffixes[i], value)) {
                outXML = WrapAsTrivialGroup (ProfileParameterCriterionXML (ProfileParameterHeaderByNameXML (paramName), value, kOps[i]));
                matched = true;
            }
        }
        if (!matched) {
            outError = "'profileParameter' requires exactly one of: equals, notEquals, lessThan, greaterThan, lessOrEqual, greaterOrEqual.";
            return false;
        }
        return true;
    }

    GS::ObjectState customPropertyBool, customPropertyString, customPropertyNumber, customPropertyInteger;
    if (node.Get ("customPropertyBool", customPropertyBool)) {
        GS::UniString name, group;
        if (!customPropertyBool.Get ("name", name) || !customPropertyBool.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyBool' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        bool eqValue = false;
        if (TryGetMetaOperator (customPropertyBool, metaOp)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, BoolVariantXML (false)));
        } else if (customPropertyBool.Get ("equals", eqValue)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, 0, BoolVariantXML (eqValue)));
        } else {
            outError = "'customPropertyBool' requires exactly one of: equals, hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }
    if (node.Get ("customPropertyString", customPropertyString)) {
        GS::UniString name, group, strValue;
        if (!customPropertyString.Get ("name", name) || !customPropertyString.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyString' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        if (TryGetMetaOperator (customPropertyString, metaOp)) {
            // See FindFirstOptionValueForCustomProperty: an option-list-backed property needs a real
            // option value as its (practically ignored) meta-operator placeholder, or ArchiCAD
            // silently discards the whole criterion. A free-text property has no such constraint, so
            // an empty placeholder is fine when the lookup finds no option list.
            GS::UniString placeholder;
            FindFirstOptionValueForCustomProperty (name, group, placeholder);
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, StringVariantXML (placeholder), true));
        } else if (customPropertyString.Get ("equals", strValue)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, 0, StringVariantXML (strValue), true));
        } else if (customPropertyString.Get ("notEquals", strValue)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, 1, StringVariantXML (strValue), true));
        } else {
            outError = "'customPropertyString' requires exactly one of: equals, notEquals, hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }
    if (node.Get ("customPropertyNumber", customPropertyNumber)) {
        GS::UniString name, group;
        if (!customPropertyNumber.Get ("name", name) || !customPropertyNumber.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyNumber' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        double numValue = 0.0;
        static const char* kNumSuffixes[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
        static const int   kNumOps[]      = { 0,         1,           2,          3,             4,             5 };
        bool matched = false;
        if (TryGetMetaOperator (customPropertyNumber, metaOp)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, NumVariantXML (0)));
            matched = true;
        } else {
            for (int i = 0; i < 6 && !matched; ++i) {
                if (customPropertyNumber.Get (kNumSuffixes[i], numValue)) {
                    outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, kNumOps[i], NumVariantXML (numValue)));
                    matched = true;
                }
            }
        }
        if (!matched) {
            outError = "'customPropertyNumber' requires exactly one of: equals, notEquals, lessThan, greaterThan, lessOrEqual, greaterOrEqual, "
                       "hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }
    if (node.Get ("customPropertyInteger", customPropertyInteger)) {
        GS::UniString name, group;
        if (!customPropertyInteger.Get ("name", name) || !customPropertyInteger.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyInteger' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        int intValue = 0;
        static const char* kIntSuffixes[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
        static const int   kIntOps[]      = { 0,         1,           2,          3,             4,             5 };
        bool matched = false;
        if (TryGetMetaOperator (customPropertyInteger, metaOp)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, IntVariantXML (0)));
            matched = true;
        } else {
            for (int i = 0; i < 6 && !matched; ++i) {
                if (customPropertyInteger.Get (kIntSuffixes[i], intValue)) {
                    outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, kIntOps[i], IntVariantXML (intValue)));
                    matched = true;
                }
            }
        }
        if (!matched) {
            outError = "'customPropertyInteger' requires exactly one of: equals, notEquals, lessThan, greaterThan, lessOrEqual, greaterOrEqual, "
                       "hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }

    if (TryEmitStringField6Ops (node, "libraryPartName", kLibraryPartNamePropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringListField (node, "subTypeName", kSubTypeNamePropertyGuid, outXML, kIdCategoryGroupGuid3))
        return true;
    if (TryEmitNumField (node, "sillHeight", kSillHeightPropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "lintelHeight", kLintelHeightPropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "screedThickness", kScreedThicknessPropertyGuid, outXML))
        return true;
    int linkedUpperStoryAbsolute = 0;
    if (node.Get ("linkedUpperStoryAbsolute", linkedUpperStoryAbsolute)) {
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kLinkedUpperStoryAbsolutePropertyGuid, linkedUpperStoryAbsolute, 0, kPositioningPropertyGroupGuid));
        return true;
    }
    int linkedUpperStoryRelative = 0;
    if (node.Get ("linkedUpperStoryRelative", linkedUpperStoryRelative)) {
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kLinkedUpperStoryRelativePropertyGuid, linkedUpperStoryRelative, 0, kPositioningPropertyGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "jambThickness1", kJambThickness1PropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "jambThickness2", kJambThickness2PropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString openingEndSurface, openingEndSurfaceNot;
    if (node.Get ("openingEndSurface", openingEndSurface) || node.Get ("openingEndSurfaceNot", openingEndSurfaceNot)) {
        const bool isNot = !openingEndSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? openingEndSurfaceNot : openingEndSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kOpeningEndSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    GS::UniString openingExtrusionSurface, openingExtrusionSurfaceNot;
    if (node.Get ("openingExtrusionSurface", openingExtrusionSurface) || node.Get ("openingExtrusionSurfaceNot", openingExtrusionSurfaceNot)) {
        const bool isNot = !openingExtrusionSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? openingExtrusionSurfaceNot : openingExtrusionSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kOpeningExtrusionSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "curtainWallPanelOffset", kCurtainWallPanelOffsetPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString curtainWallFrameCutLine, curtainWallFrameCutLineNot;
    if (node.Get ("curtainWallFrameCutLine", curtainWallFrameCutLine) || node.Get ("curtainWallFrameCutLineNot", curtainWallFrameCutLineNot)) {
        const bool isNot = !curtainWallFrameCutLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? curtainWallFrameCutLineNot : curtainWallFrameCutLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kCurtainWallFrameCutLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString curtainWallPanelCutLine, curtainWallPanelCutLineNot;
    if (node.Get ("curtainWallPanelCutLine", curtainWallPanelCutLine) || node.Get ("curtainWallPanelCutLineNot", curtainWallPanelCutLineNot)) {
        const bool isNot = !curtainWallPanelCutLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? curtainWallPanelCutLineNot : curtainWallPanelCutLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kCurtainWallPanelCutLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int curtainWallFrameCutLinePen = 0, curtainWallFrameCutLinePenNot = 0;
    if (node.Get ("curtainWallFrameCutLinePen", curtainWallFrameCutLinePen) || node.Get ("curtainWallFrameCutLinePenNot", curtainWallFrameCutLinePenNot)) {
        const bool isNot = node.Get ("curtainWallFrameCutLinePenNot", curtainWallFrameCutLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kCurtainWallFrameCutLinePenPropertyGuid, isNot ? curtainWallFrameCutLinePenNot : curtainWallFrameCutLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int curtainWallPanelCutLinePen = 0, curtainWallPanelCutLinePenNot = 0;
    if (node.Get ("curtainWallPanelCutLinePen", curtainWallPanelCutLinePen) || node.Get ("curtainWallPanelCutLinePenNot", curtainWallPanelCutLinePenNot)) {
        const bool isNot = node.Get ("curtainWallPanelCutLinePenNot", curtainWallPanelCutLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kCurtainWallPanelCutLinePenPropertyGuid, isNot ? curtainWallPanelCutLinePenNot : curtainWallPanelCutLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "stairRiserInclineAngle", kStairRiserInclineAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "stairMinHeadroom", kStairMinHeadroomPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    bool stairInvalidHeight = false;
    if (node.Get ("stairInvalidHeight", stairInvalidHeight)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kStairInvalidHeightPropertyGuid, stairInvalidHeight, kWallGeometryGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "stairDefaultWidth", kStairDefaultWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "stairSlope", kStairSlopePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "stairMaxRiserHeight", kStairMaxRiserHeightPropertyGuid, outXML, kStairRulesGroupGuid))
        return true;
    if (TryEmitNumField (node, "stairMinRiserHeight", kStairMinRiserHeightPropertyGuid, outXML, kStairRulesGroupGuid))
        return true;
    if (TryEmitNumField (node, "stairMaxGoing", kStairMaxGoingPropertyGuid, outXML, kStairRulesGroupGuid))
        return true;
    if (TryEmitNumField (node, "stairMinGoing", kStairMinGoingPropertyGuid, outXML, kStairRulesGroupGuid))
        return true;
    if (TryEmitIntField (node, "stairRiserCount", kStairRiserCountPropertyGuid, outXML, kStairRulesGroupGuid))
        return true;
    if (TryEmitIntField (node, "stairStepCount", kStairStepCountPropertyGuid, outXML, kStairRulesGroupGuid))
        return true;
    int stairStepCountPerFlight = 0;
    if (node.Get ("stairStepCountPerFlight", stairStepCountPerFlight)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kStairStepCountPerFlightPropertyGuid, stairStepCountPerFlight, 0, kStairRulesGroupGuid));
        return true;
    }

    // Enum fields stored as a bare IntVariant, "is" and, where the editor offers it, "is not".
    struct BareEnumField {
        const char*      field;
        const char*      notField;
        const char*      propertyGuid;
        const char*      groupGuid;
        const EnumValue* values;
        int              count;
    };
    static const BareEnumField kBareEnumFields[] = {
        { "morphBodyType",      "morphBodyTypeNot",      kMorphBodyTypePropertyGuid,        kConstructionPropertyGroupGuid, kMorphBodyTypes,      2 },
        { "beamCrossSectionType", nullptr,               kBeamCrossSectionTypePropertyGuid, kWallGeometryGroupGuid,         kBeamCrossSections,   3 },
        { "hatchCategory",      nullptr,                 kHatchCategoryPropertyGuid,        kPlanSectionPropertyGroupGuid,  kHatchCategories,     7 },
        { "openingShape",       "openingShapeNot",       kOpeningShapePropertyGuid,         kWallGeometryGroupGuid,         kOpeningShapes,       5 },
        { "openingLimits",      "openingLimitsNot",      kOpeningLimitsPropertyGuid,        kWallGeometryGroupGuid,         kOpeningLimits,       3 },
        { "openingOrientation", "openingOrientationNot", kOpeningOrientationPropertyGuid,   kWallGeometryGroupGuid,         kOpeningOrientations, 4 },
    };
    for (const BareEnumField& f : kBareEnumFields) {
        GS::UniString name;
        const bool isNot = f.notField != nullptr && node.Get (f.notField, name);
        if (!isNot && !node.Get (f.field, name)) {
            continue;
        }
        int value = 0;
        if (!FindEnumValue (f.field, name, f.values, f.count, value, outError))
            return false;
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (f.propertyGuid, value, f.groupGuid, isNot));
        return true;
    }

    GS::UniString textAlignment;
    if (node.Get ("textAlignment", textAlignment)) {
        int value = 0;
        if (!FindEnumValue ("textAlignment", textAlignment, kTextAlignments, 4, value, outError))
            return false;
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextAlignmentPropertyGuid, value, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    // The text style is one value made of flags; the rule matches texts with exactly these styles.
    GS::Array<GS::UniString> textStyle;
    if (node.Get ("textStyle", textStyle)) {
        int mask = 0;
        for (const GS::UniString& style : textStyle) {
            int bit = 0;
            if (!FindEnumValue ("textStyle", style, kTextStyles, 6, bit, outError))
                return false;
            mask |= bit;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextStylePropertyGuid, mask, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int textLineSpacing = 0;
    if (node.Get ("textLineSpacing", textLineSpacing)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextLineSpacingPropertyGuid, textLineSpacing, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }

    int meshEdgePen = 0;
    if (node.Get ("meshEdgePen", meshEdgePen)) {
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kMeshEdgePenPropertyGuid, meshEdgePen, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString meshSkirtSurface, meshSkirtSurfaceNot;
    if (node.Get ("meshSkirtSurface", meshSkirtSurface) || node.Get ("meshSkirtSurfaceNot", meshSkirtSurfaceNot)) {
        const bool isNot = !meshSkirtSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? meshSkirtSurfaceNot : meshSkirtSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kMeshSkirtSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }

    if (TryEmitNumField (node, "dimensionMarkerSize", kDimensionMarkerSizePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    int dimensionMarkerPen = 0;
    if (node.Get ("dimensionMarkerPen", dimensionMarkerPen)) {
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kDimensionMarkerPenPropertyGuid, dimensionMarkerPen, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int dimensionMarkerPenContains = 0;
    if (node.Get ("dimensionMarkerPenContains", dimensionMarkerPenContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kDimensionMarkerPenListPropertyGuid, dimensionMarkerPenContains, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int dimensionPointerPenContains = 0;
    if (node.Get ("dimensionPointerPenContains", dimensionPointerPenContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kDimensionPointerPenPropertyGuid, dimensionPointerPenContains, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString dimensionPointerLineTypeContains;
    if (node.Get ("dimensionPointerLineTypeContains", dimensionPointerLineTypeContains)) {
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, dimensionPointerLineTypeContains, idx)) {
            outError = "Unknown line type '" + dimensionPointerLineTypeContains + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kDimensionPointerLineTypePropertyGuid, idx, false, kPlanSectionPropertyGroupGuid));
        return true;
    }

    int stairRisersPerFlight = 0;
    if (node.Get ("stairRisersPerFlight", stairRisersPerFlight)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kStairRisersPerFlightPropertyGuid, stairRisersPerFlight, 0, kStairRulesGroupGuid));
        return true;
    }

    GS::UniString renovationStatus;
    if (node.Get ("renovationStatus", renovationStatus)) {
        const char* statusGuid = FindGuidEnumValue (renovationStatus, kRenovationStatusValues, 3);
        if (statusGuid == nullptr) {
            outError = "Invalid renovationStatus '" + renovationStatus + "'. Must be one of: Existing, ToBeDemolished, New.";
            return false;
        }
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kRenovationStatusPropertyGuid, statusGuid, false, kIdCategoryGroupGuid1, true));
        return true;
    }

    GS::UniString variantSetNameContains;
    if (node.Get ("variantSetNameContains", variantSetNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantSetNamePropertyGuid, variantSetNameContains, 6, kIdCategoryGroupGuid4));
        return true;
    }
    GS::UniString variantNameContains;
    if (node.Get ("variantNameContains", variantNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantNamePropertyGuid, variantNameContains, 6, kIdCategoryGroupGuid4));
        return true;
    }

    bool mepOpenConnection = false;
    if (node.Get ("mepOpenConnection", mepOpenConnection)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kMepOpenConnectionPropertyGuid, mepOpenConnection, kMepCommonPropertyGroupGuid));
        return true;
    }

    GS::ObjectState profileParameterList;
    if (node.Get ("profileParameterList", profileParameterList)) {
        GS::UniString paramName;
        double value = 0.0;
        if (!profileParameterList.Get ("name", paramName) || paramName.IsEmpty () || !profileParameterList.Get ("equals", value)) {
            outError = "'profileParameterList' requires a non-empty 'name' and 'equals'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (ProfileParameterListCriterionXML (paramName, value));
        return true;
    }

    outError = "A criterion node must contain exactly one of: and, or, criterionXML, elementType, elementTypeNot, classification, classificationNot, "
               "classificationIs, classificationIsNot, classificationChildOf, classificationNotChildOf, classificationHasValue, classificationHasNoValue, "
               "morphBodyType(Not), beamCrossSectionType, hatchCategory, openingShape(Not), openingLimits(Not), openingOrientation(Not), "
               "textAlignment, textStyle, textLineSpacing, meshEdgePen, meshSkirtSurface(Not), dimensionMarkerSize(...), dimensionMarkerPen, "
               "dimensionMarkerPenContains, dimensionPointerPenContains, dimensionPointerLineTypeContains, stairRisersPerFlight, renovationStatus, "
               "variantSetNameContains, variantNameContains, mepOpenConnection, profileParameterList, "
               "all3DTypes, all2DTypes, missingAttributes, missingLibraryPart, layer, layerNot, layerLocked, layerVisible, layerCombination, "
               "layerCombinationNot, layerNameIs, layerNameIsNot, layerNameContains, layerNameNotContains, layerNameStartsWith, layerNameEndsWith, "
               "compositeStructure, compositeStructureNot, complexProfile, complexProfileNot, compositeStructureNameIs, compositeStructureNameIsNot, "
               "compositeStructureNameContains, compositeStructureNameNotContains, compositeStructureNameStartsWith, compositeStructureNameEndsWith, "
               "complexProfileNameIs, complexProfileNameIsNot, complexProfileNameContains, complexProfileNameNotContains, complexProfileNameStartsWith, "
               "complexProfileNameEndsWith, roofConnected, structureType, seaLevelAltitude(Not/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual), "
               "projectZeroAltitude(...), groundFloorAltitude(...), roofAltitude(...), floorIndex, sillHeight(...), lintelHeight(...), "
               "screedThickness(...), linkedUpperStoryAbsolute, linkedUpperStoryRelative, libraryPartNameIs(/IsNot/Contains/NotContains/StartsWith/EndsWith), "
               "subTypeNameIs(...), hatchFill, hatchFillNot, lineAttribute, lineAttributeNot, "
               "fontIs, fontContains, fontNotContains, penReplacement, penReplacementNot, linePenContains, linePenNotContains, "
               "hatchContourPen, hatchContourPenNot, "
               "textPenIs, textPenContains, textPenNotContains, constructionMaterial, constructionMaterialNot, surface, surfaceNot, "
               "constructionMaterialId(Is/IsNot/Contains/NotContains/StartsWith/EndsWith), constructionMaterialName(...), surfaceName(...), "
               "renovationFilterIs, renovationFilterIsNot, structuralFunction, structuralFunctionNot, position, positionNot, "
               "variantStateIs, variantStateIsNot, variantSetIs, variantSetIsNot, elementId(Is/IsNot/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual/Contains/NotContains/StartsWith/EndsWith), "
               "uniqueId(Is/IsNot/Contains/NotContains/StartsWith/EndsWith), linkMainId(...10 ops...), linkedElementId(...10 ops...), name(...6 ops...), "
               "hasLinkSource, hasLinkSourceNot, missingVariant, profileStretchHeight(Not/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual), "
               "profileStretchWidth(...), profileNominalHeight(...), profileTotalHeight(...), profileTotalWidth(...), profileNominalWidth(...), "
               "profileParameter ({name, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual}), "
               "customPropertyBool ({name, group, equals|hasDefaultValue|hasCustomValue|isAvailable|isNotAvailable|isUndefined|isNotUndefined}), "
               "customPropertyString ({name, group, equals|notEquals|hasDefaultValue|...}), "
               "customPropertyNumber ({name, group, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual|hasDefaultValue|...}), "
               "customPropertyInteger ({name, group, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual|hasDefaultValue|...}), "
               "mepDiameter(Not/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual), mepDiameter2(...), mepDN(...), mepDN2(...), mepHeight(...), "
               "mepHeight2(...), mepWidth(...), mepWidth2(...), mepAngle(...), mepCustomAngle(...), mepInsulationThickness(...), mepLength(...), "
               "mepDescription(Is/IsNot/Contains/NotContains/StartsWith/EndsWith), mepMaterialName(...6 ops...).";
    return false;
}

// Generates a criterionXML string (in ArchiCAD's own reverse-engineered format) from a structured
// GraphicOverrideCriterion ObjectState. Returns false (with outError set) if the input uses an
// unsupported/unrecognized shape, rather than risk silently generating XML that matches nothing.
bool GraphicOverrideCriterionToXML (const GS::ObjectState& criterionOS, GS::UniString& outXML, GS::UniString& outError)
{
    // All property-based leaves (classification, layer*, missingAttributes, ...) are always emitted
    // bare now (no per-leaf ModelElementCriterion pairing) - a bare PropertyCriterion is rejected by
    // ArchiCAD only when there is no type-establishing condition (elementType/all3DTypes/all2DTypes)
    // ANYWHERE in the whole rule; when there is one, it doesn't matter how deeply nested it is
    // relative to the property leaves, one shared ModelElementCriterion (or the real ElemTypeCriterion
    // already present) is enough for the entire rule. Confirmed live: putting more than one
    // ModelElementCriterion ("Type d'element est Types 3D") in the same rule corrupts the criteria
    // editor's rendering of *other* rows in that rule (not just the duplicated one) - so this must be
    // injected at most once, globally, rather than once per property leaf as earlier versions did.
    // Stylo Texte/Police fields (see AllLeavesAreTextPenOrFont) never need the default 3D-model-
    // element anchor - confirmed live that BOTH need the "Types 2D" (DrawingElementCriteiron)
    // anchor instead when otherwise unanchored (Police outright fails pairing with Types 3D;
    // Stylo Texte fails with no anchor at all, even though it happened to succeed once in an
    // earlier, buggy version of this check that silently fell through to Types 3D by accident).
    const bool hasAnyAnchor        = HasAndReachableAnchor (criterionOS);
    const bool needsFont2DAnchor   = !hasAnyAnchor && AllLeavesAreTextPenOrFont (criterionOS);
    const bool hasAnchor = hasAnyAnchor;

    GS::UniString rootNodeXML;
    bool rootIsSelfRooting = false;
    if (!EmitCriterionNode (criterionOS, rootNodeXML, outError, rootIsSelfRooting))
        return false;

    GS::UniString rootCompositeXML;
    if (hasAnchor) {
        // CriterionExpression's single child is always a CompositeCriterion. A bare leaf (a single
        // elementType/all3DTypes/all2DTypes with no explicit and/or) represents just one "row" and
        // still needs one more wrapping level to become that root (confirmed against the DevKit's
        // single-ElemType example: root always has CriteriaCount=1 there). An explicit "and"/"or"
        // already returns a real, multi-item CompositeCriterion that IS the root by itself (confirmed
        // live, byte-for-byte, against a hand-corrected "ElemType AND property" rule) - wrapping it
        // again produced XML the criteria editor accepted without error but silently failed to render.
        rootCompositeXML = rootIsSelfRooting
            ? rootNodeXML
            : GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
              + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>2</LogicalOperator><CriteriaCount>1</CriteriaCount>"
              + rootNodeXML
              + "</CompositeCriterion>";
    } else {
        // No type-establishing condition anywhere - inject exactly one shared ModelElementCriterion,
        // AND-combined with the whole rest of the tree. rootNodeXML is always a self-contained
        // ClassGuid+Tag pair regardless of whether it was self-rooting, so it can be used directly as
        // the anchor's AND-sibling without any extra wrapping (confirmed byte-for-byte against the
        // real "Classification - X" rules, which are exactly this shape: root CompositeCriterion with
        // CriteriaCount=2 directly containing the ModelElement pair and the property leaf pair).
        rootCompositeXML = GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
            + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>2</LogicalOperator><CriteriaCount>2</CriteriaCount>"
            + WrapAsTrivialGroup (needsFont2DAnchor ? DrawingElementLeafXML () : ModelElementLeafXML ())
            + rootNodeXML
            + "</CompositeCriterion>";
    }

    outXML = GS::UniString ("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>")
           + "<CriterionExpression Mv=\"2\" Sv=\"2\"><TextMatchType>1</TextMatchType><Size>1</Size>"
           + rootCompositeXML
           + "</CriterionExpression>";
    return true;
}

// ---------------------------------------------------------------------------
// criterionXML -> structured criterion reader (issue #1816)
//
// The layout, read from rules made in the criteria editor of Archicad 29 and 30: CriterionExpression
// holds one root CompositeCriterion, an AND of the rows of the editor's table. A row is a
// CompositeCriterion of its values, OR or AND as the editor joins them, and values joined with "and"
// within an "or" row are one more CompositeCriterion. Each child is preceded by a ClassGuid naming
// its class. The reader returns this nesting one to one (a row with one value as its plain leaf),
// which GraphicOverrideCriterionToXML writes back the same way.
//
// A leaf is only returned by name when the generator writes it back to the same condition: the
// property conditions are checked by generating their XML and comparing it with what was read. A
// leaf that is not decoded, or not in the form the generator writes, comes back as a raw
// {"criterionXML": "<ClassGuid>...</ClassGuid><...>"} leaf, which the generator writes back as it is.
// ---------------------------------------------------------------------------

static const char* kClassificationGroupName = "ClassificationSystemPropertyDefinitionGroup";

static bool SameGuid (const std::string& guid, const char* expected)
{
    const size_t length = std::strlen (expected);
    if (guid.size () != length)
        return false;
    for (size_t i = 0; i < length; ++i) {
        char c = guid[i];
        if (c >= 'a' && c <= 'z')
            c = static_cast<char> (c - 'a' + 'A');
        if (c != expected[i])
            return false;
    }
    return true;
}

static const CriterionXmlNode* FindChild (const CriterionXmlNode& node, const char* name)
{
    for (const CriterionXmlNode& child : node.children) {
        if (child.name == name)
            return &child;
    }
    return nullptr;
}

static bool ParseInt (const std::string& text, int& outValue)
{
    if (text.empty ())
        return false;
    char* end = nullptr;
    const long value = std::strtol (text.c_str (), &end, 10);
    if (end == nullptr || *end != '\0')
        return false;
    outValue = static_cast<int> (value);
    return true;
}

static bool ParseDouble (const std::string& text, double& outValue)
{
    if (text.empty ())
        return false;
    char* end = nullptr;
    outValue = std::strtod (text.c_str (), &end);
    return end != nullptr && *end == '\0';
}

static GS::ObjectState RawCriterionLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf)
{
    return GS::ObjectState ("criterionXML", FromUtf8 (CanonicalXml (classGuid) + CanonicalXml (leaf)));
}

// Whether the generator writes exactly this ClassGuid and leaf for generatedXML.
static bool MatchesGenerated (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, const GS::UniString& generatedXML)
{
    std::vector<CriterionXmlNode> nodes;
    return ReadCriterionXmlSequence (generatedXML, nodes) && nodes.size () == 2
        && CanonicalXml (nodes[0]) == CanonicalXml (classGuid) && CanonicalXml (nodes[1]) == CanonicalXml (leaf);
}

static bool ReadElemTypeLeaf (const CriterionXmlNode& leaf, GS::ObjectState& outLeaf)
{
    if (leaf.children.size () != 2 || leaf.children[0].name != "VBEF::CritToolType" || leaf.children[1].name != "LogicalOperator")
        return false;
    const CriterionXmlNode& toolType = leaf.children[0];
    const std::string& logicalOperator = leaf.children[1].text;
    if (logicalOperator != "1" && logicalOperator != "2")
        return false;
    const bool isNot = logicalOperator == "2";

    if (toolType.children.size () != 2 || toolType.children[1].name != "ElemCreatorToolUnID")
        return false;
    const CriterionXmlNode& typeId = toolType.children[0];
    const CriterionXmlNode& unId = toolType.children[1];
    if (unId.children.size () != 2 || unId.children[0].name != "MainGuid" || unId.children[1].name != "RevGuid")
        return false;
    const std::string& mainGuid = unId.children[0].text;
    const std::string& revGuid = unId.children[1].text;

    const char* typeName = nullptr;
    if (typeId.name == "ElemType") {
        // Variation holds the API_ElemVariationID of a type that has one (e.g. a Grid Element is the
        // Object ClassId with APIVarId_GridElement); those are not decoded.
        if (typeId.children.size () != 2 || typeId.children[0].name != "ClassId" || typeId.children[1].name != "Variation" || typeId.children[1].text != "0")
            return false;
        const std::string& classId = typeId.children[0].text;
        if (SameGuid (classId, kZeroGuid)) {
            typeName = kAllTypesElemTypeName;
        } else {
            for (const ElemTypeClassIdInfo& info : kElemTypeClassIdTable) {
                if (SameGuid (classId, info.classId)) {
                    typeName = info.name;
                    break;
                }
            }
        }
    } else if (typeId.name == "ElemRegistryRefId") {
        for (const ElemTypeInfo& info : kElemTypeTable) {
            if (typeId.text == info.refId) {
                typeName = info.name;
                break;
            }
        }
    }
    if (typeName == nullptr)
        return false;

    const bool hasSubtype = !SameGuid (mainGuid, kZeroGuid) || !SameGuid (revGuid, kZeroGuid);
    const bool isAllTypes = std::strcmp (typeName, kAllTypesElemTypeName) == 0;
    if (isAllTypes && (isNot || hasSubtype))
        return false;
    // The generator writes a type by its ElemRegistryRefId, so a type without one (AllTypes aside)
    // cannot be written back.
    if (!isAllTypes && FindElemTypeInfo (typeName) == nullptr)
        return false;

    outLeaf = GS::ObjectState (isNot ? "elementTypeNot" : "elementType", GS::UniString (typeName));
    if (hasSubtype)
        outLeaf.Add ("libraryPartSubtype", GS::ObjectState ("mainGuid", FromUtf8 (mainGuid), "revGuid", FromUtf8 (revGuid)));
    return true;
}

static bool ReadClassificationLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, const GS::UniString& systemLabel, int op, const CriterionXmlNode& valueNode, GS::ObjectState& outLeaf)
{
    const CriterionXmlNode* itemNode = FindChild (valueNode, "ClassificationItemUserID");
    if (valueNode.name != "ClassificationValue" || valueNode.children.size () != 1 || itemNode == nullptr)
        return false;
    const GS::UniString itemId = FromUtf8 (itemNode->text);
    if (!MatchesGenerated (classGuid, leaf, ClassificationCriterionXML (itemId, op, systemLabel)))
        return false;

    // See the operator comment above ClassificationSystemLabel.
    static const int   kOps[]  = { 0,                  1,                     12,               13,                      14,                  15 };
    static const char* kKeys[] = { "classificationIs", "classificationIsNot", "classification", "classificationChildOf", "classificationNot", "classificationNotChildOf" };
    for (int i = 0; i < 6; ++i) {
        if (op == kOps[i]) {
            outLeaf = GS::ObjectState (kKeys[i], itemId, "classificationSystem", systemLabel);
            return true;
        }
    }
    if (op == 10 || op == 11) {
        // These ignore the item; the generator writes the system's first one.
        outLeaf = GS::ObjectState (op == 10 ? "classificationHasValue" : "classificationHasNoValue", true, "classificationSystem", systemLabel);
        return true;
    }
    return false;
}

// The availability operators every custom property has, see CustomPropertyHeaderXML.
static const char* MetaOperatorKey (int op)
{
    static const char* kKeys[] = { "hasDefaultValue", "hasCustomValue", "isAvailable", "isNotAvailable", "isUndefined", "isNotUndefined" };
    return (op >= 16 && op <= 21) ? kKeys[op - 16] : nullptr;
}

static const char* ComparisonOperatorKey (int op)
{
    static const char* kKeys[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
    return (op >= 0 && op <= 5) ? kKeys[op] : nullptr;
}

static bool ReadCustomPropertyLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, const GS::UniString& name, const GS::UniString& group, int op, const CriterionXmlNode& valueNode, GS::ObjectState& outLeaf)
{
    const bool bareVariant = valueNode.name == "Variant";
    const CriterionXmlNode* variant = bareVariant ? &valueNode : nullptr;
    if (valueNode.name == "Value" && valueNode.children.size () == 1 && valueNode.children[0].name == "Variant")
        variant = &valueNode.children[0];
    if (variant == nullptr || variant->children.size () != 2 || variant->children[0].name != "Status" || variant->children[1].name != "Value" || !variant->children[1].children.empty ())
        return false;
    const std::string& valueText = variant->children[1].text;
    const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
    const char* metaKey = MetaOperatorKey (op);

    GS::ObjectState condition ("name", name, "group", group);
    const char* conditionKey = nullptr;
    GS::UniString generatedXML;
    if (variant->attributes == "Type=\"BoolVariant\"" && !bareVariant) {
        if (valueText != "true" && valueText != "false")
            return false;
        const bool value = valueText == "true";
        if (metaKey == nullptr && op != 0)
            return false;
        conditionKey = "customPropertyBool";
        generatedXML = CustomPropertyCriterionXML (headerXML, op, BoolVariantXML (value));
        if (metaKey == nullptr)
            condition.Add ("equals", value);
    } else if (variant->attributes == "Type=\"StringVariant\"" && bareVariant) {
        // The generator writes a custom text property's value without the <Value> wrapper. The
        // editor writes some with it (issue #1816); which properties take which form is not known
        // yet, so those stay raw rather than be written back in the other form.
        if (metaKey == nullptr && op != 0 && op != 1)
            return false;
        const GS::UniString value = FromUtf8 (valueText);
        conditionKey = "customPropertyString";
        generatedXML = CustomPropertyCriterionXML (headerXML, op, StringVariantXML (value), true);
        if (metaKey == nullptr)
            condition.Add (op == 0 ? "equals" : "notEquals", value);
    } else if (variant->attributes == "Type=\"NumVariant\"" && !bareVariant) {
        double value = 0.0;
        if (!ParseDouble (valueText, value) || (metaKey == nullptr && ComparisonOperatorKey (op) == nullptr))
            return false;
        conditionKey = "customPropertyNumber";
        // Compared with the number as Archicad wrote it; written back, it is formatted with %.15g.
        generatedXML = CustomPropertyCriterionXML (headerXML, op, GS::UniString ("<Variant Type=\"NumVariant\"><Status>Normal</Status><Value>") + FromUtf8 (valueText) + "</Value></Variant>");
        if (metaKey == nullptr)
            condition.Add (ComparisonOperatorKey (op), value);
    } else if (variant->attributes == "Type=\"IntVariant\"" && !bareVariant) {
        int value = 0;
        if (!ParseInt (valueText, value) || (metaKey == nullptr && ComparisonOperatorKey (op) == nullptr))
            return false;
        conditionKey = "customPropertyInteger";
        generatedXML = CustomPropertyCriterionXML (headerXML, op, GS::UniString ("<Variant Type=\"IntVariant\"><Status>Normal</Status><Value>") + FromUtf8 (valueText) + "</Value></Variant>");
        if (metaKey == nullptr)
            condition.Add (ComparisonOperatorKey (op), static_cast<Int32> (value));
    } else {
        return false;
    }
    if (!MatchesGenerated (classGuid, leaf, generatedXML))
        return false;
    // The value of an availability operator is a placeholder, which the generator picks itself.
    if (metaKey != nullptr)
        condition.Add (metaKey, true);
    outLeaf = GS::ObjectState (conditionKey, condition);
    return true;
}

// The text of the one value of a condition: <Value><Variant> or a bare <Variant>, holding a <Value>
// or a <ValueArray> of one <ValueItem>. Its form is checked by comparing the generated XML.
static bool ReadSingleVariantValue (const CriterionXmlNode& valueNode, std::string& outText)
{
    const CriterionXmlNode* variant = nullptr;
    if (valueNode.name == "Variant")
        variant = &valueNode;
    else if (valueNode.name == "Value" && valueNode.children.size () == 1 && valueNode.children[0].name == "Variant")
        variant = &valueNode.children[0];
    if (variant == nullptr || variant->children.size () != 2 || variant->children[0].name != "Status")
        return false;
    const CriterionXmlNode& value = variant->children[1];
    if (value.name == "Value" && value.children.empty ()) {
        outText = value.text;
        return true;
    }
    if (value.name == "ValueArray" && value.children.size () == 1 && value.children[0].name == "ValueItem" && value.children[0].children.empty ()) {
        outText = value.children[0].text;
        return true;
    }
    return false;
}

// The name of the attribute with this index, when the generator resolves that name back to the same
// index (it takes the first attribute with the name).
static bool FindAttributeNameByIndex (API_AttrTypeID typeID, Int32 index, GS::UniString& outName)
{
    GS::Array<API_Attribute> attrs;
    ACAPI_Attribute_GetAttributesByType (typeID, attrs);
    bool found = false;
    for (API_Attribute& attr : attrs) {
        if (!found && GetAttributeIndex (attr.header.index) == index) {
            GS::UniString attrName;
            API_Attribute named = {};
            named.header.typeID = attr.header.typeID;
            named.header.index = attr.header.index;
            named.header.guid = attr.header.guid;
            named.header.uniStringNamePtr = &attrName;
            if (ACAPI_Attribute_Get (&named) == NoError && !attrName.IsEmpty ()) {
                outName = attrName;
                found = true;
            }
            DisposeAttribute (named);
        }
        DisposeAttribute (attr);
    }
    Int32 resolvedIndex = 0;
    return found && FindAttributeIndexByName (typeID, outName, resolvedIndex) && resolvedIndex == index;
}

// The built-in properties of the Layers, Model View and Construction categories (part 2 of #1816),
// each written by EmitCriterionNode with the same key, guid and group, and those of the Plan and
// Section and Surfaces and Materials categories that need an attribute or an enum (part 4).
enum class BuiltInFieldKind {
    Bool,           // key: true/false, "is"
    AttributeIs,    // key / keyNot (when notAllowed): an attribute name, "is" / "is not"
    AttributeHas,   // key / keyNot (when notAllowed): an attribute name, "contains" / "does not contain"
    String,         // key + Is, IsNot, Contains, NotContains, StartsWith, EndsWith: a text
    Enum,           // key / keyNot (when notAllowed): an enum name, "is" / "is not"
    ListEnum,       // key: an enum name, "is", stored as a list of one integer
    ListFlags       // key: a list of enum names, "is", stored as one integer of their flags
};

struct BuiltInPropertyField {
    const char*      key;
    BuiltInFieldKind kind;
    const char*      propertyGuid;
    const char*      groupGuid;
    API_AttrTypeID   attributeType;
    const EnumValue* values;
    int              valueCount;
    bool             notAllowed;
};

static const BuiltInPropertyField kBuiltInPropertyFields[] = {
    { "missingAttributes",      BuiltInFieldKind::Bool,         kMissingAttributesPropertyGuid,      kModelViewPropertyGroupGuid,    API_ZombieAttrID,  nullptr,         0, false },
    { "missingLibraryPart",     BuiltInFieldKind::Bool,         kMissingLibraryPartPropertyGuid,     kModelViewPropertyGroupGuid,    API_ZombieAttrID,  nullptr,         0, false },
    { "layerLocked",            BuiltInFieldKind::Bool,         kLayerLockedPropertyGuid,            kModelViewPropertyGroupGuid,    API_ZombieAttrID,  nullptr,         0, false },
    { "layerVisible",           BuiltInFieldKind::Bool,         kLayerVisiblePropertyGuid,           kModelViewPropertyGroupGuid,    API_ZombieAttrID,  nullptr,         0, false },
    { "layer",                  BuiltInFieldKind::AttributeIs,  kLayerPropertyGuid,                  kModelViewPropertyGroupGuid,    API_LayerID,       nullptr,         0, true  },
    { "layerCombination",       BuiltInFieldKind::AttributeHas, kLayerCombinationPropertyGuid,       kModelViewPropertyGroupGuid,    API_LayerCombID,   nullptr,         0, true  },
    { "layerName",              BuiltInFieldKind::String,       kLayerNamePropertyGuid,              kModelViewPropertyGroupGuid,    API_ZombieAttrID,  nullptr,         0, false },
    { "compositeStructure",     BuiltInFieldKind::AttributeIs,  kCompositeStructurePropertyGuid,     kConstructionPropertyGroupGuid, API_CompWallID,    nullptr,         0, true  },
    { "complexProfile",         BuiltInFieldKind::AttributeIs,  kComplexProfilePropertyGuid,         kConstructionPropertyGroupGuid, API_ProfileID,     nullptr,         0, true  },
    { "compositeStructureName", BuiltInFieldKind::String,       kCompositeStructureNamePropertyGuid, kConstructionPropertyGroupGuid, API_ZombieAttrID,  nullptr,         0, false },
    { "complexProfileName",     BuiltInFieldKind::String,       kComplexProfileNamePropertyGuid,     kConstructionPropertyGroupGuid, API_ZombieAttrID,  nullptr,         0, false },
    { "roofConnected",          BuiltInFieldKind::Bool,         kRoofConnectedPropertyGuid,          kConstructionPropertyGroupGuid, API_ZombieAttrID,  nullptr,         0, false },
    { "structureType",          BuiltInFieldKind::Enum,         kStructureTypePropertyGuid,          kConstructionPropertyGroupGuid, API_ZombieAttrID,  kStructureTypes, 3, false },
    { "morphBodyType",          BuiltInFieldKind::Enum,         kMorphBodyTypePropertyGuid,          kConstructionPropertyGroupGuid, API_ZombieAttrID,  kMorphBodyTypes, 2, true  },
    // ID and Categories (part 3 of #1816), the rest of which is in kBuiltInValueFields.
    { "missingVariant",         BuiltInFieldKind::Bool,         kMissingVariantPropertyGuid,         kIdCategoryGroupGuid4,          API_ZombieAttrID,  nullptr,         0, false },
    // Plan and Section (part 4 of #1816), the rest of which is in kBuiltInValueFields.
    { "hatchFill",              BuiltInFieldKind::AttributeHas, kHatchFillPropertyGuid,              kPlanSectionPropertyGroupGuid,  API_FilltypeID,    nullptr,         0, true  },
    { "lineAttribute",          BuiltInFieldKind::AttributeHas, kLineTypeAttrPropertyGuid,           kPlanSectionPropertyGroupGuid,  API_LinetypeID,    nullptr,         0, true  },
    { "dimensionPointerLineTypeContains", BuiltInFieldKind::AttributeHas, kDimensionPointerLineTypePropertyGuid, kPlanSectionPropertyGroupGuid, API_LinetypeID, nullptr, 0, false },
    { "penReplacement",         BuiltInFieldKind::Enum,         kPenReplacementPropertyGuid,         kPlanSectionPropertyGroupGuid,  API_ZombieAttrID,  kPenReplacementValues, 4, true  },
    { "hatchCategory",          BuiltInFieldKind::Enum,         kHatchCategoryPropertyGuid,          kPlanSectionPropertyGroupGuid,  API_ZombieAttrID,  kHatchCategories,      7, false },
    { "textAlignment",          BuiltInFieldKind::ListEnum,     kTextAlignmentPropertyGuid,          kPlanSectionPropertyGroupGuid,  API_ZombieAttrID,  kTextAlignments,       4, false },
    { "textStyle",              BuiltInFieldKind::ListFlags,    kTextStylePropertyGuid,              kPlanSectionPropertyGroupGuid,  API_ZombieAttrID,  kTextStyles,           6, false },
    // Surfaces and Materials (part 4 of #1816).
    { "constructionMaterial",   BuiltInFieldKind::AttributeHas, kConstructionMaterialPropertyGuid,   kSurfaceMaterialPropertyGroupGuid, API_BuildingMaterialID, nullptr,   0, true  },
    { "surface",                BuiltInFieldKind::AttributeHas, kSurfacePropertyGuid,                kSurfaceMaterialPropertyGroupGuid, API_MaterialID,     nullptr,        0, true  },
    // Geometry and tool fields (part 5 of #1816), the rest of which is in kBuiltInValueFields.
    { "wallHeightReversed",     BuiltInFieldKind::Bool,         kWallHeightReversedPropertyGuid,     kWallGeometryGroupGuid,         API_ZombieAttrID,  nullptr,         0, false },
    { "stairInvalidHeight",     BuiltInFieldKind::Bool,         kStairInvalidHeightPropertyGuid,     kWallGeometryGroupGuid,         API_ZombieAttrID,  nullptr,         0, false },
    { "mepOpenConnection",      BuiltInFieldKind::Bool,         kMepOpenConnectionPropertyGuid,      kMepCommonPropertyGroupGuid,    API_ZombieAttrID,  nullptr,         0, false },
    { "wallComplexity",         BuiltInFieldKind::Enum,         kWallComplexityPropertyGuid,         kConstructionPropertyGroupGuid, API_ZombieAttrID,  kWallComplexities,            3, true  },
    { "wallReferenceLinePosition", BuiltInFieldKind::Enum,      kWallReferenceLinePositionPropertyGuid, kPositioningPropertyGroupGuid, API_ZombieAttrID, kWallReferenceLinePositions, 6, true  },
    { "wallGeometryType",       BuiltInFieldKind::Enum,         kWallGeometryTypePropertyGuid,       kWallGeometryGroupGuid,         API_ZombieAttrID,  kWallGeometryTypes,           3, true  },
    { "columnCoatingType",      BuiltInFieldKind::Enum,         kColumnCoatingTypePropertyGuid,      kConstructionPropertyGroupGuid, API_ZombieAttrID,  kColumnCoatingTypes,          3, true  },
    { "columnCrossSectionType", BuiltInFieldKind::Enum,         kColumnCrossSectionTypePropertyGuid, kWallGeometryGroupGuid,         API_ZombieAttrID,  kColumnCrossSectionTypes,     3, false },
    { "beamCrossSectionType",   BuiltInFieldKind::Enum,         kBeamCrossSectionTypePropertyGuid,   kWallGeometryGroupGuid,         API_ZombieAttrID,  kBeamCrossSections,           3, false },
    { "slabReferencePlaneLocation", BuiltInFieldKind::Enum,     kSlabReferencePlaneLocationPropertyGuid, kPositioningPropertyGroupGuid, API_ZombieAttrID, kSlabReferencePlaneLocations, 4, true  },
    { "slabEdgeAngle",          BuiltInFieldKind::Enum,         kSlabEdgeAnglePropertyGuid,          kWallGeometryGroupGuid,         API_ZombieAttrID,  kSlabEdgeAngles,              2, true  },
    { "roofCutBodyType",        BuiltInFieldKind::Enum,         kRoofCutBodyTypePropertyGuid,        kConstructionPropertyGroupGuid, API_ZombieAttrID,  kRoofCutBodyTypes,            2, true  },
    { "roofEaveOverhangType",   BuiltInFieldKind::Enum,         kRoofEaveOverhangTypePropertyGuid,   kConstructionPropertyGroupGuid, API_ZombieAttrID,  kRoofEaveOverhangTypes,       2, false },
    { "roofEdgeAngle",          BuiltInFieldKind::Enum,         kRoofEdgeAnglePropertyGuid,          kWallGeometryGroupGuid,         API_ZombieAttrID,  kRoofEdgeAngles,              3, true  },
    { "roofGeometryType",       BuiltInFieldKind::Enum,         kRoofGeometryTypePropertyGuid,       kWallGeometryGroupGuid,         API_ZombieAttrID,  kRoofGeometryTypes,           2, true  },
    { "shellCutBodyType",       BuiltInFieldKind::Enum,         kShellCutBodyTypePropertyGuid,       kConstructionPropertyGroupGuid, API_ZombieAttrID,  kShellCutBodyTypes,           3, false },
    { "shellGeometryType",      BuiltInFieldKind::Enum,         kShellGeometryTypePropertyGuid,      kWallGeometryGroupGuid,         API_ZombieAttrID,  kShellGeometryTypes,          3, false },
    { "openingShape",           BuiltInFieldKind::Enum,         kOpeningShapePropertyGuid,           kWallGeometryGroupGuid,         API_ZombieAttrID,  kOpeningShapes,               5, true  },
    { "openingLimits",          BuiltInFieldKind::Enum,         kOpeningLimitsPropertyGuid,          kWallGeometryGroupGuid,         API_ZombieAttrID,  kOpeningLimits,               3, true  },
    { "openingOrientation",     BuiltInFieldKind::Enum,         kOpeningOrientationPropertyGuid,     kWallGeometryGroupGuid,         API_ZombieAttrID,  kOpeningOrientations,         4, true  },
    { "wallTopLine",            BuiltInFieldKind::AttributeIs,  kWallTopLinePropertyGuid,            kPlanSectionPropertyGroupGuid,  API_LinetypeID,    nullptr,         0, true  },
    { "wallCutLine",            BuiltInFieldKind::AttributeIs,  kWallCutLinePropertyGuid,            kPlanSectionPropertyGroupGuid,  API_LinetypeID,    nullptr,         0, true  },
    { "wallUncutLine",          BuiltInFieldKind::AttributeIs,  kWallUncutLinePropertyGuid,          kPlanSectionPropertyGroupGuid,  API_LinetypeID,    nullptr,         0, true  },
    { "columnHiddenLineType",   BuiltInFieldKind::AttributeIs,  kColumnHiddenLineTypePropertyGuid,   kPlanSectionPropertyGroupGuid,  API_LinetypeID,    nullptr,         0, true  },
    { "beamAxisLineType",       BuiltInFieldKind::AttributeIs,  kBeamAxisLineTypePropertyGuid,       kPlanSectionPropertyGroupGuid,  API_LinetypeID,    nullptr,         0, true  },
    { "curtainWallFrameCutLine", BuiltInFieldKind::AttributeIs, kCurtainWallFrameCutLinePropertyGuid, kPlanSectionPropertyGroupGuid, API_LinetypeID,    nullptr,         0, true  },
    { "curtainWallPanelCutLine", BuiltInFieldKind::AttributeIs, kCurtainWallPanelCutLinePropertyGuid, kPlanSectionPropertyGroupGuid, API_LinetypeID,    nullptr,         0, true  },
    { "columnCoveringHatch",    BuiltInFieldKind::AttributeIs,  kColumnCoveringHatchPropertyGuid,    kPlanSectionPropertyGroupGuid,  API_FilltypeID,    nullptr,         0, true  },
    { "columnCoatingHatch",     BuiltInFieldKind::AttributeIs,  kColumnCoatingHatchPropertyGuid,     kPlanSectionPropertyGroupGuid,  API_FilltypeID,    nullptr,         0, true  },
    { "slabConstructionMaterialFill", BuiltInFieldKind::AttributeIs, kSlabConstructionMaterialFillPropertyGuid, kPlanSectionPropertyGroupGuid, API_FilltypeID, nullptr, 0, true  },
    { "wallOutsideSurface",     BuiltInFieldKind::AttributeIs,  kWallOutsideSurfacePropertyGuid,     kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "wallInsideSurface",      BuiltInFieldKind::AttributeIs,  kWallInsideSurfacePropertyGuid,      kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "wallSideSurface",        BuiltInFieldKind::AttributeIs,  kWallSideSurfacePropertyGuid,        kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "columnCoreSurface",      BuiltInFieldKind::AttributeIs,  kColumnCoreSurfacePropertyGuid,      kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "columnCoatingSurface",   BuiltInFieldKind::AttributeIs,  kColumnCoatingSurfacePropertyGuid,   kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "columnExtrusionSurface", BuiltInFieldKind::AttributeIs,  kColumnExtrusionSurfacePropertyGuid, kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "beamEndSurface",         BuiltInFieldKind::AttributeIs,  kBeamEndSurfacePropertyGuid,         kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "beamRightSurface",       BuiltInFieldKind::AttributeIs,  kBeamRightSurfacePropertyGuid,       kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "beamLeftSurface",        BuiltInFieldKind::AttributeIs,  kBeamLeftSurfacePropertyGuid,        kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "beamBottomSurface",      BuiltInFieldKind::AttributeIs,  kBeamBottomSurfacePropertyGuid,      kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "beamTopSurface",         BuiltInFieldKind::AttributeIs,  kBeamTopSurfacePropertyGuid,         kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "openingEndSurface",      BuiltInFieldKind::AttributeIs,  kOpeningEndSurfacePropertyGuid,      kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
    { "openingExtrusionSurface", BuiltInFieldKind::AttributeIs, kOpeningExtrusionSurfacePropertyGuid, kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,        0, true  },
    { "meshSkirtSurface",       BuiltInFieldKind::AttributeIs,  kMeshSkirtSurfacePropertyGuid,       kSurfaceMaterialPropertyGroupGuid, API_MaterialID, nullptr,         0, true  },
};

static bool ReadBuiltInPropertyField (const BuiltInPropertyField& field, const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, int op, const std::string& valueText, GS::ObjectState& outLeaf)
{
    const GS::String key (field.key);
    const GS::String notKey = key + "Not";
    switch (field.kind) {
        case BuiltInFieldKind::Bool: {
            if (op != 0 || (valueText != "true" && valueText != "false"))
                return false;
            const bool value = valueText == "true";
            if (!MatchesGenerated (classGuid, leaf, BoolModelViewCriterionXML (field.propertyGuid, value, field.groupGuid)))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (key, value);
            return true;
        }
        case BuiltInFieldKind::AttributeIs:
        case BuiltInFieldKind::AttributeHas: {
            const bool isHas = field.kind == BuiltInFieldKind::AttributeHas;
            const int isOp = isHas ? 6 : 0;
            const int isNotOp = isHas ? 7 : 1;
            int index = 0;
            if ((op != isOp && !(op == isNotOp && field.notAllowed)) || !ParseInt (valueText, index))
                return false;
            const bool isNot = op == isNotOp;
            const GS::UniString generatedXML = isHas
                ? AttrIndexContainsModelViewCriterionXML (field.propertyGuid, index, isNot, field.groupGuid)
                : AttrIndexEqualsModelViewCriterionXML (field.propertyGuid, index, isNot, field.groupGuid);
            GS::UniString name;
            // An attribute that is not in the project (any more) cannot be named, so it stays raw.
            if (!MatchesGenerated (classGuid, leaf, generatedXML) || !FindAttributeNameByIndex (field.attributeType, index, name))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (isNot ? notKey : key, name);
            return true;
        }
        case BuiltInFieldKind::String: {
            static const int   kOps[]      = { 0,    1,       6,          7,             8,            9 };
            static const char* kSuffixes[] = { "Is", "IsNot", "Contains", "NotContains", "StartsWith", "EndsWith" };
            for (int i = 0; i < 6; ++i) {
                if (op != kOps[i])
                    continue;
                const GS::UniString value = FromUtf8 (valueText);
                if (!MatchesGenerated (classGuid, leaf, StringModelViewCriterionXML (field.propertyGuid, value, op, field.groupGuid)))
                    return false;
                outLeaf = GS::ObjectState ();
                outLeaf.Add (key + kSuffixes[i], value);
                return true;
            }
            return false;
        }
        case BuiltInFieldKind::Enum: {
            int value = 0;
            if ((op != 0 && !(op == 1 && field.notAllowed)) || !ParseInt (valueText, value))
                return false;
            for (int i = 0; i < field.valueCount; ++i) {
                if (field.values[i].value != value)
                    continue;
                if (!MatchesGenerated (classGuid, leaf, IntEqualsPropertyCriterionXML (field.propertyGuid, value, field.groupGuid, op == 1)))
                    return false;
                outLeaf = GS::ObjectState ();
                outLeaf.Add (op == 1 ? notKey : key, GS::UniString (field.values[i].name));
                return true;
            }
            return false;
        }
        case BuiltInFieldKind::ListEnum: {
            int value = 0;
            if (op != 0 || !ParseInt (valueText, value))
                return false;
            for (int i = 0; i < field.valueCount; ++i) {
                if (field.values[i].value != value)
                    continue;
                if (!MatchesGenerated (classGuid, leaf, IntListCriterionXML (field.propertyGuid, value, 0, field.groupGuid)))
                    return false;
                outLeaf = GS::ObjectState ();
                outLeaf.Add (key, GS::UniString (field.values[i].name));
                return true;
            }
            return false;
        }
        case BuiltInFieldKind::ListFlags: {
            // A flag not in values cannot be named, so the condition stays raw. No flag at all is the
            // empty list, which the generator writes as 0.
            int mask = 0;
            if (op != 0 || !ParseInt (valueText, mask) || !MatchesGenerated (classGuid, leaf, IntListCriterionXML (field.propertyGuid, mask, 0, field.groupGuid)))
                return false;
            int remaining = mask;
            GS::Array<GS::UniString> names;
            for (int i = 0; i < field.valueCount; ++i) {
                if ((mask & field.values[i].value) != 0) {
                    names.Push (GS::UniString (field.values[i].name));
                    remaining &= ~field.values[i].value;
                }
            }
            if (remaining != 0)
                return false;
            outLeaf = GS::ObjectState ();
            const auto& nameList = outLeaf.AddList<GS::UniString> (key);
            for (const GS::UniString& name : names)
                nameList (name);
            return true;
        }
    }
    return false;
}

// The built-in properties of the Positioning and ID and Categories categories (part 3 of #1816), each
// written by EmitCriterionNode with the same key, guid, group and operators, and those of the Plan and
// Section and Surfaces and Materials categories that hold a value (part 4), and the geometry and tool
// fields (wallLevelOffset, wallParentId, meshEdgePen, ..., part 5).
enum class BuiltInValueKind {
    Number,     // key + "", Not, LessThan, GreaterThan, LessOrEqual, GreaterOrEqual: a number
    NumList,    // the same, written as a list of one number
    IntNumber,  // the same, an integer
    Integer,    // key / keyNot (when notAllowed): an integer, "is" / "is not"
    Text,       // key + the suffix of each operator in textOps (kTextOperatorSuffixes): a text
    TextList,   // the same, written as a list of one text
    IntList,    // the same, an integer written as a list of one integer
    IntListIs,  // key: an integer written as a list of one integer, "is"
    Font,       // like IntList, the font named by its name (the list holds its index)
    GuidEnum,   // key / keyNot (when notAllowed): an enum name, "is" / "is not"
    GuidFlag    // key / keyNot: true, "is" / "is not" the one value of values
};

// The key suffix of each number operator (CriteriaOperatorEnum 0 to 5), as TryEmitNumField reads them.
static const char* kNumberOperatorSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
// The key suffix of each text operator, by CriteriaOperatorEnum.
static const char* kTextOperatorSuffixes[] = { "Is", "IsNot", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual", "Contains", "NotContains", "StartsWith", "EndsWith" };
static const unsigned kTextIsOps       = 0x003; // is, is not
static const unsigned kTextContainsOps = 0x040; // contains
static const unsigned kText6Ops        = 0x3C3; // is, is not, contains, does not contain, starts with, ends with
static const unsigned kText10Ops       = 0x3FF; // and less than, greater than, less or equal, greater or equal
static const unsigned kListHasOps      = 0x0C0; // contains, does not contain
static const unsigned kListIsHasOps    = 0x0C1; // is, contains, does not contain

struct BuiltInValueField {
    const char*          key;
    BuiltInValueKind     kind;
    const char*          propertyGuid;
    const char*          groupGuid;
    unsigned             textOps;       // Text, TextList, IntList, Font: bit n accepts operator n
    bool                 bareVariant;   // Text, GuidEnum: the <Variant> has no <Value> wrapper
    const GuidEnumValue* values;        // GuidEnum, GuidFlag
    int                  valueCount;
    bool                 notAllowed;    // Integer, GuidEnum
};

static const BuiltInValueField kBuiltInValueFields[] = {
    { "seaLevelAltitude",    BuiltInValueKind::Number,   kSeaLevelAltitudePropertyGuid,    kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "projectZeroAltitude", BuiltInValueKind::Number,   kProjectZeroAltitudePropertyGuid, kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "groundFloorAltitude", BuiltInValueKind::Number,   kGroundFloorAltitudePropertyGuid, kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "roofAltitude",        BuiltInValueKind::Number,   kRoofAltitudePropertyGuid,        kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "sillHeight",          BuiltInValueKind::Number,   kSillHeightPropertyGuid,          kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "lintelHeight",        BuiltInValueKind::Number,   kLintelHeightPropertyGuid,        kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "floorIndex",          BuiltInValueKind::Integer,  kPlacementStoryPropertyGuid,      kPositioningPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "renovationFilter",    BuiltInValueKind::Text,     kRenovationFilterPropertyGuid,    kIdCategoryGroupGuid1,         kTextIsOps,       true,  nullptr,                   0, false },
    { "structuralFunction",  BuiltInValueKind::GuidEnum, kStructuralFunctionPropertyGuid,  kIdCategoryGroupGuid1,         0,                true,  kStructuralFunctionValues, 3, true  },
    { "position",            BuiltInValueKind::GuidEnum, kPositionPropertyGuid,            kIdCategoryGroupGuid1,         0,                true,  kPositionValues,           3, true  },
    { "renovationStatus",    BuiltInValueKind::GuidEnum, kRenovationStatusPropertyGuid,    kIdCategoryGroupGuid1,         0,                true,  kRenovationStatusValues,   3, false },
    { "variantState",        BuiltInValueKind::Text,     kVariantStatePropertyGuid,        kIdCategoryGroupGuid2,         kTextIsOps,       true,  nullptr,                   0, false },
    { "variantSet",          BuiltInValueKind::Text,     kVariantSetPropertyGuid,          kIdCategoryGroupGuid2,         kTextIsOps,       true,  nullptr,                   0, false },
    { "elementId",           BuiltInValueKind::Text,     kElementIdPropertyGuid,           kIdCategoryGroupGuid3,         kText10Ops,       false, nullptr,                   0, false },
    { "uniqueId",            BuiltInValueKind::Text,     kUniqueIdPropertyGuid,            kIdCategoryGroupGuid3,         kText6Ops,        false, nullptr,                   0, false },
    { "linkMainId",          BuiltInValueKind::Text,     kLinkMainIdPropertyGuid,          kIdCategoryGroupGuid3,         kText10Ops,       false, nullptr,                   0, false },
    { "linkedElementId",     BuiltInValueKind::Text,     kLinkedElementIdPropertyGuid,     kIdCategoryGroupGuid3,         kText10Ops,       false, nullptr,                   0, false },
    { "name",                BuiltInValueKind::Text,     kNamePropertyGuid,                kIdCategoryGroupGuid3,         kText6Ops,        false, nullptr,                   0, false },
    { "libraryPartName",     BuiltInValueKind::Text,     kLibraryPartNamePropertyGuid,     kIdCategoryGroupGuid3,         kText6Ops,        false, nullptr,                   0, false },
    { "subTypeName",         BuiltInValueKind::TextList, kSubTypeNamePropertyGuid,         kIdCategoryGroupGuid3,         kText6Ops,        false, nullptr,                   0, false },
    { "hasLinkSource",       BuiltInValueKind::GuidFlag, kLinkSourcePropertyGuid,          kIdCategoryGroupGuid3,         0,                false, kLinkSourceValues,         1, false },
    { "variantSetName",      BuiltInValueKind::Text,     kVariantSetNamePropertyGuid,      kIdCategoryGroupGuid4,         kTextContainsOps, false, nullptr,                   0, false },
    { "variantName",         BuiltInValueKind::Text,     kVariantNamePropertyGuid,         kIdCategoryGroupGuid4,         kTextContainsOps, false, nullptr,                   0, false },
    // Plan and Section (part 4 of #1816). Pens are plain pen numbers.
    { "font",                BuiltInValueKind::Font,      kFontPropertyGuid,                  kPlanSectionPropertyGroupGuid, kListIsHasOps,    false, nullptr,                   0, false },
    { "linePen",             BuiltInValueKind::IntList,   kLinePenPropertyGuid,               kPlanSectionPropertyGroupGuid, kListHasOps,      false, nullptr,                   0, false },
    { "textPen",             BuiltInValueKind::IntList,   kTextPenPropertyGuid,               kPlanSectionPropertyGroupGuid, kListIsHasOps,    false, nullptr,                   0, false },
    { "hatchContourPen",     BuiltInValueKind::Integer,   kHatchContourPenPropertyGuid,       kPlanSectionPropertyGroupGuid, 0,                false, nullptr,                   0, true  },
    { "textLineSpacing",     BuiltInValueKind::IntListIs, kTextLineSpacingPropertyGuid,       kPlanSectionPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "dimensionMarkerSize", BuiltInValueKind::Number,    kDimensionMarkerSizePropertyGuid,   kWallGeometryGroupGuid,        0,                false, nullptr,                   0, false },
    { "dimensionMarkerPen",  BuiltInValueKind::Integer,   kDimensionMarkerPenPropertyGuid,    kPlanSectionPropertyGroupGuid, 0,                false, nullptr,                   0, false },
    { "dimensionMarkerPen",  BuiltInValueKind::IntList,   kDimensionMarkerPenListPropertyGuid, kPlanSectionPropertyGroupGuid, kTextContainsOps, false, nullptr,                  0, false },
    { "dimensionPointerPen", BuiltInValueKind::IntList,   kDimensionPointerPenPropertyGuid,   kPlanSectionPropertyGroupGuid, kTextContainsOps, false, nullptr,                   0, false },
    // Surfaces and Materials (part 4 of #1816).
    { "constructionMaterialId",   BuiltInValueKind::TextList, kConstructionMaterialIdPropertyGuid,   kSurfaceMaterialPropertyGroupGuid, kText6Ops, false, nullptr, 0, false },
    { "constructionMaterialName", BuiltInValueKind::TextList, kConstructionMaterialNamePropertyGuid, kSurfaceMaterialPropertyGroupGuid, kText6Ops, false, nullptr, 0, false },
    { "surfaceName",              BuiltInValueKind::TextList, kSurfaceNamePropertyGuid,              kSurfaceMaterialPropertyGroupGuid, kText6Ops, false, nullptr, 0, false },
    // Geometry and tool fields (part 5 of #1816). Pens are plain pen numbers.
    { "screedThickness",          BuiltInValueKind::Number,   kScreedThicknessPropertyGuid,          kPositioningPropertyGroupGuid, 0, false, nullptr, 0, false },
    { "wallLevelOffset",          BuiltInValueKind::Number,   kWallLevelOffsetPropertyGuid,          kPositioningPropertyGroupGuid, 0, false, nullptr, 0, false },
    { "wallTopOffset",            BuiltInValueKind::Number,   kWallTopOffsetPropertyGuid,            kPositioningPropertyGroupGuid, 0, false, nullptr, 0, false },
    { "linkedUpperStoryAbsolute", BuiltInValueKind::Integer,  kLinkedUpperStoryAbsolutePropertyGuid, kPositioningPropertyGroupGuid, 0, false, nullptr, 0, false },
    { "linkedUpperStoryRelative", BuiltInValueKind::Integer,  kLinkedUpperStoryRelativePropertyGuid, kPositioningPropertyGroupGuid, 0, false, nullptr, 0, false },
    { "wallOutsideAngle",         BuiltInValueKind::Number,   kWallOutsideAnglePropertyGuid,         kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "wallInsideAngle",          BuiltInValueKind::Number,   kWallInsideAnglePropertyGuid,          kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "wallThickness",            BuiltInValueKind::Number,   kWallThicknessPropertyGuid,            kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "wallHeight",               BuiltInValueKind::Number,   kWallHeightPropertyGuid,               kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "wallProfileHeight",        BuiltInValueKind::Number,   kWallProfileHeightPropertyGuid,        kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "wallWidth",                BuiltInValueKind::Number,   kWallWidthPropertyGuid,                kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "wallProfileWidth",         BuiltInValueKind::Number,   kWallProfileWidthPropertyGuid,         kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "objectLength",             BuiltInValueKind::Number,   kObjectLengthPropertyGuid,             kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "columnInclinationAngle",   BuiltInValueKind::Number,   kColumnInclinationAnglePropertyGuid,   kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "columnCoreDiameter",       BuiltInValueKind::Number,   kColumnCoreDiameterPropertyGuid,       kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "columnCoatingThickness",   BuiltInValueKind::Number,   kColumnCoatingThicknessPropertyGuid,   kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "columnBottomCrossSectionHeightCut",           BuiltInValueKind::Number, kColumnBottomCrossSectionHeightCutPropertyGuid,           kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnBottomCrossSectionHeightPerpendicular", BuiltInValueKind::Number, kColumnBottomCrossSectionHeightPerpendicularPropertyGuid, kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnTopCrossSectionHeightCut",              BuiltInValueKind::Number, kColumnTopCrossSectionHeightCutPropertyGuid,              kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnTopCrossSectionHeightPerpendicular",    BuiltInValueKind::Number, kColumnTopCrossSectionHeightPerpendicularPropertyGuid,    kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnProfileCoreHeight",                     BuiltInValueKind::Number, kColumnProfileCoreHeightPropertyGuid,                     kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnCoreHeightDiameterRatio",               BuiltInValueKind::Number, kColumnCoreHeightDiameterRatioPropertyGuid,               kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnBottomCrossSectionWidthCut",            BuiltInValueKind::Number, kColumnBottomCrossSectionWidthCutPropertyGuid,            kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnBottomCrossSectionWidthPerpendicular",  BuiltInValueKind::Number, kColumnBottomCrossSectionWidthPerpendicularPropertyGuid,  kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnTopCrossSectionWidthCut",               BuiltInValueKind::Number, kColumnTopCrossSectionWidthCutPropertyGuid,               kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnTopCrossSectionWidthPerpendicular",     BuiltInValueKind::Number, kColumnTopCrossSectionWidthPerpendicularPropertyGuid,     kWallGeometryGroupGuid, 0, false, nullptr, 0, false },
    { "columnCoreWidth",          BuiltInValueKind::Number,   kColumnCoreWidthPropertyGuid,          kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "columnProfileCoreWidth",   BuiltInValueKind::Number,   kColumnProfileCoreWidthPropertyGuid,   kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "slabDefaultEdgeAngle",     BuiltInValueKind::Number,   kSlabDefaultEdgeAnglePropertyGuid,     kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "roofEaveOverhang",         BuiltInValueKind::Number,   kRoofEaveOverhangPropertyGuid,         kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "roofPitch",                BuiltInValueKind::NumList,  kRoofPitchPropertyGuid,                kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "roofLevelNumber",          BuiltInValueKind::IntNumber, kRoofLevelNumberPropertyGuid,         kConstructionPropertyGroupGuid, 0, false, nullptr, 0, false },
    { "shellTiltAngle",           BuiltInValueKind::Number,   kShellTiltAnglePropertyGuid,           kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "jambThickness1",           BuiltInValueKind::Number,   kJambThickness1PropertyGuid,           kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "jambThickness2",           BuiltInValueKind::Number,   kJambThickness2PropertyGuid,           kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "curtainWallPanelOffset",   BuiltInValueKind::Number,   kCurtainWallPanelOffsetPropertyGuid,   kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "stairRiserInclineAngle",   BuiltInValueKind::Number,   kStairRiserInclineAnglePropertyGuid,   kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "stairMinHeadroom",         BuiltInValueKind::Number,   kStairMinHeadroomPropertyGuid,         kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "stairDefaultWidth",        BuiltInValueKind::Number,   kStairDefaultWidthPropertyGuid,        kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "stairSlope",               BuiltInValueKind::Number,   kStairSlopePropertyGuid,               kWallGeometryGroupGuid,        0, false, nullptr, 0, false },
    { "stairMaxRiserHeight",      BuiltInValueKind::Number,   kStairMaxRiserHeightPropertyGuid,      kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairMinRiserHeight",      BuiltInValueKind::Number,   kStairMinRiserHeightPropertyGuid,      kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairMaxGoing",            BuiltInValueKind::Number,   kStairMaxGoingPropertyGuid,            kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairMinGoing",            BuiltInValueKind::Number,   kStairMinGoingPropertyGuid,            kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairRiserCount",          BuiltInValueKind::IntNumber, kStairRiserCountPropertyGuid,         kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairStepCount",           BuiltInValueKind::IntNumber, kStairStepCountPropertyGuid,          kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairStepCountPerFlight",  BuiltInValueKind::IntListIs, kStairStepCountPerFlightPropertyGuid, kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "stairRisersPerFlight",     BuiltInValueKind::IntListIs, kStairRisersPerFlightPropertyGuid,    kStairRulesGroupGuid,          0, false, nullptr, 0, false },
    { "wallParentId",             BuiltInValueKind::Text,     kWallParentIdPropertyGuid,             kIdCategoryGroupGuid3,         kText6Ops, false, nullptr, 0, false },
    { "wallConnectedOpeningIds",  BuiltInValueKind::TextList, kWallConnectedOpeningIdsPropertyGuid,  kIdCategoryGroupGuid3,         kText6Ops, false, nullptr, 0, false },
    { "wallHatchForegroundPen",   BuiltInValueKind::Integer,  kWallHatchForegroundPenPropertyGuid,   kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "wallContourPen",           BuiltInValueKind::Integer,  kWallContourPenPropertyGuid,           kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "wallTopLinePen",           BuiltInValueKind::Integer,  kWallTopLinePenPropertyGuid,           kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "wallCutHatchBackgroundPen", BuiltInValueKind::Integer, kWallCutHatchBackgroundPenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "wallHatchBackgroundPen",   BuiltInValueKind::Integer,  kWallHatchBackgroundPenPropertyGuid,   kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "wallUncutLinePen",         BuiltInValueKind::Integer,  kWallUncutLinePenPropertyGuid,         kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnCoreForegroundPen",  BuiltInValueKind::Integer,  kColumnCoreForegroundPenPropertyGuid,  kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnCoatingForegroundPen", BuiltInValueKind::Integer, kColumnCoatingForegroundPenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnCoveringHatchForegroundPen", BuiltInValueKind::Integer, kColumnCoveringHatchForegroundPenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnCoveringHatchBackgroundPen", BuiltInValueKind::Integer, kColumnCoveringHatchBackgroundPenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnCoatingBackgroundPen", BuiltInValueKind::Integer, kColumnCoatingBackgroundPenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnCoreBackgroundPen",  BuiltInValueKind::Integer,  kColumnCoreBackgroundPenPropertyGuid,  kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "columnHiddenLinePen",      BuiltInValueKind::Integer,  kColumnHiddenLinePenPropertyGuid,      kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "beamAxisPen",              BuiltInValueKind::Integer,  kBeamAxisPenPropertyGuid,              kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "slabCutLinePen",           BuiltInValueKind::Integer,  kSlabCutLinePenPropertyGuid,           kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "curtainWallFrameCutLinePen", BuiltInValueKind::Integer, kCurtainWallFrameCutLinePenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "curtainWallPanelCutLinePen", BuiltInValueKind::Integer, kCurtainWallPanelCutLinePenPropertyGuid, kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, true  },
    { "meshEdgePen",              BuiltInValueKind::Integer,  kMeshEdgePenPropertyGuid,              kPlanSectionPropertyGroupGuid, 0, false, nullptr, 0, false },
    // MEP (Archicad 29; Archicad 27 drops them).
    { "mepDiameter",              BuiltInValueKind::Number,   kMepDiameterPropertyGuid,              kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepDiameter2",             BuiltInValueKind::Number,   kMepDiameter2PropertyGuid,             kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepDN",                    BuiltInValueKind::IntNumber, kMepDNPropertyGuid,                   kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepDN2",                   BuiltInValueKind::IntNumber, kMepDN2PropertyGuid,                  kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepHeight",                BuiltInValueKind::Number,   kMepHeightPropertyGuid,                kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepHeight2",               BuiltInValueKind::Number,   kMepHeight2PropertyGuid,               kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepWidth",                 BuiltInValueKind::Number,   kMepWidthPropertyGuid,                 kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepWidth2",                BuiltInValueKind::Number,   kMepWidth2PropertyGuid,                kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepAngle",                 BuiltInValueKind::Number,   kMepAnglePropertyGuid,                 kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepCustomAngle",           BuiltInValueKind::Number,   kMepCustomAnglePropertyGuid,           kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepInsulationThickness",   BuiltInValueKind::Number,   kMepInsulationThicknessPropertyGuid,   kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepLength",                BuiltInValueKind::Number,   kMepLengthPropertyGuid,                kMepPropertyGroupGuid,         0, false, nullptr, 0, false },
    { "mepDescription",           BuiltInValueKind::Text,     kMepDescriptionPropertyGuid,           kMepPropertyGroupGuid,         kText6Ops, false, nullptr, 0, false },
    { "mepMaterialName",          BuiltInValueKind::Text,     kMepMaterialNamePropertyGuid,          kMepPropertyGroupGuid,         kText6Ops, false, nullptr, 0, false },
};

// Whether the generator writes this leaf for generatedXML once its value is written as writtenText:
// a number is written back with %.15g, which may differ from the digits Archicad wrote. The leaf's
// shape was checked by ReadPropertyLeaf and ReadSingleVariantValue (for a profile parameter, on the
// <Value> inside its ProfileParameterValue or ProfileParameterValueList).
static bool MatchesGeneratedWithValue (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, const GS::UniString& generatedXML, const std::string& writtenText)
{
    CriterionXmlNode written = leaf;
    CriterionXmlNode* valueWrapper = &written.children[0].children[2];
    if (valueWrapper->name == "ProfileParameterValue" || valueWrapper->name == "ProfileParameterValueList")
        valueWrapper = &valueWrapper->children[0];
    CriterionXmlNode& valueNode = *valueWrapper;
    CriterionXmlNode& variant = valueNode.name == "Variant" ? valueNode : valueNode.children[0];
    CriterionXmlNode& value = variant.children[1];
    (value.name == "ValueArray" ? value.children[0] : value).text = writtenText;
    return MatchesGenerated (classGuid, written, generatedXML);
}

static bool ReadBuiltInValueField (const BuiltInValueField& field, const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, int op, const std::string& valueText, GS::ObjectState& outLeaf)
{
    const GS::String key (field.key);
    const GS::String notKey = key + "Not";
    switch (field.kind) {
        case BuiltInValueKind::Number:
        case BuiltInValueKind::NumList: {
            double value = 0.0;
            if (op < 0 || op > 5 || !ParseDouble (valueText, value))
                return false;
            const GS::UniString generatedXML = field.kind == BuiltInValueKind::NumList
                ? NumListCriterionXML (field.propertyGuid, value, op, field.groupGuid)
                : NumPropertyCriterionXML (field.propertyGuid, value, op, field.groupGuid);
            if (!MatchesGeneratedWithValue (classGuid, leaf, generatedXML, ToUtf8 (GS::UniString::Printf ("%.15g", value))))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (key + kNumberOperatorSuffixes[op], value);
            return true;
        }
        case BuiltInValueKind::IntNumber: {
            int value = 0;
            if (op < 0 || op > 5 || !ParseInt (valueText, value))
                return false;
            if (!MatchesGenerated (classGuid, leaf, IntPropertyCriterionXML (field.propertyGuid, value, op, field.groupGuid)))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (key + kNumberOperatorSuffixes[op], static_cast<Int32> (value));
            return true;
        }
        case BuiltInValueKind::Integer: {
            int value = 0;
            if ((op != 0 && !(op == 1 && field.notAllowed)) || !ParseInt (valueText, value))
                return false;
            if (!MatchesGenerated (classGuid, leaf, IntPropertyCriterionXML (field.propertyGuid, value, op, field.groupGuid)))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (op == 1 ? notKey : key, static_cast<Int32> (value));
            return true;
        }
        case BuiltInValueKind::IntList:
        case BuiltInValueKind::IntListIs:
        case BuiltInValueKind::Font: {
            const unsigned ops = field.kind == BuiltInValueKind::IntListIs ? 0x001u : field.textOps;
            int value = 0;
            if (op < 0 || op > 9 || (ops & (1u << op)) == 0 || !ParseInt (valueText, value))
                return false;
            if (!MatchesGenerated (classGuid, leaf, IntListCriterionXML (field.propertyGuid, value, op, field.groupGuid)))
                return false;
            const GS::String outKey = field.kind == BuiltInValueKind::IntListIs ? key : key + kTextOperatorSuffixes[op];
            outLeaf = GS::ObjectState ();
            if (field.kind != BuiltInValueKind::Font) {
                outLeaf.Add (outKey, static_cast<Int32> (value));
                return true;
            }
            // A font that is not loaded cannot be named, nor one whose name the generator resolves
            // to another font, so those stay raw.
            if (value < 1 || value > 32767)
                return false;
            const GS::Optional<GS::UniString> fontName = GetFontNameFromIndex (static_cast<short> (value));
            if (!fontName.HasValue () || fontName->IsEmpty ())
                return false;
            const GS::Optional<short> resolvedIndex = GetFontIndexFromName (*fontName);
            if (!resolvedIndex.HasValue () || *resolvedIndex != value)
                return false;
            outLeaf.Add (outKey, *fontName);
            return true;
        }
        case BuiltInValueKind::Text:
        case BuiltInValueKind::TextList: {
            if (op < 0 || op > 9 || (field.textOps & (1u << op)) == 0)
                return false;
            const GS::UniString value = FromUtf8 (valueText);
            const GS::UniString generatedXML = field.kind == BuiltInValueKind::TextList
                ? StringListCriterionXML (field.propertyGuid, value, op, field.groupGuid)
                : StringModelViewCriterionXML (field.propertyGuid, value, op, field.groupGuid, field.bareVariant);
            if (!MatchesGenerated (classGuid, leaf, generatedXML))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (key + kTextOperatorSuffixes[op], value);
            return true;
        }
        case BuiltInValueKind::GuidEnum:
        case BuiltInValueKind::GuidFlag: {
            const bool isFlag = field.kind == BuiltInValueKind::GuidFlag;
            if (op != 0 && !(op == 1 && (field.notAllowed || isFlag)))
                return false;
            for (int i = 0; i < field.valueCount; ++i) {
                if (!SameGuid (valueText, field.values[i].guid))
                    continue;
                if (!MatchesGenerated (classGuid, leaf, GuidEqualsPropertyCriterionXML (field.propertyGuid, field.values[i].guid, op == 1, field.groupGuid, field.bareVariant)))
                    return false;
                outLeaf = GS::ObjectState ();
                if (isFlag)
                    outLeaf.Add (op == 1 ? notKey : key, true);
                else
                    outLeaf.Add (op == 1 ? notKey : key, GS::UniString (field.values[i].name));
                return true;
            }
            return false;
        }
    }
    return false;
}

// Built-in properties, identified by guid. Those of the categories not decoded yet stay raw.
static bool ReadBuiltInPropertyLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, const CriterionXmlNode& definition, int op, const CriterionXmlNode& valueNode, GS::ObjectState& outLeaf)
{
    const CriterionXmlNode* guidNode = FindChild (definition, "Guid");
    const CriterionXmlNode* groupNode = FindChild (definition, "PropertyDefinitionGroupUserId");
    const CriterionXmlNode* groupGuidNode = groupNode != nullptr ? FindChild (*groupNode, "Guid") : nullptr;
    std::string valueText;
    if (guidNode == nullptr || groupGuidNode == nullptr || !ReadSingleVariantValue (valueNode, valueText))
        return false;
    for (const BuiltInPropertyField& field : kBuiltInPropertyFields) {
        if (SameGuid (guidNode->text, field.propertyGuid) && SameGuid (groupGuidNode->text, field.groupGuid))
            return ReadBuiltInPropertyField (field, classGuid, leaf, op, valueText, outLeaf);
    }
    for (const BuiltInValueField& field : kBuiltInValueFields) {
        if (SameGuid (guidNode->text, field.propertyGuid) && SameGuid (groupGuidNode->text, field.groupGuid))
            return ReadBuiltInValueField (field, classGuid, leaf, op, valueText, outLeaf);
    }
    return false;
}

// The built-in profile modifiers, each written by TryEmitProfileParamField with this key (part 5 of #1816).
struct ProfileParameterField {
    const char* key;
    const char* guid;
};

static const ProfileParameterField kProfileParameterFields[] = {
    { "profileStretchHeight", kProfileStretchHeightGuid },
    { "profileStretchWidth",  kProfileStretchWidthGuid },
    { "profileNominalHeight", kProfileNominalHeightGuid },
    { "profileTotalHeight",   kProfileTotalHeightGuid },
    { "profileTotalWidth",    kProfileTotalWidthGuid },
    { "profileNominalWidth",  kProfileNominalWidthGuid },
};

// A profile parameter condition, whose value sits in a ProfileParameterValue (a built-in modifier by
// guid, or a parameter by name) or a ProfileParameterValueList (a parameter by name, "is" only). Its
// header is checked by comparing the generated XML.
static bool ReadProfileParameterLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, const CriterionXmlNode& definition, int op, const CriterionXmlNode& valueWrapper, GS::ObjectState& outLeaf)
{
    std::string valueText;
    double value = 0.0;
    if (valueWrapper.children.size () != 1 || !ReadSingleVariantValue (valueWrapper.children[0], valueText) || !ParseDouble (valueText, value))
        return false;
    const std::string writtenText = ToUtf8 (GS::UniString::Printf ("%.15g", value));
    const bool isList = valueWrapper.name == "ProfileParameterValueList";

    const CriterionXmlNode* hasGuidId = FindChild (definition, "HasGuidId");
    if (hasGuidId != nullptr && hasGuidId->text == "true") {
        const CriterionXmlNode* guidNode = FindChild (definition, "Guid");
        if (isList || guidNode == nullptr || op < 0 || op > 5)
            return false;
        for (const ProfileParameterField& field : kProfileParameterFields) {
            if (!SameGuid (guidNode->text, field.guid))
                continue;
            if (!MatchesGeneratedWithValue (classGuid, leaf, ProfileParameterCriterionXML (ProfileParameterHeaderXML (field.guid), value, op), writtenText))
                return false;
            outLeaf = GS::ObjectState ();
            outLeaf.Add (GS::String (field.key) + kNumberOperatorSuffixes[op], value);
            return true;
        }
        return false;
    }

    const CriterionXmlNode* nameNode = FindChild (definition, "Name");
    if (nameNode == nullptr || nameNode->text.empty ())
        return false;
    const GS::UniString name = FromUtf8 (nameNode->text);
    if (isList) {
        if (op != 0 || !MatchesGeneratedWithValue (classGuid, leaf, ProfileParameterListCriterionXML (name, value), writtenText))
            return false;
        outLeaf = GS::ObjectState ("profileParameterList", GS::ObjectState ("name", name, "equals", value));
        return true;
    }
    const char* operatorKey = ComparisonOperatorKey (op);
    if (operatorKey == nullptr || !MatchesGeneratedWithValue (classGuid, leaf, ProfileParameterCriterionXML (ProfileParameterHeaderByNameXML (name), value, op), writtenText))
        return false;
    GS::ObjectState condition ("name", name);
    condition.Add (operatorKey, value);
    outLeaf = GS::ObjectState ("profileParameter", condition);
    return true;
}

// wallReferenceLineOffset, the one condition kept as a RealCriterion (see RealAttributeCriterionXML).
static bool ReadRealAttributeLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, GS::ObjectState& outLeaf)
{
    if (leaf.children.size () != 3 || leaf.children[0].name != "AttributeCriterion" || leaf.children[1].name != "ClassGuid" || leaf.children[2].name != "RealSimpleCondition")
        return false;
    const CriterionXmlNode* attributeGuid = FindChild (leaf.children[0], "AttributeGuid");
    const CriterionXmlNode& condition = leaf.children[2];
    if (attributeGuid == nullptr || !SameGuid (attributeGuid->text, kWallReferenceLineOffsetAttrGuid) || condition.children.size () != 2 ||
        condition.children[0].name != "Type" || condition.children[1].name != "Value" || !condition.children[1].children.empty ())
        return false;
    int type = 0;
    double value = 0.0;
    if (!ParseInt (condition.children[0].text, type) || !ParseDouble (condition.children[1].text, value))
        return false;
    // The Type of each key suffix of kNumberOperatorSuffixes, as TryEmitRealAttributeField writes it.
    static const int kTypes[] = { 1, 4, 2, 3, 5, 6 };
    for (int i = 0; i < 6; ++i) {
        if (kTypes[i] != type)
            continue;
        // A number is written back with %.15g, see MatchesGeneratedWithValue.
        CriterionXmlNode written = leaf;
        written.children[2].children[1].text = ToUtf8 (GS::UniString::Printf ("%.15g", value));
        if (!MatchesGenerated (classGuid, written, RealAttributeCriterionXML (kWallReferenceLineOffsetAttrGuid, value, type)))
            return false;
        outLeaf = GS::ObjectState ();
        outLeaf.Add (GS::String ("wallReferenceLineOffset") + kNumberOperatorSuffixes[i], value);
        return true;
    }
    return false;
}

// Classification and custom property conditions, which are identified by name, the built-in
// properties of kBuiltInPropertyFields and kBuiltInValueFields, identified by guid, and the profile
// parameters.
static bool ReadPropertyLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf, GS::ObjectState& outLeaf)
{
    if (leaf.children.size () != 1 || leaf.children[0].name != "VBEF::ConditionIO")
        return false;
    const CriterionXmlNode& condition = leaf.children[0];
    if (condition.children.size () != 3 || condition.children[0].name != "PropertyDefinitionUserId" || condition.children[1].name != "CriteriaOperatorEnum")
        return false;
    const CriterionXmlNode& definition = condition.children[0];
    int op = 0;
    if (!ParseInt (condition.children[1].text, op))
        return false;

    const CriterionXmlNode& valueNode = condition.children[2];
    if (valueNode.name == "ProfileParameterValue" || valueNode.name == "ProfileParameterValueList")
        return ReadProfileParameterLeaf (classGuid, leaf, definition, op, valueNode, outLeaf);

    const CriterionXmlNode* hasGuidId = FindChild (definition, "HasGuidId");
    if (hasGuidId != nullptr && hasGuidId->text == "true")
        return ReadBuiltInPropertyLeaf (classGuid, leaf, definition, op, condition.children[2], outLeaf);

    const CriterionXmlNode* hasNameId = FindChild (definition, "HasNameId");
    const CriterionXmlNode* nameNode = FindChild (definition, "Name");
    const CriterionXmlNode* groupNode = FindChild (definition, "PropertyDefinitionGroupUserId");
    if (hasNameId == nullptr || hasNameId->text != "true" || nameNode == nullptr || nameNode->text.empty () || groupNode == nullptr)
        return false;
    const CriterionXmlNode* groupNameNode = FindChild (*groupNode, "Name");
    if (groupNameNode == nullptr || groupNameNode->text.empty ())
        return false;
    const GS::UniString name = FromUtf8 (nameNode->text);
    const GS::UniString group = FromUtf8 (groupNameNode->text);

    // The generated XML is compared with what was read, so a property of another by-name group
    // (e.g. the profile parameters) is never taken for a custom property: its header differs.
    if (groupNameNode->text == kClassificationGroupName)
        return ReadClassificationLeaf (classGuid, leaf, name, op, condition.children[2], outLeaf);
    return ReadCustomPropertyLeaf (classGuid, leaf, name, group, op, condition.children[2], outLeaf);
}

static GS::ObjectState ReadCriterionLeaf (const CriterionXmlNode& classGuid, const CriterionXmlNode& leaf)
{
    GS::ObjectState result;
    const std::string& classId = classGuid.text;
    if (SameGuid (classId, kElemTypeClassGuid) && leaf.name == "ElemTypeCriterion") {
        if (ReadElemTypeLeaf (leaf, result))
            return result;
    } else if ((SameGuid (classId, kModelElemClassGuid) && leaf.name == "ModelElementCriterion") ||
               (SameGuid (classId, kDrawingElemClassGuid) && leaf.name == "DrawingElementCriteiron")) {
        if (leaf.children.size () == 1 && leaf.children[0].name == "LogicalOperator" && leaf.children[0].text == "1")
            return GS::ObjectState (leaf.name == "ModelElementCriterion" ? "all3DTypes" : "all2DTypes", true);
    } else if (SameGuid (classId, kPropertyClassGuid) && leaf.name == "PropertyCriterion") {
        if (ReadPropertyLeaf (classGuid, leaf, result))
            return result;
    } else if (SameGuid (classId, kRealCriterionClassGuid) && leaf.name == "RealCriterion") {
        if (ReadRealAttributeLeaf (classGuid, leaf, result))
            return result;
    }
    return RawCriterionLeaf (classGuid, leaf);
}

using CriterionXmlChild = std::pair<const CriterionXmlNode*, const CriterionXmlNode*>;

// A CompositeCriterion: its LogicalOperator (1 OR, 2 AND) and its (ClassGuid, criterion) children.
static bool ReadComposite (const CriterionXmlNode& composite, bool& outIsAnd, std::vector<CriterionXmlChild>& outChildren)
{
    if (composite.name != "CompositeCriterion" || composite.children.size () < 4 || composite.children.size () % 2 != 0)
        return false;
    const CriterionXmlNode& logicalOperator = composite.children[0];
    const CriterionXmlNode& count = composite.children[1];
    if (logicalOperator.name != "LogicalOperator" || (logicalOperator.text != "1" && logicalOperator.text != "2") || count.name != "CriteriaCount")
        return false;
    int criteriaCount = 0;
    if (!ParseInt (count.text, criteriaCount) || criteriaCount != static_cast<int> ((composite.children.size () - 2) / 2))
        return false;
    outIsAnd = logicalOperator.text == "2";
    outChildren.clear ();
    for (size_t i = 2; i < composite.children.size (); i += 2) {
        if (composite.children[i].name != "ClassGuid")
            return false;
        outChildren.push_back (CriterionXmlChild (&composite.children[i], &composite.children[i + 1]));
    }
    return true;
}

static bool IsCompositeChild (const CriterionXmlChild& child)
{
    return SameGuid (child.first->text, kCompositeClassGuid) && child.second->name == "CompositeCriterion";
}

// depth: 1 for a row of the editor's table, 2 for a group within a row. Deeper groups stay raw,
// because the generator writes no more levels.
static GS::ObjectState ReadCriterionNode (const CriterionXmlChild& child, int depth)
{
    bool isAnd = false;
    std::vector<CriterionXmlChild> children;
    if (depth <= 2 && IsCompositeChild (child) && ReadComposite (*child.second, isAnd, children)) {
        if (depth == 1 && children.size () == 1 && !IsCompositeChild (children[0]))
            return ReadCriterionLeaf (*children[0].first, *children[0].second);
        GS::ObjectState group;
        const auto& groupList = group.AddList<GS::ObjectState> (isAnd ? "and" : "or");
        for (const CriterionXmlChild& groupChild : children)
            groupList (ReadCriterionNode (groupChild, depth + 1));
        return group;
    }
    return ReadCriterionLeaf (*child.first, *child.second);
}

bool GraphicOverrideCriterionFromXML (const GS::UniString& criterionXML, GS::ObjectState& outCriterion)
{
    std::vector<CriterionXmlNode> nodes;
    if (!ReadCriterionXmlSequence (criterionXML, nodes) || nodes.size () != 1)
        return false;
    // The generator writes this envelope only, so another one could not be written back.
    const CriterionXmlNode& expression = nodes[0];
    if (expression.name != "CriterionExpression" || expression.attributes != "Mv=\"2\" Sv=\"2\"" || expression.children.size () != 4)
        return false;
    const CriterionXmlNode& textMatchType = expression.children[0];
    const CriterionXmlNode& size = expression.children[1];
    if (textMatchType.name != "TextMatchType" || textMatchType.text != "1" || size.name != "Size" || size.text != "1")
        return false;
    const CriterionXmlChild root (&expression.children[2], &expression.children[3]);
    if (root.first->name != "ClassGuid" || !IsCompositeChild (root) || root.second->attributes != "Mv=\"2\" Sv=\"1\"")
        return false;

    bool isAnd = false;
    std::vector<CriterionXmlChild> rows;
    if (!ReadComposite (*root.second, isAnd, rows))
        return false;
    GS::ObjectState criterion;
    const auto& rowList = criterion.AddList<GS::ObjectState> (isAnd ? "and" : "or");
    for (const CriterionXmlChild& row : rows)
        rowList (ReadCriterionNode (row, 1));
    outCriterion = criterion;
    return true;
}

#endif
