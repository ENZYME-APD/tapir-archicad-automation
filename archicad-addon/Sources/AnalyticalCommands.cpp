#include "AnalyticalCommands.hpp"
#include "MigrationHelper.hpp"

// The Structural Analytical Model functions are called with their ACAPI_Analytical_* names and the
// add-on's own token, which is how the Archicad 27 and newer DevKits declare them. Archicad 26 declares
// the same functions in ACAPI_Analytical.hpp as ACAPI_AnalyticalModel_*, and Archicad 25 in ACAPinc.h,
// without the token and with pointer outputs. The commands that only call functions of Archicad 27 and
// newer report "not supported" before; the others call the older names through the TAPIR_ wrappers below.
#ifdef ServerMainVers_2600
#include "ACAPI_Analytical.hpp"
#endif

namespace {

template <typename EnumType>
struct EnumName
{
    EnumType    value;
    const char* name;
};

template <typename EnumType, size_t N>
const char* EnumToString (const EnumName<EnumType> (&names)[N], EnumType value)
{
    for (const EnumName<EnumType>& item : names) {
        if (item.value == value) {
            return item.name;
        }
    }
    return nullptr;
}

template <typename EnumType, size_t N>
bool EnumFromString (const EnumName<EnumType> (&names)[N], const GS::UniString& name, EnumType& value)
{
    for (const EnumName<EnumType>& item : names) {
        if (name == GS::UniString (item.name)) {
            value = item.value;
            return true;
        }
    }
    return false;
}

#ifdef ServerMainVers_2600
const EnumName<API_AnalyticalMemberType> AnalyticalMemberTypeNames[] = {
    { API_AnalyticalMemberType::Unknown, "Unknown" },
    { API_AnalyticalMemberType::Beam,    "Beam" },
    { API_AnalyticalMemberType::Column,  "Column" },
    { API_AnalyticalMemberType::Roof,    "Roof" },
    { API_AnalyticalMemberType::Slab,    "Slab" },
    { API_AnalyticalMemberType::Wall,    "Wall" }
};

const EnumName<API_AM_VisibilityMode> AnalyticalVisibilityModeNames[] = {
    { API_AM_VisibilityMode::AnalyticalModelOnly,              "AnalyticalModelOnly" },
    { API_AM_VisibilityMode::AnalyticalModelWithinEntireModel, "AnalyticalModelWithinEntireModel" }
};

const EnumName<API_LoadVisibilityMode> AnalyticalLoadVisibilityModeNames[] = {
    { API_LoadVisibilityMode::API_LoadVisibilityMode_FilterByActive, "FilterByActive" },
    { API_LoadVisibilityMode::API_LoadVisibilityMode_HideAll,        "HideAll" },
    { API_LoadVisibilityMode::API_LoadVisibilityMode_ShowAll,        "ShowAll" }
};

struct VisibilityFlag
{
    bool API_AM_VisibilitySettings::* field;
    const char* name;
};

const VisibilityFlag AnalyticalVisibilityFlags[] = {
    { &API_AM_VisibilitySettings::isAnalyticalModelVisible,       "isAnalyticalModelVisible" },
    { &API_AM_VisibilitySettings::areMembersTransparent,          "areMembersTransparent" },
    { &API_AM_VisibilitySettings::areSymbolsTransparent,          "areSymbolsTransparent" },
    { &API_AM_VisibilitySettings::areNodesVisible,                "areNodesVisible" },
    { &API_AM_VisibilitySettings::areReleasesVisible,             "areReleasesVisible" },
    { &API_AM_VisibilitySettings::areCrossSectionsVisible,        "areCrossSectionsVisible" },
    { &API_AM_VisibilitySettings::areCoordinationProfilesVisible, "areCoordinationProfilesVisible" },
    { &API_AM_VisibilitySettings::areLocalCoordSystemsVisible,    "areLocalCoordSystemsVisible" }
};
#else
// Archicad 25 names the member types API_AnalyticalMember_<Name>Type; the JSON uses the names of Archicad 26 and newer.
const EnumName<API_AnalyticalMemberType> AnalyticalMemberTypeNames[] = {
    { API_AnalyticalMember_UnknownType, "Unknown" },
    { API_AnalyticalMember_BeamType,    "Beam" },
    { API_AnalyticalMember_ColumnType,  "Column" },
    { API_AnalyticalMember_RoofType,    "Roof" },
    { API_AnalyticalMember_SlabType,    "Slab" },
    { API_AnalyticalMember_WallType,    "Wall" }
};
#endif

GSErrCode TAPIR_GetAnalyticalModelVariation (const API_Guid& amGuid, API_AnalyticalModelVariation& variation)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalModelVariation (ACAPI_GetToken (), amGuid, variation);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalModelVariation (ACAPI_GetToken (), amGuid, variation);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalModelVariation (amGuid, &variation);
#endif
}

GSErrCode TAPIR_GetAnalyticalModel (const API_AnalyticalModelVariation& variation, API_Guid& amGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalModel (ACAPI_GetToken (), variation, amGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalModel (ACAPI_GetToken (), variation, amGuid);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalModel (variation, &amGuid);
#endif
}

GSErrCode TAPIR_GetComplementaryProjectMemberType (const API_Guid& elemGuid, API_AnalyticalMemberType& memberType)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetComplementaryProjectMemberType (ACAPI_GetToken (), elemGuid, memberType);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetComplementaryProjectMemberType (ACAPI_GetToken (), elemGuid, memberType);
#else
    return ACAPI_AnalyticalModel_GetComplementaryProjectMemberType (elemGuid, &memberType);
#endif
}

#ifdef ServerMainVers_2600
GSErrCode TAPIR_GetAnalyticalVisibilitySettings (API_AM_VisibilitySettings& settings)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetVisibilitySettings (ACAPI_GetToken (), settings);
#else
    return ACAPI_AnalyticalModel_GetVisibilitySettings (ACAPI_GetToken (), settings);
#endif
}

GSErrCode TAPIR_SetAnalyticalVisibilitySettings (const API_AM_VisibilitySettings& settings)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_SetVisibilitySettings (ACAPI_GetToken (), settings);
#else
    return ACAPI_AnalyticalModel_SetVisibilitySettings (ACAPI_GetToken (), settings);
#endif
}
#endif

// Reads the optional analyticalModelId parameter. Without it the model of the current view is used,
// which the API can only tell from Archicad 27 on. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalModelGuidFromParameters (const GS::ObjectState& parameters, API_Guid& amGuid)
{
    const GS::ObjectState* analyticalModelId = parameters.Get ("analyticalModelId");
    if (analyticalModelId != nullptr) {
        amGuid = GetGuidFromObjectState (*analyticalModelId);
        if (amGuid == APINULLGuid) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid analyticalModelId.");
        }
        return GS::NoValue;
    }

#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), amGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the current view.");
    }
    return GS::NoValue;
#else
    return CreateErrorResponse (APIERR_BADPARS, "analyticalModelId is required before Archicad 27.");
