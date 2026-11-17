#include "RevisionCommands.hpp"
#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"
#include "ElementCreationCommands.hpp"

#include <cmath>

GetRevisionIssuesCommand::GetRevisionIssuesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRevisionIssuesCommand::GetName () const
{
    return "GetRevisionIssues";
}

GS::Optional<GS::UniString> GetRevisionIssuesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "revisionIssues": {
                "type": "array",
                "items": {
                    "$ref": "#/RevisionIssue"
                }
            },
            "customScheme": {
                "type": "array",
                "description": "The custom scheme fields of the revision issues (key and name). Present only when the project defines any.",
                "items": {
                    "type": "object",
                    "properties": {
                        "customSchemeKey": {
                            "$ref": "#/Guid"
                        },
                        "customSchemeName": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "customSchemeKey",
                        "customSchemeName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionIssues"
        ]
    })";
}

// The custom scheme of the revision issues (Issue Scheme in Project Preferences >
// Revision Management): the key of each field and its name.
static GS::HashTable<API_Guid, GS::UniString> GetRevisionIssueCustomScheme ()
{
    GS::HashTable<API_Guid, GS::UniString> customScheme;
    ACAPI_Revision_GetRVMIssueCustomScheme (&customScheme);
    return customScheme;
}

GS::ObjectState GetRevisionIssuesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_RVMIssue> issueList;
    GSErrCode err = ACAPI_Revision_GetRVMIssues (&issueList);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive issues.");
    }

    GS::ObjectState response;
    const auto& revisionIssues = response.AddList<GS::ObjectState> ("revisionIssues");

    for (const auto& issue : issueList) {
        GS::ObjectState issueData (
            "revisionIssueId", CreateGuidObjectState (issue.guid),
            "id", issue.id,
            "description", issue.description,
            "issueTime", TIGetTimeString (issue.issueTime),
            "issuedByUser", issue.issuedByUser,
            "overrideRevisionIDOfAllIncludedLayouts", issue.isOverrideRevisionId,
            "createNewRevisionInAllIncludedLayouts", issue.isCreateNewRevision,
            "markersVisibleSinceIndex", issue.visibleMarkersInIssues,
            "isIssued", issue.issued);
        GS::Array<API_RVMDocumentRevision> documentRevisions;
        ACAPI_Revision_GetRVMIssueDocumentRevisions (&issue.guid, &documentRevisions);
        if (!documentRevisions.IsEmpty ()) {
            const auto& drs = issueData.AddList<GS::ObjectState> ("documentRevisions");
            for (auto& kv : documentRevisions) {
                drs (GS::ObjectState ("revisionId", CreateGuidObjectState (kv.guid)));
            }
        }
        if (!issue.customData.IsEmpty ()) {
            const auto& customSchemeData = issueData.AddList<GS::ObjectState> ("customSchemeData");
            for (auto& kv : issue.customData) {
                customSchemeData (GS::ObjectState (
#ifdef ServerMainVers_2800
                    "customSchemeKey", APIGuidToString (kv.key),
                    "customSchemeValue", kv.value));
#else
                    "customSchemeKey", APIGuidToString (*kv.key),
                    "customSchemeValue", *kv.value));
#endif
            }
        }
        revisionIssues (issueData);
    }

    const GS::HashTable<API_Guid, GS::UniString> customScheme = GetRevisionIssueCustomScheme ();
    if (!customScheme.IsEmpty ()) {
        const auto& customSchemeList = response.AddList<GS::ObjectState> ("customScheme");
        for (auto& kv : customScheme) {
            customSchemeList (GS::ObjectState (
#ifdef ServerMainVers_2800
                "customSchemeKey", APIGuidToString (kv.key),
                "customSchemeName", kv.value));
#else
                "customSchemeKey", APIGuidToString (*kv.key),
                "customSchemeName", *kv.value));
#endif
        }
    }

    return response;
}

ModifyRevisionIssuesCommand::ModifyRevisionIssuesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyRevisionIssuesCommand::GetName () const
{
    return "ModifyRevisionIssues";
}

