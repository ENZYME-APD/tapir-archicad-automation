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