#endif
}

GS::ObjectState CreateAnalyticalModelVariationObjectState (const API_AnalyticalModelVariation& variation)
{
    GS::ObjectState variationOS;
#ifdef ServerMainVers_2700
    variationOS.Add ("amVariationGuid", CreateGuidObjectState (variation.amVariationGuid));
#else
    variationOS.Add ("renovationFilterGuid", CreateGuidObjectState (variation.renovationFilterGuid));
#ifdef ServerMainVers_2600
    const auto& classIds = variationOS.AddList<GS::ObjectState> ("layerConnectionClassIds");
    for (const auto& kv : variation.layerConnectionClassIds) {
        // Pre-2800 GS::HashTable's const range-for pair exposes pointer members.
        GS::ObjectState classId;
        classId.Add ("layerIndex", GetAttributeIndex (*kv.key));
        classId.Add ("connectionClassId", *kv.value);
        classIds (classId);
    }
#else
    const auto& classIds = variationOS.AddList<Int32> ("layerConnectionClassIds");
    for (Int32 classId : variation.layerConnectionClassIds) {
        classIds (classId);
    }
#endif
#endif
    return variationOS;
}

// Fills the variation from its JSON form. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalModelVariationFromObjectState (const GS::ObjectState& variationOS, API_AnalyticalModelVariation& variation)
{
#ifdef ServerMainVers_2700
    const GS::ObjectState* amVariationGuid = variationOS.Get ("amVariationGuid");
    if (amVariationGuid == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "variation.amVariationGuid is required from Archicad 27.");
    }
    variation.amVariationGuid = GetGuidFromObjectState (*amVariationGuid);
#else
    const GS::ObjectState* renovationFilterGuid = variationOS.Get ("renovationFilterGuid");
    if (renovationFilterGuid == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "variation.renovationFilterGuid is required before Archicad 27.");
    }
    variation.renovationFilterGuid = GetGuidFromObjectState (*renovationFilterGuid);
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> classIds;
    variationOS.Get ("layerConnectionClassIds", classIds);
    for (const GS::ObjectState& classId : classIds) {
        Int32 layerIndex = 0;
        Int32 connectionClassId = 0;
        if (!classId.Get ("layerIndex", layerIndex) || !classId.Get ("connectionClassId", connectionClassId)) {
            return CreateErrorResponse (APIERR_BADPARS, "Each item of variation.layerConnectionClassIds needs a layerIndex and a connectionClassId in Archicad 26.");
        }
        variation.layerConnectionClassIds.Put (ACAPI_CreateAttributeIndex (layerIndex), connectionClassId);
    }
#else
    variationOS.Get ("layerConnectionClassIds", variation.layerConnectionClassIds);
#endif
#endif
    return GS::NoValue;
}

#ifdef ServerMainVers_2600
GS::ObjectState CreateAnalyticalVisibilitySettingsObjectState (const API_AM_VisibilitySettings& settings)
{
    GS::ObjectState settingsOS;
    const char* visibilityMode = EnumToString (AnalyticalVisibilityModeNames, settings.visibilityMode);
    if (visibilityMode != nullptr) {
        settingsOS.Add ("visibilityMode", GS::UniString (visibilityMode));
    }
    for (const VisibilityFlag& flag : AnalyticalVisibilityFlags) {
        settingsOS.Add (flag.name, settings.*(flag.field));
    }
    const char* loadVisibilityMode = EnumToString (AnalyticalLoadVisibilityModeNames, settings.loadVisibilityMode);
    if (loadVisibilityMode != nullptr) {
        settingsOS.Add ("loadVisibilityMode", GS::UniString (loadVisibilityMode));
    }
    return settingsOS;
}
#endif

}

GetCurrentAnalyticalModelCommand::GetCurrentAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentAnalyticalModelCommand::GetName () const
{
    return "GetCurrentAnalyticalModel";
}

GS::Optional<GS::UniString> GetCurrentAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model of the current view."
            }
        },
        "additionalProperties": false,
        "required": [
            "analyticalModelId"
        ]
    })";
}

GS::ObjectState GetCurrentAnalyticalModelCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Guid analyticalModelGuid = APINULLGuid;
    const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the current view.");
    }

    return GS::ObjectState ("analyticalModelId", CreateGuidObjectState (analyticalModelGuid));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

UpdateAnalyticalModelCommand::UpdateAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateAnalyticalModelCommand::GetName () const
{
    return "UpdateAnalyticalModel";
}

GS::Optional<GS::UniString> UpdateAnalyticalModelCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model to update. The model of the current view is updated when it is omitted."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> UpdateAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState UpdateAnalyticalModelCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::ObjectState* analyticalModelId = parameters.Get ("analyticalModelId");
    if (analyticalModelId != nullptr) {
        analyticalModelGuid = GetGuidFromObjectState (*analyticalModelId);
        if (analyticalModelGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid analyticalModelId.");
        }
    } else {
        const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to get the Structural Analytical Model of the current view.");
        }
    }

    const GSErrCode err = ACAPI_CallUndoableCommand ("Update Analytical Model", [&] () -> GSErrCode {
        return ACAPI_Analytical_UpdateAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to update the Structural Analytical Model.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_VARIABLE (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

SetAnalyticalGenerationSettingsToNoRuleCommand::SetAnalyticalGenerationSettingsToNoRuleCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetAnalyticalGenerationSettingsToNoRuleCommand::GetName () const
{
    return "SetAnalyticalGenerationSettingsToNoRule";
}

GS::Optional<GS::UniString> SetAnalyticalGenerationSettingsToNoRuleCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetAnalyticalGenerationSettingsToNoRuleCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_CallUndoableCommand ("Set Analytical Generation Settings To No Rule", [&] () -> GSErrCode {
        return ACAPI_Analytical_SetGenerationSettingsToNoRule (ACAPI_GetToken ());
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the Structural Analytical Model generation settings to no rule.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetAnalyticalModelCheckTypeIdCommand::GetAnalyticalModelCheckTypeIdCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelCheckTypeIdCommand::GetName () const
{
    return "GetAnalyticalModelCheckTypeId";
}

GS::Optional<GS::UniString> GetAnalyticalModelCheckTypeIdCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "modelCheckTypeId": {
                "$ref": "#/GuidId",
                "description": "The type identifier of the Structural Analytical Model check."
            }
        },
        "additionalProperties": false,
        "required": [
            "modelCheckTypeId"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelCheckTypeIdCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    API_Guid typeId = APINULLGuid;
    const GSErrCode err = ACAPI_Analytical_GetModelCheckTypeId (ACAPI_GetToken (), typeId);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model check type identifier.");
    }

    return GS::ObjectState ("modelCheckTypeId", CreateGuidObjectState (typeId));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

GetAnalyticalModelVariationCommand::GetAnalyticalModelVariationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelVariationCommand::GetName () const
{
    return "GetAnalyticalModelVariation";
}

GS::Optional<GS::UniString> GetAnalyticalModelVariationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model. The model of the current view is used when it is omitted; before Archicad 27 it is required."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetAnalyticalModelVariationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "variation": {
                "$ref": "#/AnalyticalModelVariation"
            }
        },
        "additionalProperties": false,
        "required": [
            "variation"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelVariationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> modelError = GetAnalyticalModelGuidFromParameters (parameters, analyticalModelGuid);
    if (modelError.HasValue ()) {
        return *modelError;
    }

    API_AnalyticalModelVariation variation = {};
    const GSErrCode err = TAPIR_GetAnalyticalModelVariation (analyticalModelGuid, variation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the variation of the Structural Analytical Model.");
    }

    return GS::ObjectState ("variation", CreateAnalyticalModelVariationObjectState (variation));
}

GetAnalyticalModelCommand::GetAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelCommand::GetName () const
{
    return "GetAnalyticalModel";
}

GS::Optional<GS::UniString> GetAnalyticalModelCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "variation": {
                "$ref": "#/AnalyticalModelVariation"
            }
        },
        "additionalProperties": false,
        "required": [
            "variation"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model of the variation."
            }
        },
        "additionalProperties": false,
        "required": [
            "analyticalModelId"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::ObjectState* variationOS = parameters.Get ("variation");
    if (variationOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "variation is missing.");
    }

    API_AnalyticalModelVariation variation = {};
    const GS::Optional<GS::ObjectState> variationError = GetAnalyticalModelVariationFromObjectState (*variationOS, variation);
    if (variationError.HasValue ()) {
        return *variationError;
    }

    API_Guid analyticalModelGuid = APINULLGuid;
    const GSErrCode err = TAPIR_GetAnalyticalModel (variation, analyticalModelGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the variation.");
    }

    return GS::ObjectState ("analyticalModelId", CreateGuidObjectState (analyticalModelGuid));
}