GS::Optional<GS::UniString> ModifyRevisionIssuesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "revisionIssues": {
                "type": "array",
                "description": "The revision issues to modify. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "revisionIssueId": {
                            "$ref": "#/RevisionIssueId"
                        },
                        "id": {
                            "type": "string",
                            "description": "The new ID of the revision issue.",
                            "minLength": 1
                        },
                        "description": {
                            "type": "string",
                            "description": "The new short description of the revision issue."
                        },
                        "customSchemeData": {
                            "type": "array",
                            "description": "The custom scheme fields to set. A field is addressed by its key (as GetRevisionIssues returns it) or by its name; the fields not listed keep their values.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "customSchemeKey": {
                                        "$ref": "#/Guid"
                                    },
                                    "customSchemeName": {
                                        "type": "string"
                                    },
                                    "customSchemeValue": {
                                        "type": "string"
                                    }
                                },
                                "additionalProperties": false,
                                "required": [
                                    "customSchemeValue"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "revisionIssueId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionIssues"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyRevisionIssuesCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyRevisionIssuesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> revisionIssues;
    if (!parameters.Get ("revisionIssues", revisionIssues)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'revisionIssues' parameter.");
    }

    GS::Array<API_RVMIssue> issueList;
    GSErrCode err = ACAPI_Revision_GetRVMIssues (&issueList);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive issues.");
    }

    // Field name -> key, for the custom scheme fields addressed by name.
    const GS::HashTable<API_Guid, GS::UniString> customScheme = GetRevisionIssueCustomScheme ();
    GS::HashTable<GS::UniString, API_Guid> customSchemeKeysByName;
    for (auto& kv : customScheme) {
#ifdef ServerMainVers_2800
        customSchemeKeysByName.Put (kv.value, kv.key);
#else
        customSchemeKeysByName.Put (*kv.value, *kv.key);
#endif
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Modify Revision Issues", [&]() -> GSErrCode {
        for (const GS::ObjectState& issueData : revisionIssues) {
            const GS::ObjectState* revisionIssueId = issueData.Get ("revisionIssueId");
            if (revisionIssueId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'revisionIssueId' parameter."));
                continue;
            }

            const API_Guid issueGuid = GetGuidFromObjectState (*revisionIssueId);
            const API_RVMIssue* foundIssue = nullptr;
            for (const API_RVMIssue& candidate : issueList) {
                if (candidate.guid == issueGuid) {
                    foundIssue = &candidate;
                    break;
                }
            }
            if (foundIssue == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Revision issue not found."));
                continue;
            }

            API_RVMIssue issue = *foundIssue;

            GS::UniString id;
            if (issueData.Get ("id", id)) {
                if (id.IsEmpty ()) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The 'id' parameter must not be empty."));
                    continue;
                }
                issue.id = id;
            }

            GS::UniString description;
            if (issueData.Get ("description", description)) {
                issue.description = description;
            }

            GS::UniString customSchemeError;
            GS::Array<GS::ObjectState> customSchemeData;
            if (issueData.Get ("customSchemeData", customSchemeData)) {
                for (const GS::ObjectState& field : customSchemeData) {
                    API_Guid key = APINULLGuid;
                    GS::UniString keyString;
                    GS::UniString name;
                    if (field.Get ("customSchemeKey", keyString)) {
                        key = APIGuidFromString (keyString.ToCStr ());
                    } else if (field.Get ("customSchemeName", name)) {
                        if (!customSchemeKeysByName.Get (name, &key)) {
                            customSchemeError = GS::UniString ("Unknown custom scheme field: ") + name;
                            break;
                        }
                    } else {
                        customSchemeError = GS::UniString ("Each custom scheme field needs 'customSchemeKey' or 'customSchemeName'.");
                        break;
                    }
                    if (key == APINULLGuid) {
                        customSchemeError = GS::UniString ("Invalid custom scheme key: ") + keyString;
                        break;
                    }

                    GS::UniString value;
                    field.Get ("customSchemeValue", value);
                    issue.customData.Put (key, value);
                }
            }
            if (!customSchemeError.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, customSchemeError));
                continue;
            }

            const GSErrCode changeErr = ACAPI_Revision_ChangeRVMIssue (&issue);
            executionResults (changeErr == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (changeErr, "Failed to modify the revision issue."));
        }
        return NoError;
    });

    return response;
}

GetRevisionChangesCommand::GetRevisionChangesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRevisionChangesCommand::GetName () const
{
    return "GetRevisionChanges";
}

GS::Optional<GS::UniString> GetRevisionChangesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "revisionChanges": {
                "type": "array",
                "items": {
                    "$ref": "#/RevisionChange"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionChanges"
        ]
    })";
}

static GS::ObjectState RVMChangeToOS (const API_RVMChange& change)
{
    GS::ObjectState changeData (
        "id", change.id,
        "description", change.description,
        "lastModifiedTime", TIGetTimeString (change.lastModifiedTime),
        "modifiedByUser", change.modifiedByUser,
        "isIssued", change.issued,
        "isArchived", change.archived);
    API_RVMIssue firstIssue = {};
    ACAPI_Revision_GetRVMChangeFirstIssue (&change.id, &firstIssue);
    if (firstIssue.guid != APINULLGuid) {
        changeData.Add ("firstRevisionIssueId", CreateGuidObjectState (firstIssue.guid));
    }
    if (!change.customData.IsEmpty ()) {
        const auto& customSchemeData = changeData.AddList<GS::ObjectState> ("customSchemeData");
        for (auto& kv : change.customData) {
            customSchemeData (GS::ObjectState (
#ifdef ServerMainVers_2800
                "customSchemeKey", APIGuidToString (kv.key),
                "customSchemeValue", kv.value));
#else
                "customSchemeKey", APIGuidToString (*kv.key),
                "customSchemeValue", *kv.value));
#endif
        }
    }

    return changeData;
}

GS::ObjectState GetRevisionChangesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_RVMChange> changeList;
    GSErrCode err = ACAPI_Revision_GetRVMChanges (&changeList);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive changes.");
    }

    GS::ObjectState response;
    const auto& revisionChanges = response.AddList<GS::ObjectState> ("revisionChanges");

    for (const auto& change : changeList) {
        revisionChanges (RVMChangeToOS (change));
    }

    return response;
}

GetRevisionChangeCustomSchemeCommand::GetRevisionChangeCustomSchemeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRevisionChangeCustomSchemeCommand::GetName () const
{
    return "GetRevisionChangeCustomScheme";
}

GS::Optional<GS::UniString> GetRevisionChangeCustomSchemeCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "customScheme": {
                "type": "array",
                "items": {
                    "$ref": "#/RevisionChangeCustomSchemeItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "customScheme"
        ]
    })";
}

