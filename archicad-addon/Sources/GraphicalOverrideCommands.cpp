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
        API_OverrideRuleGroup group = {};
        group.guid = id;
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (group) == NoError && group.name == name) {
            outGuid = id;
            return NoError;
        }
    }
    API_OverrideRuleGroup group = {};
    group.guid = APINULLGuid;
    group.name = name;
    err = ACAPI_GraphicalOverride_CreateOverrideRuleGroup (group);
    if (err != NoError) {
        return err;
    }
    outGuid = group.guid;
    return NoError;
}

// Rule names are unique in the whole project, so a rule is found by its name among all rules,
// whatever its rule group.
static GS::Optional<API_Guid> FindOverrideRuleByName (const GS::UniString& name)
{
    GS::Array<API_Guid> ruleIds;
    if (ACAPI_GraphicalOverride_GetOverrideRuleList (ruleIds) != NoError) {
        return {};
    }
    for (const API_Guid& id : ruleIds) {
        API_OverrideRule rule = {};
        rule.guid = id;
        if (ACAPI_GraphicalOverride_GetOverrideRuleById (rule) == NoError && rule.name == name) {
            return id;
        }
    }
    return {};
}

// A combination is found by its name among all combinations, in the same way as a rule.
static GS::Optional<API_Guid> FindOverrideCombinationByName (const GS::UniString& name)
{
    GS::Array<API_Guid> combinationIds;
    if (ACAPI_GraphicalOverride_GetOverrideCombinationList (combinationIds) != NoError) {
        return {};
    }
    for (const API_Guid& id : combinationIds) {
        API_OverrideCombination combination = {};
        combination.guid = id;
        if (ACAPI_GraphicalOverride_GetOverrideCombination (combination) == NoError && combination.name == name) {
            return id;
        }
    }
    return {};
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
                "description": "Name of the override rule. Rule names are unique in the project: an existing rule with this name is overwritten, in whichever rule group it is."
            },
            "ruleGroupName": {
                "type": "string",
                "description": "Rule group of a new rule, created when it does not exist. Default: 'Tapir'. An overwritten rule stays in its own group."
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
                "$ref": "#/GuidId",
                "description": "The created or overwritten override rule."
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

    API_OverrideRule rule = {};
    rule.guid = APINULLGuid;
    rule.name = name;
    rule.style = style;
    rule.criterionXML = criterionXML;

    // An existing rule with this name, in any rule group, is overwritten where it is; only a new
    // rule needs its group, which is created when missing.
    GSErrCode err = NoError;
    const GS::Optional<API_Guid> existingGuid = FindOverrideRuleByName (name);
    if (existingGuid.HasValue ()) {
        rule.guid = existingGuid.Get ();
        err = ACAPI_GraphicalOverride_ChangeOverrideRule (rule);
    } else {
        API_Guid groupGuid = APINULLGuid;
        err = FindOrCreateRuleGroup (ruleGroupName, groupGuid);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to find or create the override rule group.");
        }
        err = ACAPI_GraphicalOverride_CreateOverrideRule (rule, groupGuid);
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
                        "ruleId": { "$ref": "#/GuidId" },
                        "name": { "type": "string" },
                        "ruleGroupName": { "type": "string", "description": "Name of the rule group the rule belongs to." },
                        "criterionXML": { "type": "string", "description": "The criterion in Archicad's own XML format. Change its values and pass it to CreateGraphicalOverrideRule to make similar rules." }
                    },
                    "required": ["ruleId", "name", "ruleGroupName", "criterionXML"],
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

    // A rule does not name its group; the groups list their rules.
    GS::HashTable<API_Guid, GS::UniString> groupNameOfRule;
    GS::Array<API_Guid> groupIds;
    if (ACAPI_GraphicalOverride_GetOverrideRuleGroupList (groupIds) == NoError) {
        for (const API_Guid& groupId : groupIds) {
            API_OverrideRuleGroup group = {};
            group.guid = groupId;
            GS::Array<API_Guid> groupRuleIds;
            if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (group, &groupRuleIds) == NoError) {
                for (const API_Guid& ruleId : groupRuleIds) {
                    groupNameOfRule.Put (ruleId, group.name);
                }
            }
        }
    }

    for (const API_Guid& id : ruleIds) {
        API_OverrideRule rule = {};
        rule.guid = id;
        if (ACAPI_GraphicalOverride_GetOverrideRuleById (rule) != NoError) {
            continue;
        }
        GS::UniString ruleGroupName;
        groupNameOfRule.Get (rule.guid, &ruleGroupName);
        GS::ObjectState ruleOS;
        ruleOS.Add ("ruleId", CreateGuidObjectState (rule.guid));
        ruleOS.Add ("name", rule.name);
        ruleOS.Add ("ruleGroupName", ruleGroupName);
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
                "description": "Names of the override rules in the combination, in order. Rule names are unique in the project, whatever the rule group. An empty list makes a combination without rules.",
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
                "$ref": "#/GuidId",
                "description": "The created or changed override combination."
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

    GS::Array<API_Guid> ruleGuids;
    for (const GS::UniString& ruleName : ruleNames) {
        const GS::Optional<API_Guid> ruleGuid = FindOverrideRuleByName (ruleName);
        if (!ruleGuid.HasValue ()) {
            return CreateErrorResponse (APIERR_BADPARS, "Override rule not found: " + ruleName);
        }
        ruleGuids.Push (ruleGuid.Get ());
    }

    API_OverrideCombination combination = {};
    combination.guid = APINULLGuid;
    combination.name = name;

    // An existing combination with this name gets the new rule list; otherwise it is created.
    GSErrCode err = NoError;
    const GS::Optional<API_Guid> existingGuid = FindOverrideCombinationByName (name);
    if (existingGuid.HasValue ()) {
        combination.guid = existingGuid.Get ();
        err = ACAPI_GraphicalOverride_ChangeOverrideCombination (combination, &ruleGuids);
    } else {
        err = ACAPI_GraphicalOverride_CreateOverrideCombination (combination, ruleGuids);
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
