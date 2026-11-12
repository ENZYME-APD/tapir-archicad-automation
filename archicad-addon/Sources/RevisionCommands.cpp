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
    return R"({
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
                        },
                        "markerComponent": {
                            "$ref": "#/ChangeMarkerComponent",
                            "description": "Which parts the marker shows. Default: HeadAndCloud."
                        },
                        "position": {
                            "$ref": "#/Coordinate2D",
                            "description": "The point the Change Marker points to. Default: the centre of the polygon's bounding box."
                        },
                        "markerPosition": {
                            "$ref": "#/Coordinate2D",
                            "description": "The position of the marker head. Default: position."
                        },
                        "changeId": {
                            "type": "string",
                            "minLength": 1,
                            "description": "The ID of the Change the marker is linked to. Without createChange it must be the ID of an existing Change (GetRevisionChanges). When omitted, the marker is not linked to any Change."
                        },
                        "createChange": {
                            "type": "boolean",
                            "description": "True: Archicad creates a new Change with changeId and changeDescription, and links the marker to it. A Change with the same ID must not exist yet. Default: false."
                        },
                        "changeDescription": {
                            "type": "string",
                            "description": "The description (short name) of the new Change. Only with createChange."
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
    })";
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
// the marker head's position, then ACAPI_Element_CreateExt. With APICMLT_CreateNewChange Archicad
// creates the Change itself; the API has no other way to create one.
static GS::ObjectState CreateOneChangeMarker (const GS::ObjectState& data)
{
    // The input is checked before anything is passed to Archicad.
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

    API_ChangeMarkerComponentType markerComponent = APICMCT_HeadAndCloud;
    GS::UniString markerComponentName;
    if (data.Get ("markerComponent", markerComponentName) && !StringToChangeMarkerComponent (markerComponentName, markerComponent)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown markerComponent: ") + markerComponentName);
    }

    GS::UniString changeId;
    const bool hasChangeId = data.Get ("changeId", changeId);
    bool createChange = false;
    data.Get ("createChange", createChange);
    GS::UniString changeDescription;
    const bool hasChangeDescription = data.Get ("changeDescription", changeDescription);

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
    if (hasChangeId) {
        const bool changeExists = RevisionChangeExists (changeId);
        if (createChange && changeExists) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("A Change with the ID '") + changeId + "' already exists. Omit createChange to link the marker to it.");
        }
        if (!createChange && !changeExists) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("There is no Change with the ID '") + changeId + "'. Set createChange to create it.");
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

    if (hasChangeId) {
        CopyChangeMarkerString (element.changeMarker.changeId, changeId);
        element.changeMarker.linkType = createChange ? APICMLT_CreateNewChange : APICMLT_Linked;
    } else {
        element.changeMarker.changeId[0] = 0;
        element.changeMarker.linkType = APICMLT_Unlinked;
    }
    if (createChange) {
        CopyChangeMarkerString (element.changeMarker.changeName, changeDescription);
    }
    element.changeMarker.markerComponent = markerComponent;
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