GS::ObjectState GetRevisionChangeCustomSchemeCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::HashTable<API_Guid, GS::UniString> customScheme;
    GSErrCode err = ACAPI_Revision_GetRVMChangeCustomScheme (&customScheme);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the custom scheme of changes.");
    }

    GS::ObjectState response;
    const auto& schemeList = response.AddList<GS::ObjectState> ("customScheme");

    for (const auto& kv : customScheme) {
        schemeList (GS::ObjectState (
#ifdef ServerMainVers_2800
            "customSchemeKey", APIGuidToString (kv.key),
            "customSchemeName", kv.value));
#else
            "customSchemeKey", APIGuidToString (*kv.key),
            "customSchemeName", *kv.value));
#endif
    }

    return response;
}

GetDocumentRevisionsCommand::GetDocumentRevisionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDocumentRevisionsCommand::GetName () const
{
    return "GetDocumentRevisions";
}

GS::Optional<GS::UniString> GetDocumentRevisionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "documentRevisions": {
                "type": "array",
                "items": {
                    "$ref": "#/DocumentRevision"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "documentRevisions"
        ]
    })";
}

GS::ObjectState GetDocumentRevisionsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_RVMDocumentRevision> documentRevisionsList;
    GSErrCode err = ACAPI_Revision_GetRVMDocumentRevisions (&documentRevisionsList);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive revisions.");
    }

    GS::ObjectState response;
    const auto& documentRevisions = response.AddList<GS::ObjectState> ("documentRevisions");

    for (const auto& documentRevision : documentRevisionsList) {
        GS::ObjectState documentRevisionData;
        documentRevisionData.Add ("revisionId", CreateGuidObjectState (documentRevision.guid));
        documentRevisionData.Add ("id", documentRevision.id);
        documentRevisionData.Add ("finalId", documentRevision.finalId);
        GS::UniString ownerUser;
        ACAPI_Teamwork_GetUsernameFromId (documentRevision.userId, &ownerUser);
        documentRevisionData.Add ("ownerUser", ownerUser);
        documentRevisionData.Add ("status", documentRevision.status == API_RVMDocumentRevisionStatusActual ? "Actual" : "Issued");
        GS::Array<API_RVMChange> changes;
        ACAPI_Revision_GetRVMDocumentRevisionChanges (&documentRevision.guid, &changes);
        if (!changes.IsEmpty ()) {
            const auto& c = documentRevisionData.AddList<GS::ObjectState> ("changes");
            for (auto& kv : changes) {
                c (GS::ObjectState ("id", kv.id));
            }
        }
        GS::ObjectState layoutInfo (
            "id", documentRevision.layoutInfo.id,
            "databaseId", CreateGuidObjectState (documentRevision.layoutInfo.dbId.elemSetId),
            "name", documentRevision.layoutInfo.name,
            "masterLayoutName", documentRevision.layoutInfo.masterLayoutName,
            "width", documentRevision.layoutInfo.width,
            "height", documentRevision.layoutInfo.height,
            "subsetId", documentRevision.layoutInfo.subsetId,
            "subsetName", documentRevision.layoutInfo.subsetName);
        ACAPI_Teamwork_GetUsernameFromId (documentRevision.layoutInfo.teamworkOwner, &ownerUser);
        layoutInfo.Add ("ownerUser", ownerUser);
        if (!documentRevision.layoutInfo.customData.IsEmpty ()) {
            const auto& customSchemeData = layoutInfo.AddList<GS::ObjectState> ("customSchemeData");
            for (auto& kv : documentRevision.layoutInfo.customData) {
                customSchemeData (GS::ObjectState (
#ifdef ServerMainVers_2800
                    "customSchemeKey", APIGuidToString (kv.key),
                    "customSchemeValue", kv.value));
#else
                    "customSchemeKey", APIGuidToString (*kv.key),
                    "customSchemeValue", *kv.value));
#endif
            }
        }
        documentRevisionData.Add ("layoutInfo", layoutInfo);
        documentRevisions (documentRevisionData);
    }

    return response;
}

GetCurrentRevisionChangesOfLayoutsCommand::GetCurrentRevisionChangesOfLayoutsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentRevisionChangesOfLayoutsCommand::GetName () const
{
    return "GetCurrentRevisionChangesOfLayouts";
}

GS::Optional<GS::UniString> GetCurrentRevisionChangesOfLayoutsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutDatabaseIds": {
                "$ref": "#/Databases"
            }
        },
        "additionalProperties": false,
        "required": [
            "layoutDatabaseIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetCurrentRevisionChangesOfLayoutsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "currentRevisionChangesOfLayouts": {
                "$ref": "#/RevisionChangesOfEntities"
            }
        },
        "additionalProperties": false,
        "required": [
            "currentRevisionChangesOfLayouts"
        ]
    })";
}

