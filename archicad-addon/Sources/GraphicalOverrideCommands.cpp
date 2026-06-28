#include "GraphicalOverrideCommands.hpp"

#include "MigrationHelper.hpp"

// ---------------------------------------------------------------------------
// Serialization helpers (API -> ObjectState)
// ---------------------------------------------------------------------------

static GS::ObjectState RGBColorToOS (const API_RGBColor& c)
{
    GS::ObjectState os;
    os.Add ("red",   c.f_red);
    os.Add ("green", c.f_green);
    os.Add ("blue",  c.f_blue);
    return os;
}

static GS::ObjectState OverriddenAttributeToOS (const API_OverriddenAttribute& v)
{
    GS::ObjectState os;
    os.Add ("isOverridden", v.hasValue);
    if (v.hasValue)
        os.Add ("attributeIndex", GetAttributeIndex (v.value));
    return os;
}

static GS::ObjectState OverriddenPenToOS (const API_OverriddenPen& v)
{
    GS::ObjectState os;
    os.Add ("isOverridden", v.hasValue);
    if (v.hasValue)
        os.Add ("penIndex", (int) v.value);
    return os;
}

static GS::ObjectState OverriddenPenOrRGBToOS (const API_OverriddenPenOrRGB& v)
{
    GS::ObjectState os;
    os.Add ("isOverridden", v.hasValue);
    if (v.hasValue) {
        if (v.value.IsType<API_PenIndex> ())
            os.Add ("penIndex", (int) v.value.Get<API_PenIndex> ());
        else
            os.Add ("rgbColor", RGBColorToOS (v.value.Get<API_RGBColor> ()));
    }
    return os;
}

static GS::ObjectState OverriddenAttributeOrRGBToOS (const API_OverriddenAttributeOrRGB& v)
{
    GS::ObjectState os;
    os.Add ("isOverridden", v.hasValue);
    if (v.hasValue) {
        if (v.value.IsType<API_AttributeIndex> ())
            os.Add ("attributeIndex", GetAttributeIndex (v.value.Get<API_AttributeIndex> ()));
        else
            os.Add ("rgbColor", RGBColorToOS (v.value.Get<API_RGBColor> ()));
    }
    return os;
}

static GS::ObjectState FillTypeToOS (const API_OverriddenFillType& v)
{
    GS::ObjectState os;
    os.Add ("overrideCutFill",      v.overrideCutFill);
    os.Add ("overrideCoverFill",    v.overrideCoverFill);
    os.Add ("overrideDraftingFill", v.overrideDraftingFill);
    return os;
}

static GS::ObjectState SurfaceTypeToOS (const API_OverriddenSurfaceType& v)
{
    GS::ObjectState os;
    os.Add ("overrideCutSurface",   v.overrideCutSurface);
    os.Add ("overrideUncutSurface", v.overrideUncutSurface);
    return os;
}

static GS::ObjectState RuleStyleToOS (const API_OverrideRuleStyle& s)
{
    GS::ObjectState os;
    os.Add ("lineType",                     OverriddenAttributeToOS (s.lineType));
    os.Add ("lineMarkerTextPen",            OverriddenPenToOS (s.lineMarkerTextPen));
    os.Add ("fillOverride",                 OverriddenAttributeToOS (s.fillOverride));
    os.Add ("fillType",                     FillTypeToOS (s.fillType));
    os.Add ("fillForegroundPenOverride",    OverriddenPenToOS (s.fillForegroundPenOverride));
    os.Add ("fillTypeForegroundPen",        FillTypeToOS (s.fillTypeForegroundPen));
    os.Add ("fillBackgroundPenOverride",    OverriddenPenOrRGBToOS (s.fillBackgroundPenOverride));
    os.Add ("fillTypeBackgroundPen",        FillTypeToOS (s.fillTypeBackgroundPen));
    os.Add ("surfaceOverride",              OverriddenAttributeOrRGBToOS (s.surfaceOverride));
    os.Add ("surfaceType",                  SurfaceTypeToOS (s.surfaceType));
    os.Add ("showSkinSeparators",           s.showSkinSeparators);
    os.Add ("overridePenColorAndThickness", s.overridePenColorAndThickness);
    os.Add ("hiddenContours",               SurfaceTypeToOS (s.hiddenContours));
    os.Add ("overrideContours",             s.overrideContours);
    return os;
}

// ---------------------------------------------------------------------------
// Deserialization helpers (ObjectState -> API)
// ---------------------------------------------------------------------------

