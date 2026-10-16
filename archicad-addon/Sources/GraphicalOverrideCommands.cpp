#include "GraphicalOverrideCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2700
// Override rule groups exist from Archicad 27.
static GSErrCode FindOrCreateRuleGroup (const GS::UniString& name, API_Guid& outGuid)
{
    GS::Array<API_Guid> groupIds;
    GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleGroupList (groupIds);
    if (err != NoError) {
        return err;
    }
    for (const API_Guid& id : groupIds) {
        API_OverrideRuleGroup group = {id, ""};
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (group) == NoError && group.name == name) {
            outGuid = id;
            return NoError;
        }
    }
    API_OverrideRuleGroup group = {APINULLGuid, name};
    err = ACAPI_GraphicalOverride_CreateOverrideRuleGroup (group);
    if (err != NoError) {
        return err;
    }
    outGuid = group.guid;
    return NoError;
}
#endif

CreateGraphicalOverrideRuleCommand::CreateGraphicalOverrideRuleCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateGraphicalOverrideRuleCommand::GetName () const
{
    return "CreateGraphicalOverrideRule";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRuleCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Name of the override rule. A rule with the same name in the rule group is overwritten."
            },
            "ruleGroupName": {
                "type": "string",
                "description": "Name of the rule group. Created when it does not exist. Default: 'Tapir'."
            },
            "criterionXML": {
                "type": "string",
                "description": "Optional criterion XML. Archicad only takes XML in its own format: read it from a rule made in the GUI with GetGraphicalOverrideRules and change the values. XML written from scratch is dropped without an error, and so is an empty one: the rule then applies to all elements."
            },
            "surfaceRGB": {
                "type": "object",
                "description": "Optional color override of the cut and uncut surfaces. Components are in the 0..1 range.",
                "properties": {
                    "red": { "type": "number" },
                    "green": { "type": "number" },
                    "blue": { "type": "number" }
                },
                "required": ["red", "green", "blue"],
                "additionalProperties": false
            },
            "linePenIndex": {
                "type": "integer",
                "description": "Optional pen index override for lines, markers and texts. Only a positive index takes effect."
            },
            "overrideContours": {
                "type": "boolean",
                "description": "Optional. If true, the contours of the overridden surfaces are recolored too."
            }
        },
        "additionalProperties": false,
        "required": ["name"]
    })";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRuleCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleId": {
                "type": "object",
                "description": "The created or overwritten override rule.",
                "properties": {
                    "guid": { "$ref": "#/Guid" }
                },
                "additionalProperties": false,
                "required": ["guid"]
            }
        },
        "additionalProperties": false,
        "required": ["ruleId"]
    })";
}

GS::ObjectState CreateGraphicalOverrideRuleCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString name;
    parameters.Get ("name", name);
    GS::UniString ruleGroupName = "Tapir";
    parameters.Get ("ruleGroupName", ruleGroupName);

    API_Guid groupGuid = APINULLGuid;
    GSErrCode err = FindOrCreateRuleGroup (ruleGroupName, groupGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to find or create the override rule group.");
    }

    API_OverrideRuleStyle style = {};
    style.lineMarkerTextPen = APINullValue;

    Int32 linePenIndex = 0;
    if (parameters.Get ("linePenIndex", linePenIndex) && linePenIndex > 0) {
        style.lineMarkerTextPen = static_cast<short> (linePenIndex);
    }

    const GS::ObjectState* surfaceRGBOS = parameters.Get ("surfaceRGB");
    if (surfaceRGBOS != nullptr) {
        API_RGBColor color = GetColorFromObjectState (*surfaceRGBOS);
        style.surfaceOverride = color;
        style.surfaceType.overrideCutSurface = true;
        style.surfaceType.overrideUncutSurface = true;
    }

    bool overrideContours = false;
    if (parameters.Get ("overrideContours", overrideContours)) {
        style.overrideContours = overrideContours;
    }

    GS::UniString criterionXML;
    parameters.Get ("criterionXML", criterionXML);

    API_OverrideRule rule = {APINULLGuid, name, style, criterionXML};
    err = ACAPI_GraphicalOverride_CreateOverrideRule (rule, groupGuid);
    if (err == APIERR_NAMEALREADYUSED) {
        API_OverrideRule existing = {APINULLGuid, name};
        err = ACAPI_GraphicalOverride_GetOverrideRuleByName (existing, groupGuid);
        if (err == NoError) {
            rule.guid = existing.guid;
            err = ACAPI_GraphicalOverride_ChangeOverrideRule (rule);
        }
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create or change the override rule.");
    }

    GS::ObjectState response;
    response.Add ("ruleId", CreateGuidObjectState (rule.guid));
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer (graphic override rule groups).");
#endif
}

