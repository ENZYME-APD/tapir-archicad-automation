#include "3DStyleCommands.hpp"
#include "MigrationHelper.hpp"

// The commands mirror API_3DStyle, one JSON field per struct field. The enum fields
// (model3D, shadingContours, vectorialSunShadow) and the engine id are passed as their
// integer values: read an existing style to see the values Archicad uses.
// The ACAPI_View_*3DStyle functions are used from Archicad 27; older versions return an error.

static const char* Style3DSchemaProperties = R"(
            "name": {
                "type": "string",
                "description": "The name of the 3D style, unique within the project."
            },
            "engineId": {
                "type": "integer",
                "description": "The id of the 3D rendering engine (API_3DStyle.engineId)."
            },
            "model3D": {
                "type": "integer",
                "description": "The 3D model type: Block, Wireframe, Hidden Line or Shading, as the integer value of API_3DModelTypeID."
            },
            "transparency": {
                "type": "boolean",
                "description": "Transparency in shading."
            },
            "monochromeEnabled": {
                "type": "boolean",
                "description": "The model is shown in monochrome colors."
            },
            "monochromeSurfaceColor": {
                "$ref": "#/ColorRGB",
                "description": "The surface color of the monochrome model."
            },
            "monochromeContourColor": {
                "$ref": "#/ColorRGB",
                "description": "The contour color of the monochrome model."
            },
            "backgroundAsInRendering": {
                "type": "boolean",
                "description": "The background is the same as in the rendering settings."
            },
            "chainBackgroundColors": {
                "type": "boolean",
                "description": "The sky and the ground background colors are chained."
            },
            "backgroundSkyColor": {
                "$ref": "#/ColorRGB",
                "description": "The sky color of the background (API_3DStyle.bkgSkyColor)."
            },
            "backgroundColor": {
                "$ref": "#/ColorRGB",
                "description": "The (ground) color of the background (API_3DStyle.backGroundRGB)."
            },
            "shadingContours": {
                "type": "integer",
                "description": "The contours in shaded mode, as the integer value of API_ShadingContoursID."
            },
            "vectorialSunShadow": {
                "type": "integer",
                "description": "The vectorial sun shadow options, as the integer value of API_VectorShadowID."
            },
            "vectorialHatching": {
                "type": "boolean",
                "description": "Vectorial 3D hatching is on."
            },
            "contourThickness": {
                "type": "integer",
                "description": "The thickness of the contours, in pixels."
            },
            "silhouetteThickness": {
                "type": "integer",
                "description": "The thickness of the silhouettes, in pixels."
            },
            "vectorialHatchingThickness": {
                "type": "integer",
                "description": "The thickness of the vectorial hatching, in pixels."
            },
            "shadowContourThickness": {
                "type": "integer",
                "description": "The thickness of the shadow contours, in pixels."
            },
            "castShadowPercent": {
                "type": "integer",
                "description": "The intensity of the cast shadows, in percent."
            },
            "shadingPercent": {
                "type": "integer",
                "description": "The intensity of the shading, in percent."
            },
            "shadowElevation": {
                "type": "number",
                "description": "The elevation of the shadow plane, used by the one level vectorial sun shadow options."
            },
            "show3DHotSpotsOnly": {
                "type": "boolean",
                "description": "Only the 3D hotspots are shown."
            })";

#ifdef ServerMainVers_2700

static GS::ObjectState Create3DStyleColorObjectState (const API_RGBColor& color)
{
    return GS::ObjectState ("red", color.f_red, "green", color.f_green, "blue", color.f_blue);
}