GS::ObjectState GetCurrentRevisionChangesOfLayoutsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> layoutDatabaseIds;
    parameters.Get ("layoutDatabaseIds", layoutDatabaseIds);
    const GS::Array<API_Guid> databaseIds = layoutDatabaseIds.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

    GS::ObjectState response;
    const auto& currentRevisionChangesOfLayouts = response.AddList<GS::ObjectState> ("currentRevisionChangesOfLayouts");

    for (const API_Guid& databaseId : databaseIds) {
        API_DatabaseUnId dbUnId;
        dbUnId.elemSetId = databaseId;
        GS::Array<API_RVMChange> changeList;
        GSErrCode err = ACAPI_Revision_GetRVMLayoutCurrentRevisionChanges (&dbUnId, &changeList);
        if (err != NoError) {
            currentRevisionChangesOfLayouts (CreateErrorResponse (err, "Failed to retrive changes of layout."));
            continue;
        }

        GS::ObjectState os;
        const auto& revisionChanges = os.AddList<GS::ObjectState> ("revisionChanges");

        for (const auto& change : changeList) {
            revisionChanges (RVMChangeToOS (change));
        }

        currentRevisionChangesOfLayouts (os);
    }

    return response;
}

GetRevisionChangesOfElementsCommand::GetRevisionChangesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRevisionChangesOfElementsCommand::GetName () const
{
    return "GetRevisionChangesOfElements";
}

GS::Optional<GS::UniString> GetRevisionChangesOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetRevisionChangesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "revisionChangesOfElements": {
                "$ref": "#/RevisionChangesOfEntities"
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionChangesOfElements"
        ]
    })";
}

GS::ObjectState GetRevisionChangesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);
    const GS::Array<API_Guid> elementIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

    GS::ObjectState response;
    const auto& revisionChangesOfElements = response.AddList<GS::ObjectState> ("revisionChangesOfElements");

    for (const API_Guid& elemId : elementIds) {
        GS::Array<GS::UniString> changeIds;
        GSErrCode err = ACAPI_Revision_GetRVMElemChangeIds (&elemId, &changeIds);
        if (err != NoError) {
            revisionChangesOfElements (CreateErrorResponse (err, "Failed to retrive changes of elements."));
            continue;
        }

        GS::Array<API_RVMChange> changeList;
        ACAPI_Revision_GetRVMChangesFromChangeIds (&changeIds, &changeList);

        GS::ObjectState os;
        const auto& revisionChanges = os.AddList<GS::ObjectState> ("revisionChanges");

        for (const auto& change : changeList) {
            revisionChanges (RVMChangeToOS (change));
        }

        revisionChangesOfElements (os);
    }

    return response;
}
GS::UniString ChangeMarkerLinkTypeToString (API_ChangeMarkerLinkType linkType)
{
    switch (linkType) {
        case APICMLT_CreateNewChange:   return "NewChange";
        case APICMLT_Linked:            return "Linked";
        case APICMLT_Unlinked:          return "Unlinked";
        default:                        return "Unknown";
    }
}

GS::UniString ChangeMarkerComponentToString (API_ChangeMarkerComponentType markerComponent)
{
    switch (markerComponent) {
        case APICMCT_HeadOnly:      return "HeadOnly";
        case APICMCT_CloudOnly:     return "CloudOnly";
        case APICMCT_HeadAndCloud:  return "HeadAndCloud";
        default:                    return "Unknown";
    }
}

static bool StringToChangeMarkerComponent (const GS::UniString& name, API_ChangeMarkerComponentType& markerComponent)
{
    if (name == "HeadOnly") {
        markerComponent = APICMCT_HeadOnly;
    } else if (name == "CloudOnly") {
        markerComponent = APICMCT_CloudOnly;
    } else if (name == "HeadAndCloud") {
        markerComponent = APICMCT_HeadAndCloud;
    } else {
        return false;
    }
    return true;
}

template <USize N>
static void CopyChangeMarkerString (GS::uchar_t (&target)[N], const GS::UniString& source)
{
    GS::ucsncpy (target, source.ToUStr (), N);
    target[N - 1] = 0;
}

// Points of a Change Marker polygon closer than this (in meters) are taken as the same point.
static constexpr double ChangeMarkerPointEps = 0.0001;

static bool RevisionChangeExists (const GS::UniString& changeId)
{
    GS::Array<API_RVMChange> changeList;
    if (ACAPI_Revision_GetRVMChanges (&changeList) != NoError) {
        return false;
    }
    for (const API_RVMChange& change : changeList) {
        if (change.id == changeId) {
            return true;
        }
    }
    return false;
}

// The settings CreateChangeMarkers and ModifyChangeMarkers both take beside the geometry, written by
// ApplyChangeMarkerSettings with the keys GetDetailsOfElements returns.
static const char* ChangeMarkerSettingsSchemaProperties = R"(
                        "markerComponent": {
                            "$ref": "#/ChangeMarkerComponent",
                            "description": "Which parts the marker shows. A new marker gets HeadAndCloud when it is left out."
                        },
                        "changeId": {
                            "type": "string",
                            "minLength": 1,
                            "description": "The ID of the Change the marker is linked to. Without createChange it must be the ID of an existing Change (GetRevisionChanges). A new marker created without it is not linked to any Change."
                        },
                        "createChange": {
                            "type": "boolean",
                            "description": "True: Archicad creates a new Change with changeId and changeDescription, and links the marker to it. A Change with the same ID must not exist yet. Default: false."
                        },
                        "changeDescription": {
                            "type": "string",
                            "description": "The description (short name) of the new Change. Only with createChange: the description of an existing Change cannot be changed."
                        },
                        "angle": {
                            "type": "number",
                            "description": "The rotation angle of the marker head in radians (API_ChangeMarkerType::angle), as GetDetailsOfElements returns it. A new marker gets the angle of the Change Marker tool defaults when it is left out."
                        },
                        "horizontalMarker": {
                            "type": "boolean",
                            "description": "Whether the marker head is always horizontal (API_ChangeMarkerType::horizontalMarker), as GetDetailsOfElements returns it. A new marker gets the setting of the Change Marker tool defaults when it is left out."
                        },)";

