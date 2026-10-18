#include "GridElementCommands.hpp"
#include "MigrationHelper.hpp"

#include <cmath>

namespace {

// Everything in here is used only by the Archicad 26+ code paths; AC25 builds treat
// unused functions as errors, so the whole block is version-guarded.
#ifdef ServerMainVers_2600
API_ElemType GridElemType ()
{
    return API_ElemType (API_ObjectID, APIVarId_GridElement);
}

GS::UniString VariationToString (Int32 v)
{
    char s[5] = {
        (char) ((v >> 24) & 0xFF),
        (char) ((v >> 16) & 0xFF),
        (char) ((v >> 8) & 0xFF),
        (char) (v & 0xFF),
        0
    };
    return GS::UniString (s);
}

const char* ParamTypeToString (API_AddParID t)
{
    switch (t) {
        case APIParT_Integer:          return "Integer";
        case APIParT_Length:           return "Length";
        case APIParT_Angle:            return "Angle";
        case APIParT_RealNum:          return "RealNum";
        case APIParT_LightSw:          return "LightSwitch";
        case APIParT_ColRGB:           return "ColorRGB";
        case APIParT_Intens:           return "Intensity";
        case APIParT_LineTyp:          return "LineType";
        case APIParT_Mater:            return "Material";
        case APIParT_FillPat:          return "FillPattern";
        case APIParT_PenCol:           return "PenColor";
        case APIParT_CString:          return "String";
        case APIParT_Boolean:          return "Boolean";
        case APIParT_Separator:        return "Separator";
        case APIParT_Title:            return "Title";
        case APIParT_BuildingMaterial: return "BuildingMaterial";
        case APIParT_Profile:          return "Profile";
        case APIParT_Dictionary:       return "Dictionary";
        default:                       return "Unknown";
    }
}

bool IsNumericParam (API_AddParID t)
{
    switch (t) {
        case APIParT_Integer:
        case APIParT_Length:
        case APIParT_Angle:
        case APIParT_RealNum:
        case APIParT_LightSw:
        case APIParT_ColRGB:
        case APIParT_Intens:
        case APIParT_LineTyp:
        case APIParT_Mater:
        case APIParT_FillPat:
        case APIParT_PenCol:
        case APIParT_Boolean:
        case APIParT_BuildingMaterial:
        case APIParT_Profile:
            return true;
        default:
            return false;
    }
}

GS::UniString LibraryPartName (Int32 libInd)
{
    if (libInd <= 0) {
        return GS::UniString ();
    }
    API_LibPart libPart = {};
    libPart.index = libInd;
    const GSErrCode err = ACAPI_LibraryPart_Get (&libPart);
    if (libPart.location != nullptr) {
        delete libPart.location;
    }
    if (err != NoError) {
        return GS::UniString ();
    }
    return GS::UniString (libPart.docu_UName);
}

API_AddParType* FindParameter (API_AddParType** params, const char* name)
{
    if (params == nullptr) {
        return nullptr;
    }
    const GSSize n = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < n; ++ii) {
        API_AddParType& p = (*params)[ii];
        if (p.typeMod != API_ParArray && IsNumericParam (p.typeID) && CHEqualCStrings (p.name, name)) {
            return &p;
        }
    }
    return nullptr;
}

bool GetNumericParameter (API_AddParType** params, const char* name, double& value)
{
    const API_AddParType* p = FindParameter (params, name);
    if (p == nullptr) {
        return false;
    }
    value = p->value.real;
    return true;
}

bool SetNumericParameter (API_AddParType** params, const char* name, double value)
{
    API_AddParType* p = FindParameter (params, name);
    if (p == nullptr) {
        return false;
    }
    p->value.real = value;
    return true;
}

// The library part's own A and B, as CreateObjects reads them; a and b stay as they are when it
// has none to divide by.
GSErrCode GetLibraryPartOwnSize (Int32 libInd, double& a, double& b)
{
    double libraryPartA = 0.0;
    double libraryPartB = 0.0;
    Int32 addParNum = 0;
    API_AddParType** addPars = nullptr;
    const GSErrCode err = ACAPI_LibraryPart_GetParams (libInd, &libraryPartA, &libraryPartB, &addParNum, &addPars);
    if (addPars != nullptr) {
        ACAPI_DisposeAddParHdl (&addPars);
    }
    if (err != NoError) {
        return err;
    }
    if (libraryPartA > 1e-6) {
        a = libraryPartA;
    }
    if (libraryPartB > 1e-6) {
        b = libraryPartB;
    }
    return NoError;
}

