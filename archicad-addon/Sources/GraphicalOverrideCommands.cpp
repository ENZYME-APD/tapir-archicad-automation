#include "GraphicalOverrideCommands.hpp"

#include "MigrationHelper.hpp"

// The ACAPI_GraphicalOverride_* functions (and the API_OverrideRule/RuleGroup/Combination/
// OverrideRuleStyle types they operate on) were introduced in Archicad 27 - they don't exist at
// all pre-2700, confirmed by their absence from the AC25 DevKit headers. Gate the whole file so
// AC25/26 builds simply don't register these commands, rather than failing to compile.
#ifdef ServerMainVers_2700

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

// attributeId is a guid (via GetAttributeGuidFromIndex/GetAttributeIndexFromGuid), not a raw
// index, and the flag is named "overridden" not "isOverridden" - matching the convention already
// established by CreateOverriddenMaterialObjectState/CreateOverriddenPenObjectState in
// CommandBase.cpp for every other overridable-attribute field in Tapir's public API. Those
// existing helpers can't be reused directly: they hardcode API_MaterialID, whereas a rule style's
// overrides span three different attribute kinds (line type, fill, material), so the type is a
// parameter here instead.
static GS::ObjectState OverriddenAttributeToOS (const API_OverriddenAttribute& v, API_AttrTypeID attrType)
{
    GS::ObjectState os;
    os.Add ("overridden", v.hasValue);
    if (v.hasValue)
        os.Add ("attributeId", CreateGuidObjectState (GetAttributeGuidFromIndex (attrType, v.value)));
    return os;
}

static GS::ObjectState OverriddenPenToOS (const API_OverriddenPen& v)
{
    GS::ObjectState os;
    os.Add ("overridden", v.hasValue);
    if (v.hasValue)
        os.Add ("penIndex", (int) v.value);
    return os;
}

static GS::ObjectState OverriddenPenOrRGBToOS (const API_OverriddenPenOrRGB& v)
{
    GS::ObjectState os;
    os.Add ("overridden", v.hasValue);
    if (v.hasValue) {
        if (v.value.IsType<API_PenIndex> ())
            os.Add ("penIndex", (int) v.value.Get<API_PenIndex> ());
        else
            os.Add ("rgbColor", RGBColorToOS (v.value.Get<API_RGBColor> ()));
    }
    return os;
}

