#include "RevisionCommands.hpp"
#include "MigrationHelper.hpp"

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
    const GS::Array<API_Guid> elementIds = elements.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

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