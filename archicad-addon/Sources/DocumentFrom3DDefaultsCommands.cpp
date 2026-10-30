#include "DocumentFrom3DDefaultsCommands.hpp"
#include "MigrationHelper.hpp"

#include <type_traits>

// Both commands mirror API_DocumentFrom3DDefaults, the settings a newly created 3D Document
// starts with. Existing 3D Documents are not affected. The fields are named after the
// native struct members. Not exposed yet: the skeleton options and the cut surface fill,
// uncut surface fill type, hidden edge visibility and on cut plane visibility enums of
// the uniform attribute options; Set keeps their current values.

namespace {

// Some index fields of the struct are API_AttributeIndex and others plain shorts, and
// API_AttributeIndex is itself a plain integer before AC27; these work with all of them.
template <typename T>
Int32 GetIndexValue (const T& value, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr)
{
    return static_cast<Int32> (value);
}

template <typename T>
Int32 GetIndexValue (const T& value, typename std::enable_if<!std::is_arithmetic<T>::value>::type* = nullptr)
{
    return GetAttributeIndex (value);
}

template <typename T>
void SetIndexValue (T& field, Int32 value, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr)
{
    field = static_cast<T> (value);
}

template <typename T>
void SetIndexValue (T& field, Int32 value, typename std::enable_if<!std::is_arithmetic<T>::value>::type* = nullptr)
{
    field = ACAPI_CreateAttributeIndex (value);
}

GS::Optional<GS::UniString> GetCutPlanePlanConnName (API_CutPlanePlanConn intPlanConn)
{
    switch (intPlanConn) {
        case APICutPl_Online:  return GS::UniString ("Online");
        case APICutPl_Offline: return GS::UniString ("Offline");
        case APICutPl_Drawing: return GS::UniString ("Drawing");
        default:               return GS::NoValue;
    }
}

GS::Optional<API_CutPlanePlanConn> GetCutPlanePlanConnFromName (const GS::UniString& name)
{
    if (name == "Online") {
        return APICutPl_Online;
    } else if (name == "Offline") {
        return APICutPl_Offline;
    } else if (name == "Drawing") {
        return APICutPl_Drawing;
    }
    return GS::NoValue;
}

GS::Optional<GS::UniString> GetBoundaryDisplayName (API_BoundaryDisplay boundaryDisplay)
{
    switch (boundaryDisplay) {
        case APIBound_UncutContours:    return GS::UniString ("UncutContours");
        case APIBound_NoContours:       return GS::UniString ("NoContours");
        case APIBound_OverrideContours: return GS::UniString ("OverrideContours");
        default:                        return GS::NoValue;
    }
}

GS::Optional<API_BoundaryDisplay> GetBoundaryDisplayFromName (const GS::UniString& name)
{
    if (name == "UncutContours") {
        return APIBound_UncutContours;
    } else if (name == "NoContours") {
        return APIBound_NoContours;
    } else if (name == "OverrideContours") {
        return APIBound_OverrideContours;
    }
    return GS::NoValue;
}

// Reads an optional integer parameter into a short field; false when it is present but
// does not fit.
bool GetOptionalShort (const GS::ObjectState& os, const char* name, short& field)
{
    Int32 value = 0;
    if (!os.Get (name, value)) {
        return true;
    }
    if (value < -32768 || value > 32767) {
        return false;
    }
    field = static_cast<short> (value);
    return true;
}

template <typename T>
void GetOptionalIndex (const GS::ObjectState& os, const char* name, T& field)
{
    Int32 value = 0;
    if (os.Get (name, value)) {
        SetIndexValue (field, value);
    }
}

void GetOptionalBool (const GS::ObjectState& os, const char* name, bool& field)
{
    bool value = false;
    if (os.Get (name, value)) {
        field = value;
    }
}

bool GetOptionalName (const GS::ObjectState& os, const char* name, GS::uchar_t (&field)[API_UniLongNameLen])
{
    GS::UniString value;
    if (!os.Get (name, value)) {
        return true;
    }
    if (value.GetLength () >= API_UniLongNameLen) {
        return false;
    }
    GS::ucscpy (field, value.ToUStr ().Get ());
    return true;
}

}