static API_RGBColor RGBColorFromOS (const GS::ObjectState& os)
{
    API_RGBColor c = {};
    os.Get ("red",   c.f_red);
    os.Get ("green", c.f_green);
    os.Get ("blue",  c.f_blue);
    return c;
}

static API_OverriddenAttribute OverriddenAttributeFromOS (const GS::ObjectState& os)
{
    API_OverriddenAttribute v;
    v.hasValue = false;
    bool isOverridden = false;
    os.Get ("isOverridden", isOverridden);
    if (isOverridden) {
        Int32 idx = 0;
        os.Get ("attributeIndex", idx);
        v = ACAPI_CreateAttributeIndex (idx);
    } else {
        v = APINullValue;
    }
    return v;
}

static API_OverriddenPen OverriddenPenFromOS (const GS::ObjectState& os)
{
    API_OverriddenPen v;
    v.hasValue = false;
    bool isOverridden = false;
    os.Get ("isOverridden", isOverridden);
    if (isOverridden) {
        int penIdx = 0;
        os.Get ("penIndex", penIdx);
        v = (API_PenIndex) penIdx;
    } else {
        v = APINullValue;
    }
    return v;
}

static API_OverriddenPenOrRGB OverriddenPenOrRGBFromOS (const GS::ObjectState& os)
{
    API_OverriddenPenOrRGB v;
    v.hasValue = false;
    bool isOverridden = false;
    os.Get ("isOverridden", isOverridden);
    if (isOverridden) {
        int penIdx = -1;
        GS::ObjectState rgbOS;
        if (os.Get ("penIndex", penIdx)) {
            v = (API_PenIndex) penIdx;
        } else if (os.Get ("rgbColor", rgbOS)) {
            v = RGBColorFromOS (rgbOS);
        }
    }
    return v;
}

static API_OverriddenAttributeOrRGB OverriddenAttributeOrRGBFromOS (const GS::ObjectState& os)
{
    API_OverriddenAttributeOrRGB v;
    v.hasValue = false;
    bool isOverridden = false;
    os.Get ("isOverridden", isOverridden);
    if (isOverridden) {
        Int32 attrIdx = -1;
        GS::ObjectState rgbOS;
        if (os.Get ("attributeIndex", attrIdx)) {
            v = ACAPI_CreateAttributeIndex (attrIdx);
        } else if (os.Get ("rgbColor", rgbOS)) {
            v = RGBColorFromOS (rgbOS);
        }
    }
    return v;
}

static API_OverriddenFillType FillTypeFromOS (const GS::ObjectState& os)
{
    API_OverriddenFillType v = {};
    os.Get ("overrideCutFill",      v.overrideCutFill);
    os.Get ("overrideCoverFill",    v.overrideCoverFill);
    os.Get ("overrideDraftingFill", v.overrideDraftingFill);
    return v;
}

static API_OverriddenSurfaceType SurfaceTypeFromOS (const GS::ObjectState& os)
{
    API_OverriddenSurfaceType v = {};
    os.Get ("overrideCutSurface",   v.overrideCutSurface);
    os.Get ("overrideUncutSurface", v.overrideUncutSurface);
    return v;
}

static API_OverrideRuleStyle RuleStyleFromOS (const GS::ObjectState& os)
{
    API_OverrideRuleStyle s = {};
    GS::ObjectState fieldOS;

    if (os.Get ("lineType", fieldOS))                  s.lineType                    = OverriddenAttributeFromOS (fieldOS);
    if (os.Get ("lineMarkerTextPen", fieldOS))         s.lineMarkerTextPen           = OverriddenPenFromOS (fieldOS);
    if (os.Get ("fillOverride", fieldOS))              s.fillOverride                = OverriddenAttributeFromOS (fieldOS);
    if (os.Get ("fillType", fieldOS))                  s.fillType                    = FillTypeFromOS (fieldOS);
    if (os.Get ("fillForegroundPenOverride", fieldOS)) s.fillForegroundPenOverride   = OverriddenPenFromOS (fieldOS);
    if (os.Get ("fillTypeForegroundPen", fieldOS))     s.fillTypeForegroundPen       = FillTypeFromOS (fieldOS);
    if (os.Get ("fillBackgroundPenOverride", fieldOS)) s.fillBackgroundPenOverride   = OverriddenPenOrRGBFromOS (fieldOS);
    if (os.Get ("fillTypeBackgroundPen", fieldOS))     s.fillTypeBackgroundPen       = FillTypeFromOS (fieldOS);
    if (os.Get ("surfaceOverride", fieldOS))           s.surfaceOverride             = OverriddenAttributeOrRGBFromOS (fieldOS);
    if (os.Get ("surfaceType", fieldOS))               s.surfaceType                 = SurfaceTypeFromOS (fieldOS);
    os.Get ("showSkinSeparators",           s.showSkinSeparators);
    os.Get ("overridePenColorAndThickness", s.overridePenColorAndThickness);
    if (os.Get ("hiddenContours", fieldOS))            s.hiddenContours              = SurfaceTypeFromOS (fieldOS);
    os.Get ("overrideContours", s.overrideContours);
    return s;
}

