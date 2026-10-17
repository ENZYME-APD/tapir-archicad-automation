#include "GraphicOverrideCommands.hpp"

#ifdef ServerMainVers_2700

namespace {

GS::ObjectState CreateColorObjectState (const API_RGBColor& rgb)
{
    return GS::ObjectState ("red", rgb.f_red, "green", rgb.f_green, "blue", rgb.f_blue);
}

GS::ObjectState CreateFillTypesObjectState (const API_OverriddenFillType& fillType)
{
    return GS::ObjectState ("cutFill", fillType.overrideCutFill, "coverFill", fillType.overrideCoverFill, "draftingFill", fillType.overrideDraftingFill);
}

API_OverriddenFillType GetFillTypesFromObjectState (const GS::ObjectState& os)
{
    API_OverriddenFillType fillType = {};
    os.Get ("cutFill", fillType.overrideCutFill);
    os.Get ("coverFill", fillType.overrideCoverFill);
    os.Get ("draftingFill", fillType.overrideDraftingFill);
    return fillType;
}

// An override given without its ...AppliesTo object applies to every fill.
API_OverriddenFillType GetAllFillTypes ()
{
    API_OverriddenFillType fillType = {};
    fillType.overrideCutFill = true;
    fillType.overrideCoverFill = true;
    fillType.overrideDraftingFill = true;
    return fillType;
}

GS::ObjectState CreateSurfaceTypesObjectState (const API_OverriddenSurfaceType& surfaceType)
{
    return GS::ObjectState ("cutSurface", surfaceType.overrideCutSurface, "uncutSurface", surfaceType.overrideUncutSurface);
}

API_OverriddenSurfaceType GetSurfaceTypesFromObjectState (const GS::ObjectState& os)
{
    API_OverriddenSurfaceType surfaceType = {};
    os.Get ("cutSurface", surfaceType.overrideCutSurface);
    os.Get ("uncutSurface", surfaceType.overrideUncutSurface);
    return surfaceType;
}

// An override given without its ...AppliesTo object applies to every surface.
API_OverriddenSurfaceType GetAllSurfaceTypes ()
{
    API_OverriddenSurfaceType surfaceType = {};
    surfaceType.overrideCutSurface = true;
    surfaceType.overrideUncutSurface = true;
    return surfaceType;
}

void AddAttributeIfOverridden (GS::ObjectState& os, const char* fieldName, API_AttrTypeID typeID, const API_OverriddenAttribute& attribute)
{
    if (attribute.hasValue) {
        os.Add (fieldName, CreateGuidObjectState (GetAttributeGuidFromIndex (typeID, attribute.value)));
    }
}

GSErrCode GetAttributeFromField (const GS::ObjectState& os, const char* fieldName, API_AttrTypeID typeID, API_AttributeIndex& index, GS::UniString& errorMessage)
{
    const GS::ObjectState* attributeId = os.Get (fieldName);
    if (attributeId == nullptr) {
        errorMessage = GS::UniString::Printf ("The attribute in '%s' is not an attribute identifier.", fieldName);
        return APIERR_BADPARS;
    }
    const API_Guid guid = GetGuidFromObjectState (*attributeId);
    index = GetAttributeIndexFromGuid (typeID, guid);
    if (guid == APINULLGuid || GetAttributeGuidFromIndex (typeID, index) != guid) {
        errorMessage = GS::UniString::Printf ("The attribute in '%s' does not exist or has the wrong type.", fieldName);
        return APIERR_BADPARS;
    }
    return NoError;
}

GS::ObjectState CreateStyleObjectState (const API_OverrideRuleStyle& style)
{
    GS::ObjectState os;
    AddAttributeIfOverridden (os, "lineType", API_LinetypeID, style.lineType);
    if (style.lineMarkerTextPen.hasValue) {
        os.Add ("lineMarkerTextPen", static_cast<Int32> (style.lineMarkerTextPen.value));
    }
    AddAttributeIfOverridden (os, "fillType", API_FilltypeID, style.fillOverride);
    os.Add ("fillTypeAppliesTo", CreateFillTypesObjectState (style.fillType));
    if (style.fillForegroundPenOverride.hasValue) {
        os.Add ("fillForegroundPen", static_cast<Int32> (style.fillForegroundPenOverride.value));
    }
    os.Add ("fillForegroundPenAppliesTo", CreateFillTypesObjectState (style.fillTypeForegroundPen));
    if (style.fillBackgroundPenOverride.hasValue) {
        const auto& background = style.fillBackgroundPenOverride.value;
        if (background.IsType<API_PenIndex> ()) {
            os.Add ("fillBackgroundPen", static_cast<Int32> (background.Get<API_PenIndex> ()));
        } else {
            os.Add ("fillBackgroundColor", CreateColorObjectState (background.Get<API_RGBColor> ()));
        }
    }
    os.Add ("fillBackgroundPenAppliesTo", CreateFillTypesObjectState (style.fillTypeBackgroundPen));
    if (style.surfaceOverride.hasValue) {
        const auto& surface = style.surfaceOverride.value;
        if (surface.IsType<API_AttributeIndex> ()) {
            os.Add ("surface", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, surface.Get<API_AttributeIndex> ())));
        } else {
            os.Add ("surfaceColor", CreateColorObjectState (surface.Get<API_RGBColor> ()));
        }
    }
    os.Add ("surfaceAppliesTo", CreateSurfaceTypesObjectState (style.surfaceType));
    os.Add ("showSkinSeparators", style.showSkinSeparators);
    os.Add ("overridePenColorAndThickness", style.overridePenColorAndThickness);
    os.Add ("hiddenContours", CreateSurfaceTypesObjectState (style.hiddenContours));
    os.Add ("overrideContours", style.overrideContours);
    return os;
}