GetGraphicalOverrideRulesCommand::GetGraphicalOverrideRulesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGraphicalOverrideRulesCommand::GetName () const
{
    return "GetGraphicalOverrideRules";
}

GS::Optional<GS::UniString> GetGraphicalOverrideRulesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "rules": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ruleId": {
                            "type": "object",
                            "properties": {
                                "guid": { "$ref": "#/Guid" }
                            },
                            "additionalProperties": false,
                            "required": ["guid"]
                        },
                        "name": { "type": "string" },
                        "criterionXML": { "type": "string", "description": "The criterion in Archicad's own XML format. Change its values and pass it to CreateGraphicalOverrideRule to make similar rules." }
                    },
                    "required": ["ruleId", "name", "criterionXML"],
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["rules"]
    })";
}

GS::ObjectState GetGraphicalOverrideRulesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::ObjectState response;
    const auto& rules = response.AddList<GS::ObjectState> ("rules");

    GS::Array<API_Guid> ruleIds;
    GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleList (ruleIds);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the override rule list.");
    }
    for (const API_Guid& id : ruleIds) {
        API_OverrideRule rule = {id};
        if (ACAPI_GraphicalOverride_GetOverrideRuleById (rule) != NoError) {
            continue;
        }
        GS::ObjectState ruleOS;
        ruleOS.Add ("ruleId", CreateGuidObjectState (rule.guid));
        ruleOS.Add ("name", rule.name);
        ruleOS.Add ("criterionXML", rule.criterionXML);
        rules (ruleOS);
    }
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer (graphic override rule groups).");
#endif
}

CreateGraphicalOverrideCombinationCommand::CreateGraphicalOverrideCombinationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateGraphicalOverrideCombinationCommand::GetName () const
{
    return "CreateGraphicalOverrideCombination";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideCombinationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Name of the override combination. When a combination with this name exists, its rule list is replaced."
            },
            "ruleNames": {
                "type": "array",
                "description": "Names of the override rules in the combination, in order. Each name takes the first rule of that name in any rule group. An empty list makes a combination without rules.",
                "items": { "type": "string" }
            }
        },
        "additionalProperties": false,
        "required": ["name", "ruleNames"]
    })";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideCombinationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "combinationId": {
                "type": "object",
                "description": "The created or changed override combination.",
                "properties": {
                    "guid": { "$ref": "#/Guid" }
                },
                "additionalProperties": false,
                "required": ["guid"]
            }
        },
        "additionalProperties": false,
        "required": ["combinationId"]
    })";
}

GS::ObjectState CreateGraphicalOverrideCombinationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString name;
    parameters.Get ("name", name);
    GS::Array<GS::UniString> ruleNames;
    parameters.Get ("ruleNames", ruleNames);

    GS::Array<API_Guid> allRuleIds;
    GSErrCode err = ACAPI_GraphicalOverride_GetOverrideRuleList (allRuleIds);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the override rule list.");
    }

    GS::Array<API_Guid> ruleGuids;
    for (const GS::UniString& ruleName : ruleNames) {
        bool found = false;
        for (const API_Guid& id : allRuleIds) {
            API_OverrideRule rule = {id};
            if (ACAPI_GraphicalOverride_GetOverrideRuleById (rule) == NoError && rule.name == ruleName) {
                ruleGuids.Push (id);
                found = true;
                break;
            }
        }
        if (!found) {
            return CreateErrorResponse (APIERR_BADPARS, "Override rule not found: " + ruleName);
        }
    }

    API_OverrideCombination combination = {APINULLGuid, name};
    err = ACAPI_GraphicalOverride_CreateOverrideCombination (combination, ruleGuids);
    if (err == APIERR_NAMEALREADYUSED) {
        API_OverrideCombination existing = {APINULLGuid, name};
        err = ACAPI_GraphicalOverride_GetOverrideCombination (existing);
        if (err == NoError) {
            combination.guid = existing.guid;
            err = ACAPI_GraphicalOverride_ChangeOverrideCombination (combination, &ruleGuids);
        }
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create or change the override combination.");
    }

    GS::ObjectState response;
    response.Add ("combinationId", CreateGuidObjectState (combination.guid));
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer (graphic override rule groups).");
#endif
}