GetAnalyticalVisibilitySettingsCommand::GetAnalyticalVisibilitySettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalVisibilitySettingsCommand::GetName () const
{
    return "GetAnalyticalVisibilitySettings";
}

GS::Optional<GS::UniString> GetAnalyticalVisibilitySettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "visibilitySettings": {
                "$ref": "#/AnalyticalVisibilitySettings",
                "description": "The Structural Analytical Model visibility settings, with every field."
            }
        },
        "additionalProperties": false,
        "required": [
            "visibilitySettings"
        ]
    })";
}

GS::ObjectState GetAnalyticalVisibilitySettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    API_AM_VisibilitySettings settings;
    const GSErrCode err = TAPIR_GetAnalyticalVisibilitySettings (settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model visibility settings.");
    }

    return GS::ObjectState ("visibilitySettings", CreateAnalyticalVisibilitySettingsObjectState (settings));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

SetAnalyticalVisibilitySettingsCommand::SetAnalyticalVisibilitySettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetAnalyticalVisibilitySettingsCommand::GetName () const
{
    return "SetAnalyticalVisibilitySettings";
}

GS::Optional<GS::UniString> SetAnalyticalVisibilitySettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "visibilitySettings": {
                "$ref": "#/AnalyticalVisibilitySettings",
                "description": "The fields to change. The fields left out keep their current value."
            }
        },
        "additionalProperties": false,
        "required": [
            "visibilitySettings"
        ]
    })";
}

GS::Optional<GS::UniString> SetAnalyticalVisibilitySettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetAnalyticalVisibilitySettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    const GS::ObjectState* settingsOS = parameters.Get ("visibilitySettings");
    if (settingsOS == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "visibilitySettings is missing.");
    }

    API_AM_VisibilitySettings settings;
    GSErrCode err = TAPIR_GetAnalyticalVisibilitySettings (settings);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the Structural Analytical Model visibility settings.");
    }

    GS::UniString visibilityMode;
    if (settingsOS->Get ("visibilityMode", visibilityMode) && !EnumFromString (AnalyticalVisibilityModeNames, visibilityMode, settings.visibilityMode)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Unknown visibilityMode '%T'.", visibilityMode.ToPrintf ()));
    }
    for (const VisibilityFlag& flag : AnalyticalVisibilityFlags) {
        settingsOS->Get (flag.name, settings.*(flag.field));
    }
    GS::UniString loadVisibilityMode;
    if (settingsOS->Get ("loadVisibilityMode", loadVisibilityMode) && !EnumFromString (AnalyticalLoadVisibilityModeNames, loadVisibilityMode, settings.loadVisibilityMode)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Unknown loadVisibilityMode '%T'.", loadVisibilityMode.ToPrintf ()));
    }

    err = ACAPI_CallUndoableCommand ("Set Analytical Visibility Settings", [&] () -> GSErrCode {
        return TAPIR_SetAnalyticalVisibilitySettings (settings);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the Structural Analytical Model visibility settings.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_VARIABLE (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

GetAnalyticalMemberTypesOfElementsCommand::GetAnalyticalMemberTypesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalMemberTypesOfElementsCommand::GetName () const
{
    return "GetAnalyticalMemberTypesOfElements";
}

GS::Optional<GS::UniString> GetAnalyticalMemberTypesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalMemberTypesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "memberTypesOfElements": {
                "type": "array",
                "description": "The analytical member type of each element, in the order of the input. An element the type cannot be got for is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "memberType": {
                                    "$ref": "#/AnalyticalMemberType"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "memberType"
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
            "memberTypesOfElements"
        ]
    })";
}

GS::ObjectState GetAnalyticalMemberTypesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& memberTypesOfElements = response.AddList<GS::ObjectState> ("memberTypesOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            memberTypesOfElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_AnalyticalMemberType memberType = {};
        const GSErrCode err = TAPIR_GetComplementaryProjectMemberType (GetGuidFromObjectState (*elementId), memberType);
        if (err != NoError) {
            memberTypesOfElements (CreateErrorResponse (err, "Failed to get the analytical member type of the element."));
            continue;
        }

        const char* memberTypeName = EnumToString (AnalyticalMemberTypeNames, memberType);
        memberTypesOfElements (GS::ObjectState ("memberType", GS::UniString (memberTypeName != nullptr ? memberTypeName : "Unknown")));
    }

    return response;
}