// The length of the grid line: the GDL parameter AC_Length of the Grid Tool's grid marker, or for
// a library part without it the object's A size. As in CreateObjects, xRatio holds the size itself
// only with useXYFixSize on, otherwise the size divided by the library part's own A.
double GridLength (const API_Element& element, API_AddParType** params)
{
    double length = 0.0;
    if (GetNumericParameter (params, "AC_Length", length)) {
        return length;
    }
    if (element.object.useXYFixSize) {
        return element.object.xRatio;
    }
    double libraryPartA = 1.0;
    double libraryPartB = 1.0;
    GetLibraryPartOwnSize ((Int32) element.object.libInd, libraryPartA, libraryPartB);
    return element.object.xRatio * libraryPartA;
}

void AddParameters (GS::ObjectState& os, API_AddParType** params)
{
    const auto& list = os.AddList<GS::ObjectState> ("parameters");
    if (params == nullptr) {
        return;
    }
    const GSSize n = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < n; ++ii) {
        const API_AddParType& p = (*params)[ii];
        if (p.typeID == APIParT_Separator) {
            continue;
        }
        GS::ObjectState d;
        d.Add ("name", GS::UniString (p.name));
        d.Add ("displayName", GS::UniString (p.uDescname));
        d.Add ("type", GS::UniString (ParamTypeToString (p.typeID)));
        if (p.typeMod == API_ParArray) {
            d.Add ("dimension1", (Int32) p.dim1);
            d.Add ("dimension2", (Int32) p.dim2);
        } else if (IsNumericParam (p.typeID)) {
            d.Add ("value", p.value.real);
        } else if (p.typeID == APIParT_CString || p.typeID == APIParT_Title) {
            d.Add ("value", GS::UniString (p.value.uStr));
        }
        list (d);
    }
}

GS::ObjectState DescribeGridElement (const API_Element& element, API_AddParType** params, bool includeParameters)
{
    GS::ObjectState os;
    os.Add ("elementId", CreateGuidObjectState (element.header.guid));
    os.Add ("variation", VariationToString ((Int32) element.header.type.variationID));
    os.Add ("floorIndex", (Int32) element.header.floorInd);
    os.Add ("layerIndex", GetAttributeIndex (element.header.layer));
    GS::ObjectState pos;
    pos.Add ("x", element.object.pos.x);
    pos.Add ("y", element.object.pos.y);
    os.Add ("pos", pos);
    os.Add ("angle", element.object.angle);
    os.Add ("length", GridLength (element, params));
    os.Add ("xRatio", element.object.xRatio);
    os.Add ("yRatio", element.object.yRatio);
    os.Add ("useXYFixSize", element.object.useXYFixSize);
    os.Add ("level", element.object.level);
    os.Add ("libInd", (Int32) element.object.libInd);
    os.Add ("libraryPartName", LibraryPartName ((Int32) element.object.libInd));
    if (includeParameters) {
        AddParameters (os, params);
    }
    return os;
}

