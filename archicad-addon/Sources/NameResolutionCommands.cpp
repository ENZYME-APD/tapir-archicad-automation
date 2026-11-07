#include "NameResolutionCommands.hpp"
#include "MigrationHelper.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

// ResolveNames (#2259) checks words taken from a user's question against the names in the open
// project (stories, attributes, element types, properties and classifications), so that a script or an AI assistant does not run a command with a guessed name. The
// matching is lexical and deterministic: an exact match is "found"; otherwise the nearest names are
// ranked by how they differ (case, accents, spacing and separators only; the same words; one name
// containing the other; a few typed characters). It only reads the project.

namespace {

enum class NameKind
{
    Story,
    Layer,
    LayerCombination,
    BuildingMaterial,
    Composite,
    Profile,
    Surface,
    Fill,
    Line,
    ZoneCategory,
    ElementType,
    Property,
    Classification
};

struct NameKindInfo
{
    NameKind        kind;
    const char*     name;
    API_AttrTypeID  attributeType;
};

// The order is the order of the kinds in the schema and among close matches of equal score.
const NameKindInfo NameKinds[] = {
    { NameKind::Story,            "Story",            API_ZombieAttrID },
    { NameKind::Layer,            "Layer",            API_LayerID },
    { NameKind::LayerCombination, "LayerCombination", API_LayerCombID },
    { NameKind::BuildingMaterial, "BuildingMaterial", API_BuildingMaterialID },
    { NameKind::Composite,        "Composite",        API_CompWallID },
    { NameKind::Profile,          "Profile",          API_ProfileID },
    { NameKind::Surface,          "Surface",          API_MaterialID },
    { NameKind::Fill,             "Fill",             API_FilltypeID },
    { NameKind::Line,             "Line",             API_LinetypeID },
    { NameKind::ZoneCategory,     "ZoneCategory",     API_ZoneCatID },
    { NameKind::ElementType,      "ElementType",      API_ZombieAttrID },
    { NameKind::Property,         "Property",         API_ZombieAttrID },
    { NameKind::Classification,   "Classification",   API_ZombieAttrID }
};

const double MinCloseMatchScore = 0.6;
const Int32 DefaultMaxCloseMatches = 5;

// A name broken into lowercase words without accents. A word ends at a separator (anything that is
// not a letter or digit), between a lowercase and an uppercase letter (FireRating) and between a
// letter and a digit (Level2). Leading zeros of a number are dropped, so "02" and "2" are the same word.
struct NormalizedName
{
    std::u32string              compact;    // the words joined without spaces
    std::vector<std::u32string> words;
};

struct Candidate
{
    const NameKindInfo*         kindInfo = nullptr;
    GS::UniString               name;
    GS::Array<GS::UniString>    otherNames;  // matched as well, e.g. the API name of an element type
    std::vector<NormalizedName> normalized;  // of name and otherNames

    // Kind-specific identification.
    GS::Optional<short>         storyIndex;
    GS::Optional<double>        storyLevel;
    GS::Optional<API_Guid>      attributeGuid;
    GS::Optional<Int32>         attributeIndex;
    GS::UniString               elementType;

    // A property definition.
    GS::Optional<API_Guid>      propertyGuid;
    GS::Optional<API_Guid>      propertyGroupGuid;
    GS::UniString               propertyGroupName;
    const char*                 propertyType = nullptr;
    const char*                 propertyCollectionType = nullptr;
    const char*                 propertyValueType = nullptr;
    const char*                 propertyMeasureType = nullptr;
    bool                        propertyIsEditable = false;
    bool                        hasPropertyAvailability = false;   // only custom properties have one
    GS::Array<API_Guid>         propertyAvailability;