static const char* DocumentFrom3DDefaultsProperties = R"(
        "name": {
            "type": "string",
            "description": "The name of a new 3D Document."
        },
        "refId": {
            "type": "string",
            "description": "The reference ID of a new 3D Document."
        },
        "vect3DHatching": {
            "type": "boolean",
            "description": "Vectorial 3D hatching."
        },
        "vectSunShadow": {
            "type": "boolean",
            "description": "Vectorial sun shadows."
        },
        "mustBeTransparent": {
            "type": "boolean",
            "description": "Transparency of the model."
        },
        "intPlanConn": {
            "type": "string",
            "description": "The connection of the 3D Document to the model.",
            "enum": [
                "Online",
                "Offline",
                "Drawing"
            ]
        },
        "materialFrom3D": {
            "type": "boolean",
            "description": "Use the surfaces of the 3D model; when false, cMater is used for every element."
        },
        "contureOff": {
            "type": "boolean",
            "description": "Contours off."
        },
        "cMater": {
            "type": "integer",
            "description": "The index of the uniform surface (material), used when materialFrom3D is false."
        },
        "boundaryDisplay": {
            "type": "string",
            "description": "How the boundary contours are displayed.",
            "enum": [
                "UncutContours",
                "NoContours",
                "OverrideContours"
            ]
        },
        "boundaryPen": {
            "type": "integer",
            "description": "The pen of the boundary contours, used with OverrideContours."
        },
        "boundaryLineType": {
            "type": "integer",
            "description": "The index of the line type of the boundary contours, used with OverrideContours."
        },
        "attributeOptions": {
            "type": "object",
            "description": "The uniform attribute options (cut and uncut surfaces, shading, hidden edges).",
            "properties": {
                "shouldUseUniformCutAttributes": { "type": "boolean", "description": "Use uniform attributes on the cut surfaces." },
                "uniformLineType": { "type": "integer", "description": "The index of the uniform line type of the cut surfaces." },
                "uniformFillPen": { "type": "integer", "description": "The uniform fill pen of the cut surfaces." },
                "uniformFillBkgPen": { "type": "integer", "description": "The uniform fill background pen of the cut surfaces." },
                "shouldUseUncutSurfaceFill": { "type": "boolean", "description": "Fill the uncut surfaces." },
                "uncutSurfaceBkgPen": { "type": "integer", "description": "The background pen of the uncut surfaces." },
                "shouldUseElemPen": { "type": "boolean", "description": "Use a uniform pen for the element contours." },
                "elemPen": { "type": "integer", "description": "The uniform pen of the element contours." },
                "shadFillPen": { "type": "integer", "description": "The pen of the shadow fill." },
                "shadFill": { "type": "integer", "description": "The index of the shadow fill." },
                "shadFillBkgPen": { "type": "integer", "description": "The background pen of the shadow fill." },
                "shadContPen": { "type": "integer", "description": "The pen of the shadow contours." },
                "hiddenEdgeLineType": { "type": "integer", "description": "The index of the line type of the hidden edges." },
                "hiddenEdgeLinePen": { "type": "integer", "description": "The pen of the hidden edges." }
            },
            "additionalProperties": false
        }
)";

GetDocumentFrom3DDefaultsCommand::GetDocumentFrom3DDefaultsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String GetDocumentFrom3DDefaultsCommand::GetName () const
{
    return "GetDocumentFrom3DDefaults";
}

GS::Optional<GS::UniString> GetDocumentFrom3DDefaultsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {)") + DocumentFrom3DDefaultsProperties + R"(
    },
    "additionalProperties": false,
    "required": [
        "name",
        "refId",
        "vect3DHatching",
        "vectSunShadow",
        "mustBeTransparent",
        "materialFrom3D",
        "contureOff",
        "cMater",
        "boundaryPen",
        "boundaryLineType",
        "attributeOptions"
    ]
})";
}