// Sets one GDL parameter by name. Returns false (and a warning) when the name is unknown
// or the value has the wrong JSON type for the parameter.
bool ApplyParameter (API_AddParType** params, const GS::ObjectState& p, GS::Array<GS::UniString>& warnings)
{
    GS::UniString name;
    if (!p.Get ("name", name) || params == nullptr) {
        warnings.Push ("parameter without a name was ignored");
        return false;
    }
    const GSSize n = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < n; ++ii) {
        API_AddParType& actParam = (*params)[ii];
        if (GS::UniString (actParam.name) != name) {
            continue;
        }
        if (actParam.typeMod == API_ParArray) {
            warnings.Push ("array parameter '" + name + "' is not supported");
            return false;
        }
        if (IsNumericParam (actParam.typeID)) {
            double d = 0.0;
            bool b = false;
            if (p.Get ("value", d)) {
                actParam.value.real = d;
            } else if (p.Get ("value", b)) {
                actParam.value.real = b ? 1.0 : 0.0;
            } else {
                warnings.Push ("parameter '" + name + "' needs a number or boolean value");
                return false;
            }
            return true;
        }
        if (actParam.typeID == APIParT_CString || actParam.typeID == APIParT_Title) {
            GS::UniString s;
            if (!p.Get ("value", s)) {
                warnings.Push ("parameter '" + name + "' needs a string value");
                return false;
            }
            GS::ucscpy (actParam.value.uStr, s.ToUStr (0, GS::Min (s.GetLength (), (USize) API_UAddParStrLen - 1)).Get ());
            return true;
        }
        warnings.Push ("parameter '" + name + "' has an unsupported type");
        return false;
    }
    warnings.Push ("unknown parameter '" + name + "'");
    return false;
}

#endif

// The fields of one grid element in the responses of GetGridElementDefaults and GetGridElements.
const char* GridElementDetailsSchema = R"(
        "elementId": { "$ref": "#/ElementId", "description": "The grid element. The zero GUID for the tool defaults." },
        "variation": { "type": "string", "description": "Four-character variation of the object: 'GREL' for a grid element." },
        "floorIndex": { "type": "integer", "description": "Home story index." },
        "layerIndex": { "type": "integer", "description": "Layer attribute index." },
        "pos": { "$ref": "#/Coordinate2D", "description": "Insertion point: the start of the grid line." },
        "angle": { "type": "number", "description": "Direction of the grid line in radians." },
        "length": { "type": "number", "description": "Length of the grid line: the GDL parameter AC_Length of the Grid Tool's grid marker, or the object's A size for a library part without it." },
        "xRatio": { "type": "number", "description": "The object's xRatio as stored: its A size with useXYFixSize on, otherwise the A size divided by the library part's own A." },
        "yRatio": { "type": "number", "description": "The object's yRatio as stored, like xRatio for the B size." },
        "useXYFixSize": { "type": "boolean", "description": "Whether xRatio and yRatio hold the sizes themselves." },
        "level": { "type": "number", "description": "Elevation of the object from its home story." },
        "libInd": { "type": "integer", "description": "Library part index." },
        "libraryPartName": { "type": "string", "description": "Localized name of the library part." },
        "parameters": {
            "type": "array",
            "description": "GDL parameters, separators left out. Only when requested.",
            "items": {
                "type": "object",
                "properties": {
                    "name": { "type": "string" },
                    "displayName": { "type": "string" },
                    "type": { "type": "string", "description": "Integer, Length, Angle, RealNum, LightSwitch, ColorRGB, Intensity, LineType, Material, FillPattern, PenColor, String, Boolean, Title, BuildingMaterial, Profile, Dictionary or Unknown." },
                    "value": { "description": "Number for numeric and boolean types, string for String and Title. Not given for arrays." },
                    "dimension1": { "type": "integer", "description": "First dimension of an array parameter." },
                    "dimension2": { "type": "integer", "description": "Second dimension of an array parameter." }
                },
                "required": ["name", "displayName", "type"]
            }
        })";

} // namespace

// ---------------------------------------------------------------------------
// GetGridElementDefaults
// ---------------------------------------------------------------------------

GetGridElementDefaultsCommand::GetGridElementDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGridElementDefaultsCommand::GetName () const
{
    return "GetGridElementDefaults";
}

GS::Optional<GS::UniString> GetGridElementDefaultsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {},
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetGridElementDefaultsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {)") + GridElementDetailsSchema + R"(
        },
        "required": ["variation", "floorIndex", "layerIndex", "pos", "angle", "length", "xRatio", "yRatio", "useXYFixSize", "level", "libInd", "libraryPartName", "parameters"]
    })";
}

GS::ObjectState GetGridElementDefaultsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    API_Element element = {};
    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    element.header.type = GridElemType ();
    const GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the Grid Element defaults.");
    }
    return DescribeGridElement (element, memo.params, true);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Grid Element commands need Archicad 26 or later.");
#endif
}