static GS::ObjectState Create3DStyleObjectState (const API_3DStyle& style)
{
    GS::ObjectState os;
    os.Add ("name", GS::UniString (style.name));
    os.Add ("engineId", static_cast<Int32> (style.engineId));
    os.Add ("model3D", static_cast<Int32> (style.model3D));
    os.Add ("transparency", style.transparency);
    os.Add ("monochromeEnabled", style.monochromeEnabled);
    os.Add ("monochromeSurfaceColor", Create3DStyleColorObjectState (style.monochromeSurfaceRGB));
    os.Add ("monochromeContourColor", Create3DStyleColorObjectState (style.monochromeContourRGB));
    os.Add ("backgroundAsInRendering", style.bkgAsInRendering);
    os.Add ("chainBackgroundColors", style.chainBkgColors);
    os.Add ("backgroundSkyColor", Create3DStyleColorObjectState (style.bkgSkyColor));
    os.Add ("backgroundColor", Create3DStyleColorObjectState (style.backGroundRGB));
    os.Add ("shadingContours", static_cast<Int32> (style.shadCont));
    os.Add ("vectorialSunShadow", static_cast<Int32> (style.vectSunShadow));
    os.Add ("vectorialHatching", style.vectHatchOn);
    os.Add ("contourThickness", static_cast<Int32> (style.contourThickness));
    os.Add ("silhouetteThickness", static_cast<Int32> (style.silhouetteThickness));
    os.Add ("vectorialHatchingThickness", static_cast<Int32> (style.vectorialHatchingThickness));
    os.Add ("shadowContourThickness", static_cast<Int32> (style.shadowContourThickness));
    os.Add ("castShadowPercent", static_cast<Int32> (style.castShadowPercent));
    os.Add ("shadingPercent", static_cast<Int32> (style.shadingPercent));
    os.Add ("shadowElevation", style.shadowElevation);
    os.Add ("show3DHotSpotsOnly", style.show3DHotSpotsOnly);
    return os;
}

template <typename T>
static void GetIntegerField (const GS::ObjectState& os, const char* fieldName, T& target)
{
    Int32 value = 0;
    if (os.Get (fieldName, value)) {
        target = static_cast<T> (value);
    }
}

static void GetBoolField (const GS::ObjectState& os, const char* fieldName, bool& target)
{
    bool value = false;
    if (os.Get (fieldName, value)) {
        target = value;
    }
}

static void Set3DStyleName (API_3DStyle& style, const GS::UniString& name)
{
    const USize maxLength = static_cast<USize> (API_UniLongNameLen - 1);
    GS::ucscpy (style.name, name.ToUStr (0, GS::Min (name.GetLength (), maxLength)).Get ());
}

// Changes only the fields given in os; the name is not touched.
static void Apply3DStyleFields (const GS::ObjectState& os, API_3DStyle& style)
{
    GetIntegerField (os, "engineId", style.engineId);
    GetIntegerField (os, "model3D", style.model3D);
    GetBoolField (os, "transparency", style.transparency);
    GetBoolField (os, "monochromeEnabled", style.monochromeEnabled);
    GetColor (os, "monochromeSurfaceColor", style.monochromeSurfaceRGB);
    GetColor (os, "monochromeContourColor", style.monochromeContourRGB);
    GetBoolField (os, "backgroundAsInRendering", style.bkgAsInRendering);
    GetBoolField (os, "chainBackgroundColors", style.chainBkgColors);
    GetColor (os, "backgroundSkyColor", style.bkgSkyColor);
    GetColor (os, "backgroundColor", style.backGroundRGB);
    GetIntegerField (os, "shadingContours", style.shadCont);
    GetIntegerField (os, "vectorialSunShadow", style.vectSunShadow);
    GetBoolField (os, "vectorialHatching", style.vectHatchOn);
    GetIntegerField (os, "contourThickness", style.contourThickness);
    GetIntegerField (os, "silhouetteThickness", style.silhouetteThickness);
    GetIntegerField (os, "vectorialHatchingThickness", style.vectorialHatchingThickness);
    GetIntegerField (os, "shadowContourThickness", style.shadowContourThickness);
    GetIntegerField (os, "castShadowPercent", style.castShadowPercent);
    GetIntegerField (os, "shadingPercent", style.shadingPercent);
    double shadowElevation = 0.0;
    if (os.Get ("shadowElevation", shadowElevation)) {
        style.shadowElevation = shadowElevation;
    }
    GetBoolField (os, "show3DHotSpotsOnly", style.show3DHotSpotsOnly);
}

#endif

Get3DStyleDetailsCommand::Get3DStyleDetailsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Get3DStyleDetailsCommand::GetName () const
{
    return "Get3DStyleDetails";
}