GSErrCode GetStyleFromObjectState (const GS::ObjectState& os, API_OverrideRuleStyle& style, GS::UniString& errorMessage)
{
    style = {};
    API_AttributeIndex index = {};
    Int32 pen = 0;
    API_RGBColor color = {};
    const GS::ObjectState* fieldOs = nullptr;

    if (os.Contains ("lineType")) {
        GSErrCode err = GetAttributeFromField (os, "lineType", API_LinetypeID, index, errorMessage);
        if (err != NoError) {
            return err;
        }
        style.lineType = index;
    }
    if (os.Get ("lineMarkerTextPen", pen)) {
        style.lineMarkerTextPen = static_cast<API_PenIndex> (pen);
    }
    if (os.Contains ("fillType")) {
        GSErrCode err = GetAttributeFromField (os, "fillType", API_FilltypeID, index, errorMessage);
        if (err != NoError) {
            return err;
        }
        style.fillOverride = index;
    }
    if ((fieldOs = os.Get ("fillTypeAppliesTo")) != nullptr) {
        style.fillType = GetFillTypesFromObjectState (*fieldOs);
    } else if (style.fillOverride.hasValue) {
        style.fillType = GetAllFillTypes ();
    }
    if (os.Get ("fillForegroundPen", pen)) {
        style.fillForegroundPenOverride = static_cast<API_PenIndex> (pen);
    }
    if ((fieldOs = os.Get ("fillForegroundPenAppliesTo")) != nullptr) {
        style.fillTypeForegroundPen = GetFillTypesFromObjectState (*fieldOs);
    } else if (style.fillForegroundPenOverride.hasValue) {
        style.fillTypeForegroundPen = GetAllFillTypes ();
    }
    if (os.Contains ("fillBackgroundPen") && os.Contains ("fillBackgroundColor")) {
        errorMessage = "Give either 'fillBackgroundPen' or 'fillBackgroundColor', not both.";
        return APIERR_BADPARS;
    }
    if (os.Get ("fillBackgroundPen", pen)) {
        APIVariant<API_PenIndex, API_RGBColor> background;
        background = static_cast<API_PenIndex> (pen);
        style.fillBackgroundPenOverride = background;
    } else if (GetColor (os, "fillBackgroundColor", color)) {
        APIVariant<API_PenIndex, API_RGBColor> background;
        background = color;
        style.fillBackgroundPenOverride = background;
    }
    if ((fieldOs = os.Get ("fillBackgroundPenAppliesTo")) != nullptr) {
        style.fillTypeBackgroundPen = GetFillTypesFromObjectState (*fieldOs);
    } else if (style.fillBackgroundPenOverride.hasValue) {
        style.fillTypeBackgroundPen = GetAllFillTypes ();
    }
    if (os.Contains ("surface") && os.Contains ("surfaceColor")) {
        errorMessage = "Give either 'surface' or 'surfaceColor', not both.";
        return APIERR_BADPARS;
    }
    if (os.Contains ("surface")) {
        GSErrCode err = GetAttributeFromField (os, "surface", API_MaterialID, index, errorMessage);
        if (err != NoError) {
            return err;
        }
        APIVariant<API_AttributeIndex, API_RGBColor> surface;
        surface = index;
        style.surfaceOverride = surface;
    } else if (GetColor (os, "surfaceColor", color)) {
        APIVariant<API_AttributeIndex, API_RGBColor> surface;
        surface = color;
        style.surfaceOverride = surface;
    }
    if ((fieldOs = os.Get ("surfaceAppliesTo")) != nullptr) {
        style.surfaceType = GetSurfaceTypesFromObjectState (*fieldOs);
    } else if (style.surfaceOverride.hasValue) {
        style.surfaceType = GetAllSurfaceTypes ();
    }
    os.Get ("showSkinSeparators", style.showSkinSeparators);
    os.Get ("overridePenColorAndThickness", style.overridePenColorAndThickness);
    if ((fieldOs = os.Get ("hiddenContours")) != nullptr) {
        style.hiddenContours = GetSurfaceTypesFromObjectState (*fieldOs);
    }
    os.Get ("overrideContours", style.overrideContours);
    return NoError;
}

