#include "GraphicOverrideCommands.hpp"
#include "GraphicOverrideCriterion.hpp"

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

// A rule does not name its group; the group is given when it is known.
GS::ObjectState CreateRuleObjectState (const API_OverrideRule& rule, const GS::Optional<GS::UniString>& ruleGroupName)
{
    GS::ObjectState ruleOs (
        "graphicOverrideRuleId", CreateGuidObjectState (rule.guid),
        "name", rule.name,
        "criterionXML", rule.criterionXML,
        "style", CreateStyleObjectState (rule.style));
    if (ruleGroupName.HasValue ()) {
        ruleOs.Add ("ruleGroupName", ruleGroupName.Get ());
    }
    return ruleOs;
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

// The criterion of a rule comes either as Archicad's XML ("criterionXML") or as the structured
// "criterion", which is turned into that XML. Neither given leaves criterionXML unchanged.
GSErrCode GetCriterionXMLFromObjectState (const GS::ObjectState& item, GS::UniString& criterionXML, GS::UniString& errorMessage)
{
    const GS::ObjectState* criterion = item.Get ("criterion");
    if (criterion == nullptr) {
        item.Get ("criterionXML", criterionXML);
        return NoError;
    }
    if (item.Contains ("criterionXML")) {
        errorMessage = "Give either 'criterion' or 'criterionXML', not both.";
        return APIERR_BADPARS;
    }
    GS::UniString criterionError;
    if (!GraphicOverrideCriterionToXML (*criterion, criterionXML, criterionError)) {
        errorMessage = "Invalid 'criterion': " + criterionError;
        return APIERR_BADPARS;
    }
    return NoError;
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
                    "$ref": "#/GraphicOverrideRule"
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
        GS::Optional<GS::UniString> ruleGroupName;
        if (ruleGroupNames.ContainsKey (ruleId)) {
            ruleGroupName = ruleGroupNames[ruleId];
        }
        graphicOverrideRulesList (CreateRuleObjectState (rule, ruleGroupName));
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetGraphicOverrideRulesByNameCommand::GetGraphicOverrideRulesByNameCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGraphicOverrideRulesByNameCommand::GetName () const
{
    return "GetGraphicOverrideRulesByName";
}

GS::Optional<GS::UniString> GetGraphicOverrideRulesByNameCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules to get, by name.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the rule. Rule names are unique within the project."
                        },
                        "ruleGroupName": {
                            "type": "string",
                            "description": "The name of the rule group to search the rule in. Left out: every rule group is searched."
                        }
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
            "graphicOverrideRules"
        ]
    })";
}

GS::Optional<GS::UniString> GetGraphicOverrideRulesByNameCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The rules, or errors, in the order of the input.",
                "items": {
                    "$ref": "#/GraphicOverrideRuleOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
        ]
    })";
}