static GS::ObjectState OverriddenAttributeOrRGBToOS (const API_OverriddenAttributeOrRGB& v, API_AttrTypeID attrType)
{
    GS::ObjectState os;
    os.Add ("overridden", v.hasValue);
    if (v.hasValue) {
        if (v.value.IsType<API_AttributeIndex> ())
            os.Add ("attributeId", CreateGuidObjectState (GetAttributeGuidFromIndex (attrType, v.value.Get<API_AttributeIndex> ())));
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
    os.Add ("lineType",                     OverriddenAttributeToOS (s.lineType, API_LinetypeID));
    os.Add ("lineMarkerTextPen",            OverriddenPenToOS (s.lineMarkerTextPen));
    os.Add ("fillOverride",                 OverriddenAttributeToOS (s.fillOverride, API_FilltypeID));
    os.Add ("fillType",                     FillTypeToOS (s.fillType));
    os.Add ("fillForegroundPenOverride",    OverriddenPenToOS (s.fillForegroundPenOverride));
    os.Add ("fillTypeForegroundPen",        FillTypeToOS (s.fillTypeForegroundPen));
    os.Add ("fillBackgroundPenOverride",    OverriddenPenOrRGBToOS (s.fillBackgroundPenOverride));
    os.Add ("fillTypeBackgroundPen",        FillTypeToOS (s.fillTypeBackgroundPen));
    os.Add ("surfaceOverride",              OverriddenAttributeOrRGBToOS (s.surfaceOverride, API_MaterialID));
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

static API_OverriddenAttribute OverriddenAttributeFromOS (const GS::ObjectState& os, API_AttrTypeID attrType)
{
    API_OverriddenAttribute v;
    v.hasValue = false;
    bool overridden = false;
    os.Get ("overridden", overridden);
    if (overridden) {
        GS::ObjectState attributeIdOs;
        if (os.Get ("attributeId", attributeIdOs)) {
            v = GetAttributeIndexFromGuid (attrType, GetGuidFromObjectState (attributeIdOs));
        } else {
            v = APINullValue;
        }
    } else {
        v = APINullValue;
    }
    return v;
}

static API_OverriddenPen OverriddenPenFromOS (const GS::ObjectState& os)
{
    API_OverriddenPen v;
    v.hasValue = false;
    bool overridden = false;
    os.Get ("overridden", overridden);
    if (overridden) {
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
    bool overridden = false;
    os.Get ("overridden", overridden);
    if (overridden) {
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

static API_OverriddenAttributeOrRGB OverriddenAttributeOrRGBFromOS (const GS::ObjectState& os, API_AttrTypeID attrType)
{
    API_OverriddenAttributeOrRGB v;
    v.hasValue = false;
    bool overridden = false;
    os.Get ("overridden", overridden);
    if (overridden) {
        GS::ObjectState attributeIdOs, rgbOS;
        if (os.Get ("attributeId", attributeIdOs)) {
            v = GetAttributeIndexFromGuid (attrType, GetGuidFromObjectState (attributeIdOs));
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

    if (os.Get ("lineType", fieldOS))                  s.lineType                    = OverriddenAttributeFromOS (fieldOS, API_LinetypeID);
    if (os.Get ("lineMarkerTextPen", fieldOS))         s.lineMarkerTextPen           = OverriddenPenFromOS (fieldOS);
    if (os.Get ("fillOverride", fieldOS))              s.fillOverride                = OverriddenAttributeFromOS (fieldOS, API_FilltypeID);
    if (os.Get ("fillType", fieldOS))                  s.fillType                    = FillTypeFromOS (fieldOS);
    if (os.Get ("fillForegroundPenOverride", fieldOS)) s.fillForegroundPenOverride   = OverriddenPenFromOS (fieldOS);
    if (os.Get ("fillTypeForegroundPen", fieldOS))     s.fillTypeForegroundPen       = FillTypeFromOS (fieldOS);
    if (os.Get ("fillBackgroundPenOverride", fieldOS)) s.fillBackgroundPenOverride   = OverriddenPenOrRGBFromOS (fieldOS);
    if (os.Get ("fillTypeBackgroundPen", fieldOS))     s.fillTypeBackgroundPen       = FillTypeFromOS (fieldOS);
    if (os.Get ("surfaceOverride", fieldOS))           s.surfaceOverride             = OverriddenAttributeOrRGBFromOS (fieldOS, API_MaterialID);
    if (os.Get ("surfaceType", fieldOS))               s.surfaceType                 = SurfaceTypeFromOS (fieldOS);
    os.Get ("showSkinSeparators",           s.showSkinSeparators);
    os.Get ("overridePenColorAndThickness", s.overridePenColorAndThickness);
    if (os.Get ("hiddenContours", fieldOS))            s.hiddenContours              = SurfaceTypeFromOS (fieldOS);
    os.Get ("overrideContours", s.overrideContours);
    return s;
}

// ---------------------------------------------------------------------------
// Structured criterion -> criterionXML generator
//
// Reverse-engineered from real criterionXML exported by ArchiCAD (there is no official
// documentation of this format beyond "save the rule as XML in ArchiCAD and look at it" - see
// API_OverrideRule::criterionXML's doc comment). ClassGuids and the element type table below were
// captured live: ClassGuids from ArchiCAD's own DevKit sample (Override_Test.cpp) and from rules
// exported by the running project; the element type table (ElemRegistryRefId / tool UnID guids) by
// creating one single-criterion rule per tool in ArchiCAD's own criteria editor and reading back
// its generated criterionXML through GetGraphicalOverrideRules.
//
// Structure: <CriterionExpression> always wraps exactly one root CompositeCriterion (even for a
// single bare condition), and every leaf condition (ElemTypeCriterion, PropertyCriterion, ...) is
// itself always wrapped in its own trivial single-child CompositeCriterion before being placed as
// a composite's child - confirmed identical in every real example captured this session. Also
// confirmed (from the DevKit's own "Window or Door" sample, where the two conditions are known to
// be OR-combined): LogicalOperator 1 = OR, 2 = AND - the reverse of what the values might suggest.
// ---------------------------------------------------------------------------

static const char* kCompositeClassGuid   = "C6EBD1BD-7702-46FF-8ED9-9CC37648A7C7";
static const char* kElemTypeClassGuid    = "B4B7B134-EC56-4D40-8D4C-71D7C5A2493A";
static const char* kPropertyClassGuid    = "58E4905F-AD57-45F5-8D26-0100000F60BF";
static const char* kModelElemClassGuid   = "2C7FF58A-DE0F-4B5A-82EE-EA6A943E1951";
static const char* kDrawingElemClassGuid = "FD699EDF-F0A1-4728-87B8-092C09E4DE71";

struct ElemTypeInfo { const char* name; const char* refId; const char* mainGuid; const char* revGuid; };

// Only element types created with a fixed, non-library-part tool are listed here: their
// ElemCreatorToolUnID guids are all-zero (no GDL library part backs them), so ElemRegistryRefId
// alone identifies the type reliably. Library-part-based tools (Door, Window, Object, Lamp, Zone,
// Label, Grid, and most MEP part types) were confirmed THIS session to use a real MainGuid/RevGuid
// pointing at Archicad's built-in library part - and that guid was confirmed to differ between the
// DevKit's own sample (an older Archicad build) and this live project for the exact same tool
// (Door), meaning it is not a portable constant. Hardcoding it here would only work on the Archicad
// version/build it was captured from, so those types are intentionally left unsupported for now.
static const ElemTypeInfo kElemTypeTable[] = {
    { "Wall",              "1463897138", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Column",            "1129270357", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Beam",              "1111834957", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Slab",              "1128614220", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Roof",              "1314017094", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Shell",              "1397245260", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Stair",              "1398030665", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Railing",            "1380010316", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CurtainWall",        "1129791820", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Opening",            "1330661703", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Mesh",               "1296388936", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Morph",              "1179800392", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Dimension",          "1145654623", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "LevelDimension",     "1279543629", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "RadialDimension",    "1380206925", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "AngleDimension",     "1094994253", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Text",               "1464816196", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "ChangeMarker",       "1128810317", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Hatch",              "1212240963", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Line",               "1279872581", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Arc",                "1095910239", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PolyLine",           "1347176782", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Spline",             "1397771342", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Hotspot",            "1213158495", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Section",            "1397048148", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Elevation",          "1162626390", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "InteriorElevation",  "1229735254", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Detail",             "1145394241", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Worksheet",          "1146242644", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Camera",             "1128353093", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Figure",             "1347371594", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "Drawing",            "1146241367", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "StructuralLink",     "1095584841", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "StructuralSupport",  "1095586645", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "LinearLoad",         "1095583052", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PointLoad",          "1095585868", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "SurfaceLoad",        "1095586636", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "DuctRouting",        "1296323141", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "CableCarrierRouting","1296257605", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
    { "PipeRouting",        "1297109573", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000" },
};

static const ElemTypeInfo* FindElemTypeInfo (const GS::UniString& name)
{
    for (const ElemTypeInfo& info : kElemTypeTable) {
        if (name == info.name)
            return &info;
    }
    return nullptr;
}

static GS::UniString ElemTypeCriterionXML (const ElemTypeInfo& info)
{
    return GS::UniString ("<ClassGuid>") + kElemTypeClassGuid + "</ClassGuid>"
         + "<ElemTypeCriterion Mv=\"2\" Sv=\"3\">"
         +     "<VBEF::CritToolType Mv=\"1\" Sv=\"0\">"
         +         "<ElemRegistryRefId>" + info.refId + "</ElemRegistryRefId>"
         +         "<ElemCreatorToolUnID>"
         +             "<MainGuid>" + info.mainGuid + "</MainGuid>"
         +             "<RevGuid>"  + info.revGuid  + "</RevGuid>"
         +         "</ElemCreatorToolUnID>"
         +     "</VBEF::CritToolType>"
         +     "<LogicalOperator>1</LogicalOperator>"
         + "</ElemTypeCriterion>";
}

// CriteriaOperatorEnum values were confirmed live by creating one rule per operator in ArchiCAD's
// own criteria editor and reading back which value each produced: 1 = "is not" (plain), 10 = "has
// value", 11 = "has no value", 12 = "is" (confirmed identical to "is in the branch of" - ArchiCAD
// treats them as the same operator for a hierarchical Classification property), 13 = "is the
// direct child of", 14 = "is not in the branch of", 15 = "is not the direct child of". 12/"is" is
// what the built-in "Classification - X" system rules use; 14 (not just 11 - "has no value" is a
// different, unrelated operator) is its natural negation, matching an item and all its descendants
// resp. excluding all of them.
//
// The classification SYSTEM name is by-name (HasNameId, like every custom property), not a fixed
// guid - and, confirmed live cross-project, is NOT a universal constant: a project can contain
// several classification systems (Archicad's own default, Uniclass, Omniclass, project-specific
// ones, ...), and even Archicad's own default system's exact label can differ between projects/
// versions (seen "Classification Archicad - 2.0" in one project, "Classification Archicad - v 2.0"
// in another) - a mismatched name here silently discards the whole criterion, just like a
// mismatched custom property/group name does. kDefaultClassificationSystemName is only a fallback
// for the common case; callers targeting a different system must pass classificationSystem
// explicitly.
static const char* kDefaultClassificationSystemName = "Classification Archicad - 2.0";

static GS::UniString ClassificationCriterionXML (const GS::UniString& itemId, bool isNot, const GS::UniString& systemName)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\">"
         +     "<VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         +         "<PropertyDefinitionUserId Version=\"2\">"
         +             "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +             "<Guid>00000000-0000-0000-0000-000000000000</Guid>"
         +             "<HasNameId>true</HasNameId><Name>" + systemName + "</Name>"
         +             "<PropertyDefinitionGroupUserId Version=\"2\">"
         +                 "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +                 "<Guid>00000000-0000-0000-0000-000000000000</Guid>"
         +                 "<HasNameId>true</HasNameId><Name>ClassificationSystemPropertyDefinitionGroup</Name>"
         +             "</PropertyDefinitionGroupUserId>"
         +         "</PropertyDefinitionUserId>"
         +         GS::UniString (isNot ? "<CriteriaOperatorEnum>14</CriteriaOperatorEnum>" : "<CriteriaOperatorEnum>12</CriteriaOperatorEnum>")
         +         "<ClassificationValue><ClassificationItemUserID>" + itemId + "</ClassificationItemUserID></ClassificationValue>"
         +     "</VBEF::ConditionIO>"
         + "</PropertyCriterion>";
}

// Wraps a leaf's own <ClassGuid>/<Tag> pair in the trivial single-child CompositeCriterion every
// leaf is always nested in, whether it sits at the root or inside an explicit and/or group.
static GS::UniString WrapAsTrivialGroup (const GS::UniString& leafXML)
{
    // LogicalOperator is logically irrelevant here (CriteriaCount=1), but every real single-item
    // wrap captured this session used 1 (OR) - and the criteria editor was confirmed to silently
    // fail to render a classification row as such when 2 was used instead, despite ArchiCAD
    // accepting the rule without error. Match the real value exactly rather than relying on it
    // being a don't-care.
    return GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
         + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>1</LogicalOperator><CriteriaCount>1</CriteriaCount>"
         + leafXML
         + "</CompositeCriterion>";
}

// "Model View" (Vue modele) system properties - reverse-engineered from a rule containing every
// operator of every field in that criteria category, created live in ArchiCAD and read back
// through GetGraphicalOverrideRules. Unlike Classification, these are identified purely by GUID
// (HasNameId=false) and share one fixed PropertyDefinitionGroupUserId guid. Confirmed generic
// operator values across property kinds: 0=is, 1=is not, 6=contains, 7=does not contain, 8=starts
// with, 9=ends with (string only). "Layer" and "Layer combination" are attribute-index-valued (the
// UI shows the attribute's name, but the XML stores its index), so their generator functions
// resolve a name to an index via ACAPI_Attribute_GetAttributesByType at generation time.
static const char* kModelViewPropertyGroupGuid = "517BE4C1-0FAF-4CA2-9C89-B631C2DEAFAE";
static const char* kMissingAttributesPropertyGuid = "45D43C12-49F3-4818-A56B-C033DEA90C8B";
static const char* kLayerPropertyGuid            = "0F43553E-58E7-4661-8CEB-8B4E5D2ED50A";
static const char* kLayerLockedPropertyGuid      = "194656B7-E624-466F-A527-50804AB99E8D";
static const char* kLayerVisiblePropertyGuid     = "708E442F-1EDF-4E5D-84F9-F97C66D68FE7";
static const char* kLayerCombinationPropertyGuid = "C0FFBC04-E003-4357-A1E4-A6A092BFCF86";
static const char* kLayerNamePropertyGuid        = "812CD483-40FB-4E75-966F-E76C3E68B9F7";

static bool FindAttributeIndexByName (API_AttrTypeID typeID, const GS::UniString& name, Int32& outIndex)
{
    GS::Array<API_Attribute> attrs;
    ACAPI_Attribute_GetAttributesByType (typeID, attrs);
    for (API_Attribute& attr : attrs) {
        if (GS::UniString (attr.header.name) == name) {
            outIndex = GetAttributeIndex (attr.header.index);
            return true;
        }
    }
    return false;
}

// Fonts are not part of the generic Attribute system (no API_AttrTypeID for them) - looked up via
// the dedicated ACAPI_Font_SearchFont instead. Confirmed live: unlike every attribute-based lookup
// above, this does NOT fail for an unrecognized name - ArchiCAD's font table grows on demand (fonts
// are OS-level resources referenced by name, not a fixed project attribute list), so searching for
// a typo'd font name silently registers a new font entry and returns success rather than an error.
static bool FindFontIndexByName (const GS::UniString& name, Int32& outIndex)
{
    API_FontType font = {};
    font.head.index = 0;
    GS::UniString mutableName = name;
    font.head.uniStringNamePtr = &mutableName;
    if (ACAPI_Font_SearchFont (font) != NoError)
        return false;
    outIndex = font.head.index;
    return true;
}

// "Construction" criteria category system properties - same reverse-engineering method as "Model
// View" (a rule exercising every field/operator in the category, created live and read back).
static const char* kConstructionPropertyGroupGuid   = "AFBE1CD9-B14D-4AB8-A16F-99686F399F6C";
static const char* kCompositeStructurePropertyGuid  = "17743909-E801-4C87-A1B8-D4168A10D115";
static const char* kCompositeStructureNamePropertyGuid = "704E9212-3E21-4790-BAE4-CF3DE2395481";
static const char* kComplexProfileNamePropertyGuid  = "75F1B979-F6C1-40ED-A478-E950CB729E81";
static const char* kComplexProfilePropertyGuid      = "78781057-3451-4282-8799-558D15BCADAA";
static const char* kRoofConnectedPropertyGuid       = "CC564BEC-6D0B-476E-9D1F-762AFCCA3627";
static const char* kStructureTypePropertyGuid       = "98A26D3B-3BAF-4019-BE7A-09285FFA597C";

static GS::UniString PropertyHeaderXML (const char* propertyGuid, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>0</PrimaryId><HasGuidId>true</HasGuidId>"
         +     "<Guid>" + propertyGuid + "</Guid><HasNameId>false</HasNameId><Name/>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>0</PrimaryId><HasGuidId>true</HasGuidId>"
         +         "<Guid>" + groupGuid + "</Guid><HasNameId>false</HasNameId><Name/>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

// Looks up a custom property by its (name, group) pair and, if it is backed by an option list (a
// "jeu d'options" - API_PropertySingleChoiceEnumerationCollectionType), returns one of its real
// option values. Needed only for the meta-operators (see TryGetMetaOperator): ArchiCAD validates a
// customPropertyString meta-operator's placeholder value against the property's own valid value set
// when it's option-list-backed - confirmed live that ANY placeholder that isn't a real option value
// (including an empty string, or any other string not in the list) causes rule creation to silently
// succeed while ArchiCAD discards the whole criterion internally, with no error reported. A
// genuinely free-text (non-option-list) custom string property has no such constraint.
static bool FindFirstOptionValueForCustomProperty (const GS::UniString& propertyName, const GS::UniString& groupName, GS::UniString& outValue)
{
    GS::Array<API_PropertyGroup> groups;
    if (ACAPI_Property_GetPropertyGroups (groups) != NoError)
        return false;

    for (const API_PropertyGroup& group : groups) {
        if (group.name != groupName)
            continue;
        GS::Array<API_PropertyDefinition> definitions;
        if (ACAPI_Property_GetPropertyDefinitions (group.guid, definitions) != NoError)
            continue;
        for (const API_PropertyDefinition& definition : definitions) {
            if (definition.name != propertyName)
                continue;
            if (definition.collectionType != API_PropertySingleChoiceEnumerationCollectionType || definition.possibleEnumValues.IsEmpty ())
                return false;
            outValue = definition.possibleEnumValues[0].displayVariant.uniStringValue;
            return true;
        }
    }
    return false;
}

// Custom (user-defined) properties, identified purely by name + group name (both HasNameId=true,
// no guid) - reverse-engineered from a "TAPIR8" rule. Confirmed live: EVERY custom property type,
// regardless of its own value type, additionally supports 6 shared "availability" meta-operators on
// top of its normal value-comparison operators (16=has default value, 17=has custom value,
// 18=available, 19=not available, 20=is undefined, 21=is not undefined) - counted live as exactly
// 7 total operators for a boolean property (1 value-op + 6 meta), 8 for a string/option-set
// property (2 value-ops + 6 meta), 12 for a numeric property (6 value-ops + 6 meta).
static GS::UniString CustomPropertyHeaderXML (const GS::UniString& propertyName, const GS::UniString& groupName)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +     "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +     "<Name>" + propertyName + "</Name>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>" + groupName + "</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

// The 6 shared availability meta-operators (see CustomPropertyHeaderXML's doc). valueVariantXML is
// a placeholder <Variant> matching the property's own type - confirmed live that ArchiCAD still
// requires a syntactically valid (if practically ignored) value here even for these meta-operators.
static bool TryGetMetaOperator (const GS::ObjectState& node, int& outOp)
{
    bool flag = false;
    if (node.Get ("hasDefaultValue", flag) && flag)   { outOp = 16; return true; }
    if (node.Get ("hasCustomValue", flag) && flag)     { outOp = 17; return true; }
    if (node.Get ("isAvailable", flag) && flag)        { outOp = 18; return true; }
    if (node.Get ("isNotAvailable", flag) && flag)     { outOp = 19; return true; }
    if (node.Get ("isUndefined", flag) && flag)        { outOp = 20; return true; }
    if (node.Get ("isNotUndefined", flag) && flag)     { outOp = 21; return true; }
    return false;
}

// bareVariant: confirmed live that a custom String/Option-Set property omits the <Value> wrapper
// entirely (for every operator, including the meta-operators) - unlike Bool/Number custom
// properties, which do wrap. Yet another per-property-type quirk, not inferable from anything else.
static GS::UniString CustomPropertyCriterionXML (const GS::UniString& headerXML, int op, const GS::UniString& valueVariantXML, bool bareVariant = false)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + headerXML
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", op) + "</CriteriaOperatorEnum>"
         + (bareVariant ? valueVariantXML : (GS::UniString ("<Value>") + valueVariantXML + "</Value>"))
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString BoolVariantXML (bool value)
{
    return GS::UniString ("<Variant Type=\"BoolVariant\"><Status>Normal</Status><Value>") + (value ? "true" : "false") + "</Value></Variant>";
}

static GS::UniString StringVariantXML (const GS::UniString& value)
{
    return GS::UniString ("<Variant Type=\"StringVariant\"><Status>Normal</Status><Value>") + value + "</Value></Variant>";
}

static GS::UniString NumVariantXML (double value)
{
    return GS::UniString ("<Variant Type=\"NumVariant\"><Status>Normal</Status><Value>") + GS::UniString::Printf ("%g", value) + "</Value></Variant>";
}

// Custom properties of Integer valueType (e.g. "Occupants") use a distinct IntVariant, not
// NumVariant - confirmed against a real "TAPIR10" export. Length/Volume/Angle/Area custom
// properties are all just Real valueType with a different display unit and already use NumVariant
// like plain numbers, so they need no separate handling.
static GS::UniString IntVariantXML (int value)
{
    return GS::UniString ("<Variant Type=\"IntVariant\"><Status>Normal</Status><Value>") + GS::UniString::Printf ("%d", value) + "</Value></Variant>";
}

// "Modificateurs de profil" (Profile Modifiers) criteria category, reverse-engineered from a
// "TAPIR7" rule - GEOMETRIE tab. Every field here is numeric with all 6 comparison operators
// (=,!=,<,>,<=,>=). Unlike every other property criterion this session, the value sits inside an
// extra <ProfileParameterValue> wrapper around the usual <Value><Variant>. The 6 built-in
// parameters (Etirement hauteur/largeur, Hauteur nominale/totale, Largeur totale/nominale) are
// identified by a fixed GUID; any other profile modifier (a parameter specific to one Complex
// Profile attribute in the project, e.g. "Debord tole") is identified by its plain name instead -
// both forms share the same by-name "ProfileParameterPropertyDefinitionGroup" group.
static const char* kProfileParameterGroupName = "ProfileParameterPropertyDefinitionGroup";
static const char* kProfileStretchHeightGuid = "B9FFB90F-9C1A-4BC4-8A4E-5EE02A800513";
static const char* kProfileStretchWidthGuid  = "B6FB6EFE-ED97-4F4D-814B-D9FEC39E63FC";
static const char* kProfileNominalHeightGuid = "F7028EBA-7D6D-4475-A9B3-61470DF7FDDD";
static const char* kProfileTotalHeightGuid   = "D1A91234-F6F1-45D9-966F-407A686062FB";
static const char* kProfileTotalWidthGuid    = "6D5B0B90-2266-40B6-8CDE-B2638101F78D";
static const char* kProfileNominalWidthGuid  = "98E721E3-372F-4D9E-85EC-2C763372CEBD";

static GS::UniString ProfileParameterHeaderXML (const char* propertyGuid)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>0</PrimaryId><HasGuidId>true</HasGuidId>"
         +     "<Guid>" + propertyGuid + "</Guid><HasNameId>false</HasNameId><Name/>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>" + kProfileParameterGroupName + "</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

static GS::UniString ProfileParameterHeaderByNameXML (const GS::UniString& propertyName)
{
    return GS::UniString ("<PropertyDefinitionUserId Version=\"2\">")
         +     "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +     "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +     "<Name>" + propertyName + "</Name>"
         +     "<PropertyDefinitionGroupUserId Version=\"2\">"
         +         "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +         "<Guid>00000000-0000-0000-0000-000000000000</Guid><HasNameId>true</HasNameId>"
         +         "<Name>" + kProfileParameterGroupName + "</Name>"
         +     "</PropertyDefinitionGroupUserId>"
         + "</PropertyDefinitionUserId>";
}

// numOp: 0==, 1=not-equal, 2=less-than, 3=greater-than, 4=less-or-equal, 5=greater-or-equal.
static GS::UniString ProfileParameterCriterionXML (const GS::UniString& headerXML, double value, int numOp)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + headerXML
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<ProfileParameterValue><Value><Variant Type=\"NumVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%g", value) + "</Value></Variant></Value></ProfileParameterValue>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// Tries each of the 6 numeric-comparison field name suffixes for a fixed-guid profile parameter.
static bool TryEmitProfileParamField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (ProfileParameterCriterionXML (ProfileParameterHeaderXML (propertyGuid), value, kOps[i]));
            return true;
        }
    }
    return false;
}

static GS::UniString BoolModelViewCriterionXML (const char* propertyGuid, bool value, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>0</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"BoolVariant\"><Status>Normal</Status><Value>" + GS::UniString (value ? "true" : "false") + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString AttrIndexEqualsModelViewCriterionXML (const char* propertyGuid, Int32 attrIndex, bool isNot, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "1" : "0") + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"Property::AttributeIndexValue\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%d", attrIndex) + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString AttrIndexContainsModelViewCriterionXML (const char* propertyGuid, Int32 attrIndex, bool isNot, const char* groupGuid = kModelViewPropertyGroupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "7" : "6") + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"AttributeIndexIndexValueListValueVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%d", attrIndex) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// intOp: 0=is, 6=contains, 7=does not contain (only ops confirmed for Font/Pen fields - no "is not"
// was seen for any of them in the captured "TAPIR3" rule, only for the separate Pen Replacement
// field which uses IntEqualsPropertyCriterionXML/op 0-1 instead).
static GS::UniString IntListCriterionXML (const char* propertyGuid, int value, int intOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", intOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"IntListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%d", value) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "Surface et Materiaux" (Surface and Materials) criteria category - reverse-engineered from a
// "TAPIR4" rule. Unlike "Nom Calque" (Model View), its string fields use StringListVariant (a
// <ValueArray> of <ValueItem> even for a single value) rather than a plain StringVariant.
static const char* kSurfaceMaterialPropertyGroupGuid = "6D514CD0-C97B-4363-BEBD-3B0E78192001";
static const char* kConstructionMaterialIdPropertyGuid   = "5E7EAF3C-AD5F-43C5-9C0C-223B0833742D";
static const char* kConstructionMaterialPropertyGuid     = "DE3E93A6-2D3B-4123-BAAE-2F8DABA6A41C";
static const char* kConstructionMaterialNamePropertyGuid = "38B71747-1B59-4486-A06E-A6D491AB5B24";
static const char* kSurfaceNamePropertyGuid              = "716D5B45-15FB-4473-AD28-FE358AC94B70";
static const char* kSurfacePropertyGuid                  = "E800D234-AD63-47BF-897A-99A98B04C76B";

// stringOp: 0=is, 1=is not, 6=contains, 7=does not contain, 8=starts with, 9=ends with.
static GS::UniString StringListCriterionXML (const char* propertyGuid, const GS::UniString& value, int stringOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", stringOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"StringListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + value + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// stringOp: 0=is, 1=is not, 6=contains, 7=does not contain, 8=starts with, 9=ends with.
// bareVariant: see GuidEqualsPropertyCriterionXML's doc - some property definitions (e.g.
// "Affichage sur filtre de renovation", "Etat de Variante", "Jeu de Variantes") omit the <Value>
// wrapper around their <Variant> entirely, confirmed live; most others (e.g. "Nom", "ID unique")
// don't, hence the default of false here.
static GS::UniString StringModelViewCriterionXML (const char* propertyGuid, const GS::UniString& value, int stringOp, const char* groupGuid = kModelViewPropertyGroupGuid, bool bareVariant = false)
{
    const GS::UniString variantXML = GS::UniString ("<Variant Type=\"StringVariant\"><Status>Normal</Status><Value>") + value + "</Value></Variant>";
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", stringOp) + "</CriteriaOperatorEnum>"
         + (bareVariant ? variantXML : (GS::UniString ("<Value>") + variantXML + "</Value>"))
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "ID et Categories" criteria category - reverse-engineered from a "TAPIR5" rule. Several distinct
// sub-groups exist within this one category (each field below lists its own). "ID de l'element",
// "Identifiant principal de Lien" and "Lien et ID d'element" support all 10 operators (is/is not/
// less-than/greater-than/less-or-equal/greater-or-equal/contains/does not contain/starts with/ends
// with) and, notably, still use plain StringVariant (not a numeric type) even for the numeric-
// looking comparison operators - confirmed live, e.g. "less than" stores its value as the string
// "1", not a number. "ID unique" and "Nom" support only the 6 text operators (no numeric ones).
// Only fields with an unambiguous, fully captured value mapping are implemented - "Calcul de Zone"
// and "Etat de renovation" were exercised in the same rule but their exact value-to-label mapping
// could not be disambiguated from the captured data (fewer distinct values were saved than there
// are dropdown options), so they are intentionally left unsupported rather than guessed.
static const char* kIdCategoryGroupGuid1     = "21AFD5A3-5024-457F-BC50-1AC8B973BA81"; // shared with Classification's ModelElement-adjacent group seen earlier this session
static const char* kIdCategoryGroupGuid2     = "7480DAA4-9E45-49BF-AE44-8AD0F57F95B2";
static const char* kIdCategoryGroupGuid3     = "953BBE44-8DD0-4527-85D8-DC6FF12E57D3";
static const char* kIdCategoryGroupGuid4     = "C77496B3-B63A-4272-ABC3-0A8C530E2B45";

static const char* kRenovationFilterPropertyGuid = "3FC77583-1284-478B-BA80-9C81F0A091B1"; // group1
static const char* kStructuralFunctionPropertyGuid = "9B2F38A8-198B-4125-9E73-5646F072C10E"; // group1
static const char* kPositionPropertyGuid     = "53402BE5-29D9-44A5-B6D6-4FA90938DDC9"; // group1
static const char* kVariantStatePropertyGuid = "5C3FB768-C349-49E7-B6E6-7675A26A2AEC"; // group2
static const char* kVariantSetPropertyGuid   = "29914344-AE69-41BE-983F-856EFD41AC33"; // group2
static const char* kElementIdPropertyGuid    = "B1B54D45-C951-42C9-9AF8-898F0BF212AB"; // group3
static const char* kUniqueIdPropertyGuid     = "B028B081-606E-4465-B530-6DC2457E1E78"; // group3
static const char* kLinkMainIdPropertyGuid   = "1E6DD646-48B8-47AB-B4C5-727A7B42DC0C"; // group3
static const char* kLinkedElementIdPropertyGuid = "D6FD38B0-DCD1-4F61-AA7E-3E6FD1D81F90"; // group3
static const char* kNamePropertyGuid         = "7B8277EB-5891-4222-A03B-4B7F432739CC"; // group3
static const char* kLinkSourcePropertyGuid   = "848C0C29-BD43-47D1-8B35-403F64280ADA"; // group3
static const char* kMissingVariantPropertyGuid = "5CC582DF-EC76-4A09-AFB9-B97FADA4F2EC"; // group4

// Same idea as TryEmitStringListField, for the 6-operator plain-StringVariant field family (ID et
// Categories category: "ID unique", "Nom").
static bool TryEmitStringField6Ops (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, const char* groupGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Same idea, for the 10-operator hybrid field family ("ID de l'element", "Identifiant principal de
// Lien", "Lien et ID d'element") - adds LessThan/GreaterThan/LessOrEqual/GreaterOrEqual, still all
// plain StringVariant (see the category comment above).
static bool TryEmitStringField10Ops (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, const char* groupGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       2,          3,             4,             5,                6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 10; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Enum-valued properties identified by a fixed internal GuidVariant value (Fonction structurelle,
// Position, Source de lien) - each option's guid is a hardcoded constant with no name lookup
// possible/needed. bareVariant: some property definitions omit the <Value> wrapper around their
// <Variant> entirely (confirmed live, e.g. Fonction structurelle/Position do, Source de lien
// doesn't) - a per-property quirk of ArchiCAD's own property schema, not tied to type or operator
// count, so it must be passed in explicitly rather than inferred.
static GS::UniString GuidEqualsPropertyCriterionXML (const char* propertyGuid, const char* valueGuid, bool isNot, const char* groupGuid, bool bareVariant = false)
{
    const GS::UniString variantXML = GS::UniString ("<Variant Type=\"GuidVariant\"><Status>Normal</Status><Value>") + valueGuid + "</Value></Variant>";
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "1" : "0") + "</CriteriaOperatorEnum>"
         + (bareVariant ? variantXML : (GS::UniString ("<Value>") + variantXML + "</Value>"))
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "Type de structure" (structureType) uses a bare <Variant> directly under <VBEF::ConditionIO> -
// no <Value> wrapper - unlike every other property criterion captured this session. Confirmed via
// the exact byte structure of a live-captured "TAPIR" rule exercising this field.
static GS::UniString IntEqualsPropertyCriterionXML (const char* propertyGuid, int value, const char* groupGuid, bool isNot = false)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString (isNot ? "1" : "0") + "</CriteriaOperatorEnum>"
         + "<Variant Type=\"IntVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%d", value) + "</Value></Variant>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

// "Positionnement" (Positioning) criteria category. numOp: 0==, 1=not-equal, 2=less-than,
// 3=greater-than, 4=less-or-equal, 5=greater-or-equal - confirmed live via a rule exercising all 6
// operators on a single field ("Altitude a niveau de la mer" in a "TAPIR2" rule).
static const char* kPositioningPropertyGroupGuid   = "C42CED8B-DD12-4782-A0C5-8FF167D55DBB";
static const char* kSeaLevelAltitudePropertyGuid   = "C35800B4-5126-405B-964B-EF0C43B1D2C2";
static const char* kProjectZeroAltitudePropertyGuid = "5294566C-2BB7-4ADC-A6F7-F8E947B1E500";
static const char* kGroundFloorAltitudePropertyGuid = "6EC682F2-34E5-419F-BC0E-4B1020A5DFC5";
static const char* kRoofAltitudePropertyGuid       = "D204DBB0-D697-47F2-9FD6-EA988BBF35DF";

// "MEP" (mechanical/electrical/plumbing element) criteria category - reverse-engineered from a
// "TAPIR1" rule on Archicad 29. Only applicable to MEP elements (ducts, pipes, cable carriers, ...).
// The 12 numeric fields all use the same generic 6-operator NumVariant pattern as Positionnement;
// Description/Nom du materiau use the same 6-operator plain StringVariant pattern as ID et
// Categories' "ID unique"/"Nom" - confirmed live for the "contains" operator on both, the rest of
// the operator set matches by direct analogy with every other field using these two shared helpers.
static const char* kMepPropertyGroupGuid           = "85599253-0EF2-4CA9-8086-A49B507DA053";
static const char* kMepDiameterPropertyGuid        = "8CF9515E-3BE6-4D59-AD21-3534E75AEAA8";
static const char* kMepDiameter2PropertyGuid       = "3B4312F8-C31F-493B-B19F-A2A07E908C3A";
static const char* kMepDNPropertyGuid              = "4A94BDEC-3A8C-47F6-A7D0-BE1866CC8E69";
static const char* kMepDN2PropertyGuid             = "9407DEA8-D393-41ED-91F3-3540F83B935F";
static const char* kMepHeightPropertyGuid          = "F2074948-92A3-41C7-B036-6E8C31EA135F";
static const char* kMepHeight2PropertyGuid         = "AA6A2A58-1AAE-49E7-B14F-080B8F1EADD9";
static const char* kMepWidthPropertyGuid           = "8300E1CC-B910-44AB-AC3A-529D6863836E";
static const char* kMepWidth2PropertyGuid          = "B4DD3D87-3231-4860-A15B-75451254B109";
static const char* kMepAnglePropertyGuid           = "671B771B-2437-4649-BF49-0AFDA391739A";
static const char* kMepCustomAnglePropertyGuid     = "6DA39F69-F58B-42C4-B260-27BD70A5B696";
static const char* kMepInsulationThicknessPropertyGuid = "FE791816-C043-4878-B1A3-E96C0BE01D2F";
static const char* kMepLengthPropertyGuid          = "F4D30EE5-C430-4CF9-BFA4-67DF4A4EE15A";
static const char* kMepDescriptionPropertyGuid     = "DB605867-4DC8-458C-8159-83B12B539A1E";
static const char* kMepMaterialNamePropertyGuid    = "8D4F0D8F-9915-4E89-90F6-340B0CC1BB20";

// Wall-specific criteria - reverse-engineered from a "TESTTAPIR_MUR" rule on Archicad 29. Unlike
// every category so far, these fields are scattered across several different property groups:
// already-known ones (Positionnement, Construction, Plan et Coupe, ID et Categories, Surface et
// Materiaux) plus one new one only ever seen paired with wall-specific fields - kWallGeometryGroupGuid
// ("Geometrie" tab fields specific to walls: thickness/height/width/angles/...).
// Note: the structural/FEM analytical-model fields (offsets, edge releases, member type/position,
// group guid CD27E698-F857-47A6-833B-6F0F558DA5F5) were reverse-engineered and briefly supported here
// too, but were deliberately removed by user decision: too many rules for Column/Beam with no clear
// "not covered" boundary, so analytical-model criteria are entirely out of scope for this generator.
static const char* kWallGeometryGroupGuid       = "554FDF95-B57F-427D-85F5-C00CFBF7406D";

static const char* kWallComplexityPropertyGuid           = "1BDC244B-242B-4DF9-A77C-20B079491495"; // Construction group
static const char* kWallLevelOffsetPropertyGuid          = "015C36FD-F3C5-4AE0-AF60-EBC2935CC5E9"; // Positioning group
static const char* kWallTopOffsetPropertyGuid            = "501D6492-507A-47D8-9158-EEA2F0882B7F"; // Positioning group
static const char* kWallReferenceLinePositionPropertyGuid = "54FEFC6B-6BCA-4535-8311-BBE04203CB07"; // Positioning group
static const char* kWallOutsideAnglePropertyGuid         = "7EF7271A-5EAB-4805-86AB-120869542EAA";
static const char* kWallInsideAnglePropertyGuid          = "654377FC-E480-4C7A-9906-25892080710B";
static const char* kWallThicknessPropertyGuid            = "AC754097-00D3-4301-9E20-328A4780FC72";
static const char* kWallHeightPropertyGuid               = "7CAB47B5-D80B-4E2F-AB2B-71CF0BB57856";
static const char* kWallProfileHeightPropertyGuid        = "7EDA68CA-D646-4180-B840-4327181C3E62";
static const char* kWallHeightReversedPropertyGuid       = "E0B209EB-A5B1-4198-A86A-462F3BB59955";
static const char* kWallWidthPropertyGuid                = "19A99799-5B44-4A6F-9904-F132D7A3F0AC";
static const char* kWallProfileWidthPropertyGuid         = "A69ECF2A-DFEE-4B64-A1CF-C562F57FB503";
static const char* kWallGeometryTypePropertyGuid         = "3FEBB287-6D09-4629-8F48-099D15927732";
static const char* kWallReferenceLineOffsetAttrGuid      = "D625E555-CBB8-443E-BFC5-F29F575472F8"; // RealCriterion/AttributeCriterion pattern
static const char* kWallTopLinePropertyGuid              = "E0FB1BED-9779-4547-839C-666C65B4F961"; // Plan et Coupe group
static const char* kWallCutLinePropertyGuid              = "801FA71E-EAF1-44C5-B6F5-F85671367FB7"; // Plan et Coupe group
static const char* kWallUncutLinePropertyGuid            = "B672C3D2-EC21-4D47-B0FE-FB34899D0CFF"; // Plan et Coupe group
static const char* kWallHatchForegroundPenPropertyGuid   = "B9482485-AF7B-49A9-B012-553C4366EA86"; // Plan et Coupe group
static const char* kWallContourPenPropertyGuid           = "51B1930E-12AB-4A8B-9C25-99BF0997FE4C"; // Plan et Coupe group
static const char* kWallTopLinePenPropertyGuid           = "66B0FA67-6487-4585-9168-AD0F0586A213"; // Plan et Coupe group
static const char* kWallCutHatchBackgroundPenPropertyGuid = "7AF8E5FF-4DC2-4869-9BED-14A2E8D8247D"; // Plan et Coupe group
static const char* kWallHatchBackgroundPenPropertyGuid   = "ED2B9124-DE9C-46D9-9E42-9796ECB73C3F"; // Plan et Coupe group
static const char* kWallUncutLinePenPropertyGuid         = "245B9933-064A-4B59-BE0A-F4517AE79E5D"; // Plan et Coupe group
static const char* kWallOutsideSurfacePropertyGuid       = "CFD1BA5B-19BF-48CB-A007-03C017190DE7"; // Surface et Materiaux group
static const char* kWallInsideSurfacePropertyGuid        = "ED36D500-8AC8-41AD-A316-5178D7786B16"; // Surface et Materiaux group
static const char* kWallSideSurfacePropertyGuid          = "553149E3-242B-4686-8190-68C3BB122B64"; // Surface et Materiaux group
static const char* kWallParentIdPropertyGuid             = "652333FC-B73A-4D25-92A2-ACAD3BCF847E"; // ID et Categories group3
static const char* kWallConnectedOpeningIdsPropertyGuid  = "917EE466-F091-4794-AC4C-1ED044C9F802"; // ID et Categories group3

// "Decalage Ligne de Reference" uses an entirely different mechanism from every other numeric field
// captured this session: a RealCriterion/AttributeCriterion pair (not PropertyCriterion), with its
// own "Type" enum for the operator (1=is, 2=lessThan, 3=greaterThan, 4=isNot, 5=lessOrEqual,
// 6=greaterOrEqual - NOT the usual 0-5 CriteriaOperatorEnum numbering) - confirmed live against all
// 6 rows of a captured rule.
static const char* kRealCriterionClassGuid = "52E38E9F-7E9F-44F1-A295-4873E529D52E";
static const char* kRealConditionClassGuid = "43ACB724-3561-4277-98FA-456D782D6D67";

static GS::UniString RealAttributeCriterionXML (const char* attributeGuid, double value, int realType)
{
    return GS::UniString ("<ClassGuid>") + kRealCriterionClassGuid + "</ClassGuid>"
         + "<RealCriterion Mv=\"2\" Sv=\"0\">"
         +     "<AttributeCriterion Mv=\"1\" Sv=\"0\"><AttributeGuid>" + attributeGuid + "</AttributeGuid></AttributeCriterion>"
         +     "<ClassGuid>" + kRealConditionClassGuid + "</ClassGuid>"
         +     "<RealSimpleCondition Mv=\"1\" Sv=\"0\"><Type>" + GS::UniString::Printf ("%d", realType) + "</Type>"
         +         "<Value>" + GS::UniString::Printf ("%g", value) + "</Value></RealSimpleCondition>"
         + "</RealCriterion>";
}

static bool TryEmitRealAttributeField (const GS::ObjectState& node, const char* fieldPrefix, const char* attributeGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kTypes[]    = { 1,   4,     2,          3,             5,             6 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (RealAttributeCriterionXML (attributeGuid, value, kTypes[i]));
            return true;
        }
    }
    return false;
}

// Column-specific criteria - reverse-engineered from a "TESTTAPIR_POTEAU" rule on Archicad 29. Most
// of Column's fields turned out to be the exact same properties already captured for Wall (Height,
// ProfileHeight, HeightReversed, Width, ProfileWidth, the 6 Plan et Coupe pens, TopLine, UncutLine,
// ParentId, ConnectedOpeningIds, wallLevelOffset/wallTopOffset) - reused as-is via their existing
// "wallXxx" JSON field names rather than duplicated under new column-prefixed names, since the
// underlying guid/group/XML shape is byte-identical. Only the genuinely column-specific fields below
// are new. Analytical/structural (FEM) fields are entirely out of scope for this generator - see the
// note above kWallGeometryGroupGuid.
static const char* kColumnCoatingTypePropertyGuid          = "2810579D-4FDE-4021-9A45-8909DC9491F4"; // Construction group
static const char* kColumnInclinationAnglePropertyGuid     = "1A448310-F491-4C72-B457-29ED14337221";
static const char* kColumnCrossSectionTypePropertyGuid     = "CD4B0B73-1D23-4C6C-A2E8-A9F785795ECE";
static const char* kColumnCoreDiameterPropertyGuid         = "B801C842-A167-4C70-A333-521608A2E906";
static const char* kColumnCoatingThicknessPropertyGuid     = "98E22B3A-C663-4249-8D85-BC5C4F32D487";
static const char* kColumnBottomCrossSectionHeightCutPropertyGuid           = "8362B613-A280-448F-9A18-5CFFE7D2F396";
static const char* kColumnBottomCrossSectionHeightPerpendicularPropertyGuid = "44B25D67-1443-48BD-823B-FED0F337EE2A";
static const char* kColumnTopCrossSectionHeightCutPropertyGuid              = "2252E6BF-8732-44A2-BCBD-DEDB4755B401";
static const char* kColumnTopCrossSectionHeightPerpendicularPropertyGuid    = "DA523AF3-DDF3-4CB3-918A-2E351AEDBF28";
static const char* kColumnProfileCoreHeightPropertyGuid                     = "EC45A091-9916-48B0-8A8F-1EF9CD875558";
static const char* kColumnCoreHeightDiameterRatioPropertyGuid               = "C9D4DA15-A7A7-4B5D-8E48-854F3D2A7053";
static const char* kColumnBottomCrossSectionWidthCutPropertyGuid            = "AF929A6D-7987-4D67-85DB-54DCC4108C50";
static const char* kColumnBottomCrossSectionWidthPerpendicularPropertyGuid  = "60229C62-3E47-4465-83E6-EAF08EAA58ED";
static const char* kColumnTopCrossSectionWidthCutPropertyGuid               = "D802D20B-7C6E-4C3B-BD47-DF9B493275F6";
static const char* kColumnTopCrossSectionWidthPerpendicularPropertyGuid     = "AC42946C-D4CF-429E-9B96-5EC771A70F34";
static const char* kColumnCoreWidthPropertyGuid                             = "2E32E96D-6220-4AE2-86C2-FEDB45450DC2";
static const char* kColumnProfileCoreWidthPropertyGuid                      = "98A67AD0-6B13-46CE-B675-7C121375FF89";
static const char* kColumnCoveringHatchPropertyGuid         = "127C75B5-E910-499A-9ADC-D96C177EEE94"; // Plan et Coupe group
static const char* kColumnCoatingHatchPropertyGuid          = "4D8A160B-C77F-457A-90B6-F5DC20DFA3D5"; // Plan et Coupe group
static const char* kColumnCoreForegroundPenPropertyGuid           = "89E86F63-D06B-4C79-8FE9-3AA79367E01E";
static const char* kColumnCoatingForegroundPenPropertyGuid        = "CDD27246-A306-441B-927C-B5B848057978";
static const char* kColumnCoveringHatchForegroundPenPropertyGuid  = "080B21EF-26A0-4549-BA5D-4296CCD9DFAB";
static const char* kColumnCoveringHatchBackgroundPenPropertyGuid  = "8DDAC1BD-539C-4770-A3CF-0AD197858016";
static const char* kColumnCoatingBackgroundPenPropertyGuid        = "E6381AC7-4CBE-4017-9AE9-7B7323FB7901";
static const char* kColumnCoreBackgroundPenPropertyGuid           = "B969A0F1-C258-4552-B231-D470AD8B9697";
static const char* kColumnHiddenLinePenPropertyGuid               = "A707B974-39B9-43F5-8B6A-37157903265B";
static const char* kColumnHiddenLineTypePropertyGuid        = "2DFDC696-E0E3-4122-A796-A872CB861ED2"; // Plan et Coupe group
static const char* kColumnCoreSurfacePropertyGuid           = "782C3212-829A-4BBE-B0F3-278CFD3FD3C0"; // Surface et Materiaux group
static const char* kColumnCoatingSurfacePropertyGuid        = "277C094D-89F6-4B93-BA37-82C7D3C619EC"; // Surface et Materiaux group
static const char* kColumnExtrusionSurfacePropertyGuid      = "45CD5C6B-92AB-4872-B774-B6CC8E0920E0"; // Surface et Materiaux group

// Beam-specific criteria - reverse-engineered from a "TESTTAPIR_POUTRE" rule on Archicad 29. Almost
// every field turned out to be shared with Wall/Column (same guids, reused via their existing
// "wallXxx"/"columnXxx" JSON field names) - only these 7 are genuinely new to Beam.
static const char* kBeamAxisPenPropertyGuid          = "4A2025B4-8CDA-44F4-B4D1-0DA9FE50E8F4"; // Plan et Coupe group
static const char* kBeamAxisLineTypePropertyGuid     = "E0E205E4-F84D-45A4-B9F4-90C61E87CFF3"; // Plan et Coupe group
static const char* kBeamEndSurfacePropertyGuid       = "BA800584-43FD-458E-B644-A848E4B28268"; // Surface et Materiaux group
static const char* kBeamRightSurfacePropertyGuid     = "F99C4755-8DD4-4740-AEE9-D6DF185BA2A7"; // Surface et Materiaux group
static const char* kBeamLeftSurfacePropertyGuid      = "EC6BEC35-B802-4D08-96D1-5D40FB4754E3"; // Surface et Materiaux group
static const char* kBeamBottomSurfacePropertyGuid    = "9592AFF2-C655-4E1A-8434-D5CC9409D609"; // Surface et Materiaux group
static const char* kBeamTopSurfacePropertyGuid       = "8B571B29-384C-4D2C-A87F-594E60A6686F"; // Surface et Materiaux group

// Slab-specific criteria - reverse-engineered from a "TESTTAPIR_DALLE" rule on Archicad 29. Again,
// most fields are shared with Wall/Column/Beam (reused via their existing JSON field names) - only
// these 5 are genuinely new to Slab.
static const char* kSlabReferencePlaneLocationPropertyGuid = "EF2304F7-4AD4-4D29-AD23-904A92244982"; // Positionnement group
static const char* kSlabEdgeAnglePropertyGuid               = "FF81335A-A237-44A5-9E22-02B07585DFE3"; // Geometrie group
static const char* kSlabDefaultEdgeAnglePropertyGuid         = "B00BB8A0-2F90-4B15-B6EA-4FEFA4DE2DE1"; // Geometrie group
static const char* kSlabCutLinePenPropertyGuid               = "34408308-7AE6-4A11-B4C4-DF041DB2C0DC"; // Plan et Coupe group
static const char* kSlabConstructionMaterialFillPropertyGuid = "A9B69EE4-6AEE-4F29-B9E7-F8C4BA7ADB01"; // Plan et Coupe group

// Roof-specific criteria - reverse-engineered from a "TESTTAPIR_TOIT" rule on Archicad 29. Again,
// most fields are shared with Wall/Column/Beam/Slab (reused via their existing JSON field names,
// including structureType, which Roof also exposes) - only these 7 are genuinely new to Roof.
static const char* kRoofCutBodyTypePropertyGuid       = "7F100F87-6D8F-43AA-97B5-8194C600B3FE"; // Construction group
static const char* kRoofLevelNumberPropertyGuid       = "599C4BCA-CC94-46F8-BF4B-EA787984CC4E"; // Construction group
static const char* kRoofEaveOverhangTypePropertyGuid  = "00A1F4A4-B63C-4226-BB80-D4FFCEBDDC2E"; // Construction group
static const char* kRoofEdgeAnglePropertyGuid         = "C06EA0C1-93EB-49A4-A3A0-AC0D842C778B"; // Geometrie group
static const char* kRoofEaveOverhangPropertyGuid      = "7513211D-FB9D-45DE-9C1B-CFC9BBFD8E49"; // Geometrie group
static const char* kRoofGeometryTypePropertyGuid      = "0E58BDFD-8B5F-443E-AD3F-15F92A53F65A"; // Geometrie group
static const char* kRoofPitchPropertyGuid             = "96CA70EA-53AE-4966-9679-AE6699CD78E3"; // Geometrie group

static GS::UniString NumListCriterionXML (const char* propertyGuid, double value, int numOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"NumListVariant\"><Status>Normal</Status><ValueArray><ValueItem>" + GS::UniString::Printf ("%g", value) + "</ValueItem></ValueArray></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static bool TryEmitNumListField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (NumListCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Shell-specific criteria - reverse-engineered from a "TESTTAPIR_COQUE" rule on Archicad 29. Again,
// most fields are shared with Wall/Column/Beam/Slab/Roof (reused via their existing JSON field
// names) - only these 3 are genuinely new to Shell. shellGeometryType's 3 values (Extrude/Revolved/
// Ruled) were all confirmed live, re-captured after the user set each row to a distinct value.
static const char* kShellCutBodyTypePropertyGuid  = "626C831E-F093-4983-A3C8-3723CE5D24D8"; // Construction group
static const char* kShellTiltAnglePropertyGuid     = "1E6BCA80-2880-4D63-9DA2-CF8B66CA28A5"; // Geometrie group
static const char* kShellGeometryTypePropertyGuid  = "5D862E7D-3DD8-4FEB-BC3F-090B3D96B27B"; // Geometrie group

// "Plan et Coupe" (Plan and Section) criteria category - reverse-engineered from a "TAPIR3" rule.
// Hachure/Type Ligne reference a real Fill/Line attribute (contains/does not contain only, like
// layerCombination). Police/Stylo/Stylo Texte reference a font/pen by its plain numeric ID (no
// attribute name lookup - pens in particular are always plain 1-255 numbers in ArchiCAD, not
// separate named attributes), each confirmed to support only the operator subset actually shown in
// the criteria editor for that field (Stylo: contains/does not contain only, no "is"; Stylo Texte
// and Police: is/contains/does not contain, no "is not"). Remplacement stylo (Pen Replacement) is
// a plain int equals/not-equals like structureType.
static const char* kPlanSectionPropertyGroupGuid = "50C777CB-6E83-4E01-911D-F45ECAA1A454";
static const char* kHatchFillPropertyGuid        = "AF2AC4AD-2176-4D0F-A20F-2CD2A6214288";
static const char* kFontPropertyGuid             = "06C5E0BC-B83A-4415-AD9A-3155EAB92CBF";
static const char* kPenReplacementPropertyGuid   = "2E2D1387-3642-4BAC-AE05-C16C68EA9A71";
static const char* kLinePenPropertyGuid          = "528B2B52-F122-4B1D-A0CA-5F3FFCE26BB2";
static const char* kTextPenPropertyGuid          = "06FC1DC4-A694-49F5-A0F3-6A5BF1C3CCC0";
static const char* kLineTypeAttrPropertyGuid     = "E8523625-0F5C-45DE-8A64-FD941F92F27F";

static GS::UniString NumPropertyCriterionXML (const char* propertyGuid, double value, int numOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"NumVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%g", value) + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString IntPropertyCriterionXML (const char* propertyGuid, int value, int numOp, const char* groupGuid)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\"><VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         + PropertyHeaderXML (propertyGuid, groupGuid)
         + "<CriteriaOperatorEnum>" + GS::UniString::Printf ("%d", numOp) + "</CriteriaOperatorEnum>"
         + "<Value><Variant Type=\"IntVariant\"><Status>Normal</Status><Value>" + GS::UniString::Printf ("%d", value) + "</Value></Variant></Value>"
         + "</VBEF::ConditionIO></PropertyCriterion>";
}

static GS::UniString ModelElementLeafXML ()
{
    return GS::UniString ("<ClassGuid>") + kModelElemClassGuid + "</ClassGuid>"
         + "<ModelElementCriterion Mv=\"2\" Sv=\"0\"><LogicalOperator>1</LogicalOperator></ModelElementCriterion>";
}

static GS::UniString DrawingElementLeafXML ()
{
    return GS::UniString ("<ClassGuid>") + kDrawingElemClassGuid + "</ClassGuid>"
         + "<DrawingElementCriteiron Mv=\"1\" Sv=\"0\"><LogicalOperator>1</LogicalOperator></DrawingElementCriteiron>";
}

// Tries each of the 6 numeric-comparison field name suffixes ("", "Not", "LessThan",
// "GreaterThan", "LessOrEqual", "GreaterOrEqual") for fieldPrefix against node; if one is present,
// emits the corresponding bare NumVariant PropertyCriterion into outXML and returns true (matched).
static bool TryEmitNumField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid = kPositioningPropertyGroupGuid)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (NumPropertyCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Same idea as TryEmitNumField, but for fields that store an IntVariant (DN/2eme DN in the MEP
// category) rather than a NumVariant - confirmed live these fail creation silently (rule accepted,
// criterion discarded) when sent as NumVariant, unlike every other numeric MEP field.
static bool TryEmitIntField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    int value = 0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// Same idea as TryEmitNumField, for the 6-suffix string-comparison field family emitted as
// StringListVariant (Surface et Materiaux category).
static bool TryEmitStringListField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML, const char* groupGuid = kSurfaceMaterialPropertyGroupGuid)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringListCriterionXML (propertyGuid, value, kOps[i], groupGuid));
            return true;
        }
    }
    return false;
}

// A shallow check for whether a node directly establishes an element type/model-element-ness
// (elementType, all3DTypes, all2DTypes).
static bool IsTypeEstablishingNode (const GS::ObjectState& node)
{
    GS::UniString dummyStr;
    bool dummyBool = false;
    return node.Get ("elementType", dummyStr) ||
           (node.Get ("all3DTypes", dummyBool) && dummyBool) ||
           (node.Get ("all2DTypes", dummyBool) && dummyBool);
}

// Checks whether a type-establishing node is reachable from the given node by following ONLY
// "and" edges (never descending into "or"). This intentionally does NOT mean "is there a
// type-establishing node anywhere in the tree": one sitting inside a single "or" branch does not
// make a shared anchor available to that branch's siblings (each still needs its own, since only
// one of the OR's branches is guaranteed to hold) - it only helps when it's on every path a
// candidate element must satisfy simultaneously, i.e. reachable purely through "and". Confirmed
// live: a rule combining "classificationNot" OR "elementType" failed to create at all when the
// generator (wrongly, in an earlier version) treated the elementType inside the "or" as covering
// the whole rule and skipped injecting a shared anchor.
static bool HasAndReachableAnchor (const GS::ObjectState& node)
{
    if (IsTypeEstablishingNode (node))
        return true;
    GS::Array<GS::ObjectState> andChildren;
    if (node.Get ("and", andChildren)) {
        for (const GS::ObjectState& child : andChildren) {
            if (HasAndReachableAnchor (child))
                return true;
        }
    }
    return false;
}

// Stylo Texte and Police are 2D annotation (Text/Label) properties, not applicable to 3D model
// elements - confirmed live that pairing them with the ModelElementCriterion ("Type d'element est
// Types 3D") anchor used for every other bare property leaf produces a semantically invalid rule
// (a Text-pen-only rule silently accepted "Types 3D AND Stylo Texte" without erroring, but that
// combination can never usefully match anything real, per direct user confirmation). Police is
// stricter still: ArchiCAD outright REJECTS creating a rule pairing it with Types 3D (confirmed
// live, APIERR-level failure) - it needs the "Types 2D" (DrawingElementCriteiron) anchor instead
// when otherwise unanchored, whereas Stylo Texte needs no anchor at all.
static bool IsTextPenNode (const GS::ObjectState& node)
{
    int dummyInt = 0;
    return node.Get ("textPenIs", dummyInt) || node.Get ("textPenContains", dummyInt) || node.Get ("textPenNotContains", dummyInt);
}

static bool IsFontNode (const GS::ObjectState& node)
{
    GS::UniString dummyStr;
    return node.Get ("fontIs", dummyStr) || node.Get ("fontContains", dummyStr) || node.Get ("fontNotContains", dummyStr);
}

// True only if EVERY leaf reachable from node (through any and/or nesting) is a Stylo Texte or
// Police field - the only two kinds of leaf that don't need the default 3D-model-element anchor.
static bool AllLeavesAreTextPenOrFont (const GS::ObjectState& node)
{
    GS::Array<GS::ObjectState> children;
    if (node.Get ("and", children) || node.Get ("or", children)) {
        for (const GS::ObjectState& child : children) {
            if (!AllLeavesAreTextPenOrFont (child))
                return false;
        }
        return true;
    }
    return IsTextPenNode (node) || IsFontNode (node);
}

// outIsSelfRooting is set true only when outXML already represents a complete, real multi-item
// CompositeCriterion suitable to be CriterionExpression's root directly - see the comment on
// CriterionToXML for why this distinction is needed.
static bool EmitCriterionNode (const GS::ObjectState& node, GS::UniString& outXML, GS::UniString& outError, bool& outIsSelfRooting, int depth = 0)
{
    outIsSelfRooting = false;
    GS::Array<GS::ObjectState> andChildren, orChildren;
    GS::UniString elementType, classification, classificationNot;
    bool all3DTypes = false, all2DTypes = false;

    const bool hasAnd = node.Get ("and", andChildren);
    const bool hasOr  = !hasAnd && node.Get ("or", orChildren);
    if (hasAnd || hasOr) {
        // ArchiCAD's criteria editor was confirmed live to only reliably render/evaluate a FLAT
        // "and" or "or" of leaves - nesting a further "and"/"or" inside one (e.g. AND containing an
        // OR that itself contains an AND) reliably corrupted the rendering of the property-based
        // leaves inside it, even with the ModelElementCriterion anchoring rules above correctly
        // applied. The only working fix found for a rule using that shape was to flatten it
        // entirely into one "and" of leaves - so reject deeper nesting explicitly here rather than
        // silently generating XML that produces a broken rule.
        if (depth > 0) {
            outError = "'and'/'or' cannot be nested inside another 'and'/'or' - ArchiCAD's criteria "
                       "editor does not reliably support that. Flatten into a single 'and' or 'or' "
                       "of leaves instead.";
            return false;
        }
        const bool isAnd = hasAnd;
        const GS::Array<GS::ObjectState>& children = isAnd ? andChildren : orChildren;
        if (children.IsEmpty ()) {
            outError = "'and'/'or' must contain at least one criterion.";
            return false;
        }
        GS::UniString childrenXML;
        for (const GS::ObjectState& child : children) {
            GS::UniString childXML;
            bool childIsSelfRooting = false;
            if (!EmitCriterionNode (child, childXML, outError, childIsSelfRooting, depth + 1))
                return false;
            childrenXML += childXML;
        }
        outXML = GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
               + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>" + GS::UniString (isAnd ? "2" : "1") + "</LogicalOperator>"
               + "<CriteriaCount>" + GS::UniString::Printf ("%d", (int) children.GetSize ()) + "</CriteriaCount>"
               + childrenXML
               + "</CompositeCriterion>";
        // An explicit "and"/"or" already represents the full top-level criteria table by itself
        // (confirmed live: hand-correcting "ElemType AND property" produced a root CompositeCriterion
        // whose own CriteriaCount is 2 directly, not a root of 1 wrapping a nested group of 2) - so,
        // like the ModelElement-paired leaves, it needs no further wrapping when used as the root.
        outIsSelfRooting = true;
        return true;
    }

    if (node.Get ("elementType", elementType)) {
        const ElemTypeInfo* info = FindElemTypeInfo (elementType);
        if (info == nullptr) {
            outError = "Unknown or unsupported elementType '" + elementType + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (ElemTypeCriterionXML (*info));
        return true;
    }

    if (node.Get ("classification", classification) || node.Get ("classificationNot", classificationNot)) {
        const bool isNot = !classificationNot.IsEmpty ();
        const GS::UniString& itemId = isNot ? classificationNot : classification;
        GS::UniString classificationSystem;
        if (!node.Get ("classificationSystem", classificationSystem) || classificationSystem.IsEmpty ())
            classificationSystem = kDefaultClassificationSystemName;
        outXML = WrapAsTrivialGroup (ClassificationCriterionXML (itemId, isNot, classificationSystem));
        return true;
    }

    if (node.Get ("all3DTypes", all3DTypes) && all3DTypes) {
        outXML = WrapAsTrivialGroup (ModelElementLeafXML ());
        return true;
    }
    if (node.Get ("all2DTypes", all2DTypes) && all2DTypes) {
        outXML = WrapAsTrivialGroup (DrawingElementLeafXML ());
        return true;
    }

    bool missingAttributes = false, layerLocked = false, layerVisible = false;
    GS::UniString layer, layerNot, layerCombination, layerCombinationNot;
    GS::UniString layerNameIs, layerNameIsNot, layerNameContains, layerNameNotContains, layerNameStartsWith, layerNameEndsWith;

    if (node.Get ("missingAttributes", missingAttributes)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kMissingAttributesPropertyGuid, missingAttributes));
        return true;
    }
    if (node.Get ("layerLocked", layerLocked)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kLayerLockedPropertyGuid, layerLocked));
        return true;
    }
    if (node.Get ("layerVisible", layerVisible)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kLayerVisiblePropertyGuid, layerVisible));
        return true;
    }
    if (node.Get ("layer", layer) || node.Get ("layerNot", layerNot)) {
        const bool isNot = !layerNot.IsEmpty ();
        const GS::UniString& name = isNot ? layerNot : layer;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LayerID, name, idx)) {
            outError = "Unknown layer '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kLayerPropertyGuid, idx, isNot));
        return true;
    }
    if (node.Get ("layerCombination", layerCombination) || node.Get ("layerCombinationNot", layerCombinationNot)) {
        const bool isNot = !layerCombinationNot.IsEmpty ();
        const GS::UniString& name = isNot ? layerCombinationNot : layerCombination;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LayerCombID, name, idx)) {
            outError = "Unknown layer combination '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kLayerCombinationPropertyGuid, idx, isNot));
        return true;
    }
    if (node.Get ("layerNameIs", layerNameIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameIs, 0));
        return true;
    }
    if (node.Get ("layerNameIsNot", layerNameIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameIsNot, 1));
        return true;
    }
    if (node.Get ("layerNameContains", layerNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameContains, 6));
        return true;
    }
    if (node.Get ("layerNameNotContains", layerNameNotContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameNotContains, 7));
        return true;
    }
    if (node.Get ("layerNameStartsWith", layerNameStartsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameStartsWith, 8));
        return true;
    }
    if (node.Get ("layerNameEndsWith", layerNameEndsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kLayerNamePropertyGuid, layerNameEndsWith, 9));
        return true;
    }

    GS::UniString compositeStructure, compositeStructureNot, complexProfile, complexProfileNot;
    GS::UniString compositeStructureNameIs, compositeStructureNameIsNot, compositeStructureNameContains, compositeStructureNameNotContains, compositeStructureNameStartsWith, compositeStructureNameEndsWith;
    GS::UniString complexProfileNameIs, complexProfileNameIsNot, complexProfileNameContains, complexProfileNameNotContains, complexProfileNameStartsWith, complexProfileNameEndsWith;
    bool roofConnected = false;
    GS::UniString structureType;

    if (node.Get ("compositeStructure", compositeStructure) || node.Get ("compositeStructureNot", compositeStructureNot)) {
        const bool isNot = !compositeStructureNot.IsEmpty ();
        const GS::UniString& name = isNot ? compositeStructureNot : compositeStructure;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_CompWallID, name, idx)) {
            outError = "Unknown composite structure '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kCompositeStructurePropertyGuid, idx, isNot, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfile", complexProfile) || node.Get ("complexProfileNot", complexProfileNot)) {
        const bool isNot = !complexProfileNot.IsEmpty ();
        const GS::UniString& name = isNot ? complexProfileNot : complexProfile;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_ProfileID, name, idx)) {
            outError = "Unknown complex profile '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kComplexProfilePropertyGuid, idx, isNot, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameIs", compositeStructureNameIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameIs, 0, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameIsNot", compositeStructureNameIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameIsNot, 1, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameContains", compositeStructureNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameContains, 6, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameNotContains", compositeStructureNameNotContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameNotContains, 7, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameStartsWith", compositeStructureNameStartsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameStartsWith, 8, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("compositeStructureNameEndsWith", compositeStructureNameEndsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kCompositeStructureNamePropertyGuid, compositeStructureNameEndsWith, 9, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameIs", complexProfileNameIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameIs, 0, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameIsNot", complexProfileNameIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameIsNot, 1, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameContains", complexProfileNameContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameContains, 6, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameNotContains", complexProfileNameNotContains)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameNotContains, 7, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameStartsWith", complexProfileNameStartsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameStartsWith, 8, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("complexProfileNameEndsWith", complexProfileNameEndsWith)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kComplexProfileNamePropertyGuid, complexProfileNameEndsWith, 9, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("roofConnected", roofConnected)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kRoofConnectedPropertyGuid, roofConnected, kConstructionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("structureType", structureType)) {
        int value = 0;
        if (structureType == "Basic")               value = 1;
        else if (structureType == "Composite")       value = 2;
        else if (structureType == "ComplexProfile")  value = 3;
        else {
            outError = "Invalid structureType '" + structureType + "'. Must be 'Basic', 'Composite', or 'ComplexProfile'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kStructureTypePropertyGuid, value, kConstructionPropertyGroupGuid));
        return true;
    }

    if (TryEmitNumField (node, "seaLevelAltitude", kSeaLevelAltitudePropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "projectZeroAltitude", kProjectZeroAltitudePropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "groundFloorAltitude", kGroundFloorAltitudePropertyGuid, outXML))
        return true;
    if (TryEmitNumField (node, "roofAltitude", kRoofAltitudePropertyGuid, outXML))
        return true;

    if (TryEmitNumField (node, "mepDiameter", kMepDiameterPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepDiameter2", kMepDiameter2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitIntField (node, "mepDN", kMepDNPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitIntField (node, "mepDN2", kMepDN2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepHeight", kMepHeightPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepHeight2", kMepHeight2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepWidth", kMepWidthPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepWidth2", kMepWidth2PropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepAngle", kMepAnglePropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepCustomAngle", kMepCustomAnglePropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepInsulationThickness", kMepInsulationThicknessPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "mepLength", kMepLengthPropertyGuid, outXML, kMepPropertyGroupGuid))
        return true;
    if (TryEmitStringField6Ops (node, "mepDescription", kMepDescriptionPropertyGuid, kMepPropertyGroupGuid, outXML))
        return true;
    if (TryEmitStringField6Ops (node, "mepMaterialName", kMepMaterialNamePropertyGuid, kMepPropertyGroupGuid, outXML))
        return true;

    GS::UniString wallComplexity, wallComplexityNot;
    if (node.Get ("wallComplexity", wallComplexity) || node.Get ("wallComplexityNot", wallComplexityNot)) {
        const bool isNot = !wallComplexityNot.IsEmpty ();
        const GS::UniString& s = isNot ? wallComplexityNot : wallComplexity;
        int value = 0;
        if (s == "Straight")            value = 0;
        else if (s == "Slanted")        value = 3;
        else if (s == "DoubleSlanted")  value = 5;
        else {
            outError = "Invalid wallComplexity/wallComplexityNot '" + s + "'. Must be 'Straight', 'Slanted', or 'DoubleSlanted'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kWallComplexityPropertyGuid, value, kConstructionPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "wallLevelOffset", kWallLevelOffsetPropertyGuid, outXML, kPositioningPropertyGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallTopOffset", kWallTopOffsetPropertyGuid, outXML, kPositioningPropertyGroupGuid))
        return true;
    GS::UniString wallRefLinePos, wallRefLinePosNot;
    if (node.Get ("wallReferenceLinePosition", wallRefLinePos) || node.Get ("wallReferenceLinePositionNot", wallRefLinePosNot)) {
        const bool isNot = !wallRefLinePosNot.IsEmpty ();
        const GS::UniString& s = isNot ? wallRefLinePosNot : wallRefLinePos;
        int value = 0;
        if (s == "Outside")            value = 0;
        else if (s == "Center")        value = 1;
        else if (s == "Inside")        value = 2;
        else if (s == "CoreOutside")   value = 3;
        else if (s == "CoreCenter")    value = 4;
        else if (s == "CoreInside")    value = 5;
        else {
            outError = "Invalid wallReferenceLinePosition/wallReferenceLinePositionNot '" + s + "'. Must be 'Outside', 'Center', 'Inside', 'CoreOutside', 'CoreCenter', or 'CoreInside'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kWallReferenceLinePositionPropertyGuid, value, kPositioningPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "wallOutsideAngle", kWallOutsideAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallInsideAngle", kWallInsideAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallThickness", kWallThicknessPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallHeight", kWallHeightPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallProfileHeight", kWallProfileHeightPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    bool wallHeightReversed = false;
    if (node.Get ("wallHeightReversed", wallHeightReversed)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kWallHeightReversedPropertyGuid, wallHeightReversed, kWallGeometryGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "wallWidth", kWallWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "wallProfileWidth", kWallProfileWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString wallGeometryType, wallGeometryTypeNot;
    if (node.Get ("wallGeometryType", wallGeometryType) || node.Get ("wallGeometryTypeNot", wallGeometryTypeNot)) {
        const bool isNot = !wallGeometryTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? wallGeometryTypeNot : wallGeometryType;
        int value = 0;
        if (s == "Uniform")         value = 0;
        else if (s == "Trapezoid")  value = 1;
        else if (s == "Polygonal")  value = 3;
        else {
            outError = "Invalid wallGeometryType/wallGeometryTypeNot '" + s + "'. Must be 'Uniform', 'Trapezoid', or 'Polygonal'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kWallGeometryTypePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitRealAttributeField (node, "wallReferenceLineOffset", kWallReferenceLineOffsetAttrGuid, outXML))
        return true;

    GS::UniString wallTopLine, wallTopLineNot, wallCutLine, wallCutLineNot, wallUncutLine, wallUncutLineNot;
    if (node.Get ("wallTopLine", wallTopLine) || node.Get ("wallTopLineNot", wallTopLineNot)) {
        const bool isNot = !wallTopLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallTopLineNot : wallTopLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallTopLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallCutLine", wallCutLine) || node.Get ("wallCutLineNot", wallCutLineNot)) {
        const bool isNot = !wallCutLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallCutLineNot : wallCutLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallCutLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallUncutLine", wallUncutLine) || node.Get ("wallUncutLineNot", wallUncutLineNot)) {
        const bool isNot = !wallUncutLineNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallUncutLineNot : wallUncutLine;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallUncutLinePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallHatchForegroundPen = 0, wallHatchForegroundPenNot = 0;
    if (node.Get ("wallHatchForegroundPen", wallHatchForegroundPen) || node.Get ("wallHatchForegroundPenNot", wallHatchForegroundPenNot)) {
        const bool isNot = node.Get ("wallHatchForegroundPenNot", wallHatchForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallHatchForegroundPenPropertyGuid, isNot ? wallHatchForegroundPenNot : wallHatchForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallContourPen = 0, wallContourPenNot = 0;
    if (node.Get ("wallContourPen", wallContourPen) || node.Get ("wallContourPenNot", wallContourPenNot)) {
        const bool isNot = node.Get ("wallContourPenNot", wallContourPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallContourPenPropertyGuid, isNot ? wallContourPenNot : wallContourPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallTopLinePen = 0, wallTopLinePenNot = 0;
    if (node.Get ("wallTopLinePen", wallTopLinePen) || node.Get ("wallTopLinePenNot", wallTopLinePenNot)) {
        const bool isNot = node.Get ("wallTopLinePenNot", wallTopLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallTopLinePenPropertyGuid, isNot ? wallTopLinePenNot : wallTopLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallCutHatchBackgroundPen = 0, wallCutHatchBackgroundPenNot = 0;
    if (node.Get ("wallCutHatchBackgroundPen", wallCutHatchBackgroundPen) || node.Get ("wallCutHatchBackgroundPenNot", wallCutHatchBackgroundPenNot)) {
        const bool isNot = node.Get ("wallCutHatchBackgroundPenNot", wallCutHatchBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallCutHatchBackgroundPenPropertyGuid, isNot ? wallCutHatchBackgroundPenNot : wallCutHatchBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallHatchBackgroundPen = 0, wallHatchBackgroundPenNot = 0;
    if (node.Get ("wallHatchBackgroundPen", wallHatchBackgroundPen) || node.Get ("wallHatchBackgroundPenNot", wallHatchBackgroundPenNot)) {
        const bool isNot = node.Get ("wallHatchBackgroundPenNot", wallHatchBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallHatchBackgroundPenPropertyGuid, isNot ? wallHatchBackgroundPenNot : wallHatchBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int wallUncutLinePen = 0, wallUncutLinePenNot = 0;
    if (node.Get ("wallUncutLinePen", wallUncutLinePen) || node.Get ("wallUncutLinePenNot", wallUncutLinePenNot)) {
        const bool isNot = node.Get ("wallUncutLinePenNot", wallUncutLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kWallUncutLinePenPropertyGuid, isNot ? wallUncutLinePenNot : wallUncutLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString wallOutsideSurface, wallOutsideSurfaceNot, wallInsideSurface, wallInsideSurfaceNot, wallSideSurface, wallSideSurfaceNot;
    if (node.Get ("wallOutsideSurface", wallOutsideSurface) || node.Get ("wallOutsideSurfaceNot", wallOutsideSurfaceNot)) {
        const bool isNot = !wallOutsideSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallOutsideSurfaceNot : wallOutsideSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallOutsideSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallInsideSurface", wallInsideSurface) || node.Get ("wallInsideSurfaceNot", wallInsideSurfaceNot)) {
        const bool isNot = !wallInsideSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallInsideSurfaceNot : wallInsideSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallInsideSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("wallSideSurface", wallSideSurface) || node.Get ("wallSideSurfaceNot", wallSideSurfaceNot)) {
        const bool isNot = !wallSideSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? wallSideSurfaceNot : wallSideSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kWallSideSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (TryEmitStringField6Ops (node, "wallParentId", kWallParentIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringListField (node, "wallConnectedOpeningIds", kWallConnectedOpeningIdsPropertyGuid, outXML, kIdCategoryGroupGuid3))
        return true;
    GS::UniString columnCoatingType, columnCoatingTypeNot;
    if (node.Get ("columnCoatingType", columnCoatingType) || node.Get ("columnCoatingTypeNot", columnCoatingTypeNot)) {
        const bool isNot = !columnCoatingTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? columnCoatingTypeNot : columnCoatingType;
        int value = 0;
        if (s == "Core")         value = 0;
        else if (s == "Finish")  value = 1;
        else if (s == "Other")   value = 2;
        else {
            outError = "Invalid columnCoatingType/columnCoatingTypeNot '" + s + "'. Must be 'Core', 'Finish', or 'Other'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kColumnCoatingTypePropertyGuid, value, kConstructionPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "columnInclinationAngle", kColumnInclinationAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString columnCrossSectionType;
    if (node.Get ("columnCrossSectionType", columnCrossSectionType)) {
        int value = 0;
        if (columnCrossSectionType == "Rectangular")     value = 2;
        else if (columnCrossSectionType == "Circular")   value = 1;
        else if (columnCrossSectionType == "Profiled")   value = 3;
        else {
            outError = "Invalid columnCrossSectionType '" + columnCrossSectionType + "'. Must be 'Rectangular', 'Circular', or 'Profiled'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kColumnCrossSectionTypePropertyGuid, value, kWallGeometryGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "columnCoreDiameter", kColumnCoreDiameterPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnCoatingThickness", kColumnCoatingThicknessPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionHeightCut", kColumnBottomCrossSectionHeightCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionHeightPerpendicular", kColumnBottomCrossSectionHeightPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionHeightCut", kColumnTopCrossSectionHeightCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionHeightPerpendicular", kColumnTopCrossSectionHeightPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnProfileCoreHeight", kColumnProfileCoreHeightPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnCoreHeightDiameterRatio", kColumnCoreHeightDiameterRatioPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionWidthCut", kColumnBottomCrossSectionWidthCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnBottomCrossSectionWidthPerpendicular", kColumnBottomCrossSectionWidthPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionWidthCut", kColumnTopCrossSectionWidthCutPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnTopCrossSectionWidthPerpendicular", kColumnTopCrossSectionWidthPerpendicularPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnCoreWidth", kColumnCoreWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    if (TryEmitNumField (node, "columnProfileCoreWidth", kColumnProfileCoreWidthPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString columnCoveringHatch, columnCoveringHatchNot, columnCoatingHatch, columnCoatingHatchNot;
    if (node.Get ("columnCoveringHatch", columnCoveringHatch) || node.Get ("columnCoveringHatchNot", columnCoveringHatchNot)) {
        const bool isNot = !columnCoveringHatchNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoveringHatchNot : columnCoveringHatch;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoveringHatchPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("columnCoatingHatch", columnCoatingHatch) || node.Get ("columnCoatingHatchNot", columnCoatingHatchNot)) {
        const bool isNot = !columnCoatingHatchNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoatingHatchNot : columnCoatingHatch;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoatingHatchPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoreForegroundPen = 0, columnCoreForegroundPenNot = 0;
    if (node.Get ("columnCoreForegroundPen", columnCoreForegroundPen) || node.Get ("columnCoreForegroundPenNot", columnCoreForegroundPenNot)) {
        const bool isNot = node.Get ("columnCoreForegroundPenNot", columnCoreForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoreForegroundPenPropertyGuid, isNot ? columnCoreForegroundPenNot : columnCoreForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoatingForegroundPen = 0, columnCoatingForegroundPenNot = 0;
    if (node.Get ("columnCoatingForegroundPen", columnCoatingForegroundPen) || node.Get ("columnCoatingForegroundPenNot", columnCoatingForegroundPenNot)) {
        const bool isNot = node.Get ("columnCoatingForegroundPenNot", columnCoatingForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoatingForegroundPenPropertyGuid, isNot ? columnCoatingForegroundPenNot : columnCoatingForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoveringHatchForegroundPen = 0, columnCoveringHatchForegroundPenNot = 0;
    if (node.Get ("columnCoveringHatchForegroundPen", columnCoveringHatchForegroundPen) || node.Get ("columnCoveringHatchForegroundPenNot", columnCoveringHatchForegroundPenNot)) {
        const bool isNot = node.Get ("columnCoveringHatchForegroundPenNot", columnCoveringHatchForegroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoveringHatchForegroundPenPropertyGuid, isNot ? columnCoveringHatchForegroundPenNot : columnCoveringHatchForegroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoveringHatchBackgroundPen = 0, columnCoveringHatchBackgroundPenNot = 0;
    if (node.Get ("columnCoveringHatchBackgroundPen", columnCoveringHatchBackgroundPen) || node.Get ("columnCoveringHatchBackgroundPenNot", columnCoveringHatchBackgroundPenNot)) {
        const bool isNot = node.Get ("columnCoveringHatchBackgroundPenNot", columnCoveringHatchBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoveringHatchBackgroundPenPropertyGuid, isNot ? columnCoveringHatchBackgroundPenNot : columnCoveringHatchBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoatingBackgroundPen = 0, columnCoatingBackgroundPenNot = 0;
    if (node.Get ("columnCoatingBackgroundPen", columnCoatingBackgroundPen) || node.Get ("columnCoatingBackgroundPenNot", columnCoatingBackgroundPenNot)) {
        const bool isNot = node.Get ("columnCoatingBackgroundPenNot", columnCoatingBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoatingBackgroundPenPropertyGuid, isNot ? columnCoatingBackgroundPenNot : columnCoatingBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnCoreBackgroundPen = 0, columnCoreBackgroundPenNot = 0;
    if (node.Get ("columnCoreBackgroundPen", columnCoreBackgroundPen) || node.Get ("columnCoreBackgroundPenNot", columnCoreBackgroundPenNot)) {
        const bool isNot = node.Get ("columnCoreBackgroundPenNot", columnCoreBackgroundPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnCoreBackgroundPenPropertyGuid, isNot ? columnCoreBackgroundPenNot : columnCoreBackgroundPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    int columnHiddenLinePen = 0, columnHiddenLinePenNot = 0;
    if (node.Get ("columnHiddenLinePen", columnHiddenLinePen) || node.Get ("columnHiddenLinePenNot", columnHiddenLinePenNot)) {
        const bool isNot = node.Get ("columnHiddenLinePenNot", columnHiddenLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kColumnHiddenLinePenPropertyGuid, isNot ? columnHiddenLinePenNot : columnHiddenLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString columnHiddenLineType, columnHiddenLineTypeNot;
    if (node.Get ("columnHiddenLineType", columnHiddenLineType) || node.Get ("columnHiddenLineTypeNot", columnHiddenLineTypeNot)) {
        const bool isNot = !columnHiddenLineTypeNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnHiddenLineTypeNot : columnHiddenLineType;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnHiddenLineTypePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString columnCoreSurface, columnCoreSurfaceNot, columnCoatingSurface, columnCoatingSurfaceNot, columnExtrusionSurface, columnExtrusionSurfaceNot;
    if (node.Get ("columnCoreSurface", columnCoreSurface) || node.Get ("columnCoreSurfaceNot", columnCoreSurfaceNot)) {
        const bool isNot = !columnCoreSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoreSurfaceNot : columnCoreSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoreSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("columnCoatingSurface", columnCoatingSurface) || node.Get ("columnCoatingSurfaceNot", columnCoatingSurfaceNot)) {
        const bool isNot = !columnCoatingSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnCoatingSurfaceNot : columnCoatingSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnCoatingSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("columnExtrusionSurface", columnExtrusionSurface) || node.Get ("columnExtrusionSurfaceNot", columnExtrusionSurfaceNot)) {
        const bool isNot = !columnExtrusionSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? columnExtrusionSurfaceNot : columnExtrusionSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kColumnExtrusionSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }

    int beamAxisPen = 0, beamAxisPenNot = 0;
    if (node.Get ("beamAxisPen", beamAxisPen) || node.Get ("beamAxisPenNot", beamAxisPenNot)) {
        const bool isNot = node.Get ("beamAxisPenNot", beamAxisPenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kBeamAxisPenPropertyGuid, isNot ? beamAxisPenNot : beamAxisPen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString beamAxisLineType, beamAxisLineTypeNot;
    if (node.Get ("beamAxisLineType", beamAxisLineType) || node.Get ("beamAxisLineTypeNot", beamAxisLineTypeNot)) {
        const bool isNot = !beamAxisLineTypeNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamAxisLineTypeNot : beamAxisLineType;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamAxisLineTypePropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString beamEndSurface, beamEndSurfaceNot, beamRightSurface, beamRightSurfaceNot, beamLeftSurface, beamLeftSurfaceNot;
    GS::UniString beamBottomSurface, beamBottomSurfaceNot, beamTopSurface, beamTopSurfaceNot;
    if (node.Get ("beamEndSurface", beamEndSurface) || node.Get ("beamEndSurfaceNot", beamEndSurfaceNot)) {
        const bool isNot = !beamEndSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamEndSurfaceNot : beamEndSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamEndSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamRightSurface", beamRightSurface) || node.Get ("beamRightSurfaceNot", beamRightSurfaceNot)) {
        const bool isNot = !beamRightSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamRightSurfaceNot : beamRightSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamRightSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamLeftSurface", beamLeftSurface) || node.Get ("beamLeftSurfaceNot", beamLeftSurfaceNot)) {
        const bool isNot = !beamLeftSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamLeftSurfaceNot : beamLeftSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamLeftSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamBottomSurface", beamBottomSurface) || node.Get ("beamBottomSurfaceNot", beamBottomSurfaceNot)) {
        const bool isNot = !beamBottomSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamBottomSurfaceNot : beamBottomSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamBottomSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("beamTopSurface", beamTopSurface) || node.Get ("beamTopSurfaceNot", beamTopSurfaceNot)) {
        const bool isNot = !beamTopSurfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? beamTopSurfaceNot : beamTopSurface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kBeamTopSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }

    GS::UniString slabRefPlaneLoc, slabRefPlaneLocNot;
    if (node.Get ("slabReferencePlaneLocation", slabRefPlaneLoc) || node.Get ("slabReferencePlaneLocationNot", slabRefPlaneLocNot)) {
        const bool isNot = !slabRefPlaneLocNot.IsEmpty ();
        const GS::UniString& s = isNot ? slabRefPlaneLocNot : slabRefPlaneLoc;
        int value = 0;
        if (s == "Top")             value = 0;
        else if (s == "CoreTop")    value = 1;
        else if (s == "CoreBottom") value = 2;
        else if (s == "Bottom")     value = 3;
        else {
            outError = "Invalid slabReferencePlaneLocation/slabReferencePlaneLocationNot '" + s + "'. Must be 'Top', 'CoreTop', 'CoreBottom', or 'Bottom'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kSlabReferencePlaneLocationPropertyGuid, value, kPositioningPropertyGroupGuid, isNot));
        return true;
    }
    GS::UniString slabEdgeAngle, slabEdgeAngleNot;
    if (node.Get ("slabEdgeAngle", slabEdgeAngle) || node.Get ("slabEdgeAngleNot", slabEdgeAngleNot)) {
        const bool isNot = !slabEdgeAngleNot.IsEmpty ();
        const GS::UniString& s = isNot ? slabEdgeAngleNot : slabEdgeAngle;
        int value = 0;
        if (s == "Vertical")     value = 0;
        else if (s == "Custom")  value = 3;
        else {
            outError = "Invalid slabEdgeAngle/slabEdgeAngleNot '" + s + "'. Must be 'Vertical' or 'Custom'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kSlabEdgeAnglePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "slabDefaultEdgeAngle", kSlabDefaultEdgeAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    int slabCutLinePen = 0, slabCutLinePenNot = 0;
    if (node.Get ("slabCutLinePen", slabCutLinePen) || node.Get ("slabCutLinePenNot", slabCutLinePenNot)) {
        const bool isNot = node.Get ("slabCutLinePenNot", slabCutLinePenNot);
        outXML = WrapAsTrivialGroup (IntPropertyCriterionXML (kSlabCutLinePenPropertyGuid, isNot ? slabCutLinePenNot : slabCutLinePen, isNot ? 1 : 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    GS::UniString slabConstructionMaterialFill, slabConstructionMaterialFillNot;
    if (node.Get ("slabConstructionMaterialFill", slabConstructionMaterialFill) || node.Get ("slabConstructionMaterialFillNot", slabConstructionMaterialFillNot)) {
        const bool isNot = !slabConstructionMaterialFillNot.IsEmpty ();
        const GS::UniString& name = isNot ? slabConstructionMaterialFillNot : slabConstructionMaterialFill;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexEqualsModelViewCriterionXML (kSlabConstructionMaterialFillPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }

    GS::UniString roofCutBodyType, roofCutBodyTypeNot;
    if (node.Get ("roofCutBodyType", roofCutBodyType) || node.Get ("roofCutBodyTypeNot", roofCutBodyTypeNot)) {
        const bool isNot = !roofCutBodyTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? roofCutBodyTypeNot : roofCutBodyType;
        int value = 0;
        if (s == "ContoursDown")            value = 2;
        else if (s == "ReferenceLineDown")  value = 3;
        else {
            outError = "Invalid roofCutBodyType/roofCutBodyTypeNot '" + s + "'. Must be 'ContoursDown' or 'ReferenceLineDown'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofCutBodyTypePropertyGuid, value, kConstructionPropertyGroupGuid, isNot));
        return true;
    }
    if (TryEmitIntField (node, "roofLevelNumber", kRoofLevelNumberPropertyGuid, outXML, kConstructionPropertyGroupGuid))
        return true;
    GS::UniString roofEaveOverhangType;
    if (node.Get ("roofEaveOverhangType", roofEaveOverhangType)) {
        int value = 0;
        if (roofEaveOverhangType == "Offset")      value = 1;
        else if (roofEaveOverhangType == "Manual") value = 2;
        else {
            outError = "Invalid roofEaveOverhangType '" + roofEaveOverhangType + "'. Must be 'Offset' or 'Manual'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofEaveOverhangTypePropertyGuid, value, kConstructionPropertyGroupGuid));
        return true;
    }
    GS::UniString roofEdgeAngle, roofEdgeAngleNot;
    if (node.Get ("roofEdgeAngle", roofEdgeAngle) || node.Get ("roofEdgeAngleNot", roofEdgeAngleNot)) {
        const bool isNot = !roofEdgeAngleNot.IsEmpty ();
        const GS::UniString& s = isNot ? roofEdgeAngleNot : roofEdgeAngle;
        int value = 0;
        if (s == "Vertical")           value = 0;
        else if (s == "Perpendicular") value = 1;
        else if (s == "Custom")        value = 3;
        else {
            outError = "Invalid roofEdgeAngle/roofEdgeAngleNot '" + s + "'. Must be 'Vertical', 'Perpendicular', or 'Custom'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofEdgeAnglePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumField (node, "roofEaveOverhang", kRoofEaveOverhangPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString roofGeometryType, roofGeometryTypeNot;
    if (node.Get ("roofGeometryType", roofGeometryType) || node.Get ("roofGeometryTypeNot", roofGeometryTypeNot)) {
        const bool isNot = !roofGeometryTypeNot.IsEmpty ();
        const GS::UniString& s = isNot ? roofGeometryTypeNot : roofGeometryType;
        int value = 0;
        if (s == "SinglePanel")          value = 1;
        else if (s == "MultiplePanels")  value = 2;
        else {
            outError = "Invalid roofGeometryType/roofGeometryTypeNot '" + s + "'. Must be 'SinglePanel' or 'MultiplePanels'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kRoofGeometryTypePropertyGuid, value, kWallGeometryGroupGuid, isNot));
        return true;
    }
    if (TryEmitNumListField (node, "roofPitch", kRoofPitchPropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;

    GS::UniString shellCutBodyType;
    if (node.Get ("shellCutBodyType", shellCutBodyType)) {
        int value = 0;
        if (shellCutBodyType == "Editable")           value = 1;
        else if (shellCutBodyType == "ExtrudeUp")     value = 4;
        else if (shellCutBodyType == "ExtrudeDown")   value = 5;
        else {
            outError = "Invalid shellCutBodyType '" + shellCutBodyType + "'. Must be 'Editable', 'ExtrudeUp', or 'ExtrudeDown'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kShellCutBodyTypePropertyGuid, value, kConstructionPropertyGroupGuid));
        return true;
    }
    if (TryEmitNumField (node, "shellTiltAngle", kShellTiltAnglePropertyGuid, outXML, kWallGeometryGroupGuid))
        return true;
    GS::UniString shellGeometryType;
    if (node.Get ("shellGeometryType", shellGeometryType)) {
        int value = 0;
        if (shellGeometryType == "Extrude")       value = 1;
        else if (shellGeometryType == "Revolved") value = 2;
        else if (shellGeometryType == "Ruled")    value = 3;
        else {
            outError = "Invalid shellGeometryType '" + shellGeometryType + "'. Must be 'Extrude', 'Revolved', or 'Ruled'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kShellGeometryTypePropertyGuid, value, kWallGeometryGroupGuid));
        return true;
    }

    GS::UniString hatchFill, hatchFillNot, lineAttribute, lineAttributeNot;
    GS::UniString fontIs, fontContains, fontNotContains;
    GS::UniString penReplacement, penReplacementNot;
    int linePenContains = 0, linePenNotContains = 0;
    int textPenIs = 0, textPenContains = 0, textPenNotContains = 0;

    if (node.Get ("hatchFill", hatchFill) || node.Get ("hatchFillNot", hatchFillNot)) {
        const bool isNot = !hatchFillNot.IsEmpty ();
        const GS::UniString& name = isNot ? hatchFillNot : hatchFill;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_FilltypeID, name, idx)) {
            outError = "Unknown fill '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kHatchFillPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("lineAttribute", lineAttribute) || node.Get ("lineAttributeNot", lineAttributeNot)) {
        const bool isNot = !lineAttributeNot.IsEmpty ();
        const GS::UniString& name = isNot ? lineAttributeNot : lineAttribute;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_LinetypeID, name, idx)) {
            outError = "Unknown line type '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kLineTypeAttrPropertyGuid, idx, isNot, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("fontIs", fontIs)) {
        Int32 idx = 0;
        if (!FindFontIndexByName (fontIs, idx)) {
            outError = "Unknown font '" + fontIs + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kFontPropertyGuid, idx, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("fontContains", fontContains)) {
        Int32 idx = 0;
        if (!FindFontIndexByName (fontContains, idx)) {
            outError = "Unknown font '" + fontContains + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kFontPropertyGuid, idx, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("fontNotContains", fontNotContains)) {
        Int32 idx = 0;
        if (!FindFontIndexByName (fontNotContains, idx)) {
            outError = "Unknown font '" + fontNotContains + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kFontPropertyGuid, idx, 7, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("penReplacement", penReplacement) || node.Get ("penReplacementNot", penReplacementNot)) {
        const bool isNot = !penReplacementNot.IsEmpty ();
        const GS::UniString& name = isNot ? penReplacementNot : penReplacement;
        int value = 0;
        if (name == "None")                value = 1;
        else if (name == "ForegroundOnly") value = 2;
        else if (name == "BackgroundOnly") value = 3;
        else if (name == "Both")           value = 4;
        else {
            outError = "Invalid penReplacement/penReplacementNot '" + name + "'. Must be 'None', 'ForegroundOnly', 'BackgroundOnly', or 'Both'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (IntEqualsPropertyCriterionXML (kPenReplacementPropertyGuid, value, kPlanSectionPropertyGroupGuid, isNot));
        return true;
    }
    if (node.Get ("linePenContains", linePenContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kLinePenPropertyGuid, linePenContains, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("linePenNotContains", linePenNotContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kLinePenPropertyGuid, linePenNotContains, 7, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("textPenIs", textPenIs)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextPenPropertyGuid, textPenIs, 0, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("textPenContains", textPenContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextPenPropertyGuid, textPenContains, 6, kPlanSectionPropertyGroupGuid));
        return true;
    }
    if (node.Get ("textPenNotContains", textPenNotContains)) {
        outXML = WrapAsTrivialGroup (IntListCriterionXML (kTextPenPropertyGuid, textPenNotContains, 7, kPlanSectionPropertyGroupGuid));
        return true;
    }

    GS::UniString constructionMaterial, constructionMaterialNot, surface, surfaceNot;

    if (node.Get ("constructionMaterial", constructionMaterial) || node.Get ("constructionMaterialNot", constructionMaterialNot)) {
        const bool isNot = !constructionMaterialNot.IsEmpty ();
        const GS::UniString& name = isNot ? constructionMaterialNot : constructionMaterial;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_BuildingMaterialID, name, idx)) {
            outError = "Unknown construction material '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kConstructionMaterialPropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (node.Get ("surface", surface) || node.Get ("surfaceNot", surfaceNot)) {
        const bool isNot = !surfaceNot.IsEmpty ();
        const GS::UniString& name = isNot ? surfaceNot : surface;
        Int32 idx = 0;
        if (!FindAttributeIndexByName (API_MaterialID, name, idx)) {
            outError = "Unknown surface '" + name + "'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (AttrIndexContainsModelViewCriterionXML (kSurfacePropertyGuid, idx, isNot, kSurfaceMaterialPropertyGroupGuid));
        return true;
    }
    if (TryEmitStringListField (node, "constructionMaterialId", kConstructionMaterialIdPropertyGuid, outXML))
        return true;
    if (TryEmitStringListField (node, "constructionMaterialName", kConstructionMaterialNamePropertyGuid, outXML))
        return true;
    if (TryEmitStringListField (node, "surfaceName", kSurfaceNamePropertyGuid, outXML))
        return true;

    GS::UniString renovationFilterIs, renovationFilterIsNot;
    GS::UniString structuralFunction, structuralFunctionNot, position, positionNot;
    GS::UniString variantStateIs, variantStateIsNot, variantSetIs, variantSetIsNot;
    bool hasLinkSource = false, hasLinkSourceNot = false, missingVariant = false;

    if (node.Get ("renovationFilterIs", renovationFilterIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kRenovationFilterPropertyGuid, renovationFilterIs, 0, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("renovationFilterIsNot", renovationFilterIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kRenovationFilterPropertyGuid, renovationFilterIsNot, 1, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("structuralFunction", structuralFunction) || node.Get ("structuralFunctionNot", structuralFunctionNot)) {
        const bool isNot = !structuralFunctionNot.IsEmpty ();
        const GS::UniString& val = isNot ? structuralFunctionNot : structuralFunction;
        const char* valueGuid = nullptr;
        if (val == "LoadBearing")         valueGuid = "C852BD7D-1198-2802-7E37-E4925B997B8B";
        else if (val == "NonLoadBearing") valueGuid = "539EA1F2-E319-7D11-7CAC-CCAA638FAD6E";
        else if (val == "Undefined")      valueGuid = "CCCC2E1F-54E6-8534-EEA0-F571386498E1";
        else {
            outError = "Invalid structuralFunction/structuralFunctionNot '" + val + "'. Must be 'LoadBearing', 'NonLoadBearing', or 'Undefined'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kStructuralFunctionPropertyGuid, valueGuid, isNot, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("position", position) || node.Get ("positionNot", positionNot)) {
        const bool isNot = !positionNot.IsEmpty ();
        const GS::UniString& val = isNot ? positionNot : position;
        const char* valueGuid = nullptr;
        if (val == "Exterior")        valueGuid = "6F76AEB9-1C41-7E76-5254-57399CBF985B";
        else if (val == "Interior")   valueGuid = "512CBDAC-06A3-A73D-ED46-1701A8BCE536";
        else if (val == "Undefined")  valueGuid = "677677E5-E514-E73F-39B9-E7AFC2031846";
        else {
            outError = "Invalid position/positionNot '" + val + "'. Must be 'Exterior', 'Interior', or 'Undefined'.";
            return false;
        }
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kPositionPropertyGuid, valueGuid, isNot, kIdCategoryGroupGuid1, true));
        return true;
    }
    if (node.Get ("variantStateIs", variantStateIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantStatePropertyGuid, variantStateIs, 0, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (node.Get ("variantStateIsNot", variantStateIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantStatePropertyGuid, variantStateIsNot, 1, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (node.Get ("variantSetIs", variantSetIs)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantSetPropertyGuid, variantSetIs, 0, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (node.Get ("variantSetIsNot", variantSetIsNot)) {
        outXML = WrapAsTrivialGroup (StringModelViewCriterionXML (kVariantSetPropertyGuid, variantSetIsNot, 1, kIdCategoryGroupGuid2, true));
        return true;
    }
    if (TryEmitStringField10Ops (node, "elementId", kElementIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField6Ops (node, "uniqueId", kUniqueIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField10Ops (node, "linkMainId", kLinkMainIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField10Ops (node, "linkedElementId", kLinkedElementIdPropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    if (TryEmitStringField6Ops (node, "name", kNamePropertyGuid, kIdCategoryGroupGuid3, outXML))
        return true;
    // "Source de lien" only ever showed a single fixed value ("Toute source de lien") in the
    // captured example - exposed as two presence-triggered booleans (matching all3DTypes/
    // all2DTypes' convention) around that one constant, rather than a real enum.
    if (node.Get ("hasLinkSource", hasLinkSource) && hasLinkSource) {
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kLinkSourcePropertyGuid, "4BD63BB7-0A82-4680-B6A7-708DC0AF0EC2", false, kIdCategoryGroupGuid3));
        return true;
    }
    if (node.Get ("hasLinkSourceNot", hasLinkSourceNot) && hasLinkSourceNot) {
        outXML = WrapAsTrivialGroup (GuidEqualsPropertyCriterionXML (kLinkSourcePropertyGuid, "4BD63BB7-0A82-4680-B6A7-708DC0AF0EC2", true, kIdCategoryGroupGuid3));
        return true;
    }
    if (node.Get ("missingVariant", missingVariant)) {
        outXML = WrapAsTrivialGroup (BoolModelViewCriterionXML (kMissingVariantPropertyGuid, missingVariant, kIdCategoryGroupGuid4));
        return true;
    }

    if (TryEmitProfileParamField (node, "profileStretchHeight", kProfileStretchHeightGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileStretchWidth", kProfileStretchWidthGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileNominalHeight", kProfileNominalHeightGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileTotalHeight", kProfileTotalHeightGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileTotalWidth", kProfileTotalWidthGuid, outXML))
        return true;
    if (TryEmitProfileParamField (node, "profileNominalWidth", kProfileNominalWidthGuid, outXML))
        return true;

    GS::ObjectState profileParameter;
    if (node.Get ("profileParameter", profileParameter)) {
        GS::UniString paramName;
        if (!profileParameter.Get ("name", paramName) || paramName.IsEmpty ()) {
            outError = "'profileParameter' requires a non-empty 'name'.";
            return false;
        }
        static const char* kSuffixes[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
        static const int   kOps[]      = { 0,         1,           2,          3,             4,             5 };
        double value = 0.0;
        bool matched = false;
        for (int i = 0; i < 6 && !matched; ++i) {
            if (profileParameter.Get (kSuffixes[i], value)) {
                outXML = WrapAsTrivialGroup (ProfileParameterCriterionXML (ProfileParameterHeaderByNameXML (paramName), value, kOps[i]));
                matched = true;
            }
        }
        if (!matched) {
            outError = "'profileParameter' requires exactly one of: equals, notEquals, lessThan, greaterThan, lessOrEqual, greaterOrEqual.";
            return false;
        }
        return true;
    }

    GS::ObjectState customPropertyBool, customPropertyString, customPropertyNumber, customPropertyInteger;
    if (node.Get ("customPropertyBool", customPropertyBool)) {
        GS::UniString name, group;
        if (!customPropertyBool.Get ("name", name) || !customPropertyBool.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyBool' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        bool eqValue = false;
        if (TryGetMetaOperator (customPropertyBool, metaOp)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, BoolVariantXML (false)));
        } else if (customPropertyBool.Get ("equals", eqValue)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, 0, BoolVariantXML (eqValue)));
        } else {
            outError = "'customPropertyBool' requires exactly one of: equals, hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }
    if (node.Get ("customPropertyString", customPropertyString)) {
        GS::UniString name, group, strValue;
        if (!customPropertyString.Get ("name", name) || !customPropertyString.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyString' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        if (TryGetMetaOperator (customPropertyString, metaOp)) {
            // See FindFirstOptionValueForCustomProperty: an option-list-backed property needs a real
            // option value as its (practically ignored) meta-operator placeholder, or ArchiCAD
            // silently discards the whole criterion. A free-text property has no such constraint, so
            // an empty placeholder is fine when the lookup finds no option list.
            GS::UniString placeholder;
            FindFirstOptionValueForCustomProperty (name, group, placeholder);
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, StringVariantXML (placeholder), true));
        } else if (customPropertyString.Get ("equals", strValue)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, 0, StringVariantXML (strValue), true));
        } else if (customPropertyString.Get ("notEquals", strValue)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, 1, StringVariantXML (strValue), true));
        } else {
            outError = "'customPropertyString' requires exactly one of: equals, notEquals, hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }
    if (node.Get ("customPropertyNumber", customPropertyNumber)) {
        GS::UniString name, group;
        if (!customPropertyNumber.Get ("name", name) || !customPropertyNumber.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyNumber' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        double numValue = 0.0;
        static const char* kNumSuffixes[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
        static const int   kNumOps[]      = { 0,         1,           2,          3,             4,             5 };
        bool matched = false;
        if (TryGetMetaOperator (customPropertyNumber, metaOp)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, NumVariantXML (0)));
            matched = true;
        } else {
            for (int i = 0; i < 6 && !matched; ++i) {
                if (customPropertyNumber.Get (kNumSuffixes[i], numValue)) {
                    outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, kNumOps[i], NumVariantXML (numValue)));
                    matched = true;
                }
            }
        }
        if (!matched) {
            outError = "'customPropertyNumber' requires exactly one of: equals, notEquals, lessThan, greaterThan, lessOrEqual, greaterOrEqual, "
                       "hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }
    if (node.Get ("customPropertyInteger", customPropertyInteger)) {
        GS::UniString name, group;
        if (!customPropertyInteger.Get ("name", name) || !customPropertyInteger.Get ("group", group) || name.IsEmpty () || group.IsEmpty ()) {
            outError = "'customPropertyInteger' requires non-empty 'name' and 'group'.";
            return false;
        }
        const GS::UniString headerXML = CustomPropertyHeaderXML (name, group);
        int metaOp = 0;
        int intValue = 0;
        static const char* kIntSuffixes[] = { "equals", "notEquals", "lessThan", "greaterThan", "lessOrEqual", "greaterOrEqual" };
        static const int   kIntOps[]      = { 0,         1,           2,          3,             4,             5 };
        bool matched = false;
        if (TryGetMetaOperator (customPropertyInteger, metaOp)) {
            outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, metaOp, IntVariantXML (0)));
            matched = true;
        } else {
            for (int i = 0; i < 6 && !matched; ++i) {
                if (customPropertyInteger.Get (kIntSuffixes[i], intValue)) {
                    outXML = WrapAsTrivialGroup (CustomPropertyCriterionXML (headerXML, kIntOps[i], IntVariantXML (intValue)));
                    matched = true;
                }
            }
        }
        if (!matched) {
            outError = "'customPropertyInteger' requires exactly one of: equals, notEquals, lessThan, greaterThan, lessOrEqual, greaterOrEqual, "
                       "hasDefaultValue, hasCustomValue, isAvailable, isNotAvailable, isUndefined, isNotUndefined.";
            return false;
        }
        return true;
    }

    outError = "A criterion node must contain exactly one of: and, or, elementType, classification, classificationNot, "
               "all3DTypes, all2DTypes, missingAttributes, layer, layerNot, layerLocked, layerVisible, layerCombination, "
               "layerCombinationNot, layerNameIs, layerNameIsNot, layerNameContains, layerNameNotContains, layerNameStartsWith, layerNameEndsWith, "
               "compositeStructure, compositeStructureNot, complexProfile, complexProfileNot, compositeStructureNameIs, compositeStructureNameIsNot, "
               "compositeStructureNameContains, compositeStructureNameNotContains, compositeStructureNameStartsWith, compositeStructureNameEndsWith, "
               "complexProfileNameIs, complexProfileNameIsNot, complexProfileNameContains, complexProfileNameNotContains, complexProfileNameStartsWith, "
               "complexProfileNameEndsWith, roofConnected, structureType, seaLevelAltitude(Not/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual), "
               "projectZeroAltitude(...), groundFloorAltitude(...), roofAltitude(...), hatchFill, hatchFillNot, lineAttribute, lineAttributeNot, "
               "fontIs, fontContains, fontNotContains, penReplacement, penReplacementNot, linePenContains, linePenNotContains, "
               "textPenIs, textPenContains, textPenNotContains, constructionMaterial, constructionMaterialNot, surface, surfaceNot, "
               "constructionMaterialId(Is/IsNot/Contains/NotContains/StartsWith/EndsWith), constructionMaterialName(...), surfaceName(...), "
               "renovationFilterIs, renovationFilterIsNot, structuralFunction, structuralFunctionNot, position, positionNot, "
               "variantStateIs, variantStateIsNot, variantSetIs, variantSetIsNot, elementId(Is/IsNot/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual/Contains/NotContains/StartsWith/EndsWith), "
               "uniqueId(Is/IsNot/Contains/NotContains/StartsWith/EndsWith), linkMainId(...10 ops...), linkedElementId(...10 ops...), name(...6 ops...), "
               "hasLinkSource, hasLinkSourceNot, missingVariant, profileStretchHeight(Not/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual), "
               "profileStretchWidth(...), profileNominalHeight(...), profileTotalHeight(...), profileTotalWidth(...), profileNominalWidth(...), "
               "profileParameter ({name, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual}), "
               "customPropertyBool ({name, group, equals|hasDefaultValue|hasCustomValue|isAvailable|isNotAvailable|isUndefined|isNotUndefined}), "
               "customPropertyString ({name, group, equals|notEquals|hasDefaultValue|...}), "
               "customPropertyNumber ({name, group, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual|hasDefaultValue|...}), "
               "customPropertyInteger ({name, group, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual|hasDefaultValue|...}), "
               "mepDiameter(Not/LessThan/GreaterThan/LessOrEqual/GreaterOrEqual), mepDiameter2(...), mepDN(...), mepDN2(...), mepHeight(...), "
               "mepHeight2(...), mepWidth(...), mepWidth2(...), mepAngle(...), mepCustomAngle(...), mepInsulationThickness(...), mepLength(...), "
               "mepDescription(Is/IsNot/Contains/NotContains/StartsWith/EndsWith), mepMaterialName(...6 ops...).";
    return false;
}

// Generates a criterionXML string (in ArchiCAD's own reverse-engineered format) from a structured
// GraphicalOverrideCriterion ObjectState. Returns false (with outError set) if the input uses an
// unsupported/unrecognized shape, rather than risk silently generating XML that matches nothing.
static bool CriterionToXML (const GS::ObjectState& criterionOS, GS::UniString& outXML, GS::UniString& outError)
{
    // All property-based leaves (classification, layer*, missingAttributes, ...) are always emitted
    // bare now (no per-leaf ModelElementCriterion pairing) - a bare PropertyCriterion is rejected by
    // ArchiCAD only when there is no type-establishing condition (elementType/all3DTypes/all2DTypes)
    // ANYWHERE in the whole rule; when there is one, it doesn't matter how deeply nested it is
    // relative to the property leaves, one shared ModelElementCriterion (or the real ElemTypeCriterion
    // already present) is enough for the entire rule. Confirmed live: putting more than one
    // ModelElementCriterion ("Type d'element est Types 3D") in the same rule corrupts the criteria
    // editor's rendering of *other* rows in that rule (not just the duplicated one) - so this must be
    // injected at most once, globally, rather than once per property leaf as earlier versions did.
    // Stylo Texte/Police fields (see AllLeavesAreTextPenOrFont) never need the default 3D-model-
    // element anchor - confirmed live that BOTH need the "Types 2D" (DrawingElementCriteiron)
    // anchor instead when otherwise unanchored (Police outright fails pairing with Types 3D;
    // Stylo Texte fails with no anchor at all, even though it happened to succeed once in an
    // earlier, buggy version of this check that silently fell through to Types 3D by accident).
    const bool hasAnyAnchor        = HasAndReachableAnchor (criterionOS);
    const bool needsFont2DAnchor   = !hasAnyAnchor && AllLeavesAreTextPenOrFont (criterionOS);
    const bool hasAnchor = hasAnyAnchor;

    GS::UniString rootNodeXML;
    bool rootIsSelfRooting = false;
    if (!EmitCriterionNode (criterionOS, rootNodeXML, outError, rootIsSelfRooting))
        return false;

    GS::UniString rootCompositeXML;
    if (hasAnchor) {
        // CriterionExpression's single child is always a CompositeCriterion. A bare leaf (a single
        // elementType/all3DTypes/all2DTypes with no explicit and/or) represents just one "row" and
        // still needs one more wrapping level to become that root (confirmed against the DevKit's
        // single-ElemType example: root always has CriteriaCount=1 there). An explicit "and"/"or"
        // already returns a real, multi-item CompositeCriterion that IS the root by itself (confirmed
        // live, byte-for-byte, against a hand-corrected "ElemType AND property" rule) - wrapping it
        // again produced XML the criteria editor accepted without error but silently failed to render.
        rootCompositeXML = rootIsSelfRooting
            ? rootNodeXML
            : GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
              + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>2</LogicalOperator><CriteriaCount>1</CriteriaCount>"
              + rootNodeXML
              + "</CompositeCriterion>";
    } else {
        // No type-establishing condition anywhere - inject exactly one shared ModelElementCriterion,
        // AND-combined with the whole rest of the tree. rootNodeXML is always a self-contained
        // ClassGuid+Tag pair regardless of whether it was self-rooting, so it can be used directly as
        // the anchor's AND-sibling without any extra wrapping (confirmed byte-for-byte against the
        // real "Classification - X" rules, which are exactly this shape: root CompositeCriterion with
        // CriteriaCount=2 directly containing the ModelElement pair and the property leaf pair).
        rootCompositeXML = GS::UniString ("<ClassGuid>") + kCompositeClassGuid + "</ClassGuid>"
            + "<CompositeCriterion Mv=\"2\" Sv=\"1\"><LogicalOperator>2</LogicalOperator><CriteriaCount>2</CriteriaCount>"
            + WrapAsTrivialGroup (needsFont2DAnchor ? DrawingElementLeafXML () : ModelElementLeafXML ())
            + rootNodeXML
            + "</CompositeCriterion>";
    }

    outXML = GS::UniString ("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>")
           + "<CriterionExpression Mv=\"2\" Sv=\"2\"><TextMatchType>1</TextMatchType><Size>1</Size>"
           + rootCompositeXML
           + "</CriterionExpression>";
    return true;
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

GS::Optional<GS::UniString> GetGraphicalOverrideCombinationsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetGraphicalOverrideRuleGroupsCommand::GetRawResponseSchema () const
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
    CommandBase (CommonSchema::Used)
{}

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
                        "ruleId":       { "type": "string" },
                        "name":         { "type": "string" },
                        "criterionXML": { "type": "string", "description": "XML defining when this rule applies. Export a rule from ArchiCAD to obtain the format." },
                        "style":        { "$ref": "#/GraphicalOverrideRuleStyle" }
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

GS::Optional<GS::UniString> CreateGraphicalOverrideRuleGroupsCommand::GetRawResponseSchema () const
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
    CommandBase (CommonSchema::Used)
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
                        "criterionXML": { "type": "string", "description": "XML defining when this rule applies. Use GetGraphicalOverrideRules to obtain example XML. Ignored if 'criterion' is also given. Leave both out (or empty) to match every element." },
                        "criterion":    { "$ref": "#/GraphicalOverrideCriterion", "description": "Structured alternative to criterionXML - generated into the equivalent XML internally. Takes precedence over criterionXML if both are given." },
                        "style":        { "$ref": "#/GraphicalOverrideRuleStyle", "description": "The graphical override style." }
                    },
                    "additionalProperties": false,
                    "required": [ "name", "ruleGroupId", "style" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "rules" ]
    })";
}

GS::Optional<GS::UniString> CreateGraphicalOverrideRulesCommand::GetRawResponseSchema () const
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
            GS::ObjectState styleOS, criterionOS;

            if (!ruleInput.Get ("name", name) || !ruleInput.Get ("ruleGroupId", ruleGroupIdStr) ||
                !ruleInput.Get ("style", styleOS)) {
                resultsAdder (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required field."));
                GS::ObjectState emptyRule; emptyRule.Add ("ruleId", GS::UniString ()); emptyRule.Add ("name", GS::UniString ()); rulesAdder (emptyRule);
                continue;
            }

            ruleInput.Get ("criterionXML", criterionXML);
            if (ruleInput.Get ("criterion", criterionOS)) {
                GS::UniString generatedXML, criterionError;
                if (!CriterionToXML (criterionOS, generatedXML, criterionError)) {
                    resultsAdder (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'criterion' for rule '" + name + "': " + criterionError));
                    GS::ObjectState emptyRule; emptyRule.Add ("ruleId", GS::UniString ()); emptyRule.Add ("name", GS::UniString ()); rulesAdder (emptyRule);
                    continue;
                }
                criterionXML = generatedXML;
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

GS::Optional<GS::UniString> CreateGraphicalOverrideCombinationsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> DeleteGraphicalOverrideRulesCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> DeleteGraphicalOverrideRuleGroupsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> DeleteGraphicalOverrideCombinationsCommand::GetRawResponseSchema () const
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

#endif // ServerMainVers_2700