GSErrCode FindOrCreateRuleGroup (const GS::UniString& name, API_Guid& ruleGroupGuid)
{
    GS::Array<API_Guid> ruleGroupIds;
    GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleGroupList (ruleGroupIds);
    if (err != NoError) {
        return err;
    }
    for (const API_Guid& ruleGroupId : ruleGroupIds) {
        API_OverrideRuleGroup ruleGroup = {};
        ruleGroup.guid = ruleGroupId;
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (ruleGroup) == NoError && ruleGroup.name == name) {
            ruleGroupGuid = ruleGroup.guid;
            return NoError;
        }
    }
    API_OverrideRuleGroup ruleGroup = {};
    ruleGroup.name = name;
    err = ACAPI_GraphicalOverride_CreateOverrideRuleGroup (ruleGroup);
    ruleGroupGuid = ruleGroup.guid;
    return err;
}

GS::Array<API_Guid> GetRuleGuidsFromArray (const GS::Array<GS::ObjectState>& ruleIds)
{
    GS::Array<API_Guid> guids;
    for (const GS::ObjectState& ruleId : ruleIds) {
        guids.Push (GetGuidFromArrayItem ("graphicOverrideRuleId", ruleId));
    }
    return guids;
}

}

#endif

GetGraphicOverrideRulesCommand::GetGraphicOverrideRulesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGraphicOverrideRulesCommand::GetName () const
{
    return "GetGraphicOverrideRules";
}

GS::Optional<GS::UniString> GetGraphicOverrideRulesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules of the project.",
                "items": {
                    "type": "object",
                    "properties": {
                        "graphicOverrideRuleId": {
                            "$ref": "#/GraphicOverrideRuleId"
                        },
                        "name": {
                            "type": "string"
                        },
                        "ruleGroupName": {
                            "type": "string",
                            "description": "The name of the rule group the rule belongs to."
                        },
                        "criterionXML": {
                            "type": "string",
                            "description": "The criterion of the rule, in the XML format Archicad saves override rules in. Change its values and pass it to CreateGraphicOverrideRules or ModifyGraphicOverrideRules to make similar rules."
                        },
                        "style": {
                            "$ref": "#/GraphicOverrideStyle"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "graphicOverrideRuleId",
                        "name",
                        "criterionXML",
                        "style"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
        ]
    })";
}

GS::ObjectState GetGraphicOverrideRulesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<API_Guid> ruleIds;
    GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleList (ruleIds);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the graphic override rules.");
    }

    GS::HashTable<API_Guid, GS::UniString> ruleGroupNames;
    GS::Array<API_Guid> ruleGroupIds;
    if (ACAPI_GraphicalOverride_GetOverrideRuleGroupList (ruleGroupIds) == NoError) {
        for (const API_Guid& ruleGroupId : ruleGroupIds) {
            API_OverrideRuleGroup ruleGroup = {};
            ruleGroup.guid = ruleGroupId;
            GS::Array<API_Guid> ruleGroupRuleIds;
            if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (ruleGroup, &ruleGroupRuleIds) == NoError) {
                for (const API_Guid& ruleId : ruleGroupRuleIds) {
                    ruleGroupNames.Put (ruleId, ruleGroup.name);
                }
            }
        }
    }

    GS::ObjectState response;
    const auto& graphicOverrideRulesList = response.AddList<GS::ObjectState> ("graphicOverrideRules");
    for (const API_Guid& ruleId : ruleIds) {
        API_OverrideRule rule = {};
        rule.guid = ruleId;
        if (ACAPI_GraphicalOverride_GetOverrideRuleById (rule) != NoError) {
            continue;
        }
        GS::ObjectState ruleOs (
            "graphicOverrideRuleId", CreateGuidObjectState (rule.guid),
            "name", rule.name,
            "criterionXML", rule.criterionXML,
            "style", CreateStyleObjectState (rule.style));
        if (ruleGroupNames.ContainsKey (ruleId)) {
            ruleOs.Add ("ruleGroupName", ruleGroupNames[ruleId]);
        }
        graphicOverrideRulesList (ruleOs);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

CreateGraphicOverrideRulesCommand::CreateGraphicOverrideRulesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateGraphicOverrideRulesCommand::GetName () const
{
    return "CreateGraphicOverrideRules";
}

GS::Optional<GS::UniString> CreateGraphicOverrideRulesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the rule, unique within the project."
                        },
                        "ruleGroupName": {
                            "type": "string",
                            "description": "The name of the rule group to put the rule in. Created when it does not exist."
                        },
                        "criterionXML": {
                            "type": "string",
                            "description": "The criterion of the rule, in the XML format Archicad saves override rules in. Take it from GetGraphicOverrideRules and change its values: Archicad drops XML written from scratch without an error, and the rule then applies to all elements. Left out: Archicad's default criterion for new rules."
                        },
                        "style": {
                            "$ref": "#/GraphicOverrideStyle"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "ruleGroupName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
        ]
    })";
}

GS::Optional<GS::UniString> CreateGraphicOverrideRulesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRuleIds": {
                "type": "array",
                "description": "The identifiers of the created rules, or errors, in the order of the input.",
                "items": {
                    "$ref": "#/GraphicOverrideRuleIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRuleIds"
        ]
    })";
}

GS::ObjectState CreateGraphicOverrideRulesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> graphicOverrideRules;
    if (!parameters.Get ("graphicOverrideRules", graphicOverrideRules)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'graphicOverrideRules' parameter.");
    }

    GS::ObjectState response;
    const auto& graphicOverrideRuleIdsList = response.AddList<GS::ObjectState> ("graphicOverrideRuleIds");
    ACAPI_CallUndoableCommand ("Create Graphic Override Rules", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : graphicOverrideRules) {
            API_OverrideRule rule = {};
            GS::UniString ruleGroupName;
            if (!item.Get ("name", rule.name) || !item.Get ("ruleGroupName", ruleGroupName)) {
                graphicOverrideRuleIdsList (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' or 'ruleGroupName' parameter."));
                continue;
            }
            item.Get ("criterionXML", rule.criterionXML);
            const GS::ObjectState* style = item.Get ("style");
            if (style != nullptr) {
                GS::UniString errorMessage;
                const GSErrCode err = GetStyleFromObjectState (*style, rule.style, errorMessage);
                if (err != NoError) {
                    graphicOverrideRuleIdsList (CreateErrorResponse (err, errorMessage));
                    continue;
                }
            }

            API_Guid ruleGroupGuid = APINULLGuid;
            GSErrCode err = FindOrCreateRuleGroup (ruleGroupName, ruleGroupGuid);
            if (err != NoError) {
                graphicOverrideRuleIdsList (CreateErrorResponse (err, "Failed to find or create the rule group '" + ruleGroupName + "'."));
                continue;
            }
            err = ACAPI_GraphicalOverride_CreateOverrideRule (rule, ruleGroupGuid);
            if (err != NoError) {
                graphicOverrideRuleIdsList (CreateErrorResponse (err, "Failed to create the rule '" + rule.name + "'. Its name must be unique and not empty."));
                continue;
            }
            graphicOverrideRuleIdsList (GS::ObjectState ("graphicOverrideRuleId", CreateGuidObjectState (rule.guid)));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

ModifyGraphicOverrideRulesCommand::ModifyGraphicOverrideRulesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyGraphicOverrideRulesCommand::GetName () const
{
    return "ModifyGraphicOverrideRules";
}

GS::Optional<GS::UniString> ModifyGraphicOverrideRulesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules to modify.",
                "items": {
                    "type": "object",
                    "properties": {
                        "graphicOverrideRuleId": {
                            "$ref": "#/GraphicOverrideRuleId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the rule."
                        },
                        "criterionXML": {
                            "type": "string",
                            "description": "The new criterion of the rule, in the XML format Archicad saves override rules in. Take it from GetGraphicOverrideRules and change its values: Archicad drops XML written from scratch without an error, and the rule then applies to all elements."
                        },
                        "style": {
                            "$ref": "#/GraphicOverrideStyle"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "graphicOverrideRuleId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyGraphicOverrideRulesCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyGraphicOverrideRulesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> graphicOverrideRules;
    if (!parameters.Get ("graphicOverrideRules", graphicOverrideRules)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'graphicOverrideRules' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Modify Graphic Override Rules", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : graphicOverrideRules) {
            const GS::ObjectState* graphicOverrideRuleId = item.Get ("graphicOverrideRuleId");
            if (graphicOverrideRuleId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'graphicOverrideRuleId' parameter."));
                continue;
            }
            API_OverrideRule rule = {};
            rule.guid = GetGuidFromObjectState (*graphicOverrideRuleId);
            GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleById (rule);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "The graphic override rule does not exist."));
                continue;
            }
            item.Get ("name", rule.name);
            item.Get ("criterionXML", rule.criterionXML);
            const GS::ObjectState* style = item.Get ("style");
            if (style != nullptr) {
                GS::UniString errorMessage;
                err = GetStyleFromObjectState (*style, rule.style, errorMessage);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, errorMessage));
                    continue;
                }
            }

            err = ACAPI_GraphicalOverride_ChangeOverrideRule (rule);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to modify the rule '" + rule.name + "'. Its name must be unique and not empty."));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

DeleteGraphicOverrideRulesCommand::DeleteGraphicOverrideRulesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteGraphicOverrideRulesCommand::GetName () const
{
    return "DeleteGraphicOverrideRules";
}

