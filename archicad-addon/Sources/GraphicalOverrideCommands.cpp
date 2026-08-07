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
static GS::UniString ClassificationCriterionXML (const GS::UniString& itemId, bool isNot)
{
    return GS::UniString ("<ClassGuid>") + kPropertyClassGuid + "</ClassGuid>"
         + "<PropertyCriterion Mv=\"6\" Sv=\"0\">"
         +     "<VBEF::ConditionIO Mv=\"2\" Sv=\"0\">"
         +         "<PropertyDefinitionUserId Version=\"2\">"
         +             "<PrimaryId>1</PrimaryId><HasGuidId>false</HasGuidId>"
         +             "<Guid>00000000-0000-0000-0000-000000000000</Guid>"
         +             "<HasNameId>true</HasNameId><Name>Classification Archicad - 2.0</Name>"
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
static bool TryEmitNumField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "", "Not", "LessThan", "GreaterThan", "LessOrEqual", "GreaterOrEqual" };
    static const int   kOps[]      = { 0,   1,     2,          3,             4,             5 };
    double value = 0.0;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (NumPropertyCriterionXML (propertyGuid, value, kOps[i], kPositioningPropertyGroupGuid));
            return true;
        }
    }
    return false;
}

// Same idea as TryEmitNumField, for the 6-suffix string-comparison field family emitted as
// StringListVariant (Surface et Materiaux category).
static bool TryEmitStringListField (const GS::ObjectState& node, const char* fieldPrefix, const char* propertyGuid, GS::UniString& outXML)
{
    static const char* kSuffixes[] = { "Is", "IsNot", "Contains", "NotContains", "StartsWith", "EndsWith" };
    static const int   kOps[]      = { 0,     1,       6,          7,             8,            9 };
    GS::UniString value;
    for (int i = 0; i < 6; ++i) {
        const GS::String fieldName = GS::String (fieldPrefix) + kSuffixes[i];
        if (node.Get (fieldName, value)) {
            outXML = WrapAsTrivialGroup (StringListCriterionXML (propertyGuid, value, kOps[i], kSurfaceMaterialPropertyGroupGuid));
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
        outXML = WrapAsTrivialGroup (ClassificationCriterionXML (itemId, isNot));
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
               "customPropertyInteger ({name, group, equals|notEquals|lessThan|greaterThan|lessOrEqual|greaterOrEqual|hasDefaultValue|...}).";
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
                        "criterionXML": { "type": "string", "description": "XML defining when this rule applies. Use GetGraphicalOverrideRules to obtain example XML. Ignored if 'criterion' is also given. Leave both out (or empty) to match every element." },
                        "criterion":    { "$ref": "#/GraphicalOverrideCriterion", "description": "Structured alternative to criterionXML - generated into the equivalent XML internally. Takes precedence over criterionXML if both are given." },
                        "style":        { "type": "object", "description": "The graphical override style. Use GetGraphicalOverrideRules to obtain the structure." }
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

#endif // ServerMainVers_2700