namespace {

// The load enums are plain C enums with the same enumerators in Archicad 25 to 30; the JSON uses the
// enumerator names without their API_AnalyticalLoad*_ prefix.
const EnumName<API_AnalyticalLoadGroupType> AnalyticalLoadGroupTypeNames[] = {
    { API_AnalyticalLoadGroup_PermanentLoad,  "PermanentLoad" },
    { API_AnalyticalLoadGroup_VariableLoad,   "VariableLoad" },
    { API_AnalyticalLoadGroup_AccidentalLoad, "AccidentalLoad" },
    { API_AnalyticalLoadGroup_SeismicLoad,    "SeismicLoad" },
    { API_AnalyticalLoadGroup_MovingLoad,     "MovingLoad" },
    { API_AnalyticalLoadGroup_TensioningLoad, "TensioningLoad" },
    { API_AnalyticalLoadGroup_FireLoad,       "FireLoad" }
};

const EnumName<API_AnalyticalLoadGroupRelation> AnalyticalLoadGroupRelationNames[] = {
    { API_AnalyticalLoadGroup_ExclusiveRelation, "ExclusiveRelation" },
    { API_AnalyticalLoadGroup_StandardRelation,  "StandardRelation" },
    { API_AnalyticalLoadGroup_TogetherRelation,  "TogetherRelation" }
};

const EnumName<API_AnalyticalLoadCaseActionType> AnalyticalLoadCaseActionTypeNames[] = {
    { API_AnalyticalLoadCase_PermanentAction,  "PermanentAction" },
    { API_AnalyticalLoadCase_VariableAction,   "VariableAction" },
    { API_AnalyticalLoadCase_AccidentalAction, "AccidentalAction" }
};

const EnumName<API_AnalyticalLoadCaseLoadType> AnalyticalLoadCaseLoadTypeNames[] = {
    { API_AnalyticalLoadCase_SelfWeightLoad,  "SelfWeightLoad" },
    { API_AnalyticalLoadCase_PrestressLoad,   "PrestressLoad" },
    { API_AnalyticalLoadCase_DynamicLoad,     "DynamicLoad" },
    { API_AnalyticalLoadCase_StaticLoad,      "StaticLoad" },
    { API_AnalyticalLoadCase_TemperatureLoad, "TemperatureLoad" },
    { API_AnalyticalLoadCase_WindLoad,        "WindLoad" },
    { API_AnalyticalLoadCase_SnowLoad,        "SnowLoad" },
    { API_AnalyticalLoadCase_MaintenanceLoad, "MaintenanceLoad" },
    { API_AnalyticalLoadCase_FireLoad,        "FireLoad" },
    { API_AnalyticalLoadCase_MovingLoad,      "MovingLoad" },
    { API_AnalyticalLoadCase_SeismicLoad,     "SeismicLoad" },
    { API_AnalyticalLoadCase_StandardLoad,    "StandardLoad" },
    { API_AnalyticalLoadCase_OthersLoad,      "OthersLoad" }
};

const EnumName<API_AnalyticalLoadCaseDuration> AnalyticalLoadCaseDurationNames[] = {
    { API_AnalyticalLoadCase_LongDuration,          "LongDuration" },
    { API_AnalyticalLoadCase_MediumDuration,        "MediumDuration" },
    { API_AnalyticalLoadCase_ShortDuration,         "ShortDuration" },
    { API_AnalyticalLoadCase_InstantaneousDuration, "InstantaneousDuration" }
};

const EnumName<API_AnalyticalLoadCombinationCategory> AnalyticalLoadCombinationCategoryNames[] = {
    { API_AnalyticalLoadCombination_ULS,        "ULS" },
    { API_AnalyticalLoadCombination_SLS,        "SLS" },
    { API_AnalyticalLoadCombination_ALS,        "ALS" },
    { API_AnalyticalLoadCombination_ANS,        "ANS" },
    { API_AnalyticalLoadCombination_NotDefined, "NotDefined" }
};

const EnumName<API_AnalyticalLoadCombinationNationalStandard> AnalyticalLoadCombinationNationalStandardNames[] = {
    { API_AnalyticalLoadNationalStandard_ENULSSTRGEOSetB,       "ENULSSTRGEOSetB" },
    { API_AnalyticalLoadNationalStandard_ENULSSTRGEOSetC,       "ENULSSTRGEOSetC" },
    { API_AnalyticalLoadNationalStandard_ENAccidental1,         "ENAccidental1" },
    { API_AnalyticalLoadNationalStandard_ENAccidental2,         "ENAccidental2" },
    { API_AnalyticalLoadNationalStandard_ENSeismic,             "ENSeismic" },
    { API_AnalyticalLoadNationalStandard_ENSLSCharacteristic,   "ENSLSCharacteristic" },
    { API_AnalyticalLoadNationalStandard_ENSLSFrequent,         "ENSLSFrequent" },
    { API_AnalyticalLoadNationalStandard_ENSLSQuasipermanent,   "ENSLSQuasipermanent" },
    { API_AnalyticalLoadNationalStandard_IBCLRFDultimate,       "IBCLRFDultimate" },
    { API_AnalyticalLoadNationalStandard_IBCASDultimate,        "IBCASDultimate" },
    { API_AnalyticalLoadNationalStandard_IBCASDserviceability,  "IBCASDserviceability" },
    { API_AnalyticalLoadNationalStandard_IBCASDseismic,         "IBCASDseismic" },
    { API_AnalyticalLoadNationalStandard_IBCLRFDseismic,        "IBCLRFDseismic" }
};

#ifdef ServerMainVers_2700
const EnumName<API_LoadCombinationType> AnalyticalLoadCombinationTypeNames[] = {
    { API_LoadCombinationType::API_LoadCombinationType_Envelope,  "Envelope" },
    { API_LoadCombinationType::API_LoadCombinationType_Linear,    "Linear" },
    { API_LoadCombinationType::API_LoadCombinationType_Nonlinear, "Nonlinear" }
};
#endif

GSErrCode TAPIR_GetAnalyticalLoadGroups (GS::Array<API_Guid>& loadGroups)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadGroups (ACAPI_GetToken (), loadGroups);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroups (ACAPI_GetToken (), loadGroups);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroups (&loadGroups);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadGroup (const API_Guid& loadGroupGuid, API_AnalyticalLoadGroup& loadGroup)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid, loadGroup);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid, loadGroup);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroup (loadGroupGuid, &loadGroup);
#endif
}

GSErrCode TAPIR_CreateAnalyticalLoadGroup (API_AnalyticalLoadGroup& loadGroup)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_CreateAnalyticalLoadGroup (ACAPI_GetToken (), loadGroup);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadGroup (ACAPI_GetToken (), loadGroup);
#else
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadGroup (loadGroup);
#endif
}

GSErrCode TAPIR_DeleteAnalyticalLoadGroup (const API_Guid& loadGroupGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_DeleteAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid);
#else
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadGroup (loadGroupGuid);
#endif
}

GSErrCode TAPIR_GetAllAnalyticalLoadCases (GS::Array<API_Guid>& loadCases)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAllAnalyticalLoadCases (ACAPI_GetToken (), loadCases);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAllAnalyticalLoadCases (ACAPI_GetToken (), loadCases);
#else
    return ACAPI_AnalyticalModel_GetAllAnalyticalLoadCases (&loadCases);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCases (const API_Guid& loadGroupGuid, GS::Array<API_Guid>& loadCases)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCases (ACAPI_GetToken (), loadGroupGuid, loadCases);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCases (ACAPI_GetToken (), loadGroupGuid, loadCases);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCases (loadGroupGuid, &loadCases);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCase (const API_Guid& loadCaseGuid, API_AnalyticalLoadCase& loadCase)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid, loadCase);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid, loadCase);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCase (loadCaseGuid, &loadCase);