// ---------------------------------------------------------------------------
// GetGridElements
// ---------------------------------------------------------------------------

GetGridElementsCommand::GetGridElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGridElementsCommand::GetName () const
{
    return "GetGridElements";
}

GS::Optional<GS::UniString> GetGridElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "includeParameters": {
                "type": "boolean",
                "description": "Also return the GDL parameters of each grid element. Default false."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetGridElementsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "gridElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {)") + GridElementDetailsSchema + R"(
                    },
                    "required": ["elementId", "variation", "floorIndex", "layerIndex", "pos", "angle", "length", "xRatio", "yRatio", "useXYFixSize", "level", "libInd", "libraryPartName"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["gridElements"]
    })";
}

GS::ObjectState GetGridElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    bool includeParameters = false;
    parameters.Get ("includeParameters", includeParameters);

    // GetElemList (GridElemType ()) ignores the variation and returns every object (confirmed
    // live: elevation and story level markers came back too), so scan all objects and keep
    // the 'GREL' variation.
    GS::Array<API_Guid> objectGuids;
    ACAPI_Element_GetElemList (API_ObjectID, &objectGuids);
    GS::Array<API_Guid> guids;
    for (const API_Guid& guid : objectGuids) {
        API_Elem_Head head = {};
        head.guid = guid;
        if (ACAPI_Element_GetHeader (&head) == NoError && head.type.variationID == APIVarId_GridElement) {
            guids.Push (guid);
        }
    }

    GS::ObjectState response;
    const auto& list = response.AddList<GS::ObjectState> ("gridElements");
    for (const API_Guid& guid : guids) {
        API_Element element = {};
        element.header.guid = guid;
        if (ACAPI_Element_Get (&element) != NoError) {
            continue;
        }
        // The parameters are read for every element, as the length comes from AC_Length.
        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
        ACAPI_Element_GetMemo (guid, &memo, APIMemoMask_AddPars);
        list (DescribeGridElement (element, memo.params, includeParameters));
    }
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Grid Element commands need Archicad 26 or later.");
#endif
}

// ---------------------------------------------------------------------------
// CreateGridElements
// ---------------------------------------------------------------------------

CreateGridElementsCommand::CreateGridElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateGridElementsCommand::GetName () const
{
    return "CreateGridElements";
}

GS::Optional<GS::UniString> CreateGridElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "gridElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "begCoordinate": { "$ref": "#/Coordinate2D", "description": "Start point of the grid line (the object's insertion point)." },
                        "endCoordinate": { "$ref": "#/Coordinate2D", "description": "End point of the grid line. Sets the direction and, unless lengthMode is 'none', the length." },
                        "lengthMode": { "type": "string", "enum": ["length", "none"], "description": "How the line length is applied. length (default): to the GDL parameter AC_Length when the library part has it, as the Grid Tool's grid marker does; otherwise to the object's A size, converted to xRatio as CreateObjects does. none: the length is left to 'parameters'." },
                        "floorIndex": { "type": "integer", "description": "Home story index. Default: the tool default." },
                        "layerIndex": { "type": "integer", "description": "Layer attribute index. Only a positive index takes effect. Default: the tool default." },
                        "parameters": {
                            "type": "array",
                            "description": "GDL parameters to set by name (read the names with GetGridElementDefaults). The value is a number, a boolean or a string. Unknown names and wrong value types are reported in warnings and skipped. The tool defaults can look different from a grid drawn with the Grid Tool (for example a short line with a leader to the bubble); to get the same look, copy the parameters of such a grid (GetGridElements with includeParameters).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "name": { "type": "string" },
                                    "value": { "description": "A number or a boolean for a numeric parameter, a string for a String or Title parameter." }
                                },
                                "additionalProperties": false,
                                "required": ["name", "value"]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["gridElements"]
    })";
}

GS::Optional<GS::UniString> CreateGridElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": { "$ref": "#/ElementIdsOrErrors" },
            "warnings": { "type": "array", "items": { "type": "string" }, "description": "Parameters that were not set, prefixed with the index of the grid element." }
        },
        "additionalProperties": false,
        "required": ["elements", "warnings"]
    })";
}

