#include "ProjectHealthCommands.hpp"
#include "AttributeCommands.hpp"
#include "LibraryCommands.hpp"
#include "MigrationHelper.hpp"
#include "Location.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

GS::ObjectState CreateUnknownItem (const GS::UniString& reason)
{
    GS::ObjectState item;
    item.Add ("known", false);
    item.Add ("reason", reason);
    return item;
}

// The message of a response built with CreateErrorResponse, empty when the response is not an error.
GS::UniString GetErrorMessage (const GS::ObjectState& response)
{
    const GS::ObjectState* error = response.Get ("error");
    if (error == nullptr) {
        return GS::UniString ();
    }
    GS::UniString message;
    error->Get ("message", message);
    return message.IsEmpty () ? GS::UniString ("The command failed.") : message;
}

void AddWarnings (const GS::ObjectState& response, const char* prefix, GS::Array<GS::UniString>& warnings)
{
    GS::Array<GS::UniString> responseWarnings;
    if (response.Get ("warnings", responseWarnings)) {
        for (const GS::UniString& warning : responseWarnings) {
            warnings.Push (GS::UniString (prefix) + warning);
        }
    }
}

GS::ObjectState GetUnusedAttributesItem (GS::ProcessControl& processControl, GS::Array<GS::UniString>& warnings)
{
    const GS::ObjectState response = GetUnusedAttributesCommand ().Execute (GS::ObjectState (), processControl);
    const GS::UniString errorMessage = GetErrorMessage (response);
    if (!errorMessage.IsEmpty ()) {
        return CreateUnknownItem ("GetUnusedAttributes: " + errorMessage);
    }

    GS::Array<GS::ObjectState> attributeTypes;
    response.Get ("attributeTypes", attributeTypes);
    GS::ObjectState item;
    item.Add ("known", true);
    Int32 total = 0;
    const auto& types = item.AddList<GS::ObjectState> ("types");
    for (const GS::ObjectState& attributeType : attributeTypes) {
        GS::UniString type;
        attributeType.Get ("type", type);
        GS::Array<GS::ObjectState> unusedAttributes;
        attributeType.Get ("unusedAttributes", unusedAttributes);
        const Int32 count = static_cast<Int32> (unusedAttributes.GetSize ());
        total += count;

        GS::ObjectState typeItem;
        typeItem.Add ("type", type);
        typeItem.Add ("count", count);
        types (typeItem);
    }
    item.Add ("count", total);
    item.Add ("complete", !response.Contains ("warnings"));
    AddWarnings (response, "GetUnusedAttributes: ", warnings);
    return item;
}

GS::ObjectState GetUnusedLibraryPartsItem (GS::ProcessControl& processControl, GS::Array<GS::UniString>& warnings)
{
    const GS::ObjectState response = GetUnusedLibraryPartsCommand ().Execute (GS::ObjectState (), processControl);
    const GS::UniString errorMessage = GetErrorMessage (response);
    if (!errorMessage.IsEmpty ()) {
        return CreateUnknownItem ("GetUnusedLibraryParts: " + errorMessage);
    }

    GS::Array<GS::ObjectState> unusedLibraryParts;
    response.Get ("unusedLibraryParts", unusedLibraryParts);
    bool complete = false;
    response.Get ("complete", complete);

    GS::ObjectState item;
    item.Add ("known", true);
    item.Add ("count", static_cast<Int32> (unusedLibraryParts.GetSize ()));
    item.Add ("complete", complete);
    AddWarnings (response, "GetUnusedLibraryParts: ", warnings);
    return item;
}

// The library parts the project refers to whose definition is not in the loaded libraries, as the Library Manager
// lists them as missing.
GS::ObjectState GetMissingLibraryPartsItem ()
{
    Int32 partCount = 0;
    const GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        return CreateUnknownItem (GS::UniString::Printf ("The library parts could not be listed (error %d).", (int) err));
    }

    GS::ObjectState item;
    item.Add ("known", true);
    const auto& names = item.AddList<GS::UniString> ("names");
    Int32 missingCount = 0;
    Int32 unreadCount = 0;
    for (Int32 i = 1; i <= partCount; ++i) {
        API_LibPart libPart = {};
        libPart.index = i;
        if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
            // libPart.location is not allocated on failure.
            ++unreadCount;
            continue;
        }
        const bool isMissing = libPart.missingDef;
        const GS::UniString name (libPart.docu_UName);
        // ACAPI_LibraryPart_Get allocates libPart.location.
        delete libPart.location;
        libPart.location = nullptr;
        if (isMissing) {
            ++missingCount;
            names (name);
        }
    }
    item.Add ("count", missingCount);
    item.Add ("complete", unreadCount == 0);
    return item;
}