// Writes the link to a Change (changeId, createChange, changeDescription and, for ModifyChangeMarkers,
// linkType), markerComponent, angle and horizontalMarker given in data to element.changeMarker: for
// CreateChangeMarkers on the tool defaults (mask nullptr), and for ModifyChangeMarkers on the marker read
// from the project, with the given fields marked in mask. The link is checked against the Changes of the
// project first. Returns the error response of an invalid value; element is then partly written and
// must not be used.
static GS::Optional<GS::ObjectState> ApplyChangeMarkerSettings (const GS::ObjectState& data, API_Element& element, API_Element* mask)
{
    API_ChangeMarkerType& changeMarker = element.changeMarker;

    GS::UniString changeId;
    const bool hasChangeId = data.Get ("changeId", changeId);
    bool createChange = false;
    data.Get ("createChange", createChange);
    GS::UniString changeDescription;
    const bool hasChangeDescription = data.Get ("changeDescription", changeDescription);
    GS::UniString linkType;
    const bool hasLinkType = data.Get ("linkType", linkType);

    if (hasChangeId && changeId.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "changeId must not be empty.");
    }
    if (changeId.GetLength () >= static_cast<USize> (API_UniLongNameLen) || changeDescription.GetLength () >= static_cast<USize> (API_UniLongNameLen)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("changeId and changeDescription can be at most %d characters long.", (int) (API_UniLongNameLen - 1)));
    }
    if (createChange && !hasChangeId) {
        return CreateErrorResponse (APIERR_BADPARS, "createChange needs a changeId.");
    }
    if (hasChangeDescription && !createChange) {
        return CreateErrorResponse (APIERR_BADPARS, "changeDescription is only used with createChange: the description of an existing Change cannot be changed.");
    }
    if (hasLinkType) {
        if (linkType == "Unlinked") {
            if (hasChangeId || createChange) {
                return CreateErrorResponse (APIERR_BADPARS, "linkType Unlinked removes the link to the Change, it cannot be given with changeId or createChange.");
            }
        } else if (linkType == "Linked") {
            if (!hasChangeId) {
                return CreateErrorResponse (APIERR_BADPARS, "linkType Linked needs the changeId of the Change to link the marker to.");
            }
        } else {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown linkType: ") + linkType + ". It must be Linked or Unlinked.");
        }
    }
    if (hasChangeId) {
        const bool changeExists = RevisionChangeExists (changeId);
        if (createChange && changeExists) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("A Change with the ID '") + changeId + "' already exists. Omit createChange to link the marker to it.");
        }
        if (!createChange && !changeExists) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("There is no Change with the ID '") + changeId + "'. Set createChange to create it.");
        }
    }

    API_ChangeMarkerComponentType markerComponent = changeMarker.markerComponent;
    GS::UniString markerComponentName;
    const bool hasMarkerComponent = data.Get ("markerComponent", markerComponentName);
    if (hasMarkerComponent && !StringToChangeMarkerComponent (markerComponentName, markerComponent)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown markerComponent: ") + markerComponentName);
    }

    if (hasChangeId) {
        // With APICMLT_CreateNewChange Archicad creates the Change itself; the API has no other way to create one.
        CopyChangeMarkerString (changeMarker.changeId, changeId);
        changeMarker.linkType = createChange ? APICMLT_CreateNewChange : APICMLT_Linked;
        if (createChange) {
            CopyChangeMarkerString (changeMarker.changeName, changeDescription);
        }
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, linkType);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, changeId);
            if (createChange) {
                ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, changeName);
            }
        }
    } else if (hasLinkType && linkType == "Unlinked") {
        changeMarker.changeId[0] = 0;
        changeMarker.linkType = APICMLT_Unlinked;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, linkType);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, changeId);
        }
    }

    if (hasMarkerComponent) {
        changeMarker.markerComponent = markerComponent;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, markerComponent);
        }
    }
    if (data.Get ("angle", changeMarker.angle) && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, angle);
    }
    if (data.Get ("horizontalMarker", changeMarker.horizontalMarker) && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_ChangeMarkerType, horizontalMarker);
    }

    return {};
}

CreateChangeMarkersCommand::CreateChangeMarkersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateChangeMarkersCommand::GetName () const
{
    return "CreateChangeMarkers";
}

GS::Optional<GS::UniString> CreateChangeMarkersCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "changeMarkersData": {
                "type": "array",
                "description": "The Change Markers to create, in the database of the active window (the floor plan story, a section, a detail, a worksheet or a layout).",
                "items": {
                    "type": "object",
                    "properties": {
                        "polygon": {
                            "type": "array",
                            "description": "The outline of the cloud: at least 3 points, straight edges only. A closing point that repeats the first one may be given. Needed for every marker component: a head-only marker keeps it as the area it marks, as Archicad's own example does.",
                            "items": {
                                "$ref": "#/Coordinate2D"
                            },
                            "minItems": 3
                        },)") + ChangeMarkerSettingsSchemaProperties + R"(
                        "position": {
                            "$ref": "#/Coordinate2D",
                            "description": "The point the Change Marker points to. Default: the centre of the polygon's bounding box."
                        },
                        "markerPosition": {
                            "$ref": "#/Coordinate2D",
                            "description": "The position of the marker head. Default: position."
                        },
                        "layerIndex": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Optional layer attribute index to place the marker on. It must be an existing, unlocked layer. When omitted the marker gets the layer of the Change Marker tool defaults."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "polygon"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "changeMarkersData"
        ]
    })");
}