GS::ObjectState CreateGridElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> items;
    parameters.Get ("gridElements", items);

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    GS::Array<GS::UniString> warnings;

#ifdef ServerMainVers_2600
    ACAPI_CallUndoableCommand ("Create Grid Elements", [&] () -> GSErrCode {
        UIndex itemIndex = 0;
        for (const GS::ObjectState& item : items) {
            const GS::UniString prefix = GS::UniString::Printf ("item %u: ", (unsigned) itemIndex++);

            API_Element element = {};
            API_ElementMemo memo = {};
            const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            element.header.type = GridElemType ();
            GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, prefix + "Failed to read the Grid Element defaults."));
                continue;
            }

            GS::ObjectState begOS;
            GS::ObjectState endOS;
            item.Get ("begCoordinate", begOS);
            item.Get ("endCoordinate", endOS);
            const API_Coord beg = Get2DCoordinateFromObjectState (begOS);
            const API_Coord end = Get2DCoordinateFromObjectState (endOS);
            const double dx = end.x - beg.x;
            const double dy = end.y - beg.y;
            const double length = std::sqrt (dx * dx + dy * dy);
            if (length < 1e-6) {
                elements (CreateErrorResponse (APIERR_BADPARS, prefix + "begCoordinate and endCoordinate are the same point."));
                continue;
            }

            element.object.pos = beg;
            element.object.angle = std::atan2 (dy, dx);

            // The Grid Tool's grid marker draws the length given in AC_Length. A library part without
            // it gets the length as its A size, which xRatio is then converted from below.
            bool sizeChanged = false;
            GS::UniString lengthMode = "length";
            item.Get ("lengthMode", lengthMode);
            if (lengthMode == "length" && !SetNumericParameter (memo.params, "AC_Length", length)) {
                sizeChanged = SetNumericParameter (memo.params, "A", length);
            }

            Int32 floorIndex = 0;
            if (item.Get ("floorIndex", floorIndex)) {
                element.header.floorInd = (short) floorIndex;
            }
            Int32 layerIndex = 0;
            if (item.Get ("layerIndex", layerIndex) && layerIndex > 0) {
                element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
            }

            GS::Array<GS::ObjectState> params;
            if (item.Get ("parameters", params)) {
                GS::Array<GS::UniString> itemWarnings;
                for (const GS::ObjectState& p : params) {
                    GS::UniString name;
                    p.Get ("name", name);
                    if (ApplyParameter (memo.params, p, itemWarnings) && (name == "A" || name == "B")) {
                        sizeChanged = true;
                    }
                }
                for (const GS::UniString& w : itemWarnings) {
                    warnings.Push (prefix + w);
                }
            }

            // As in CreateObjects: xRatio/yRatio hold the size itself only with useXYFixSize on,
            // otherwise the size divided by the library part's own A/B, and they have to agree
            // with A and B in the memo, which Archicad reconciles them with on a later change.
            if (sizeChanged) {
                double libraryPartA = 1.0;
                double libraryPartB = 1.0;
                if (!element.object.useXYFixSize) {
                    err = GetLibraryPartOwnSize ((Int32) element.object.libInd, libraryPartA, libraryPartB);
                    if (err != NoError) {
                        elements (CreateErrorResponse (err, prefix + "Failed to read the library part's own A and B."));
                        continue;
                    }
                }
                double a = 0.0;
                double b = 0.0;
                if (GetNumericParameter (memo.params, "A", a)) {
                    element.object.xRatio = a / libraryPartA;
                }
                if (GetNumericParameter (memo.params, "B", b)) {
                    element.object.yRatio = b / libraryPartB;
                }
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, prefix + "Failed to create the Grid Element."));
                continue;
            }
            elements (CreateElementIdObjectState (element.header.guid));
        }
        return NoError;
    });
#else
    for (UIndex ii = 0; ii < items.GetSize (); ++ii) {
        elements (CreateErrorResponse (APIERR_NOTSUPPORTED, "Grid Element commands need Archicad 26 or later."));
    }
#endif

    const auto& warningList = response.AddList<GS::UniString> ("warnings");
    for (const GS::UniString& w : warnings) {
        warningList (w);
    }
    return response;
}