#ifdef ServerMainVers_2700
const char* GetSourceStatusName (API_HotlinkSourceStatus status)
{
    switch (status) {
        case API_HotlinkSource_AuthCanceld:         return "AuthCanceld";
        case API_HotlinkSource_SourceMissing:       return "SourceMissing";
        case API_HotlinkSource_SourceAvailable:     return "SourceAvailable";
        case API_HotlinkSource_SourceNotAccessible: return "SourceNotAccessible";
        case API_HotlinkSource_SourceIncompatible:  return "SourceIncompatible";
        case API_HotlinkSource_SourceOffline:       return "SourceOffline";
        default:                                    return "Unknown";
    }
}
#endif

// The hotlink nodes (modules and XRefs, placed or not) whose source file is not available.
GS::ObjectState GetBrokenHotlinksItem ()
{
#ifdef ServerMainVers_2700
    GS::ObjectState item;
    item.Add ("known", true);
    const auto& hotlinks = item.AddList<GS::ObjectState> ("hotlinks");
    Int32 checkedCount = 0;
    Int32 brokenCount = 0;
    bool complete = true;
    for (API_HotlinkTypeID type : { APIHotlink_Module, APIHotlink_XRef }) {
        bool enableUnplaced = true;
        GS::Array<API_Guid> nodes;
        if (ACAPI_Hotlink_GetHotlinkNodes (&type, &nodes, &enableUnplaced) != NoError) {
            complete = false;
            continue;
        }
        for (const API_Guid& nodeGuid : nodes) {
            API_HotlinkNode node = {};
            node.guid = nodeGuid;
            if (ACAPI_Hotlink_GetHotlinkNode (&node, &enableUnplaced) != NoError || node.sourceLocation == nullptr) {
                complete = false;
                continue;
            }
            API_HotlinkSourceStatus status = API_HotlinkSource_SourceMissing;
            if (ACAPI_Hotlink_GetHotlinkSourceStatus (node.sourceLocation, &status) != NoError) {
                complete = false;
                continue;
            }
            ++checkedCount;
            if (status == API_HotlinkSource_SourceAvailable) {
                continue;
            }
            ++brokenCount;
            GS::ObjectState hotlink;
            hotlink.Add ("hotlinkNodeId", CreateGuidObjectState (node.guid));
            hotlink.Add ("name", GS::UniString (node.name));
            hotlink.Add ("location", node.sourceLocation->ToDisplayText ());
            hotlink.Add ("status", GS::UniString (GetSourceStatusName (status)));
            hotlinks (hotlink);
        }
    }
    item.Add ("count", brokenCount);
    item.Add ("checkedCount", checkedCount);
    item.Add ("complete", complete);
    return item;
#else
    return CreateUnknownItem ("The status of a hotlink's source file can only be read from Archicad 27.");
#endif
}

// The Unicode name of an attribute; API_Attr_Head::name is the legacy 8-bit name (see GetAttributeName in
// AttributeCommands.cpp).
GS::UniString GetLayerName (const API_Attr_Head& header)
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

// The layers whose names are the same apart from case and leading or trailing spaces.
GS::ObjectState GetDuplicateLayerNamesItem ()
{
    GS::Array<API_Attribute> layers;
    const GSErrCode err = ACAPI_Attribute_GetAttributesByType (API_LayerID, layers);
    if (err != NoError) {
        for (API_Attribute& layer : layers) {
            DisposeAttribute (layer);
        }
        return CreateUnknownItem (GS::UniString::Printf ("The layers could not be listed (error %d).", (int) err));
    }

    struct LayerEntry {
        API_Guid      guid;
        Int32         index;
        GS::UniString name;
    };
    // Ordered by the normalized name, so the groups come in a stable order.
    std::map<std::string, std::vector<LayerEntry>> layersByName;
    for (API_Attribute& layer : layers) {
        const GS::UniString name = GetLayerName (layer.header);
        GS::UniString key = name;
        key.Trim ();
        key.SetToLowerCase ();
        layersByName[std::string (key.ToCStr (CC_UTF8).Get ())].push_back ({ layer.header.guid, GetAttributeIndex (layer.header.index), name });
        DisposeAttribute (layer);
    }

    GS::ObjectState item;
    item.Add ("known", true);
    const auto& groups = item.AddList<GS::ObjectState> ("groups");
    Int32 groupCount = 0;
    for (const auto& entry : layersByName) {
        if (entry.second.size () < 2) {
            continue;
        }
        ++groupCount;
        GS::ObjectState group;
        group.Add ("name", entry.second.front ().name);
        const auto& groupLayers = group.AddList<GS::ObjectState> ("layers");
        for (const LayerEntry& layer : entry.second) {
            GS::ObjectState layerItem;
            layerItem.Add ("attributeId", CreateGuidObjectState (layer.guid));
            layerItem.Add ("index", layer.index);
            layerItem.Add ("name", layer.name);
            groupLayers (layerItem);
        }
        groups (group);
    }
    item.Add ("count", groupCount);
    return item;
}