GS::ObjectState GetGraphicOverrideRulesByNameCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> graphicOverrideRules;
    if (!parameters.Get ("graphicOverrideRules", graphicOverrideRules)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'graphicOverrideRules' parameter.");
    }

    // ACAPI_GraphicalOverride_GetOverrideRuleByName searches one rule group, so the groups are
    // listed once: a rule is looked up in the group named in its item, or in every group in turn.
    GS::Array<API_Guid> ruleGroupIds;
    const GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleGroupList (ruleGroupIds);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the graphic override rule groups.");
    }
    GS::Array<API_OverrideRuleGroup> ruleGroups;
    for (const API_Guid& ruleGroupId : ruleGroupIds) {
        API_OverrideRuleGroup ruleGroup = {};
        ruleGroup.guid = ruleGroupId;
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (ruleGroup) == NoError) {
            ruleGroups.Push (ruleGroup);
        }
    }

    GS::ObjectState response;
    const auto& graphicOverrideRulesList = response.AddList<GS::ObjectState> ("graphicOverrideRules");
    for (const GS::ObjectState& item : graphicOverrideRules) {
        GS::UniString name;
        if (!item.Get ("name", name)) {
            graphicOverrideRulesList (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' parameter."));
            continue;
        }
        GS::UniString ruleGroupName;
        const bool hasRuleGroupName = item.Get ("ruleGroupName", ruleGroupName);

        bool ruleGroupFound = false;
        bool ruleFound = false;
        for (const API_OverrideRuleGroup& ruleGroup : ruleGroups) {
            if (hasRuleGroupName && ruleGroup.name != ruleGroupName) {
                continue;
            }
            ruleGroupFound = true;
            API_OverrideRule rule = {};
            rule.name = name;
            if (ACAPI_GraphicalOverride_GetOverrideRuleByName (rule, ruleGroup.guid) == NoError && rule.guid != APINULLGuid) {
                graphicOverrideRulesList (CreateRuleObjectState (rule, ruleGroup.name));
                ruleFound = true;
                break;
            }
        }
        if (ruleFound) {
            continue;
        }
        if (hasRuleGroupName && !ruleGroupFound) {
            graphicOverrideRulesList (CreateErrorResponse (APIERR_BADNAME, "No rule group is named '" + ruleGroupName + "'."));
        } else if (hasRuleGroupName) {
            graphicOverrideRulesList (CreateErrorResponse (APIERR_BADNAME, "No graphic override rule is named '" + name + "' in the rule group '" + ruleGroupName + "'."));
        } else {
            graphicOverrideRulesList (CreateErrorResponse (APIERR_BADNAME, "No graphic override rule is named '" + name + "'."));
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
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
                            "description": "The criterion of the rule, in the XML format Archicad saves override rules in. Take it from GetGraphicOverrideRules and change its values: Archicad drops XML written from scratch without an error, and the rule then applies to all elements. Left out together with 'criterion': Archicad's default criterion for new rules."
                        },
                        "criterion": {
                            "$ref": "#/GraphicOverrideCriterion",
                            "description": "The criterion of the rule as structured conditions, turned into Archicad's criterion XML. Use it instead of criterionXML, not with it."
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
            GS::UniString criterionErrorMessage;
            const GSErrCode criterionErr = GetCriterionXMLFromObjectState (item, rule.criterionXML, criterionErrorMessage);
            if (criterionErr != NoError) {
                graphicOverrideRuleIdsList (CreateErrorResponse (criterionErr, criterionErrorMessage));
                continue;
            }
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
                        "criterion": {
                            "$ref": "#/GraphicOverrideCriterion",
                            "description": "The new criterion of the rule as structured conditions, turned into Archicad's criterion XML. Use it instead of criterionXML, not with it."
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
            GS::UniString criterionErrorMessage;
            err = GetCriterionXMLFromObjectState (item, rule.criterionXML, criterionErrorMessage);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, criterionErrorMessage));
                continue;
            }
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

GetGraphicOverrideRuleGroupsCommand::GetGraphicOverrideRuleGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGraphicOverrideRuleGroupsCommand::GetName () const
{
    return "GetGraphicOverrideRuleGroups";
}

GS::Optional<GS::UniString> GetGraphicOverrideRuleGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideRuleGroups": {
                "type": "array",
                "description": "The graphic override rule groups of the project, empty ones included.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "graphicOverrideRuleIds": {
                            "type": "array",
                            "description": "The rules of the group.",
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
            "graphicOverrideRuleGroups"
        ]
    })";
}

GS::ObjectState GetGraphicOverrideRuleGroupsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<API_Guid> ruleGroupIds;
    const GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleGroupList (ruleGroupIds);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the graphic override rule groups.");
    }

    GS::ObjectState response;
    const auto& graphicOverrideRuleGroupsList = response.AddList<GS::ObjectState> ("graphicOverrideRuleGroups");
    for (const API_Guid& ruleGroupId : ruleGroupIds) {
        API_OverrideRuleGroup ruleGroup = {};
        ruleGroup.guid = ruleGroupId;
        GS::Array<API_Guid> ruleIds;
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (ruleGroup, &ruleIds) != NoError) {
            continue;
        }
        GS::ObjectState ruleGroupOs ("name", ruleGroup.name);
        const auto& ruleIdsList = ruleGroupOs.AddList<GS::ObjectState> ("graphicOverrideRuleIds");
        for (const API_Guid& ruleId : ruleIds) {
            ruleIdsList (GS::ObjectState ("graphicOverrideRuleId", CreateGuidObjectState (ruleId)));
        }
        graphicOverrideRuleGroupsList (ruleGroupOs);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

DeleteGraphicOverrideRuleGroupsCommand::DeleteGraphicOverrideRuleGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteGraphicOverrideRuleGroupsCommand::GetName () const
{
    return "DeleteGraphicOverrideRuleGroups";
}