GS::ObjectState GetDocumentFrom3DDefaultsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_DocumentFrom3DDefaults defaults {};
    GSErrCode err = ACAPI_View_GetDocumentFrom3DDefaults (&defaults);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the 3D Document defaults.");
    }

    const API_UniformAttributeOptions& options = defaults.attributeOptions;
    GS::ObjectState attributeOptions (
        "shouldUseUniformCutAttributes", options.shouldUseUniformCutAttributes,
        "uniformLineType", GetIndexValue (options.uniformLineType),
        "uniformFillPen", static_cast<Int32> (options.uniformFillPen),
        "uniformFillBkgPen", static_cast<Int32> (options.uniformFillBkgPen),
        "shouldUseUncutSurfaceFill", options.shouldUseUncutSurfaceFill,
        "uncutSurfaceBkgPen", static_cast<Int32> (options.uncutSurfaceBkgPen),
        "shouldUseElemPen", options.shouldUseElemPen,
        "elemPen", static_cast<Int32> (options.elemPen));
    attributeOptions.Add ("shadFillPen", static_cast<Int32> (options.shadFillPen));
    attributeOptions.Add ("shadFill", GetIndexValue (options.shadFill));
    attributeOptions.Add ("shadFillBkgPen", static_cast<Int32> (options.shadFillBkgPen));
    attributeOptions.Add ("shadContPen", static_cast<Int32> (options.shadContPen));
    attributeOptions.Add ("hiddenEdgeLineType", GetIndexValue (options.hiddenEdgeLineType));
    attributeOptions.Add ("hiddenEdgeLinePen", static_cast<Int32> (options.hiddenEdgeLinePen));

    GS::ObjectState response (
        "name", GS::UniString (defaults.name),
        "refId", GS::UniString (defaults.refId),
        "vect3DHatching", defaults.vect3DHatching,
        "vectSunShadow", defaults.vectSunShadow,
        "mustBeTransparent", defaults.mustBeTransparent,
        "materialFrom3D", defaults.materialFrom3D,
        "contureOff", defaults.contureOff,
        "cMater", GetIndexValue (defaults.cMater));
    response.Add ("boundaryPen", static_cast<Int32> (defaults.boundaryPen));
    response.Add ("boundaryLineType", GetIndexValue (defaults.boundaryLineType));
    response.Add ("attributeOptions", attributeOptions);

    const GS::Optional<GS::UniString> intPlanConn = GetCutPlanePlanConnName (static_cast<API_CutPlanePlanConn> (defaults.intPlanConn));
    if (intPlanConn.HasValue ()) {
        response.Add ("intPlanConn", intPlanConn.Get ());
    }
    const GS::Optional<GS::UniString> boundaryDisplay = GetBoundaryDisplayName (static_cast<API_BoundaryDisplay> (defaults.boundaryDisplay));
    if (boundaryDisplay.HasValue ()) {
        response.Add ("boundaryDisplay", boundaryDisplay.Get ());
    }

    return response;
}

SetDocumentFrom3DDefaultsCommand::SetDocumentFrom3DDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetDocumentFrom3DDefaultsCommand::GetName () const
{
    return "SetDocumentFrom3DDefaults";
}

GS::Optional<GS::UniString> SetDocumentFrom3DDefaultsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "description": "Only the given fields are changed, the rest keep their current default.",
    "properties": {)") + DocumentFrom3DDefaultsProperties + R"(
    },
    "additionalProperties": false
})";
}