    // A classification system, or an item of one (then classificationItemGuid is set).
    GS::Optional<API_Guid>      classificationSystemGuid;
    GS::UniString               classificationSystemName;
    GS::Optional<API_Guid>      classificationItemGuid;
    GS::UniString               classificationItemCode;
};

struct CloseMatch
{
    const Candidate*    candidate = nullptr;
    const char*         match = nullptr;
    double              score = 0.0;
};

std::u32string DecodeUtf8 (const GS::UniString& str)
{
    const std::string bytes (str.ToCStr (CC_UTF8).Get ());
    std::u32string result;
    for (size_t i = 0; i < bytes.size ();) {
        const unsigned char c = static_cast<unsigned char> (bytes[i]);
        size_t length = 1;
        char32_t codePoint = c;
        if (c >= 0xF0) {
            length = 4;
            codePoint = c & 0x07;
        } else if (c >= 0xE0) {
            length = 3;
            codePoint = c & 0x0F;
        } else if (c >= 0xC0) {
            length = 2;
            codePoint = c & 0x1F;
        }
        if (i + length > bytes.size ()) {
            break;
        }
        for (size_t k = 1; k < length; ++k) {
            codePoint = (codePoint << 6) | (static_cast<unsigned char> (bytes[i + k]) & 0x3F);
        }
        result.push_back (codePoint);
        i += length;
    }
    return result;
}

bool IsDigit (char32_t c)
{
    return c >= U'0' && c <= U'9';
}

bool IsLetterOrDigit (char32_t c)
{
    if (c < 0x80) {
        return IsDigit (c) || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
    }
    if (c < 0xC0) {
        return c == 0xAA || c == 0xB5 || c == 0xBA;
    }
    if (c == 0xD7 || c == 0xF7) {
        return false;
    }
    if ((c >= 0x2000 && c <= 0x2BFF) || (c >= 0x3000 && c <= 0x303F) || (c >= 0xFF00 && c <= 0xFF0F)) {
        return false;   // punctuation, symbols, arrows, CJK punctuation, fullwidth punctuation
    }
    return true;
}

bool IsUpperCase (char32_t c)
{
    return (c >= U'A' && c <= U'Z') ||
        (c >= 0xC0 && c <= 0xDE && c != 0xD7) ||
        (c >= 0x391 && c <= 0x3A9) ||
        (c >= 0x400 && c <= 0x42F);
}

// The base letters of U+0100 to U+017F (Latin Extended-A), upper and lower case alike.
const char LatinExtendedABase[] =
    "aaaaaa" "cccccccc" "dddd" "eeeeeeeeee" "gggggggg" "hhhh" "iiiiiiiiii" "ii" "jj" "kkk"
    "llllllllll" "nnnnnnnnn" "oooooo" "oo" "rrrrrr" "ssssssss" "tttttt" "uuuuuuuuuuuu" "ww"
    "yyy" "zzzzzz" "s";
static_assert (sizeof (LatinExtendedABase) - 1 == 0x80, "one base letter for every character of Latin Extended-A");

// Appends c in lower case and without its accent.
void AppendFolded (char32_t c, std::u32string& out)
{
    if (c >= U'A' && c <= U'Z') {
        out.push_back (c + 0x20);
        return;
    }
    if (c >= 0xC0 && c <= 0xFF) {
        // U+00C0 to U+00FF, upper and lower case at the same offset from 0xC0 and 0xE0.
        static const char Latin1Base[] = "aaaaaaaceeeeiiiidnooooo-ouuuuyts";
        static_assert (sizeof (Latin1Base) - 1 == 0x20, "one base letter for every character of U+00C0 to U+00DF");
        const char32_t offset = (c >= 0xE0 ? c - 0xE0 : c - 0xC0);
        switch (c) {
            case 0xC6: case 0xE6: out += U"ae"; return;
            case 0xDF: out += U"ss"; return;
            case 0xDE: case 0xFE: out += U"th"; return;
            case 0xFF: out.push_back (U'y'); return;
            default: break;
        }
        out.push_back (static_cast<char32_t> (Latin1Base[offset]));
        return;
    }
    if (c >= 0x100 && c <= 0x17F) {
        if (c == 0x152 || c == 0x153) {
            out += U"oe";
            return;
        }
        out.push_back (static_cast<char32_t> (LatinExtendedABase[c - 0x100]));
        return;
    }
    if ((c >= 0x391 && c <= 0x3A9) || (c >= 0x410 && c <= 0x42F)) {
        out.push_back (c + 0x20);
        return;
    }
    if (c >= 0x400 && c <= 0x40F) {
        out.push_back (c + 0x50);
        return;
    }
    out.push_back (c);
}

NormalizedName NormalizeName (const GS::UniString& name)
{
    NormalizedName result;
    std::u32string word;
    bool wordIsNumber = false;
    bool previousWasLower = false;

    const auto flush = [&] () {
        if (word.empty ()) {
            return;
        }
        if (wordIsNumber) {
            const size_t firstNonZero = word.find_first_not_of (U'0');
            word = (firstNonZero == std::u32string::npos) ? std::u32string (U"0") : word.substr (firstNonZero);
        }
        result.compact += word;
        result.words.push_back (word);
        word.clear ();
    };

    for (const char32_t c : DecodeUtf8 (name)) {
        if (!IsLetterOrDigit (c)) {
            flush ();
            previousWasLower = false;
            continue;
        }
        const bool isDigit = IsDigit (c);
        const bool isUpper = IsUpperCase (c);
        if (!word.empty () && (isDigit != wordIsNumber || (previousWasLower && isUpper))) {
            flush ();
        }
        wordIsNumber = isDigit;
        previousWasLower = !isDigit && !isUpper;
        AppendFolded (c, word);
    }
    flush ();
    return result;
}

size_t EditDistance (const std::u32string& a, const std::u32string& b)
{
    std::vector<size_t> previous (b.size () + 1);
    std::vector<size_t> current (b.size () + 1);
    for (size_t j = 0; j <= b.size (); ++j) {
        previous[j] = j;
    }
    for (size_t i = 1; i <= a.size (); ++i) {
        current[0] = i;
        for (size_t j = 1; j <= b.size (); ++j) {
            const size_t substitution = previous[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
            current[j] = std::min ({ previous[j] + 1, current[j - 1] + 1, substitution });
        }
        std::swap (previous, current);
    }
    return previous[b.size ()];
}

// How close two normalized names are, from 0 to 1, and how they match:
//   normalized  1.0        the same except for case, accents, spacing and separators
//   words       0.7-0.9    every word of one is a word of the other ("Fire Rating" - "Fire Rating EI")
//               0.6-0.8    at least half of all their words are shared
//   contains    0.6-0.9    one contains the other, at least 3 characters ("Wall" - "Walls Exterior")
//   similar     0.63-0.9   a few characters differ ("Concret" - "Concrete")
double ScoreMatch (const NormalizedName& query, const NormalizedName& candidate, const char*& match)
{
    match = nullptr;
    if (query.compact.empty () || candidate.compact.empty ()) {
        return 0.0;
    }
    if (query.compact == candidate.compact) {
        match = "normalized";
        return 1.0;
    }

    double best = 0.0;
    const auto consider = [&] (double score, const char* matchName) {
        if (score > best) {
            best = score;
            match = matchName;
        }
    };

    const std::set<std::u32string> queryWords (query.words.begin (), query.words.end ());
    const std::set<std::u32string> candidateWords (candidate.words.begin (), candidate.words.end ());
    size_t shared = 0;
    for (const std::u32string& word : queryWords) {
        if (candidateWords.count (word) > 0) {
            ++shared;
        }
    }
    if (shared > 0) {
        const size_t smaller = std::min (queryWords.size (), candidateWords.size ());
        const size_t larger = std::max (queryWords.size (), candidateWords.size ());
        if (shared == smaller) {
            consider (0.7 + 0.2 * static_cast<double> (smaller) / static_cast<double> (larger), "words");
        } else {
            const double jaccard = static_cast<double> (shared) / static_cast<double> (queryWords.size () + candidateWords.size () - shared);
            if (jaccard >= 0.5) {
                consider (0.4 + 0.4 * jaccard, "words");
            }
        }
    }

    const std::u32string& shorter = query.compact.size () <= candidate.compact.size () ? query.compact : candidate.compact;
    const std::u32string& longer = query.compact.size () <= candidate.compact.size () ? candidate.compact : query.compact;
    if (shorter.size () >= 3 && longer.find (shorter) != std::u32string::npos) {
        consider (0.6 + 0.3 * static_cast<double> (shorter.size ()) / static_cast<double> (longer.size ()), "contains");
    }

    const double similarity = 1.0 - static_cast<double> (EditDistance (query.compact, candidate.compact)) / static_cast<double> (longer.size ());
    if (similarity >= 0.7) {
        consider (0.9 * similarity, "similar");
    }

    return best;
}

const NameKindInfo* FindNameKind (const GS::UniString& kindName)
{
    for (const NameKindInfo& info : NameKinds) {
        if (kindName == info.name) {
            return &info;
        }
    }
    return nullptr;
}

void AddCandidate (Candidate&& candidate, std::vector<Candidate>& candidates)
{
    candidate.normalized.push_back (NormalizeName (candidate.name));
    for (const GS::UniString& otherName : candidate.otherNames) {
        candidate.normalized.push_back (NormalizeName (otherName));
    }
    candidates.push_back (std::move (candidate));
}

GSErrCode LoadStories (const NameKindInfo& kindInfo, std::vector<Candidate>& candidates)
{
    API_StoryInfo storyInfo = {};
    const GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    if (err == NoError && storyInfo.data != nullptr) {
        const short storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;
        for (short i = 0; i < storyCount; ++i) {
            const API_StoryType& story = (*storyInfo.data)[i];
            Candidate candidate;
            candidate.kindInfo = &kindInfo;
            candidate.name = GS::UniString (story.uName);
            candidate.storyIndex = story.index;
            candidate.storyLevel = story.level;
            AddCandidate (std::move (candidate), candidates);
        }
    }
    BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
    return err;
}

// API_Attr_Head::name is the legacy 8-bit name; the Unicode name is only handed out through
// uniStringNamePtr when it points at storage before the Get call (as GetAttributeName in
// AttributeCommands.cpp reads it).
GS::UniString ReadAttributeName (const API_Attr_Head& header)
{
    GS::UniString name;
    API_Attribute attr = {};
    attr.header.typeID = header.typeID;
    attr.header.index = header.index;
    attr.header.guid = header.guid;
    attr.header.uniStringNamePtr = &name;
    const bool found = (ACAPI_Attribute_Get (&attr) == NoError);
    DisposeAttribute (attr);
    return found ? name : GS::UniString (header.name);
}

GSErrCode LoadAttributes (const NameKindInfo& kindInfo, std::vector<Candidate>& candidates)
{
    GS::Array<API_Attribute> attributes;
    const GSErrCode err = ACAPI_Attribute_GetAttributesByType (kindInfo.attributeType, attributes);
    for (API_Attribute& attr : attributes) {
        Candidate candidate;
        candidate.kindInfo = &kindInfo;
        candidate.name = ReadAttributeName (attr.header);
        candidate.attributeGuid = attr.header.guid;
        candidate.attributeIndex = GetAttributeIndex (attr.header.index);
        DisposeAttribute (attr);
        AddCandidate (std::move (candidate), candidates);
    }
    return err;
}

// The element types are matched by their API name (the elementType of the commands) and, from
// Archicad 27, by the name the Archicad user interface shows, in the language of Archicad.
GSErrCode LoadElementTypes (const NameKindInfo& kindInfo, std::vector<Candidate>& candidates)
{
    for (Int32 typeId = API_ZombieElemID + 1; typeId <= API_LastElemType; ++typeId) {
        const GS::UniString apiName = GetElementTypeNonLocalizedName (static_cast<API_ElemTypeID> (typeId));
        if (apiName == "Unknown") {
            continue;
        }
        Candidate candidate;
        candidate.kindInfo = &kindInfo;
        candidate.name = apiName;
        candidate.elementType = apiName;
#ifdef ServerMainVers_2700
        GS::UniString localizedName;
        if (ACAPI_Element_GetElemTypeName (API_ElemType (static_cast<API_ElemTypeID> (typeId)), localizedName) == NoError &&
            !localizedName.IsEmpty () && localizedName != apiName) {
            candidate.name = localizedName;
            candidate.otherNames.Push (apiName);
        }
#endif
        AddCandidate (std::move (candidate), candidates);
    }
    return NoError;
}

GS::UniString JoinNames (const GS::UniString& first, const GS::UniString& second)
{
    GS::UniString joined (first);
    joined += " ";
    joined += second;
    return joined;
}

const char* PropertyDefinitionTypeName (API_PropertyDefinitionType type)
{
    switch (type) {
        case API_PropertyStaticBuiltInDefinitionType:  return "StaticBuiltIn";
        case API_PropertyDynamicBuiltInDefinitionType: return "DynamicBuiltIn";
        case API_PropertyCustomDefinitionType:         return "Custom";
        default:                                       return nullptr;
    }
}

const char* PropertyCollectionTypeName (API_PropertyCollectionType type)
{
    switch (type) {
        case API_PropertySingleCollectionType:                      return "Single";
        case API_PropertyListCollectionType:                        return "List";
        case API_PropertySingleChoiceEnumerationCollectionType:     return "SingleChoiceEnumeration";
        case API_PropertyMultipleChoiceEnumerationCollectionType:   return "MultipleChoiceEnumeration";
        default:                                                    return "Undefined";
    }
}

const char* PropertyValueTypeName (API_VariantType type)
{
    switch (type) {
        case API_PropertyIntegerValueType:  return "Integer";
        case API_PropertyRealValueType:     return "Real";
        case API_PropertyStringValueType:   return "String";
        case API_PropertyBooleanValueType:  return "Boolean";
        case API_PropertyGuidValueType:     return "Guid";
        default:                            return "Undefined";
    }
}

// The measure types GetAllProperties names; the index reference types of some built-in properties
// (newer Archicad versions) are left out of the result.
const char* PropertyMeasureTypeName (API_PropertyMeasureType type)
{
    switch (type) {
        case API_PropertyUndefinedMeasureType:  return "Undefined";
        case API_PropertyDefaultMeasureType:    return "Default";
        case API_PropertyLengthMeasureType:     return "Length";
        case API_PropertyAreaMeasureType:       return "Area";
        case API_PropertyVolumeMeasureType:     return "Volume";
        case API_PropertyAngleMeasureType:      return "Angle";
        default:                                return nullptr;
    }
}

// Every property definition of the project, built-in and custom. Besides its name, a property is
// matched by its group and name together ("Pset_DoorCommon FireRating"), so a phrase naming both
// finds it.
GSErrCode LoadProperties (const NameKindInfo& kindInfo, std::vector<Candidate>& candidates)
{
    GS::Array<API_PropertyGroup> groups;
    const GSErrCode err = ACAPI_Property_GetPropertyGroups (groups);
    if (err != NoError) {
        return err;
    }
    for (const API_PropertyGroup& group : groups) {
        GS::Array<API_PropertyDefinition> definitions;
        if (ACAPI_Property_GetPropertyDefinitions (group.guid, definitions) != NoError) {
            continue;
        }
        for (const API_PropertyDefinition& definition : definitions) {
            if (definition.name.IsEmpty ()) {
                continue;
            }
            Candidate candidate;
            candidate.kindInfo = &kindInfo;
            candidate.name = definition.name;
            if (!group.name.IsEmpty ()) {
                candidate.otherNames.Push (JoinNames (group.name, definition.name));
            }
            candidate.propertyGuid = definition.guid;
            candidate.propertyGroupGuid = group.guid;
            candidate.propertyGroupName = group.name;
            candidate.propertyType = PropertyDefinitionTypeName (definition.definitionType);
            candidate.propertyCollectionType = PropertyCollectionTypeName (definition.collectionType);
            candidate.propertyValueType = PropertyValueTypeName (definition.valueType);
            candidate.propertyMeasureType = PropertyMeasureTypeName (definition.measureType);
            candidate.propertyIsEditable = definition.canValueBeEditable;
            if (definition.definitionType == API_PropertyCustomDefinitionType) {
                candidate.hasPropertyAvailability = true;
                candidate.propertyAvailability = definition.availability;
            }
            AddCandidate (std::move (candidate), candidates);
        }
    }
    return NoError;
}

void LoadClassificationItems (const NameKindInfo& kindInfo, const API_ClassificationSystem& system,
    const GS::Array<API_ClassificationItem>& items, std::vector<Candidate>& candidates)
{
    for (const API_ClassificationItem& item : items) {
        Candidate candidate;
        candidate.kindInfo = &kindInfo;
        candidate.name = item.name.IsEmpty () ? item.id : item.name;
        if (!item.id.IsEmpty () && !item.name.IsEmpty ()) {
            candidate.otherNames.Push (item.id);
            candidate.otherNames.Push (JoinNames (item.id, item.name));
        }
        candidate.classificationSystemGuid = system.guid;
        candidate.classificationSystemName = system.name;
        candidate.classificationItemGuid = item.guid;
        candidate.classificationItemCode = item.id;
        AddCandidate (std::move (candidate), candidates);

        GS::Array<API_ClassificationItem> children;
        if (ACAPI_Classification_GetClassificationItemChildren (item.guid, children) == NoError) {
            LoadClassificationItems (kindInfo, system, children, candidates);
        }
    }
}

// Every classification system (by its name, and by its name and version together, "Uniclass 2015")
// and every item of each system, at every depth (by its name, by its ID such as "Ss_25_10" and by
// both together).
GSErrCode LoadClassifications (const NameKindInfo& kindInfo, std::vector<Candidate>& candidates)
{
    GS::Array<API_ClassificationSystem> systems;
    const GSErrCode err = ACAPI_Classification_GetClassificationSystems (systems);
    if (err != NoError) {
        return err;
    }
    for (const API_ClassificationSystem& system : systems) {
        Candidate candidate;
        candidate.kindInfo = &kindInfo;
        candidate.name = system.name;
        if (!system.editionVersion.IsEmpty ()) {
            candidate.otherNames.Push (JoinNames (system.name, system.editionVersion));
        }
        candidate.classificationSystemGuid = system.guid;
        candidate.classificationSystemName = system.name;
        AddCandidate (std::move (candidate), candidates);

        GS::Array<API_ClassificationItem> roots;
        if (ACAPI_Classification_GetClassificationSystemRootItems (system.guid, roots) == NoError) {
            LoadClassificationItems (kindInfo, system, roots, candidates);
        }
    }
    return NoError;
}

GSErrCode LoadCandidates (const NameKindInfo& kindInfo, std::vector<Candidate>& candidates)
{
    switch (kindInfo.kind) {
        case NameKind::Story:           return LoadStories (kindInfo, candidates);
        case NameKind::ElementType:     return LoadElementTypes (kindInfo, candidates);
        case NameKind::Property:        return LoadProperties (kindInfo, candidates);
        case NameKind::Classification:  return LoadClassifications (kindInfo, candidates);
        default:                        return LoadAttributes (kindInfo, candidates);
    }
}

bool IsExactMatch (const GS::UniString& word, const Candidate& candidate)
{
    if (word == candidate.name) {
        return true;
    }
    for (const GS::UniString& otherName : candidate.otherNames) {
        if (word == otherName) {
            return true;
        }
    }
    return false;
}

GS::ObjectState CreateMatchObjectState (const Candidate& candidate, const char* match, double score)
{
    GS::ObjectState matchOS;
    matchOS.Add ("kind", candidate.kindInfo->name);
    matchOS.Add ("name", candidate.name);
    matchOS.Add ("match", match);
    matchOS.Add ("score", std::round (score * 100.0) / 100.0);
    if (candidate.storyIndex.HasValue ()) {
        matchOS.Add ("storyIndex", candidate.storyIndex.Get ());
    }
    if (candidate.storyLevel.HasValue ()) {
        matchOS.Add ("level", candidate.storyLevel.Get ());
    }
    if (candidate.attributeGuid.HasValue ()) {
        matchOS.Add ("attributeId", CreateGuidObjectState (candidate.attributeGuid.Get ()));
    }
    if (candidate.attributeIndex.HasValue ()) {
        matchOS.Add ("attributeIndex", candidate.attributeIndex.Get ());
    }
    if (!candidate.elementType.IsEmpty ()) {
        matchOS.Add ("elementType", candidate.elementType);
    }
    if (candidate.propertyGuid.HasValue ()) {
        matchOS.Add ("propertyId", CreateGuidObjectState (candidate.propertyGuid.Get ()));
        matchOS.Add ("propertyGroupId", CreateGuidObjectState (candidate.propertyGroupGuid.Get ()));
        matchOS.Add ("propertyGroupName", candidate.propertyGroupName);
        if (candidate.propertyType != nullptr) {
            matchOS.Add ("propertyType", candidate.propertyType);
        }
        matchOS.Add ("propertyCollectionType", candidate.propertyCollectionType);
        matchOS.Add ("propertyValueType", candidate.propertyValueType);
        if (candidate.propertyMeasureType != nullptr) {
            matchOS.Add ("propertyMeasureType", candidate.propertyMeasureType);
        }
        matchOS.Add ("propertyIsEditable", candidate.propertyIsEditable);
        if (candidate.hasPropertyAvailability) {
            const auto& availabilityList = matchOS.AddList<GS::ObjectState> ("availability");
            for (const API_Guid& itemGuid : candidate.propertyAvailability) {
                availabilityList (CreateIdObjectState ("classificationItemId", itemGuid));
            }
        }
    }
    if (candidate.classificationSystemGuid.HasValue ()) {
        matchOS.Add ("classificationSystemId", CreateGuidObjectState (candidate.classificationSystemGuid.Get ()));
        matchOS.Add ("classificationSystemName", candidate.classificationSystemName);
    }
    if (candidate.classificationItemGuid.HasValue ()) {
        matchOS.Add ("classificationItemId", CreateGuidObjectState (candidate.classificationItemGuid.Get ()));
        matchOS.Add ("classificationItemCode", candidate.classificationItemCode);
    }
    return matchOS;
}

}

ResolveNamesCommand::ResolveNamesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ResolveNamesCommand::GetName () const
{
    return "ResolveNames";
}

GS::Optional<GS::UniString> ResolveNamesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "names": {
                "type": "array",
                "description": "The words or phrases to look up in the open project, for example taken from a user's question.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The word or phrase.",
                            "minLength": 1
                        },
                        "kind": {
                            "$ref": "#/NameKind",
                            "description": "The kind of item the word names. When it is left out, every kind is searched."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name"
                    ]
                },
                "minItems": 1
            },
            "maxCloseMatches": {
                "type": "integer",
                "description": "The most close matches returned for one word, best first. The default is 5.",
                "minimum": 1,
                "maximum": 100
            }
        },
        "additionalProperties": false,
        "required": [
            "names"
        ]
    })";
}