// ---------------------------------------------------------------------------
// GetGraphicalOverrideCombinations
// ---------------------------------------------------------------------------

GetGraphicalOverrideCombinationsCommand::GetGraphicalOverrideCombinationsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String GetGraphicalOverrideCombinationsCommand::GetName () const
{
    return "GetGraphicalOverrideCombinations";
}

GS::Optional<GS::UniString> GetGraphicalOverrideCombinationsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "combinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "combinationId": { "type": "string" },
                        "name":          { "type": "string" },
                        "ruleIds":       { "type": "array", "items": { "type": "string" } }
                    },
                    "additionalProperties": false,
                    "required": [ "combinationId", "name", "ruleIds" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "combinations" ]
    })";
}

GS::ObjectState GetGraphicalOverrideCombinationsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> combinationList;
    ACAPI_GraphicalOverride_GetOverrideCombinationList (combinationList);

    GS::ObjectState response;
    const auto& adder = response.AddList<GS::ObjectState> ("combinations");

    for (const API_Guid& guid : combinationList) {
        API_OverrideCombination combination;
        combination.guid = guid;
        GS::Array<API_Guid> ruleList;
        if (ACAPI_GraphicalOverride_GetOverrideCombination (combination, &ruleList) != NoError)
            continue;

        GS::ObjectState os;
        os.Add ("combinationId", APIGuidToString (guid));
        os.Add ("name",          combination.name);
        const auto& ruleIdsAdder = os.AddList<GS::UniString> ("ruleIds");
        for (const API_Guid& ruleGuid : ruleList)
            ruleIdsAdder (APIGuidToString (ruleGuid));
        adder (os);
    }

    return response;
}

// ---------------------------------------------------------------------------
// GetGraphicalOverrideRuleGroups
// ---------------------------------------------------------------------------

GetGraphicalOverrideRuleGroupsCommand::GetGraphicalOverrideRuleGroupsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String GetGraphicalOverrideRuleGroupsCommand::GetName () const
{
    return "GetGraphicalOverrideRuleGroups";
}

GS::Optional<GS::UniString> GetGraphicalOverrideRuleGroupsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleGroups": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ruleGroupId": { "type": "string" },
                        "name":        { "type": "string" },
                        "ruleIds":     { "type": "array", "items": { "type": "string" } }
                    },
                    "additionalProperties": false,
                    "required": [ "ruleGroupId", "name", "ruleIds" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "ruleGroups" ]
    })";
}

GS::ObjectState GetGraphicalOverrideRuleGroupsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> ruleGroupList;
    ACAPI_GraphicalOverride_GetOverrideRuleGroupList (ruleGroupList);

    GS::ObjectState response;
    const auto& adder = response.AddList<GS::ObjectState> ("ruleGroups");

    for (const API_Guid& guid : ruleGroupList) {
        API_OverrideRuleGroup ruleGroup;
        ruleGroup.guid = guid;
        GS::Array<API_Guid> ruleList;
        if (ACAPI_GraphicalOverride_GetOverrideRuleGroup (ruleGroup, &ruleList) != NoError)
            continue;

        GS::ObjectState os;
        os.Add ("ruleGroupId", APIGuidToString (guid));
        os.Add ("name",        ruleGroup.name);
        const auto& ruleIdsAdder = os.AddList<GS::UniString> ("ruleIds");
        for (const API_Guid& ruleGuid : ruleList)
            ruleIdsAdder (APIGuidToString (ruleGuid));
        adder (os);
    }

    return response;
}

// ---------------------------------------------------------------------------
// GetGraphicalOverrideRules
// ---------------------------------------------------------------------------