#endif
}

GSErrCode TAPIR_CreateAnalyticalLoadCase (API_AnalyticalLoadCase& loadCase)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_CreateAnalyticalLoadCase (ACAPI_GetToken (), loadCase);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCase (ACAPI_GetToken (), loadCase);
#else
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCase (loadCase);
#endif
}

GSErrCode TAPIR_DeleteAnalyticalLoadCase (const API_Guid& loadCaseGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_DeleteAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid);
#else
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCase (loadCaseGuid);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCombinations (GS::Array<API_Guid>& loadCombinations)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCombinations (ACAPI_GetToken (), loadCombinations);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombinations (ACAPI_GetToken (), loadCombinations);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombinations (&loadCombinations);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCombination (const API_Guid& loadCombinationGuid, API_AnalyticalLoadCombination& loadCombination)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid, loadCombination);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid, loadCombination);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombination (loadCombinationGuid, &loadCombination);
#endif
}

GSErrCode TAPIR_CreateAnalyticalLoadCombination (API_AnalyticalLoadCombination& loadCombination)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_CreateAnalyticalLoadCombination (ACAPI_GetToken (), loadCombination);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCombination (ACAPI_GetToken (), loadCombination);
#else
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCombination (loadCombination);
#endif
}

GSErrCode TAPIR_DeleteAnalyticalLoadCombination (const API_Guid& loadCombinationGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_DeleteAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid);
#else
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCombination (loadCombinationGuid);
#endif
}

GSErrCode TAPIR_AddLoadCaseToLoadCombination (const API_AnalyticalLoadFactor& loadFactor, const API_Guid& loadCombinationGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_AddLoadCaseToLoadCombination (ACAPI_GetToken (), loadFactor, loadCombinationGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_AddLoadCaseToLoadCombination (ACAPI_GetToken (), loadFactor, loadCombinationGuid);
#else
    return ACAPI_AnalyticalModel_AddLoadCaseToLoadCombination (loadFactor, loadCombinationGuid);
#endif
}

template <typename EnumType, size_t N>
void AddEnumField (GS::ObjectState& os, const char* key, const EnumName<EnumType> (&names)[N], EnumType value)
{
    const char* name = EnumToString (names, value);
    if (name != nullptr) {
        os.Add (key, GS::UniString (name));
    }
}

// Reads a required enum field. Returns an error response when it is missing or unknown.
template <typename EnumType, size_t N>
GS::Optional<GS::ObjectState> GetRequiredEnumField (const GS::ObjectState& os, const char* key, const EnumName<EnumType> (&names)[N], EnumType& value)
{
    GS::UniString name;
    if (!os.Get (key, name)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("%s is missing.", key));
    }
    if (!EnumFromString (names, name, value)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Unknown %s '%T'.", key, name.ToPrintf ()));
    }
    return GS::NoValue;
}

// Reads a guid object field. Returns an error response when it is missing or invalid.
GS::Optional<GS::ObjectState> GetRequiredGuidField (const GS::ObjectState& os, const char* key, API_Guid& guid)
{
    const GS::ObjectState* guidOS = os.Get (key);
    if (guidOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("%s is missing.", key));
    }
    guid = GetGuidFromObjectState (*guidOS);
    if (guid == APINULLGuid) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Invalid %s.", key));
    }
    return GS::NoValue;
}

// Reads the guids of the given id key from a list of objects like [{"loadGroupId": {"guid": ...}}].
// An item without a valid id gets APINULLGuid.
GS::Array<API_Guid> GetGuidsFromIdList (const GS::ObjectState& parameters, const char* listKey, const char* idKey)
{
    GS::Array<GS::ObjectState> items;
    parameters.Get (listKey, items);

    GS::Array<API_Guid> guids;
    for (const GS::ObjectState& item : items) {
        const GS::ObjectState* idOS = item.Get (idKey);
        guids.Push (idOS != nullptr ? GetGuidFromObjectState (*idOS) : APINULLGuid);
    }
    return guids;
}