GS::Optional<GS::UniString> DeleteGraphicOverrideRuleGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleGroupNames": {
                "type": "array",
                "description": "The names of the rule groups to delete.",
                "items": {
                    "type": "string"
                }
            },
            "moveRulesToGroup": {
                "type": "string",
                "description": "The name of the rule group the rules of the deleted groups are moved to, so they are kept. Created when no rule group has this name."
            },
            "deleteRules": {
                "type": "boolean",
                "description": "Delete the rules of the deleted groups too. Without this or moveRulesToGroup, a group that still contains rules is not deleted."
            }
        },
        "additionalProperties": false,
        "required": [
            "ruleGroupNames"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteGraphicOverrideRuleGroupsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteGraphicOverrideRuleGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::UniString> ruleGroupNames;
    if (!parameters.Get ("ruleGroupNames", ruleGroupNames)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'ruleGroupNames' parameter.");
    }
    GS::UniString moveRulesToGroup;
    const bool moveRules = parameters.Get ("moveRulesToGroup", moveRulesToGroup);
    bool deleteRules = false;
    parameters.Get ("deleteRules", deleteRules);
    if (moveRules && deleteRules) {
        return CreateErrorResponse (APIERR_BADPARS, "Give either 'moveRulesToGroup' or 'deleteRules', not both.");
    }

    // Archicad deletes the rules of a group together with the group, and a rule cannot live
    // outside a group. So the rules are moved first, deleted on request, or the group is kept.
    const auto findRuleGroup = [] (const GS::UniString& name, API_Guid& ruleGroupGuid, GS::Array<API_Guid>& ruleIds) -> bool {
        GS::Array<API_Guid> ruleGroupIds;
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroupList (ruleGroupIds) != NoError) {
            return false;
        }
        for (const API_Guid& ruleGroupId : ruleGroupIds) {
            API_OverrideRuleGroup ruleGroup = {};
            ruleGroup.guid = ruleGroupId;
            ruleIds.Clear ();
            if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (ruleGroup, &ruleIds) == NoError && ruleGroup.name == name) {
                ruleGroupGuid = ruleGroup.guid;
                return true;
            }
        }
        return false;
    };

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    ACAPI_CallUndoableCommand ("Delete Graphic Override Rule Groups", [&] () -> GSErrCode {
        for (const GS::UniString& ruleGroupName : ruleGroupNames) {
            API_Guid ruleGroupGuid = APINULLGuid;
            GS::Array<API_Guid> ruleIds;
            if (!findRuleGroup (ruleGroupName, ruleGroupGuid, ruleIds)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADNAME, "No rule group is named '" + ruleGroupName + "'."));
                continue;
            }

            if (!ruleIds.IsEmpty () && moveRules) {
                if (moveRulesToGroup == ruleGroupName) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The rules of '" + ruleGroupName + "' cannot be moved into the group itself."));
                    continue;
                }
                API_Guid targetGuid = APINULLGuid;
                GSErrCode err = FindOrCreateRuleGroup (moveRulesToGroup, targetGuid);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to find or create the rule group '" + moveRulesToGroup + "'."));
                    continue;
                }
                // nextRule nullptr appends the rule to the target group, so the rules keep their order.
                UIndex movedCount = 0;
                for (; movedCount < ruleIds.GetSize (); ++movedCount) {
                    err = ACAPI_GraphicalOverride_MoveOverrideRule (ruleIds[movedCount], targetGuid, nullptr);
                    if (err != NoError) {
                        break;
                    }
                }
                if (err != NoError) {
                    // Put the rules already moved back in front of the one that failed, in their order,
                    // so the group is left as it was.
                    for (UIndex i = 0; i < movedCount; ++i) {
                        ACAPI_GraphicalOverride_MoveOverrideRule (ruleIds[i], ruleGroupGuid, &ruleIds[movedCount]);
                    }
                    executionResults (CreateFailedExecutionResult (err, "Failed to move the rules of '" + ruleGroupName + "'. The group and its rules are kept."));
                    continue;
                }
            } else if (!ruleIds.IsEmpty () && !deleteRules) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The rule group '" + ruleGroupName + "' still contains " + GS::ValueToUniString (ruleIds.GetSize ()) + " rule(s). Set 'moveRulesToGroup' to keep them or 'deleteRules' to delete them."));
                continue;
            }

            const GSErrCode err = ACAPI_GraphicalOverride_DeleteOverrideRuleGroup (ruleGroupGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to delete the rule group '" + ruleGroupName + "'."));
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

DeleteGraphicOverrideCombinationsCommand::DeleteGraphicOverrideCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteGraphicOverrideCombinationsCommand::GetName () const
{
    return "DeleteGraphicOverrideCombinations";
}

GS::Optional<GS::UniString> DeleteGraphicOverrideCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "graphicOverrideCombinationIds": {
                "type": "array",
                "description": "The graphic override combinations to delete. Their rules are kept.",
                "items": {
                    "$ref": "#/GraphicOverrideCombinationIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinationIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteGraphicOverrideCombinationsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteGraphicOverrideCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> graphicOverrideCombinationIds;
    if (!parameters.Get ("graphicOverrideCombinationIds", graphicOverrideCombinationIds)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'graphicOverrideCombinationIds' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    ACAPI_CallUndoableCommand ("Delete Graphic Override Combinations", [&] () -> GSErrCode {
        for (const GS::ObjectState& combinationId : graphicOverrideCombinationIds) {
            const API_Guid combinationGuid = GetGuidFromArrayItem ("graphicOverrideCombinationId", combinationId);
            const GSErrCode err = ACAPI_GraphicalOverride_DeleteOverrideCombination (combinationGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to delete the graphic override combination."));
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