GetGraphicalOverrideRulesCommand::GetGraphicalOverrideRulesCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String GetGraphicalOverrideRulesCommand::GetName () const
{
    return "GetGraphicalOverrideRules";
}

GS::Optional<GS::UniString> GetGraphicalOverrideRulesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "rules": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ruleId":       { "type": "string" },
                        "name":         { "type": "string" },
                        "criterionXML": { "type": "string", "description": "XML defining when this rule applies. Export a rule from ArchiCAD to obtain the format." },
                        "style":        { "type": "object" }
                    },
                    "additionalProperties": false,
                    "required": [ "ruleId", "name", "criterionXML", "style" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "rules" ]
    })";
}

GS::ObjectState GetGraphicalOverrideRulesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> ruleList;
    ACAPI_GraphicalOverride_GetOverrideRuleList (ruleList);

    GS::ObjectState response;
    const auto& adder = response.AddList<GS::ObjectState> ("rules");

    for (const API_Guid& guid : ruleList) {
        API_OverrideRule rule;
        rule.guid = guid;
        if (ACAPI_GraphicalOverride_GetOverrideRuleById (rule) != NoError)
            continue;

        GS::ObjectState os;
        os.Add ("ruleId",       APIGuidToString (guid));
        os.Add ("name",         rule.name);
        os.Add ("criterionXML", rule.criterionXML);
        os.Add ("style",        RuleStyleToOS (rule.style));
        adder (os);
    }

    return response;
}

// ---------------------------------------------------------------------------
// CreateGraphicalOverrideRuleGroups
// ---------------------------------------------------------------------------

CreateGraphicalOverrideRuleGroupsCommand::CreateGraphicalOverrideRuleGroupsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String CreateGraphicalOverrideRuleGroupsCommand::GetName () const
{
    return "CreateGraphicalOverrideRuleGroups";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRuleGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleGroupNames": {
                "type": "array",
                "description": "Names for the new rule groups.",
                "items": { "type": "string" }
            }
        },
        "additionalProperties": false,
        "required": [ "ruleGroupNames" ]
    })";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRuleGroupsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleGroups": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ruleGroupId": { "type": "string" },
                        "name":        { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": [ "ruleGroupId", "name" ]
                }
            },
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": [ "ruleGroups", "executionResults" ]
    })";
}

GS::ObjectState CreateGraphicalOverrideRuleGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> names;
    parameters.Get ("ruleGroupNames", names);

    GS::ObjectState response;
    const auto& groupsAdder  = response.AddList<GS::ObjectState> ("ruleGroups");
    const auto& resultsAdder = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("CreateGraphicalOverrideRuleGroups", [&] () -> GSErrCode {
        for (const GS::UniString& name : names) {
            API_OverrideRuleGroup ruleGroup;
            ruleGroup.name = name;
            const GSErrCode err = ACAPI_GraphicalOverride_CreateOverrideRuleGroup (ruleGroup);
            if (err != NoError) {
                resultsAdder (CreateFailedExecutionResult (err, "Failed to create rule group '" + name + "'."));
                GS::ObjectState emptyGroup; emptyGroup.Add ("ruleGroupId", GS::UniString ()); emptyGroup.Add ("name", GS::UniString ()); groupsAdder (emptyGroup);
            } else {
                resultsAdder (CreateSuccessfulExecutionResult ());
                GS::ObjectState os;
                os.Add ("ruleGroupId", APIGuidToString (ruleGroup.guid));
                os.Add ("name",        ruleGroup.name);
                groupsAdder (os);
            }
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// CreateGraphicalOverrideRules
// ---------------------------------------------------------------------------

CreateGraphicalOverrideRulesCommand::CreateGraphicalOverrideRulesCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String CreateGraphicalOverrideRulesCommand::GetName () const
{
    return "CreateGraphicalOverrideRules";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRulesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "rules": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name":         { "type": "string" },
                        "ruleGroupId":  { "type": "string", "description": "GUID of the rule group this rule belongs to." },
                        "criterionXML": { "type": "string", "description": "XML defining when this rule applies. Use GetGraphicalOverrideRules to obtain example XML." },
                        "style":        { "type": "object", "description": "The graphical override style. Use GetGraphicalOverrideRules to obtain the structure." }
                    },
                    "additionalProperties": false,
                    "required": [ "name", "ruleGroupId", "criterionXML", "style" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "rules" ]
    })";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRulesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "rules": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ruleId": { "type": "string" },
                        "name":   { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": [ "ruleId", "name" ]
                }
            },
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": [ "rules", "executionResults" ]
    })";
}

GS::ObjectState CreateGraphicalOverrideRulesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> ruleInputs;
    parameters.Get ("rules", ruleInputs);

    GS::ObjectState response;
    const auto& rulesAdder   = response.AddList<GS::ObjectState> ("rules");
    const auto& resultsAdder = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("CreateGraphicalOverrideRules", [&] () -> GSErrCode {
        for (const GS::ObjectState& ruleInput : ruleInputs) {
            GS::UniString name, ruleGroupIdStr, criterionXML;
            GS::ObjectState styleOS;

            if (!ruleInput.Get ("name", name) || !ruleInput.Get ("ruleGroupId", ruleGroupIdStr) ||
                !ruleInput.Get ("criterionXML", criterionXML) || !ruleInput.Get ("style", styleOS)) {
                resultsAdder (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required field."));
                GS::ObjectState emptyRule; emptyRule.Add ("ruleId", GS::UniString ()); emptyRule.Add ("name", GS::UniString ()); rulesAdder (emptyRule);
                continue;
            }

            const API_Guid ruleGroupGuid = APIGuidFromString (ruleGroupIdStr.ToCStr ());

            API_OverrideRule rule;
            rule.name         = name;
            rule.criterionXML = criterionXML;
            rule.style        = RuleStyleFromOS (styleOS);

            const GSErrCode err = ACAPI_GraphicalOverride_CreateOverrideRule (rule, ruleGroupGuid);
            if (err != NoError) {
                resultsAdder (CreateFailedExecutionResult (err, "Failed to create rule '" + name + "'."));
                GS::ObjectState emptyRule; emptyRule.Add ("ruleId", GS::UniString ()); emptyRule.Add ("name", GS::UniString ()); rulesAdder (emptyRule);
            } else {
                resultsAdder (CreateSuccessfulExecutionResult ());
                GS::ObjectState os;
                os.Add ("ruleId", APIGuidToString (rule.guid));
                os.Add ("name",   rule.name);
                rulesAdder (os);
            }
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// CreateGraphicalOverrideCombinations
// ---------------------------------------------------------------------------

CreateGraphicalOverrideCombinationsCommand::CreateGraphicalOverrideCombinationsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String CreateGraphicalOverrideCombinationsCommand::GetName () const
{
    return "CreateGraphicalOverrideCombinations";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "combinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name":    { "type": "string" },
                        "ruleIds": { "type": "array", "items": { "type": "string" }, "description": "GUIDs of the rules to include in this combination." }
                    },
                    "additionalProperties": false,
                    "required": [ "name", "ruleIds" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "combinations" ]
    })";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideCombinationsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "combinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "combinationId": { "type": "string" },
                        "name":          { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": [ "combinationId", "name" ]
                }
            },
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": [ "combinations", "executionResults" ]
    })";
}

GS::ObjectState CreateGraphicalOverrideCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> combinationInputs;
    parameters.Get ("combinations", combinationInputs);

    GS::ObjectState response;
    const auto& combinationsAdder = response.AddList<GS::ObjectState> ("combinations");
    const auto& resultsAdder      = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("CreateGraphicalOverrideCombinations", [&] () -> GSErrCode {
        for (const GS::ObjectState& input : combinationInputs) {
            GS::UniString name;
            GS::Array<GS::UniString> ruleIdStrs;

            if (!input.Get ("name", name) || !input.Get ("ruleIds", ruleIdStrs)) {
                resultsAdder (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required field."));
                GS::ObjectState emptyCombo; emptyCombo.Add ("combinationId", GS::UniString ()); emptyCombo.Add ("name", GS::UniString ()); combinationsAdder (emptyCombo);
                continue;
            }

            GS::Array<API_Guid> ruleIds;
            for (const GS::UniString& idStr : ruleIdStrs)
                ruleIds.Push (APIGuidFromString (idStr.ToCStr ()));

            API_OverrideCombination combination;
            combination.name = name;

            const GSErrCode err = ACAPI_GraphicalOverride_CreateOverrideCombination (combination, ruleIds);
            if (err != NoError) {
                resultsAdder (CreateFailedExecutionResult (err, "Failed to create combination '" + name + "'."));
                GS::ObjectState emptyCombo; emptyCombo.Add ("combinationId", GS::UniString ()); emptyCombo.Add ("name", GS::UniString ()); combinationsAdder (emptyCombo);
            } else {
                resultsAdder (CreateSuccessfulExecutionResult ());
                GS::ObjectState os;
                os.Add ("combinationId", APIGuidToString (combination.guid));
                os.Add ("name",          combination.name);
                combinationsAdder (os);
            }
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// DeleteGraphicalOverrideRules
// ---------------------------------------------------------------------------

DeleteGraphicalOverrideRulesCommand::DeleteGraphicalOverrideRulesCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String DeleteGraphicalOverrideRulesCommand::GetName () const
{
    return "DeleteGraphicalOverrideRules";
}

GS::Optional<GS::UniString> DeleteGraphicalOverrideRulesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleIds": { "type": "array", "items": { "type": "string" }, "description": "GUIDs of the rules to delete." }
        },
        "additionalProperties": false,
        "required": [ "ruleIds" ]
    })";
}