GS::ObjectState CreateAnalyticalLoadGroupObjectState (const API_AnalyticalLoadGroup& loadGroup)
{
    GS::ObjectState loadGroupOS;
    loadGroupOS.Add ("loadGroupId", CreateGuidObjectState (loadGroup.guid));
    loadGroupOS.Add ("name", loadGroup.name);
    AddEnumField (loadGroupOS, "groupType", AnalyticalLoadGroupTypeNames, loadGroup.groupType);
    AddEnumField (loadGroupOS, "relation", AnalyticalLoadGroupRelationNames, loadGroup.relation);
    loadGroupOS.Add ("loadType", loadGroup.loadType);
    return loadGroupOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadGroupFromObjectState (const GS::ObjectState& loadGroupOS, API_AnalyticalLoadGroup& loadGroup)
{
    if (!loadGroupOS.Get ("name", loadGroup.name)) {
        return CreateErrorResponse (APIERR_BADPARS, "name is missing.");
    }
    GS::Optional<GS::ObjectState> error = GetRequiredEnumField (loadGroupOS, "groupType", AnalyticalLoadGroupTypeNames, loadGroup.groupType);
    if (error.HasValue ()) {
        return error;
    }
    error = GetRequiredEnumField (loadGroupOS, "relation", AnalyticalLoadGroupRelationNames, loadGroup.relation);
    if (error.HasValue ()) {
        return error;
    }
    loadGroupOS.Get ("loadType", loadGroup.loadType);
    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalLoadCaseObjectState (const API_AnalyticalLoadCase& loadCase)
{
    GS::ObjectState loadCaseOS;
    loadCaseOS.Add ("loadCaseId", CreateGuidObjectState (loadCase.loadCaseGuid));
    loadCaseOS.Add ("name", loadCase.name);
    loadCaseOS.Add ("description", loadCase.description);
    AddEnumField (loadCaseOS, "actionType", AnalyticalLoadCaseActionTypeNames, loadCase.actionType);
    loadCaseOS.Add ("loadGroupId", CreateGuidObjectState (loadCase.loadGroupGuid));
    AddEnumField (loadCaseOS, "loadType", AnalyticalLoadCaseLoadTypeNames, loadCase.loadType);
    if (loadCase.duration.HasValue ()) {
        AddEnumField (loadCaseOS, "duration", AnalyticalLoadCaseDurationNames, loadCase.duration.Get ());
    }
    return loadCaseOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadCaseFromObjectState (const GS::ObjectState& loadCaseOS, API_AnalyticalLoadCase& loadCase)
{
    if (!loadCaseOS.Get ("name", loadCase.name)) {
        return CreateErrorResponse (APIERR_BADPARS, "name is missing.");
    }
    loadCaseOS.Get ("description", loadCase.description);
    GS::Optional<GS::ObjectState> error = GetRequiredEnumField (loadCaseOS, "actionType", AnalyticalLoadCaseActionTypeNames, loadCase.actionType);
    if (error.HasValue ()) {
        return error;
    }
    error = GetRequiredGuidField (loadCaseOS, "loadGroupId", loadCase.loadGroupGuid);
    if (error.HasValue ()) {
        return error;
    }
    error = GetRequiredEnumField (loadCaseOS, "loadType", AnalyticalLoadCaseLoadTypeNames, loadCase.loadType);
    if (error.HasValue ()) {
        return error;
    }
    if (loadCaseOS.Contains ("duration")) {
        API_AnalyticalLoadCaseDuration duration = API_AnalyticalLoadCase_LongDuration;
        error = GetRequiredEnumField (loadCaseOS, "duration", AnalyticalLoadCaseDurationNames, duration);
        if (error.HasValue ()) {
            return error;
        }
        loadCase.duration = duration;
    }
    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalLoadFactorObjectState (const API_AnalyticalLoadFactor& loadFactor)
{
    GS::ObjectState loadFactorOS;
    loadFactorOS.Add ("loadCaseId", CreateGuidObjectState (loadFactor.loadCaseGuid));
    if (loadFactor.factor.HasValue ()) {
        loadFactorOS.Add ("factor", loadFactor.factor.Get ());
    }
    loadFactorOS.Add ("multiplier", loadFactor.multiplier);
    return loadFactorOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadFactorFromObjectState (const GS::ObjectState& loadFactorOS, API_AnalyticalLoadFactor& loadFactor)
{
    GS::Optional<GS::ObjectState> error = GetRequiredGuidField (loadFactorOS, "loadCaseId", loadFactor.loadCaseGuid);
    if (error.HasValue ()) {
        return error;
    }
    double factor = 0.0;
    if (loadFactorOS.Get ("factor", factor)) {
        loadFactor.factor = factor;
    }
    if (!loadFactorOS.Get ("multiplier", loadFactor.multiplier)) {
        return CreateErrorResponse (APIERR_BADPARS, "multiplier is missing.");
    }
    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalLoadCombinationObjectState (const API_AnalyticalLoadCombination& loadCombination)
{
    GS::ObjectState loadCombinationOS;
    loadCombinationOS.Add ("loadCombinationId", CreateGuidObjectState (loadCombination.guid));
    loadCombinationOS.Add ("name", loadCombination.name);
    loadCombinationOS.Add ("description", loadCombination.description);
    AddEnumField (loadCombinationOS, "category", AnalyticalLoadCombinationCategoryNames, loadCombination.category);
    if (loadCombination.nationalStandard.HasValue ()) {
        AddEnumField (loadCombinationOS, "nationalStandard", AnalyticalLoadCombinationNationalStandardNames, loadCombination.nationalStandard.Get ());
    }
#ifdef ServerMainVers_2700
    if (loadCombination.type.HasValue ()) {
        AddEnumField (loadCombinationOS, "type", AnalyticalLoadCombinationTypeNames, loadCombination.type.Get ());
    }
#endif
    const auto& loadFactors = loadCombinationOS.AddList<GS::ObjectState> ("loadFactors");
    for (const API_AnalyticalLoadFactor& loadFactor : loadCombination.loadFactors) {
        loadFactors (CreateAnalyticalLoadFactorObjectState (loadFactor));
    }
    return loadCombinationOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadCombinationFromObjectState (const GS::ObjectState& loadCombinationOS, API_AnalyticalLoadCombination& loadCombination)
{
    if (!loadCombinationOS.Get ("name", loadCombination.name)) {
        return CreateErrorResponse (APIERR_BADPARS, "name is missing.");
    }
    loadCombinationOS.Get ("description", loadCombination.description);
    GS::Optional<GS::ObjectState> error = GetRequiredEnumField (loadCombinationOS, "category", AnalyticalLoadCombinationCategoryNames, loadCombination.category);
    if (error.HasValue ()) {
        return error;
    }
    if (loadCombinationOS.Contains ("nationalStandard")) {
        API_AnalyticalLoadCombinationNationalStandard nationalStandard = API_AnalyticalLoadNationalStandard_ENULSSTRGEOSetB;
        error = GetRequiredEnumField (loadCombinationOS, "nationalStandard", AnalyticalLoadCombinationNationalStandardNames, nationalStandard);
        if (error.HasValue ()) {
            return error;
        }
        loadCombination.nationalStandard = nationalStandard;
    }
    if (loadCombinationOS.Contains ("type")) {
#ifdef ServerMainVers_2700
        API_LoadCombinationType type = API_LoadCombinationType::API_LoadCombinationType_Linear;
        error = GetRequiredEnumField (loadCombinationOS, "type", AnalyticalLoadCombinationTypeNames, type);
        if (error.HasValue ()) {
            return error;
        }
        loadCombination.type = type;
#else
        return CreateErrorResponse (APIERR_BADPARS, "type is available from Archicad 27.");
#endif
    }
    GS::Array<GS::ObjectState> loadFactors;
    loadCombinationOS.Get ("loadFactors", loadFactors);
    for (const GS::ObjectState& loadFactorOS : loadFactors) {
        API_AnalyticalLoadFactor loadFactor = {};
        error = GetAnalyticalLoadFactorFromObjectState (loadFactorOS, loadFactor);
        if (error.HasValue ()) {
            return error;
        }
        loadCombination.loadFactors.Push (loadFactor);
    }
    return GS::NoValue;
}

// Deletes the objects of the given ids in one undo step, with one execution result per id.
GS::ObjectState DeleteAnalyticalLoadObjects (const GS::Array<API_Guid>& guids, const char* undoName, const char* idKey, const char* failureMessage, GSErrCode (*deleteFunction) (const API_Guid&))
{
    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand (undoName, [&] () -> GSErrCode {
        for (const API_Guid& guid : guids) {
            if (guid == APINULLGuid) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("%s is missing or invalid.", idKey)));
                continue;
            }
            const GSErrCode err = deleteFunction (guid);
            executionResults (err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, failureMessage));
        }
        return NoError;
    });

    return response;
}

}

GetAnalyticalLoadGroupsCommand::GetAnalyticalLoadGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalLoadGroupsCommand::GetName () const
{
    return "GetAnalyticalLoadGroups";
}

GS::Optional<GS::UniString> GetAnalyticalLoadGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroups": {
                "type": "array",
                "description": "The load groups of the project. A load group that cannot be read is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/AnalyticalLoadGroup"
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
            "loadGroups"
        ]
    })";
}