GS::Optional<GS::UniString> SetDocumentFrom3DDefaultsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetDocumentFrom3DDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_DocumentFrom3DDefaults defaults {};
    GSErrCode err = ACAPI_View_GetDocumentFrom3DDefaults (&defaults);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current 3D Document defaults.");
    }

    if (!GetOptionalName (parameters, "name", defaults.name)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: name is too long.");
    }
    if (!GetOptionalName (parameters, "refId", defaults.refId)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: refId is too long.");
    }
    GetOptionalBool (parameters, "vect3DHatching", defaults.vect3DHatching);
    GetOptionalBool (parameters, "vectSunShadow", defaults.vectSunShadow);
    GetOptionalBool (parameters, "mustBeTransparent", defaults.mustBeTransparent);
    GetOptionalBool (parameters, "materialFrom3D", defaults.materialFrom3D);
    GetOptionalBool (parameters, "contureOff", defaults.contureOff);
    GetOptionalIndex (parameters, "cMater", defaults.cMater);
    GetOptionalIndex (parameters, "boundaryLineType", defaults.boundaryLineType);
    if (!GetOptionalShort (parameters, "boundaryPen", defaults.boundaryPen)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: boundaryPen.");
    }

    GS::UniString intPlanConnName;
    if (parameters.Get ("intPlanConn", intPlanConnName)) {
        const GS::Optional<API_CutPlanePlanConn> intPlanConn = GetCutPlanePlanConnFromName (intPlanConnName);
        if (!intPlanConn.HasValue ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: intPlanConn.");
        }
        defaults.intPlanConn = static_cast<decltype (defaults.intPlanConn)> (intPlanConn.Get ());
    }

    GS::UniString boundaryDisplayName;
    if (parameters.Get ("boundaryDisplay", boundaryDisplayName)) {
        const GS::Optional<API_BoundaryDisplay> boundaryDisplay = GetBoundaryDisplayFromName (boundaryDisplayName);
        if (!boundaryDisplay.HasValue ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: boundaryDisplay.");
        }
        defaults.boundaryDisplay = static_cast<decltype (defaults.boundaryDisplay)> (boundaryDisplay.Get ());
    }

    GS::ObjectState attributeOptions;
    if (parameters.Get ("attributeOptions", attributeOptions)) {
        API_UniformAttributeOptions& options = defaults.attributeOptions;
        GetOptionalBool (attributeOptions, "shouldUseUniformCutAttributes", options.shouldUseUniformCutAttributes);
        GetOptionalBool (attributeOptions, "shouldUseUncutSurfaceFill", options.shouldUseUncutSurfaceFill);
        GetOptionalBool (attributeOptions, "shouldUseElemPen", options.shouldUseElemPen);
        GetOptionalIndex (attributeOptions, "uniformLineType", options.uniformLineType);
        GetOptionalIndex (attributeOptions, "shadFill", options.shadFill);
        GetOptionalIndex (attributeOptions, "hiddenEdgeLineType", options.hiddenEdgeLineType);
        if (!GetOptionalShort (attributeOptions, "uniformFillPen", options.uniformFillPen) ||
            !GetOptionalShort (attributeOptions, "uniformFillBkgPen", options.uniformFillBkgPen) ||
            !GetOptionalShort (attributeOptions, "uncutSurfaceBkgPen", options.uncutSurfaceBkgPen) ||
            !GetOptionalShort (attributeOptions, "elemPen", options.elemPen) ||
            !GetOptionalShort (attributeOptions, "shadFillPen", options.shadFillPen) ||
            !GetOptionalShort (attributeOptions, "shadFillBkgPen", options.shadFillBkgPen) ||
            !GetOptionalShort (attributeOptions, "shadContPen", options.shadContPen) ||
            !GetOptionalShort (attributeOptions, "hiddenEdgeLinePen", options.hiddenEdgeLinePen)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: a pen of attributeOptions is out of range.");
        }
    }

    err = ACAPI_CallUndoableCommand ("SetDocumentFrom3DDefaultsCommand", [&]() -> GSErrCode {
        return ACAPI_View_ChangeDocumentFrom3DDefaults (&defaults);
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to set the 3D Document defaults.");
}