GS::Optional<GS::UniString> ResolveNamesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "results": {
                "type": "array",
                "description": "One result for each of the given names, in the same order.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The given word or phrase."
                        },
                        "status": {
                            "type": "string",
                            "description": "found: an item of the project has exactly this name (see found). closeMatches: no item has exactly this name, but some are close (see closeMatches). notFound: nothing in the project is close to it; the project has no such item, so it must not be guessed.",
                            "enum": [
                                "found",
                                "closeMatches",
                                "notFound"
                            ]
                        },
                        "found": {
                            "type": "array",
                            "description": "The items named exactly so (the same characters), with match 'exact'. More than one when items of different kinds, or several stories, have the name.",
                            "items": {
                                "$ref": "#/NameMatch"
                            }
                        },
                        "closeMatches": {
                            "type": "array",
                            "description": "When nothing is named exactly so: the nearest items, best first. Empty when the status is found or notFound.",
                            "items": {
                                "$ref": "#/NameMatch"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "status",
                        "found",
                        "closeMatches"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "results"
        ]
    })";
}

GS::ObjectState ResolveNamesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> names;
    parameters.Get ("names", names);

    Int32 maxCloseMatches = DefaultMaxCloseMatches;
    if (parameters.Contains ("maxCloseMatches")) {
        parameters.Get ("maxCloseMatches", maxCloseMatches);
    }
    if (maxCloseMatches < 1) {
        maxCloseMatches = 1;
    }

    // The items of each kind are read from the project once, when a name first needs them.
    std::map<NameKind, std::vector<Candidate>> candidatesByKind;
    const auto getCandidates = [&] (const NameKindInfo& kindInfo, GSErrCode& err) -> const std::vector<Candidate>& {
        err = NoError;
        auto it = candidatesByKind.find (kindInfo.kind);
        if (it == candidatesByKind.end ()) {
            std::vector<Candidate> candidates;
            err = LoadCandidates (kindInfo, candidates);
            it = candidatesByKind.emplace (kindInfo.kind, std::move (candidates)).first;
        }
        return it->second;
    };

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("results");

    for (const GS::ObjectState& nameOS : names) {
        GS::UniString word;
        nameOS.Get ("name", word);

        std::vector<const NameKindInfo*> kinds;
        if (nameOS.Contains ("kind")) {
            GS::UniString kindName;
            nameOS.Get ("kind", kindName);
            const NameKindInfo* kindInfo = FindNameKind (kindName);
            if (kindInfo == nullptr) {
                return CreateErrorResponse (APIERR_BADPARS,
                    GS::UniString::Printf ("Invalid kind '%T' for the name '%T'.", kindName.ToPrintf (), word.ToPrintf ()));
            }
            kinds.push_back (kindInfo);
        } else {
            for (const NameKindInfo& kindInfo : NameKinds) {
                kinds.push_back (&kindInfo);
            }
        }

        const NormalizedName normalizedWord = NormalizeName (word);
        std::vector<const Candidate*> found;
        std::vector<CloseMatch> closeMatches;
        for (const NameKindInfo* kindInfo : kinds) {
            GSErrCode err = NoError;
            const std::vector<Candidate>& candidates = getCandidates (*kindInfo, err);
            if (err != NoError) {
                return CreateErrorResponse (err,
                    GS::UniString::Printf ("Failed to read the items of kind %s from the project.", kindInfo->name));
            }
            for (const Candidate& candidate : candidates) {
                if (IsExactMatch (word, candidate)) {
                    found.push_back (&candidate);
                    continue;
                }
                CloseMatch best;
                for (const NormalizedName& normalizedName : candidate.normalized) {
                    const char* match = nullptr;
                    const double score = ScoreMatch (normalizedWord, normalizedName, match);
                    if (match != nullptr && score > best.score) {
                        best.candidate = &candidate;
                        best.match = match;
                        best.score = score;
                    }
                }
                if (best.candidate != nullptr && best.score >= MinCloseMatchScore) {
                    closeMatches.push_back (best);
                }
            }
        }

        GS::ObjectState result;
        result.Add ("name", word);
        const auto& foundList = result.AddList<GS::ObjectState> ("found");
        const auto& closeMatchList = result.AddList<GS::ObjectState> ("closeMatches");
        if (!found.empty ()) {
            result.Add ("status", "found");
            for (const Candidate* candidate : found) {
                foundList (CreateMatchObjectState (*candidate, "exact", 1.0));
            }
        } else if (!closeMatches.empty ()) {
            result.Add ("status", "closeMatches");
            // Best first; candidates are in kind order and then in project order, which a stable
            // sort keeps for equal scores.
            std::stable_sort (closeMatches.begin (), closeMatches.end (), [] (const CloseMatch& a, const CloseMatch& b) {
                return a.score > b.score;
            });
            const size_t count = std::min (closeMatches.size (), static_cast<size_t> (maxCloseMatches));
            for (size_t i = 0; i < count; ++i) {
                closeMatchList (CreateMatchObjectState (*closeMatches[i].candidate, closeMatches[i].match, closeMatches[i].score));
            }
        } else {
            result.Add ("status", "notFound");
        }
        results (result);
    }

    return response;
}