GS::ObjectState GetAnalyticalLoadGroupsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> loadGroupGuids;
    const GSErrCode err = TAPIR_GetAnalyticalLoadGroups (loadGroupGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the analytical load groups.");
    }

    GS::ObjectState response;
    const auto& loadGroups = response.AddList<GS::ObjectState> ("loadGroups");
    for (const API_Guid& loadGroupGuid : loadGroupGuids) {
        API_AnalyticalLoadGroup loadGroup = {};
        const GSErrCode getErr = TAPIR_GetAnalyticalLoadGroup (loadGroupGuid, loadGroup);
        if (getErr != NoError) {
            loadGroups (CreateErrorResponse (getErr, "Failed to get the analytical load group."));
            continue;
        }
        loadGroups (CreateAnalyticalLoadGroupObjectState (loadGroup));
    }

    return response;
}

GetAnalyticalLoadCasesCommand::GetAnalyticalLoadCasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalLoadCasesCommand::GetName () const
{
    return "GetAnalyticalLoadCases";
}

GS::Optional<GS::UniString> GetAnalyticalLoadCasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroupId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the load group to list the load cases of. Every load case of the project is listed when it is omitted."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetAnalyticalLoadCasesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCases": {
                "type": "array",
                "description": "The load cases. A load case that cannot be read is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/AnalyticalLoadCase"
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
            "loadCases"
        ]
    })";
}

GS::ObjectState GetAnalyticalLoadCasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> loadCaseGuids;
    GSErrCode err = NoError;
    if (parameters.Contains ("loadGroupId")) {
        API_Guid loadGroupGuid = APINULLGuid;
        const GS::Optional<GS::ObjectState> error = GetRequiredGuidField (parameters, "loadGroupId", loadGroupGuid);
        if (error.HasValue ()) {
            return *error;
        }
        err = TAPIR_GetAnalyticalLoadCases (loadGroupGuid, loadCaseGuids);
    } else {
        err = TAPIR_GetAllAnalyticalLoadCases (loadCaseGuids);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the analytical load cases.");
    }

    GS::ObjectState response;
    const auto& loadCases = response.AddList<GS::ObjectState> ("loadCases");
    for (const API_Guid& loadCaseGuid : loadCaseGuids) {
        API_AnalyticalLoadCase loadCase = {};
        const GSErrCode getErr = TAPIR_GetAnalyticalLoadCase (loadCaseGuid, loadCase);
        if (getErr != NoError) {
            loadCases (CreateErrorResponse (getErr, "Failed to get the analytical load case."));
            continue;
        }
        loadCases (CreateAnalyticalLoadCaseObjectState (loadCase));
    }

    return response;
}

GetAnalyticalLoadCombinationsCommand::GetAnalyticalLoadCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalLoadCombinationsCommand::GetName () const
{
    return "GetAnalyticalLoadCombinations";
}

GS::Optional<GS::UniString> GetAnalyticalLoadCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinations": {
                "type": "array",
                "description": "The load combinations of the project. A load combination that cannot be read is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/AnalyticalLoadCombination"
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
            "loadCombinations"
        ]
    })";
}

GS::ObjectState GetAnalyticalLoadCombinationsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> loadCombinationGuids;
    const GSErrCode err = TAPIR_GetAnalyticalLoadCombinations (loadCombinationGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the analytical load combinations.");
    }

    GS::ObjectState response;
    const auto& loadCombinations = response.AddList<GS::ObjectState> ("loadCombinations");
    for (const API_Guid& loadCombinationGuid : loadCombinationGuids) {
        API_AnalyticalLoadCombination loadCombination = {};
        const GSErrCode getErr = TAPIR_GetAnalyticalLoadCombination (loadCombinationGuid, loadCombination);
        if (getErr != NoError) {
            loadCombinations (CreateErrorResponse (getErr, "Failed to get the analytical load combination."));
            continue;
        }
        loadCombinations (CreateAnalyticalLoadCombinationObjectState (loadCombination));
    }

    return response;
}

CreateAnalyticalLoadGroupsCommand::CreateAnalyticalLoadGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAnalyticalLoadGroupsCommand::GetName () const
{
    return "CreateAnalyticalLoadGroups";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroups": {
                "type": "array",
                "description": "The load groups to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "groupType": {
                            "$ref": "#/AnalyticalLoadGroupType"
                        },
                        "relation": {
                            "$ref": "#/AnalyticalLoadGroupRelation"
                        },
                        "loadType": {
                            "type": "string",
                            "description": "The load type of the group as free text. Empty when it is omitted."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "groupType",
                        "relation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadGroups"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroupIds": {
                "type": "array",
                "description": "The identifiers of the new load groups, in the order of the input. A load group that could not be created is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "loadGroupId": {
                                    "$ref": "#/GuidId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "loadGroupId"
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
            "loadGroupIds"
        ]
    })";
}

GS::ObjectState CreateAnalyticalLoadGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> loadGroups;
    parameters.Get ("loadGroups", loadGroups);

    GS::ObjectState response;
    const auto& loadGroupIds = response.AddList<GS::ObjectState> ("loadGroupIds");

    ACAPI_CallUndoableCommand ("Create Analytical Load Groups", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadGroupOS : loadGroups) {
            API_AnalyticalLoadGroup loadGroup = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadGroupFromObjectState (loadGroupOS, loadGroup);
            if (error.HasValue ()) {
                loadGroupIds (*error);
                continue;
            }
            const GSErrCode err = TAPIR_CreateAnalyticalLoadGroup (loadGroup);
            if (err != NoError) {
                loadGroupIds (CreateErrorResponse (err, "Failed to create the analytical load group."));
                continue;
            }
            loadGroupIds (GS::ObjectState ("loadGroupId", CreateGuidObjectState (loadGroup.guid)));
        }
        return NoError;
    });

    return response;
}

CreateAnalyticalLoadCasesCommand::CreateAnalyticalLoadCasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAnalyticalLoadCasesCommand::GetName () const
{
    return "CreateAnalyticalLoadCases";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCases": {
                "type": "array",
                "description": "The load cases to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string",
                            "description": "Empty when it is omitted."
                        },
                        "actionType": {
                            "$ref": "#/AnalyticalLoadCaseActionType"
                        },
                        "loadGroupId": {
                            "$ref": "#/GuidId",
                            "description": "The identifier of the load group the load case belongs to."
                        },
                        "loadType": {
                            "$ref": "#/AnalyticalLoadCaseLoadType"
                        },
                        "duration": {
                            "$ref": "#/AnalyticalLoadCaseDuration"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "actionType",
                        "loadGroupId",
                        "loadType"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCases"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCasesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCaseIds": {
                "type": "array",
                "description": "The identifiers of the new load cases, in the order of the input. A load case that could not be created is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "loadCaseId": {
                                    "$ref": "#/GuidId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "loadCaseId"
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
            "loadCaseIds"
        ]
    })";
}

GS::ObjectState CreateAnalyticalLoadCasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> loadCases;
    parameters.Get ("loadCases", loadCases);

    GS::ObjectState response;
    const auto& loadCaseIds = response.AddList<GS::ObjectState> ("loadCaseIds");

    ACAPI_CallUndoableCommand ("Create Analytical Load Cases", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadCaseOS : loadCases) {
            API_AnalyticalLoadCase loadCase = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadCaseFromObjectState (loadCaseOS, loadCase);
            if (error.HasValue ()) {
                loadCaseIds (*error);
                continue;
            }
            const GSErrCode err = TAPIR_CreateAnalyticalLoadCase (loadCase);
            if (err != NoError) {
                loadCaseIds (CreateErrorResponse (err, "Failed to create the analytical load case."));
                continue;
            }
            loadCaseIds (GS::ObjectState ("loadCaseId", CreateGuidObjectState (loadCase.loadCaseGuid)));
        }
        return NoError;
    });

    return response;
}

CreateAnalyticalLoadCombinationsCommand::CreateAnalyticalLoadCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAnalyticalLoadCombinationsCommand::GetName () const
{
    return "CreateAnalyticalLoadCombinations";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinations": {
                "type": "array",
                "description": "The load combinations to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string",
                            "description": "Empty when it is omitted."
                        },
                        "category": {
                            "$ref": "#/AnalyticalLoadCombinationCategory"
                        },
                        "nationalStandard": {
                            "$ref": "#/AnalyticalLoadCombinationNationalStandard"
                        },
                        "type": {
                            "$ref": "#/AnalyticalLoadCombinationType"
                        },
                        "loadFactors": {
                            "type": "array",
                            "description": "The load cases of the combination with their factors.",
                            "items": {
                                "$ref": "#/AnalyticalLoadFactor"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "category"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinationIds": {
                "type": "array",
                "description": "The identifiers of the new load combinations, in the order of the input. A load combination that could not be created is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "loadCombinationId": {
                                    "$ref": "#/GuidId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "loadCombinationId"
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
            "loadCombinationIds"
        ]
    })";
}

GS::ObjectState CreateAnalyticalLoadCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> loadCombinations;
    parameters.Get ("loadCombinations", loadCombinations);

    GS::ObjectState response;
    const auto& loadCombinationIds = response.AddList<GS::ObjectState> ("loadCombinationIds");

    ACAPI_CallUndoableCommand ("Create Analytical Load Combinations", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadCombinationOS : loadCombinations) {
            API_AnalyticalLoadCombination loadCombination = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadCombinationFromObjectState (loadCombinationOS, loadCombination);
            if (error.HasValue ()) {
                loadCombinationIds (*error);
                continue;
            }
            const GSErrCode err = TAPIR_CreateAnalyticalLoadCombination (loadCombination);
            if (err != NoError) {
                loadCombinationIds (CreateErrorResponse (err, "Failed to create the analytical load combination."));
                continue;
            }
            loadCombinationIds (GS::ObjectState ("loadCombinationId", CreateGuidObjectState (loadCombination.guid)));
        }
        return NoError;
    });

    return response;
}

DeleteAnalyticalLoadGroupsCommand::DeleteAnalyticalLoadGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAnalyticalLoadGroupsCommand::GetName () const
{
    return "DeleteAnalyticalLoadGroups";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroupIds": {
                "type": "array",
                "description": "The load groups to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "loadGroupId": {
                            "$ref": "#/GuidId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "loadGroupId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadGroupIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadGroupsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteAnalyticalLoadGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return DeleteAnalyticalLoadObjects (GetGuidsFromIdList (parameters, "loadGroupIds", "loadGroupId"),
        "Delete Analytical Load Groups", "loadGroupId", "Failed to delete the analytical load group.", TAPIR_DeleteAnalyticalLoadGroup);
}

DeleteAnalyticalLoadCasesCommand::DeleteAnalyticalLoadCasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAnalyticalLoadCasesCommand::GetName () const
{
    return "DeleteAnalyticalLoadCases";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCaseIds": {
                "type": "array",
                "description": "The load cases to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "loadCaseId": {
                            "$ref": "#/GuidId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "loadCaseId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCaseIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCasesCommand::GetRawResponseSchema () const
{
    return DeleteAnalyticalLoadGroupsCommand ().GetRawResponseSchema ();
}

GS::ObjectState DeleteAnalyticalLoadCasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return DeleteAnalyticalLoadObjects (GetGuidsFromIdList (parameters, "loadCaseIds", "loadCaseId"),
        "Delete Analytical Load Cases", "loadCaseId", "Failed to delete the analytical load case.", TAPIR_DeleteAnalyticalLoadCase);
}

DeleteAnalyticalLoadCombinationsCommand::DeleteAnalyticalLoadCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAnalyticalLoadCombinationsCommand::GetName () const
{
    return "DeleteAnalyticalLoadCombinations";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinationIds": {
                "type": "array",
                "description": "The load combinations to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "loadCombinationId": {
                            "$ref": "#/GuidId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "loadCombinationId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCombinationIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCombinationsCommand::GetRawResponseSchema () const
{
    return DeleteAnalyticalLoadGroupsCommand ().GetRawResponseSchema ();
}

GS::ObjectState DeleteAnalyticalLoadCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return DeleteAnalyticalLoadObjects (GetGuidsFromIdList (parameters, "loadCombinationIds", "loadCombinationId"),
        "Delete Analytical Load Combinations", "loadCombinationId", "Failed to delete the analytical load combination.", TAPIR_DeleteAnalyticalLoadCombination);
}

AddLoadCasesToLoadCombinationCommand::AddLoadCasesToLoadCombinationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String AddLoadCasesToLoadCombinationCommand::GetName () const
{
    return "AddLoadCasesToLoadCombination";
}

GS::Optional<GS::UniString> AddLoadCasesToLoadCombinationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinationId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the load combination to add the load cases to."
            },
            "loadFactors": {
                "type": "array",
                "description": "The load cases to add, with their factors.",
                "items": {
                    "$ref": "#/AnalyticalLoadFactor"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCombinationId",
            "loadFactors"
        ]
    })";
}

GS::Optional<GS::UniString> AddLoadCasesToLoadCombinationCommand::GetRawResponseSchema () const
{
    return DeleteAnalyticalLoadGroupsCommand ().GetRawResponseSchema ();
}

GS::ObjectState AddLoadCasesToLoadCombinationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Guid loadCombinationGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> combinationError = GetRequiredGuidField (parameters, "loadCombinationId", loadCombinationGuid);
    if (combinationError.HasValue ()) {
        return *combinationError;
    }

    GS::Array<GS::ObjectState> loadFactors;
    parameters.Get ("loadFactors", loadFactors);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Add Load Cases To Load Combination", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadFactorOS : loadFactors) {
            API_AnalyticalLoadFactor loadFactor = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadFactorFromObjectState (loadFactorOS, loadFactor);
            if (error.HasValue ()) {
                executionResults (CreateFailedExecutionResult (*error));
                continue;
            }
            const GSErrCode err = TAPIR_AddLoadCaseToLoadCombination (loadFactor, loadCombinationGuid);
            executionResults (err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, "Failed to add the load case to the load combination."));
        }
        return NoError;
    });

    return response;
}