GS::Optional<GS::UniString> Get3DStyleDetailsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "names": {
            "type": "array",
            "description": "The names of the 3D styles. An empty name means the current 3D style. When omitted, the current 3D style is returned.",
            "items": {
                "type": "string"
            }
        }
    },
    "additionalProperties": false,
    "required": []
})";
}

GS::Optional<GS::UniString> Get3DStyleDetailsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "styles": {
            "type": "array",
            "description": "The details of the 3D styles, in the order of the input names.",
            "items": {
                "type": "object",
                "oneOf": [
                    {
                        "type": "object",
                        "properties": {)") + Style3DSchemaProperties + R"(
                        },
                        "additionalProperties": false,
                        "required": [
                            "name"
                        ]
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
        "styles"
    ]
})";
}

GS::ObjectState Get3DStyleDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::UniString> names;
    if (!parameters.Get ("names", names)) {
        names.Push (GS::UniString ());
    }

    GS::ObjectState response;
    const auto& styles = response.AddList<GS::ObjectState> ("styles");
    for (const GS::UniString& name : names) {
        API_3DStyle style {};
        Set3DStyleName (style, name);
        const GSErrCode err = ACAPI_View_Get3DStyle (&style);
        if (err != NoError) {
            styles (CreateErrorResponse (err, GS::UniString ("Failed to get the 3D style '") + name + "'."));
            continue;
        }
        styles (Create3DStyleObjectState (style));
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

Create3DStylesCommand::Create3DStylesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Create3DStylesCommand::GetName () const
{
    return "Create3DStyles";
}

GS::Optional<GS::UniString> Create3DStylesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "styles": {
            "type": "array",
            "description": "The 3D styles to create. Each starts from the current 3D style; only the given fields differ from it.",
            "items": {
                "type": "object",
                "properties": {)") + Style3DSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "name"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "styles"
    ]
})";
}

GS::Optional<GS::UniString> Create3DStylesCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "executionResults": {
            "$ref": "#/ExecutionResults"
        }
    },
    "additionalProperties": false,
    "required": [
        "executionResults"
    ]
})";
}

GS::ObjectState Create3DStylesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> styles;
    if (!parameters.Get ("styles", styles)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: styles.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Create3DStylesCommand", [&]() -> GSErrCode {
        for (const GS::ObjectState& styleOS : styles) {
            GS::UniString name;
            if (!styleOS.Get ("name", name) || name.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: name."));
                continue;
            }

            API_3DStyle style {};
            GSErrCode err = ACAPI_View_Get3DStyle (&style);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get the current 3D style."));
                continue;
            }

            Apply3DStyleFields (styleOS, style);
            Set3DStyleName (style, name);
            err = ACAPI_View_Create3DStyle (&style);
            executionResults (err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, GS::UniString ("Failed to create the 3D style '") + name + "'. The name may already be used."));
        }
        return NoError;
    });

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

Change3DStylesCommand::Change3DStylesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Change3DStylesCommand::GetName () const
{
    return "Change3DStyles";
}

GS::Optional<GS::UniString> Change3DStylesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "styles": {
            "type": "array",
            "description": "The 3D styles to change, identified by their name. Only the given fields change.",
            "items": {
                "type": "object",
                "properties": {)") + Style3DSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "name"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "styles"
    ]
})";
}

GS::Optional<GS::UniString> Change3DStylesCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "executionResults": {
            "$ref": "#/ExecutionResults"
        }
    },
    "additionalProperties": false,
    "required": [
        "executionResults"
    ]
})";
}

GS::ObjectState Change3DStylesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> styles;
    if (!parameters.Get ("styles", styles)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: styles.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Change3DStylesCommand", [&]() -> GSErrCode {
        for (const GS::ObjectState& styleOS : styles) {
            GS::UniString name;
            if (!styleOS.Get ("name", name) || name.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: name."));
                continue;
            }

            API_3DStyle style {};
            Set3DStyleName (style, name);
            GSErrCode err = ACAPI_View_Get3DStyle (&style);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, GS::UniString ("Failed to get the 3D style '") + name + "'."));
                continue;
            }

            Apply3DStyleFields (styleOS, style);
            err = ACAPI_View_Change3DStyle (&style);
            executionResults (err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, GS::UniString ("Failed to change the 3D style '") + name + "'."));
        }
        return NoError;
    });

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}