GS::Optional<GS::UniString> DeleteGraphicOverrideRulesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRuleIds": {
                "type": "array",
                "description": "The graphic override rules to delete.",
                "items": {
                    "$ref": "#/GraphicOverrideRuleIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRuleIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteGraphicOverrideRulesCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteGraphicOverrideRulesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> graphicOverrideRuleIds;
    if (!parameters.Get ("graphicOverrideRuleIds", graphicOverrideRuleIds)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'graphicOverrideRuleIds' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    ACAPI_CallUndoableCommand ("Delete Graphic Override Rules", [&] () -> GSErrCode {
        for (const API_Guid& ruleGuid : GetRuleGuidsFromArray (graphicOverrideRuleIds)) {
            const GSErrCode err = ACAPI_GraphicalOverride_DeleteOverrideRule (ruleGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to delete the graphic override rule."));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetGraphicOverrideCombinationsCommand::GetGraphicOverrideCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGraphicOverrideCombinationsCommand::GetName () const
{
    return "GetGraphicOverrideCombinations";
}

GS::Optional<GS::UniString> GetGraphicOverrideCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideCombinations": {
                "type": "array",
                "description": "The graphic override combinations of the project.",
                "items": {
                    "type": "object",
                    "properties": {
                        "graphicOverrideCombinationId": {
                            "$ref": "#/GraphicOverrideCombinationId"
                        },
                        "name": {
                            "type": "string"
                        },
                        "graphicOverrideRuleIds": {
                            "type": "array",
                            "description": "The rules of the combination, in priority order.",
                            "items": {
                                "$ref": "#/GraphicOverrideRuleIdArrayItem"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "graphicOverrideCombinationId",
                        "name",
                        "graphicOverrideRuleIds"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinations"
        ]
    })";
}

GS::ObjectState GetGraphicOverrideCombinationsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<API_Guid> combinationIds;
    const GSErrCode err = ACAPI_GraphicalOverride_GetOverrideCombinationList (combinationIds);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the graphic override combinations.");
    }

    GS::ObjectState response;
    const auto& graphicOverrideCombinationsList = response.AddList<GS::ObjectState> ("graphicOverrideCombinations");
    for (const API_Guid& combinationId : combinationIds) {
        API_OverrideCombination combination = {};
        combination.guid = combinationId;
        GS::Array<API_Guid> ruleIds;
        if (ACAPI_GraphicalOverride_GetOverrideCombination (combination, &ruleIds) != NoError) {
            continue;
        }
        GS::ObjectState combinationOs (
            "graphicOverrideCombinationId", CreateGuidObjectState (combination.guid),
            "name", combination.name);
        const auto& ruleIdsList = combinationOs.AddList<GS::ObjectState> ("graphicOverrideRuleIds");
        for (const API_Guid& ruleId : ruleIds) {
            ruleIdsList (GS::ObjectState ("graphicOverrideRuleId", CreateGuidObjectState (ruleId)));
        }
        graphicOverrideCombinationsList (combinationOs);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

CreateGraphicOverrideCombinationsCommand::CreateGraphicOverrideCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateGraphicOverrideCombinationsCommand::GetName () const
{
    return "CreateGraphicOverrideCombinations";
}

GS::Optional<GS::UniString> CreateGraphicOverrideCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideCombinations": {
                "type": "array",
                "description": "The graphic override combinations to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the combination, unique within the project."
                        },
                        "graphicOverrideRuleIds": {
                            "type": "array",
                            "description": "The rules of the combination, in priority order.",
                            "items": {
                                "$ref": "#/GraphicOverrideRuleIdArrayItem"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "graphicOverrideRuleIds"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> CreateGraphicOverrideCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideCombinationIds": {
                "type": "array",
                "description": "The identifiers of the created combinations, or errors, in the order of the input.",
                "items": {
                    "$ref": "#/GraphicOverrideCombinationIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinationIds"
        ]
    })";
}

GS::ObjectState CreateGraphicOverrideCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> graphicOverrideCombinations;
    if (!parameters.Get ("graphicOverrideCombinations", graphicOverrideCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'graphicOverrideCombinations' parameter.");
    }

    GS::ObjectState response;
    const auto& graphicOverrideCombinationIdsList = response.AddList<GS::ObjectState> ("graphicOverrideCombinationIds");
    ACAPI_CallUndoableCommand ("Create Graphic Override Combinations", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : graphicOverrideCombinations) {
            API_OverrideCombination combination = {};
            GS::Array<GS::ObjectState> ruleIds;
            if (!item.Get ("name", combination.name) || !item.Get ("graphicOverrideRuleIds", ruleIds)) {
                graphicOverrideCombinationIdsList (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' or 'graphicOverrideRuleIds' parameter."));
                continue;
            }
            const GSErrCode err = ACAPI_GraphicalOverride_CreateOverrideCombination (combination, GetRuleGuidsFromArray (ruleIds));
            if (err != NoError) {
                graphicOverrideCombinationIdsList (CreateErrorResponse (err, "Failed to create the combination '" + combination.name + "'. Its name must be unique and not empty, and its rules must exist."));
                continue;
            }
            graphicOverrideCombinationIdsList (GS::ObjectState ("graphicOverrideCombinationId", CreateGuidObjectState (combination.guid)));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}