GS::Optional<GS::UniString> CreateChangeMarkersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

// Follows Do_CreateChangeMarker of the DevKit's Element_Test example (#2658): the tool defaults
// with the main marker, the Change's ID and name with the link type, the cloud polygon in the memo,
// the marker head's position, then ACAPI_Element_CreateExt. The link, the marker components, the
// angle and horizontalMarker go through ApplyChangeMarkerSettings, as in ModifyChangeMarkers.
static GS::ObjectState CreateOneChangeMarker (const GS::ObjectState& data)
{
    // The polygon is checked before anything is passed to Archicad.
    GS::Array<GS::ObjectState> polygonData;
    data.Get ("polygon", polygonData);
    GS::Array<API_Coord> points;
    for (const GS::ObjectState& pointData : polygonData) {
        points.Push (Get2DCoordinateFromObjectState (pointData));
    }
    if (points.GetSize () >= 2) {
        const API_Coord& first = points[0];
        const API_Coord& last = points[points.GetSize () - 1];
        if (std::abs (first.x - last.x) < ChangeMarkerPointEps && std::abs (first.y - last.y) < ChangeMarkerPointEps) {
            points.Pop ();
        }
    }
    if (points.GetSize () < 3) {
        return CreateErrorResponse (APIERR_BADPARS, "The polygon needs at least 3 different points.");
    }
    for (UIndex i = 0; i < points.GetSize (); ++i) {
        const API_Coord& c1 = points[i];
        const API_Coord& c2 = points[(i + 1) % points.GetSize ()];
        if (std::abs (c1.x - c2.x) < ChangeMarkerPointEps && std::abs (c1.y - c2.y) < ChangeMarkerPointEps) {
            return CreateErrorResponse (APIERR_BADPARS, "The polygon has two consecutive points at the same place.");
        }
    }

    API_Coord position = {};
    if (const GS::ObjectState* positionData = data.Get ("position")) {
        position = Get2DCoordinateFromObjectState (*positionData);
    } else {
        API_Coord minC = points[0];
        API_Coord maxC = points[0];
        for (const API_Coord& c : points) {
            minC.x = GS::Min (minC.x, c.x);
            minC.y = GS::Min (minC.y, c.y);
            maxC.x = GS::Max (maxC.x, c.x);
            maxC.y = GS::Max (maxC.y, c.y);
        }
        position.x = (minC.x + maxC.x) / 2.0;
        position.y = (minC.y + maxC.y) / 2.0;
    }
    API_Coord markerPosition = position;
    if (const GS::ObjectState* markerPositionData = data.Get ("markerPosition")) {
        markerPosition = Get2DCoordinateFromObjectState (*markerPositionData);
    }

    API_Element element = {};
    API_ElementMemo memo = {};
    API_SubElement marker = {};
    const GS::OnExit cleanup ([&] () {
        ACAPI_DisposeElemMemoHdls (&memo);
        ACAPI_DisposeElemMemoHdls (&marker.memo);
    });

#ifdef ServerMainVers_2600
    element.header.type = API_ChangeMarkerID;
#else
    element.header.typeID = API_ChangeMarkerID;
#endif
    marker.subType = static_cast<API_SubElementType> (APISubElement_MainMarker);

    GSErrCode err = ACAPI_Element_GetDefaultsExt (&element, &memo, 1UL, &marker);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the Change Marker defaults.");
    }

    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (data, element);
    if (layerError.HasValue ()) {
        return layerError.Get ();
    }

    // A new marker is linked to no Change and shows its head and its cloud, unless the item says otherwise.
    element.changeMarker.changeId[0] = 0;
    element.changeMarker.linkType = APICMLT_Unlinked;
    element.changeMarker.markerComponent = APICMCT_HeadAndCloud;
    const GS::Optional<GS::ObjectState> settingsError = ApplyChangeMarkerSettings (data, element, nullptr);
    if (settingsError.HasValue ()) {
        return settingsError.Get ();
    }
    element.changeMarker.pos = position;

    const Int32 nCoords = static_cast<Int32> (points.GetSize ()) + 1;
    element.changeMarker.poly.nCoords = nCoords;
    element.changeMarker.poly.nSubPolys = 1;
    element.changeMarker.poly.nArcs = 0;

    // The defaults may come with a polygon of their own: it is replaced.
    if (memo.coords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.coords));
    }
    if (memo.pends != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.pends));
    }
    if (memo.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
    }
    if (memo.vertexIDs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.vertexIDs));
    }
    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
    if (memo.coords == nullptr || memo.pends == nullptr) {
        return CreateErrorResponse (APIERR_MEMFULL, "Failed to allocate the polygon of the Change Marker.");
    }
    for (Int32 i = 0; i < nCoords - 1; ++i) {
        (*memo.coords)[i + 1] = points[i];
    }
    (*memo.coords)[nCoords] = (*memo.coords)[1];
    (*memo.pends)[0] = 0;
    (*memo.pends)[1] = nCoords;

    marker.subElem.object.pos = markerPosition;
    marker.subType = APISubElement_MainMarker;

    err = ACAPI_Element_CreateExt (&element, &memo, 1UL, &marker);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create the Change Marker.");
    }
    return CreateElementIdObjectState (element.header.guid);
}