GS::Optional<GS::UniString> DeleteGraphicalOverrideRulesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    })";
}

GS::ObjectState DeleteGraphicalOverrideRulesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> ruleIdStrs;
    parameters.Get ("ruleIds", ruleIdStrs);

    GS::ObjectState response;
    const auto& resultsAdder = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteGraphicalOverrideRules", [&] () -> GSErrCode {
        for (const GS::UniString& idStr : ruleIdStrs) {
            const API_Guid guid = APIGuidFromString (idStr.ToCStr ());
            const GSErrCode err = ACAPI_GraphicalOverride_DeleteOverrideRule (guid);
            if (err != NoError)
                resultsAdder (CreateFailedExecutionResult (err, "Failed to delete rule " + idStr + "."));
            else
                resultsAdder (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// DeleteGraphicalOverrideRuleGroups
// ---------------------------------------------------------------------------

DeleteGraphicalOverrideRuleGroupsCommand::DeleteGraphicalOverrideRuleGroupsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String DeleteGraphicalOverrideRuleGroupsCommand::GetName () const
{
    return "DeleteGraphicalOverrideRuleGroups";
}

GS::Optional<GS::UniString> DeleteGraphicalOverrideRuleGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ruleGroupIds": { "type": "array", "items": { "type": "string" }, "description": "GUIDs of the rule groups to delete (also deletes all rules they contain)." }
        },
        "additionalProperties": false,
        "required": [ "ruleGroupIds" ]
    })";
}

GS::Optional<GS::UniString> DeleteGraphicalOverrideRuleGroupsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    })";
}

GS::ObjectState DeleteGraphicalOverrideRuleGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> groupIdStrs;
    parameters.Get ("ruleGroupIds", groupIdStrs);

    GS::ObjectState response;
    const auto& resultsAdder = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteGraphicalOverrideRuleGroups", [&] () -> GSErrCode {
        for (const GS::UniString& idStr : groupIdStrs) {
            const API_Guid guid = APIGuidFromString (idStr.ToCStr ());
            const GSErrCode err = ACAPI_GraphicalOverride_DeleteOverrideRuleGroup (guid);
            if (err != NoError)
                resultsAdder (CreateFailedExecutionResult (err, "Failed to delete rule group " + idStr + "."));
            else
                resultsAdder (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// DeleteGraphicalOverrideCombinations
// ---------------------------------------------------------------------------

DeleteGraphicalOverrideCombinationsCommand::DeleteGraphicalOverrideCombinationsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String DeleteGraphicalOverrideCombinationsCommand::GetName () const
{
    return "DeleteGraphicalOverrideCombinations";
}

GS::Optional<GS::UniString> DeleteGraphicalOverrideCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "combinationIds": { "type": "array", "items": { "type": "string" }, "description": "GUIDs of the combinations to delete." }
        },
        "additionalProperties": false,
        "required": [ "combinationIds" ]
    })";
}

GS::Optional<GS::UniString> DeleteGraphicalOverrideCombinationsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    })";
}

GS::ObjectState DeleteGraphicalOverrideCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> combinationIdStrs;
    parameters.Get ("combinationIds", combinationIdStrs);

    GS::ObjectState response;
    const auto& resultsAdder = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteGraphicalOverrideCombinations", [&] () -> GSErrCode {
        for (const GS::UniString& idStr : combinationIdStrs) {
            const API_Guid guid = APIGuidFromString (idStr.ToCStr ());
            const GSErrCode err = ACAPI_GraphicalOverride_DeleteOverrideCombination (guid);
            if (err != NoError)
                resultsAdder (CreateFailedExecutionResult (err, "Failed to delete combination " + idStr + "."));
            else
                resultsAdder (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}