// The stories that are the home story of no element of the floor plan.
GS::ObjectState GetEmptyStoriesItem ()
{
    API_StoryInfo storyInfo = {};
    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    if (err != NoError) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
        return CreateUnknownItem (GS::UniString::Printf ("The stories could not be read (error %d).", (int) err));
    }
    struct StoryEntry {
        short         index;
        GS::UniString name;
    };
    std::vector<StoryEntry> stories;
    const short storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;
    for (short i = 0; i < storyCount; ++i) {
        const API_StoryType& story = (*storyInfo.data)[i];
        stories.push_back ({ story.index, GS::UniString (story.uName) });
    }
    BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));

    API_DatabaseInfo startingDatabase = {};
    err = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return CreateUnknownItem ("The current database could not be read.");
    }
    API_DatabaseInfo floorPlan = {};
    floorPlan.typeID = APIWind_FloorPlanID;
    err = ACAPI_Window_GetDatabaseInfo (&floorPlan);
    if (err == NoError) {
        err = ACAPI_Database_ChangeCurrentDatabase (&floorPlan);
    }
    if (err != NoError) {
        ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
        return CreateUnknownItem ("The floor plan database could not be opened.");
    }
    GS::Array<API_Guid> guids;
    err = ACAPI_Element_GetElemList (API_ZombieElemID, &guids);
    std::set<short> usedStories;
    for (const API_Guid& guid : guids) {
        API_Elem_Head head = {};
        head.guid = guid;
        if (ACAPI_Element_GetHeader (&head) == NoError) {
            usedStories.insert (head.floorInd);
        }
    }
    ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return CreateUnknownItem ("The elements of the floor plan could not be listed.");
    }

    GS::ObjectState item;
    item.Add ("known", true);
    const auto& emptyStories = item.AddList<GS::ObjectState> ("stories");
    Int32 emptyCount = 0;
    for (const StoryEntry& story : stories) {
        if (usedStories.find (story.index) != usedStories.end ()) {
            continue;
        }
        ++emptyCount;
        GS::ObjectState storyItem;
        storyItem.Add ("index", static_cast<Int32> (story.index));
        storyItem.Add ("name", story.name);
        emptyStories (storyItem);
    }
    item.Add ("count", emptyCount);
    item.Add ("storyCount", static_cast<Int32> (stories.size ()));
    return item;
}

}

GetProjectHealthCommand::GetProjectHealthCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetProjectHealthCommand::GetName () const
{
    return "GetProjectHealth";
}