GS::ObjectState CreateChangeMarkersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> changeMarkersData;
    parameters.Get ("changeMarkersData", changeMarkersData);

    GS::Array<GS::ObjectState> results;

    API_NotifyElementType notification = {};
    notification.notifID = APINotifyElement_BeginEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    const GSErrCode err = ACAPI_CallUndoableCommand ("Create Change Markers", [&] () -> GSErrCode {
        for (const GS::ObjectState& data : changeMarkersData) {
            results.Push (CreateOneChangeMarker (data));
        }
        return NoError;
    });

    notification = {};
    notification.notifID = APINotifyElement_EndEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create the Change Markers.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    for (const GS::ObjectState& result : results) {
        elements (result);
    }
    return response;
}

ModifyChangeMarkersCommand::ModifyChangeMarkersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyChangeMarkersCommand::GetName () const
{
    return "ModifyChangeMarkers";
}

GS::Optional<GS::UniString> ModifyChangeMarkersCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "changeMarkersWithDetails": {
                "type": "array",
                "description": "The Change Markers to change, in the database of the active window, each with the settings to change. Only the given settings are changed; the keys are those CreateChangeMarkers takes and GetDetailsOfElements returns. The cloud polygon cannot be changed: besides the settings, ACAPI_Element_Change only changes the position of a marker without a polygon, so delete the marker and create it again with CreateChangeMarkers for a new polygon.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },)") + ChangeMarkerSettingsSchemaProperties + R"(
                        "linkType": {
                            "type": "string",
                            "enum": ["Linked", "Unlinked"],
                            "description": "Unlinked removes the link of the marker to its Change (the Change stays in the project); it cannot be given with changeId or createChange. Linked links the marker to the Change of changeId, which it needs; changeId alone does the same."
                        },
                        "position": {
                            "$ref": "#/Coordinate2D",
                            "description": "The new point the Change Marker points to. Only for a marker without a cloud polygon, the only case in which Archicad takes it (ACAPI_Element_Change); every marker CreateChangeMarkers makes has a polygon, so it is refused for those."
                        },
                        "markerPosition": {
                            "$ref": "#/Coordinate2D",
                            "description": "The new position of the marker head, as CreateChangeMarkers takes it. Refused for a marker without a head (markerComponent CloudOnly)."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "changeMarkersWithDetails"
        ]
    })");
}

GS::Optional<GS::UniString> ModifyChangeMarkersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

// Archicad may store an angle normalized to another full turn, so angles are compared modulo 2 pi,
// as ModifyDetailMarkers compares them.
static bool IsSameChangeMarkerAngle (double angle1, double angle2)
{
    const double fullTurn = 2.0 * 3.14159265358979323846;
    double diff = std::fmod (angle1 - angle2, fullTurn);
    if (diff < 0.0) {
        diff += fullTurn;
    }
    return diff < 1e-6 || fullTurn - diff < 1e-6;
}