GS::Optional<GS::UniString> GetProjectHealthCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {},
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetProjectHealthCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "unusedAttributes": {
                "type": "object",
                "description": "The unused attributes per type, as GetUnusedAttributes lists them (with no input).",
                "properties": {
                    "known": { "type": "boolean", "description": "False when GetUnusedAttributes could not run (it needs Archicad 27 or later); then only reason is given." },
                    "reason": { "type": "string", "description": "Why the item is not known." },
                    "count": { "type": "integer", "description": "The number of unused attributes of all types." },
                    "complete": { "type": "boolean", "description": "False when something could not be checked (see warnings): then a listed attribute may still be used." },
                    "types": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "type": { "type": "string", "description": "The attribute type, as in GetUnusedAttributes." },
                                "count": { "type": "integer" }
                            },
                            "additionalProperties": false,
                            "required": ["type", "count"]
                        }
                    }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "unusedLibraryParts": {
                "type": "object",
                "description": "The loaded library parts no element is placed from, as GetUnusedLibraryParts lists them (with no input).",
                "properties": {
                    "known": { "type": "boolean" },
                    "reason": { "type": "string", "description": "Why the item is not known." },
                    "count": { "type": "integer" },
                    "complete": { "type": "boolean", "description": "False when a view, layout or library part could not be read (see warnings)." }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "missingLibraryParts": {
                "type": "object",
                "description": "The library parts the project refers to whose definition is not in the loaded libraries (shown as missing in the Library Manager).",
                "properties": {
                    "known": { "type": "boolean" },
                    "reason": { "type": "string", "description": "Why the item is not known." },
                    "count": { "type": "integer" },
                    "names": { "type": "array", "items": { "type": "string" } },
                    "complete": { "type": "boolean", "description": "False when a library part could not be read." }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "brokenHotlinks": {
                "type": "object",
                "description": "The hotlink nodes (modules and XRefs, placed or not) whose source file is not available, as GetHotlinkSourceStatus reports it. Known from Archicad 27.",
                "properties": {
                    "known": { "type": "boolean" },
                    "reason": { "type": "string", "description": "Why the item is not known." },
                    "count": { "type": "integer" },
                    "checkedCount": { "type": "integer", "description": "The number of hotlink nodes whose source was checked." },
                    "complete": { "type": "boolean", "description": "False when a hotlink node or its status could not be read." },
                    "hotlinks": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "hotlinkNodeId": { "$ref": "#/HotlinkNodeId" },
                                "name": { "type": "string" },
                                "location": { "type": "string", "description": "The source file." },
                                "status": {
                                    "type": "string",
                                    "enum": ["AuthCanceld", "SourceMissing", "SourceNotAccessible", "SourceIncompatible", "SourceOffline", "Unknown"]
                                }
                            },
                            "additionalProperties": false,
                            "required": ["hotlinkNodeId", "name", "location", "status"]
                        }
                    }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "outdatedHotlinks": {
                "type": "object",
                "description": "The hotlinks whose source changed since their last update. Always known: false, because the Archicad API does not tell it.",
                "properties": {
                    "known": { "type": "boolean" },
                    "reason": { "type": "string" }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "duplicateLayerNames": {
                "type": "object",
                "description": "The layers whose names are the same apart from upper and lower case and leading or trailing spaces, in groups.",
                "properties": {
                    "known": { "type": "boolean" },
                    "reason": { "type": "string", "description": "Why the item is not known." },
                    "count": { "type": "integer", "description": "The number of groups." },
                    "groups": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "name": { "type": "string", "description": "The name of the first layer of the group." },
                                "layers": {
                                    "type": "array",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "attributeId": { "$ref": "#/AttributeId" },
                                            "index": { "type": "integer" },
                                            "name": { "type": "string" }
                                        },
                                        "additionalProperties": false,
                                        "required": ["attributeId", "index", "name"]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": ["name", "layers"]
                        }
                    }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "emptyStories": {
                "type": "object",
                "description": "The stories that are the home story of no element of the floor plan. An element reaching up into a story from a lower home story does not count for it.",
                "properties": {
                    "known": { "type": "boolean" },
                    "reason": { "type": "string", "description": "Why the item is not known." },
                    "count": { "type": "integer" },
                    "storyCount": { "type": "integer", "description": "The number of stories of the project." },
                    "stories": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "index": { "type": "integer", "description": "The story index, as in GetStories." },
                                "name": { "type": "string" }
                            },
                            "additionalProperties": false,
                            "required": ["index", "name"]
                        }
                    }
                },
                "additionalProperties": false,
                "required": ["known"]
            },
            "warnings": {
                "type": "array",
                "description": "The warnings of GetUnusedAttributes and GetUnusedLibraryParts, each starting with the command's name.",
                "items": { "type": "string" }
            }
        },
        "additionalProperties": false,
        "required": ["unusedAttributes", "unusedLibraryParts", "missingLibraryParts", "brokenHotlinks", "outdatedHotlinks", "duplicateLayerNames", "emptyStories", "warnings"]
    })";
}

GS::ObjectState GetProjectHealthCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& processControl) const
{
    GS::Array<GS::UniString> warnings;

    GS::ObjectState response;
    response.Add ("unusedAttributes", GetUnusedAttributesItem (processControl, warnings));
    response.Add ("unusedLibraryParts", GetUnusedLibraryPartsItem (processControl, warnings));
    response.Add ("missingLibraryParts", GetMissingLibraryPartsItem ());
    response.Add ("brokenHotlinks", GetBrokenHotlinksItem ());
    response.Add ("outdatedHotlinks", CreateUnknownItem ("The Archicad API does not tell whether a hotlink's source changed since its last update; the Hotlink Manager shows it."));
    response.Add ("duplicateLayerNames", GetDuplicateLayerNamesItem ());
    response.Add ("emptyStories", GetEmptyStoriesItem ());

    const auto& warningAdder = response.AddList<GS::UniString> ("warnings");
    for (const GS::UniString& warning : warnings) {
        warningAdder (warning);
    }
    return response;
}