// Changes one Change Marker: the settings through ApplyChangeMarkerSettings, as in CreateChangeMarkers,
// and pos (position), which ACAPI_Element_Change edits only on a marker without a polygon (ACAPinc.h),
// in one ACAPI_Element_Change of the marker; then the marker head, a placed library part of its own
// (markerGuid), is moved to markerPosition with an ACAPI_Element_Change of its pos, as the DevKit's
// Element_Test (Do_ChangeMarker_Edit) changes the head's pen. Both are read back, so a value Archicad
// did not keep is reported as a failure.
static GS::ObjectState ModifyOneChangeMarker (const GS::ObjectState& item)
{
    const GS::ObjectState* elementId = item.Get ("elementId");
    if (elementId == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing");
    }

    API_Element element = {};
    element.header.guid = GetGuidFromObjectState (*elementId);
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to find the element");
    }
    if (GetElemTypeId (element.header) != API_ChangeMarkerID) {
        return CreateFailedExecutionResult (APIERR_BADID, "Element is not a Change Marker.");
    }
    if (item.GetFieldCount () <= 1) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No Change Marker settings to modify.");
    }

    const GS::ObjectState* positionOS = item.Get ("position");
    if (positionOS != nullptr && element.changeMarker.poly.nCoords > 0) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "'position' is only taken for a Change Marker without a cloud polygon, and this one has a polygon: Archicad keeps the position of such a marker.");
    }
    const GS::ObjectState* markerPositionOS = item.Get ("markerPosition");

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    const GS::Optional<GS::ObjectState> settingsError = ApplyChangeMarkerSettings (item, element, &mask);
    if (settingsError.HasValue ()) {
        return CreateFailedExecutionResult (settingsError.Get ());
    }
    if (positionOS != nullptr) {
        element.changeMarker.pos = Get2DCoordinateFromObjectState (*positionOS);
        ACAPI_ELEMENT_MASK_SET (mask, API_ChangeMarkerType, pos);
    }

    // ACAPI_Element_Change may answer NoError on a hidden or locked layer and change nothing.
    GS::UniString layerReason;
    err = GetLayerChangeError (element.header.layer, layerReason);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "The Change Marker was not changed, because " + layerReason + ".");
    }

    const bool hasMarkerSettings = item.GetFieldCount () > (markerPositionOS != nullptr ? 2u : 1u);
    if (hasMarkerSettings) {
        err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
        if (err != NoError) {
            const GS::UniString refusal = DescribeElementEditRefusal (err, element.header);
            return CreateFailedExecutionResult (err, refusal.IsEmpty () ? GS::UniString ("Failed to modify the Change Marker.") : "Failed to modify the Change Marker, because " + refusal + ".");
        }
    }

    // Read back, so a value Archicad did not take is reported instead of a success. The marker is read
    // before its head is moved, as a new marker component may give it a head.
    API_Element changed = {};
    changed.header.guid = element.header.guid;
    err = ACAPI_Element_Get (&changed);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to read the Change Marker back after the change.");
    }

    API_Coord markerPosition = {};
    API_Guid movedHeadGuid = APINULLGuid;
    if (markerPositionOS != nullptr) {
        if (changed.changeMarker.markerGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, hasMarkerSettings
                ? GS::UniString ("The Change Marker has no marker head (markerComponent CloudOnly), so markerPosition was not applied; the other settings were.")
                : GS::UniString ("The Change Marker has no marker head (markerComponent CloudOnly), so markerPosition cannot be applied."));
        }
        API_Element head = {};
        head.header.guid = changed.changeMarker.markerGuid;
        err = ACAPI_Element_Get (&head);
        if (err != NoError || GetElemTypeId (head.header) != API_ObjectID) {
            return CreateFailedExecutionResult (err != NoError ? err : APIERR_BADID, "Failed to load the marker head of the Change Marker.");
        }
        markerPosition = Get2DCoordinateFromObjectState (*markerPositionOS);
        head.object.pos = markerPosition;
        API_Element headMask = {};
        ACAPI_ELEMENT_MASK_CLEAR (headMask);
        ACAPI_ELEMENT_MASK_SET (headMask, API_ObjectType, pos);
        err = ACAPI_Element_Change (&head, &headMask, nullptr, 0, true);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to move the marker head of the Change Marker.");
        }
        // ACAPI_Element_Change fills in the guid of the changed element.
        movedHeadGuid = head.header.guid;
    }

    const API_ChangeMarkerType& given = element.changeMarker;
    const API_ChangeMarkerType& kept = changed.changeMarker;
    const double tolerance = 1e-6;
    GS::UniString notApplied;
    if (item.Contains ("changeId") &&
        (kept.linkType != APICMLT_Linked || GS::UniString (kept.changeId) != GS::UniString (given.changeId))) {
        notApplied += item.Contains ("createChange") ? " changeId createChange" : " changeId";
    }
    if (!item.Contains ("changeId") && item.Contains ("linkType") && kept.linkType != APICMLT_Unlinked) {
        notApplied += " linkType";
    }
    if (item.Contains ("markerComponent") && kept.markerComponent != given.markerComponent) {
        notApplied += " markerComponent";
    }
    if (item.Contains ("horizontalMarker") && kept.horizontalMarker != given.horizontalMarker) {
        notApplied += " horizontalMarker";
    }
    if (item.Contains ("angle") && !kept.horizontalMarker && !IsSameChangeMarkerAngle (kept.angle, given.angle)) {
        notApplied += " angle";
    }
    if (positionOS != nullptr && (std::abs (kept.pos.x - given.pos.x) > tolerance || std::abs (kept.pos.y - given.pos.y) > tolerance)) {
        notApplied += " position";
    }
    if (markerPositionOS != nullptr) {
        API_Element movedHead = {};
        movedHead.header.guid = movedHeadGuid;
        if (ACAPI_Element_Get (&movedHead) != NoError ||
            std::abs (movedHead.object.pos.x - markerPosition.x) > tolerance || std::abs (movedHead.object.pos.y - markerPosition.y) > tolerance) {
            notApplied += " markerPosition";
        }
    }
    if (!notApplied.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_REFUSEDPAR, "Archicad did not apply:" + notApplied + ".");
    }

    return CreateSuccessfulExecutionResult ();
}

GS::ObjectState ModifyChangeMarkersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("changeMarkersWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // The marker head is an element of its own, so it is reported too when it moves.
    GS::Array<API_Guid> elementsToWatch;
    for (const GS::ObjectState& item : itemsWithDetails) {
        const GS::ObjectState* elementId = item.Get ("elementId");
        if (elementId == nullptr) {
            continue;
        }
        API_Element element = {};
        element.header.guid = GetGuidFromObjectState (*elementId);
        elementsToWatch.Push (element.header.guid);
        if (ACAPI_Element_Get (&element) == NoError && GetElemTypeId (element.header) == API_ChangeMarkerID && element.changeMarker.markerGuid != APINULLGuid) {
            elementsToWatch.Push (element.changeMarker.markerGuid);
        }
    }
    ElementChangeReporter changeReporter (elementsToWatch);

    ACAPI_CallUndoableCommand ("ModifyChangeMarkers", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState result = ModifyOneChangeMarker (item);
            changeReporter.MarkChangedIfSucceeded (item, result);
            executionResults (result);
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
}
