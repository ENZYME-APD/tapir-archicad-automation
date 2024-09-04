#include "IssueCommands.hpp"
#include "MigrationHelper.hpp"

static API_IFCTranslatorIdentifier GetExportTranslator ()
{
    GS::Array<API_IFCTranslatorIdentifier> ifcExportTranslators;
    ACAPI_IFC_GetIFCExportTranslatorsList (ifcExportTranslators);

    if (ifcExportTranslators.GetSize () > 0)
        return ifcExportTranslators[0];

    API_IFCTranslatorIdentifier ifcTranslator;
    ifcTranslator.innerReference = nullptr;

    return ifcTranslator;
}


static API_IFCRelationshipData GetCurrentProjectIFCRelationshipData ()
{
    API_IFCRelationshipData ifcRelationshipData;
    API_IFCTranslatorIdentifier ifcTranslator = GetExportTranslator ();

    if (ifcTranslator.innerReference != nullptr)
        ACAPI_IFC_GetIFCRelationshipData (ifcTranslator, ifcRelationshipData);

    return ifcRelationshipData;
}


#ifdef ServerMainVers_2800
static GSErrCode GetIFCRelationshipData (GS::HashTable<GS::UniString, API_IFCRelationshipData>* apiIfcRelationshipDataTable, const void* par1)
{
    const API_IFCRelationshipData* ifcRelationshipDataTable = reinterpret_cast<const API_IFCRelationshipData*> (par1);

    GS::UniString ifcProjectId;
    ifcRelationshipDataTable->containmentTable.EnumerateValues ([&](const GS::UniString& value) {
        if (!ifcRelationshipDataTable->containmentTable.ContainsKey (value)) {
            DBASSERT (ifcProjectId.IsEmpty ());
            ifcProjectId = value;
        }
    });
    if (ifcProjectId.IsEmpty () && ifcRelationshipDataTable->containmentTable.GetSize () != 0) {
        DBASSERT (ifcProjectId != APINULLGuid);
    }
    apiIfcRelationshipDataTable->Put (ifcProjectId, *ifcRelationshipDataTable);

    return NoError;
}
#else
static GSErrCode GetIFCRelationshipData (GS::HashTable<API_Guid, API_IFCRelationshipData>* apiIfcRelationshipDataTable, const void* par1)
{
    const API_IFCRelationshipData* ifcRelationshipDataTable = reinterpret_cast<const API_IFCRelationshipData*> (par1);

    API_Guid ifcProjectId = APINULLGuid;
    ifcRelationshipDataTable->containmentTable.EnumerateValues ([&](const API_Guid& value) {
        if (!ifcRelationshipDataTable->containmentTable.ContainsKey (value)) {
            DBASSERT (ifcProjectId == APINULLGuid);
            ifcProjectId = value;
        }
    });
    if (ifcProjectId == APINULLGuid && ifcRelationshipDataTable->containmentTable.GetSize () != 0) {
        DBASSERT (ifcProjectId != APINULLGuid);
    }
    apiIfcRelationshipDataTable->Put (ifcProjectId, *ifcRelationshipDataTable);

    return NoError;
}
#endif


CreateIssueCommand::CreateIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateIssueCommand::GetName () const
{
    return "CreateIssue";
}

GS::Optional<GS::UniString> CreateIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the issue."
            },
            "parentIssueId": {
                "$ref": "#/IssueId"
            },
            "tagText": {
                "type": "string",
                "description": "Tag text of the issue, optional."
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    })";
}

GS::Optional<GS::UniString> CreateIssueCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId"
        ]
    })";
}

GS::ObjectState CreateIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    GS::ObjectState parentIssueId;
    GS::UniString tagText = "";

    parameters.Get ("tagText", tagText);
    if (!parameters.Get ("name", name)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid input parameters.");
    }

    API_MarkUpType issue (name);
    issue.tagText = tagText;
    if (parameters.Get ("parentIssueId", parentIssueId))
        issue.parentGuid = GetGuidFromObjectState (parentIssueId);

    GSErrCode err = ACAPI_CallUndoableCommand ("Create issue", [&]() -> GSErrCode {
        return ACAPI_Markup_Create (issue);
    });

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create issue.");
    }

    return CreateIdObjectState ("issueId", issue.guid);
}

DeleteIssueCommand::DeleteIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteIssueCommand::GetName () const
{
    return "DeleteIssue";
}

GS::Optional<GS::UniString> DeleteIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "acceptAllElements": {
                "type": "boolean",
                "description": "Accept all creation/deletion/modification of the deleted issue. By default false."
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteIssueCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState DeleteIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState issueId;
    bool acceptAllElements = false;

    if (!parameters.Get ("issueId", issueId)) {
        return CreateFailedExecutionResult (Error, "Invalid input parameters.");
    }

    parameters.Get ("acceptAllElements", acceptAllElements);
    API_Guid guid = GetGuidFromObjectState (issueId);
    GSErrCode err = ACAPI_CallUndoableCommand ("Delete issue", [&]() -> GSErrCode {
        return ACAPI_Markup_Delete (guid, acceptAllElements);
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to delete issue.");
    }

    return CreateSuccessfulExecutionResult ();
}

GetIssuesCommand::GetIssuesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIssuesCommand::GetName () const
{
    return "GetIssues";
}

GS::Optional<GS::UniString> GetIssuesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issues": {
                "type": "array",
                "description": "A list of existing issues.",
                "items": {
                    "type": "object",
                    "properties": {
                        "issueId": {
                            "$ref": "#/IssueId"
                        },
                        "name": {
                            "type": "string",
                            "description": "Issue name"
                        },
                        "parentIssueId": {
                            "$ref": "#/IssueId"
                        },
                        "creaTime": {
                            "type": "integer",
                            "description": "Issue creation time"
                        },
                        "modiTime": {
                            "type": "integer",
                            "description": "Issue modification time"
                        },
                        "tagText": {
                            "type": "string",
                            "description": "Issue tag text - labels"
                        },
                        "tagTextElementId": {
                            "$ref": "#/ElementId"
                        },
                        "isTagTextElemVisible": {
                            "type": "boolean",
                            "description": "The visibility of the attached tag text element"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "issueId",
                        "name",
                        "parentIssueId",
                        "creaTime",
                        "modiTime",
                        "tagText",
                        "tagTextElementId",
                        "isTagTextElemVisible"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "issues"
        ]
    })";
}

GS::ObjectState GetIssuesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_MarkUpType> issueList;
    GSErrCode err = ACAPI_Markup_GetList (APINULLGuid, &issueList);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive issues.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("issues");

    for (auto i = issueList.Enumerate (); i != nullptr; ++i) {
        GS::ObjectState issueData;
        issueData.Add ("issueId", CreateGuidObjectState (i->guid));
        issueData.Add ("name", i->name);
        issueData.Add ("parentIssueId", CreateGuidObjectState (i->parentGuid));
        issueData.Add ("creaTime", i->creaTime);
        issueData.Add ("modiTime", i->modiTime);
        issueData.Add ("tagText", i->tagText);
        issueData.Add ("tagTextElementId", CreateGuidObjectState (i->tagTextElemGuid));
        issueData.Add ("isTagTextElemVisible", i->isTagTextElemVisible);
        listAdder (issueData);
    }

    return response;
}

AddCommentToIssueCommand::AddCommentToIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String AddCommentToIssueCommand::GetName () const
{
    return "AddCommentToIssue";
}

GS::Optional<GS::UniString> AddCommentToIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "author": {
                "type": "string",
                "description": "The author of the new comment."
            },
            "status": {
                "$ref": "#/IssueCommentStatus"
            },
            "text": {
                "type": "string",
                "description": "Comment text to add."
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "text"
        ]
    })";
}

GS::Optional<GS::UniString> AddCommentToIssueCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

static API_MarkUpCommentStatusID ConvertStringToMarkUpCommentStatusID (const GS::UniString& str)
{
    if (str == "Error") return APIComment_Error;
    if (str == "Warning") return APIComment_Warning;
    if (str == "Info") return APIComment_Info;
    return APIComment_Unknown;
}

static GS::UniString ConvertMarkUpCommentStatusIDToString (API_MarkUpCommentStatusID id)
{
    switch (id) {
        case APIComment_Error: return "Error";
        case APIComment_Warning: return "Warning";
        case APIComment_Info: return "Info";
        default: return "Unknown";
    }
}

GS::ObjectState AddCommentToIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState issueId;
    GS::UniString text;
    GS::UniString author = "API";
    GS::UniString status;

    parameters.Get ("author", author);
    parameters.Get ("status", status);
    if (!parameters.Get ("issueId", issueId) || !parameters.Get ("text", text)) {
        return CreateFailedExecutionResult (Error, "Invalid input parameters.");
    }

    API_Guid guid = GetGuidFromObjectState (issueId);
    GSErrCode err = ACAPI_CallUndoableCommand ("Add comment", [&]() -> GSErrCode {
        API_MarkUpCommentType comment (author, text, ConvertStringToMarkUpCommentStatusID (status));
        return ACAPI_Markup_AddComment (guid, comment);
    });

    if (err != NoError)
        return CreateFailedExecutionResult (err, "Failed to create a comment.");

    return CreateSuccessfulExecutionResult ();
}

GetCommentsFromIssueCommand::GetCommentsFromIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCommentsFromIssueCommand::GetName () const
{
    return "GetCommentsFromIssue";
}

GS::Optional<GS::UniString> GetCommentsFromIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId"
        ]
    })";
}

GS::Optional<GS::UniString> GetCommentsFromIssueCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "comments": {
                "type": "array",
                "description": "A list of existing comments.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "$ref": "#/Guid",
                            "description": "Comment identifier"
                        },
                        "author": {
                            "type": "string",
                            "description": "Comment author"
                        },
                        "text": {
                            "type": "string",
                            "description": "Comment text"
                        },
                        "status": {
                            "$ref": "#/IssueCommentStatus"
                        },
                        "creaTime": {
                            "type": "integer",
                            "description": "Comment creation time"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid",
                        "author",
                        "text",
                        "status",
                        "creaTime"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "comments"
        ]
    })";
}

GS::ObjectState GetCommentsFromIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState issueId;
    if (!parameters.Get ("issueId", issueId)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid input parameters.");
    }

    GS::Array<API_MarkUpCommentType> comments;
    GSErrCode err = ACAPI_Markup_GetComments (GetGuidFromObjectState (issueId), &comments);

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get comments for the given issue.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("comments");

    for (const API_MarkUpCommentType& comment : comments) {
        GS::ObjectState commentData;
        commentData.Add ("guid", APIGuidToString (comment.guid));
        commentData.Add ("author", comment.author);
        commentData.Add ("text", comment.text);
        commentData.Add ("status", ConvertMarkUpCommentStatusIDToString (comment.status));
        commentData.Add ("creaTime", comment.creaTime);
        listAdder (commentData);
    }

    return response;
}

AttachElementsToIssueCommand::AttachElementsToIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String AttachElementsToIssueCommand::GetName () const
{
    return "AttachElementsToIssue";
}

GS::Optional<GS::UniString> AttachElementsToIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "elements": {
                "$ref": "#/Elements"
            },
            "type": {
                "$ref": "#/IssueElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "elements",
            "type"
        ]
    })";
}

GS::Optional<GS::UniString> AttachElementsToIssueCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

static API_MarkUpComponentTypeID ConvertStringToMarkUpComponentTypeID (const GS::UniString& type)
{
    if (type == "Creation") {
        return API_MarkUpComponentTypeID (0);
    } else if (type == "Highlight") {
        return API_MarkUpComponentTypeID (1);
    } else if (type == "Deletion") {
        return API_MarkUpComponentTypeID (2);
    } else if (type == "Modification") {
        return API_MarkUpComponentTypeID (3);
    }

    return API_MarkUpComponentTypeID (0);
}

GS::ObjectState AttachElementsToIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::UniString type;
    GS::ObjectState issueId;
    GS::Array<GS::ObjectState> elements;

    if (!parameters.Get ("type", type) || !parameters.Get ("issueId", issueId) || !parameters.Get ("elements", elements)) {
        return CreateFailedExecutionResult (Error, "Invalid input parameters.");
    }

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

    GSErrCode err = ACAPI_CallUndoableCommand ("Attach elements", [&]() -> GSErrCode {
        return TAPIR_MarkUp_AttachElements (GetGuidFromObjectState (issueId), elemIds, ConvertStringToMarkUpComponentTypeID (type));
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (Error, "Failed to attach elements.");
    }

    return CreateSuccessfulExecutionResult ();
}

DetachElementsFromIssueCommand::DetachElementsFromIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DetachElementsFromIssueCommand::GetName () const
{
    return "DetachElementsFromIssue";
}

GS::Optional<GS::UniString> DetachElementsFromIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> DetachElementsFromIssueCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState DetachElementsFromIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState issueId;
    GS::Array<GS::ObjectState> elements;

    if (!parameters.Get ("issueId", issueId) || !parameters.Get ("elements", elements)) {
        return CreateFailedExecutionResult (Error, "Invalid input parameters.");
    }

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

    GSErrCode err = ACAPI_CallUndoableCommand ("Detach elements", [&]() -> GSErrCode {
        return ACAPI_Markup_DetachElements (GetGuidFromObjectState (issueId), elemIds);
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (Error, "Failed to detach elements.");
    }

    return CreateSuccessfulExecutionResult ();
}

GetElementsAttachedToIssueCommand::GetElementsAttachedToIssueCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementsAttachedToIssueCommand::GetName () const
{
    return "GetElementsAttachedToIssue";
}

GS::Optional<GS::UniString> GetElementsAttachedToIssueCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "type": {
                "$ref": "#/IssueElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "type"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementsAttachedToIssueCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": true,
        "required": [
            "elements"
        ]
    })";
}

GS::ObjectState GetElementsAttachedToIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString type;
    GS::ObjectState issueId;

    if (!parameters.Get ("type", type) || !parameters.Get ("issueId", issueId)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid input parameters.");
    }

    GS::ObjectState response;
    GS::Array<API_Guid> attachedElements;

    GSErrCode err = TAPIR_MarkUp_GetAttachedElements (GetGuidFromObjectState (issueId), ConvertStringToMarkUpComponentTypeID (type), attachedElements);
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve attached elements.");
    }

    for (const API_Guid& guid : attachedElements) {
        elements (CreateElementIdObjectState (guid));
    }

    return response;
}

ExportIssuesToBCFCommand::ExportIssuesToBCFCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ExportIssuesToBCFCommand::GetName () const
{
    return "ExportIssuesToBCF";
}

GS::Optional<GS::UniString> ExportIssuesToBCFCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issues": {
                "$ref": "#/Issues",
                "description": "Leave it empty to export all issues."
            },
            "exportPath": {
                "type": "string",
                "description": "The os path to the bcf file, including it's name."
            },
            "useExternalId": {
                "type": "boolean",
                "description": "Use external IFC ID or Archicad IFC ID as referenced in BCF topics."
            },
            "alignBySurveyPoint": {
                "type": "boolean",
                "description": "Align BCF views by Archicad Survey Point or Archicad Project Origin."
            }
        },
        "additionalProperties": false,
        "required": [
            "exportPath",
            "useExternalId",
            "alignBySurveyPoint"
        ]
    })";
}

GS::Optional<GS::UniString> ExportIssuesToBCFCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ExportIssuesToBCFCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString exportPath;
    GS::Array<GS::ObjectState> issues;
    GS::Array<API_Guid> issueIds;
    bool useExternalId = false;
    bool alignBySurveyPoint = true;

    if (!parameters.Get ("exportPath", exportPath) || !parameters.Get ("useExternalId", useExternalId) || !parameters.Get ("alignBySurveyPoint", alignBySurveyPoint)) {
        return CreateFailedExecutionResult (Error, "Invalid input parameters.");
    }

    parameters.Get ("issues", issues);
    if (issues.IsEmpty ()) {
        GS::Array<API_MarkUpType> issues;
        GSErrCode err = ACAPI_Markup_GetList (APINULLGuid, &issues);
        if (err == NoError) {
            for (const auto& i : issues) {
                issueIds.Push (i.guid);
            }
        }
    } else {
        issueIds = issues.Transform<API_Guid> (GetGuidFromIssuesArrayItem);
    }

    IO::Location bcfFilePath (exportPath);
    GSErrCode err = ACAPI_Markup_ExportToBCF (bcfFilePath, issueIds, useExternalId, alignBySurveyPoint);

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to export issues.");
    }

    return CreateSuccessfulExecutionResult ();
}

ImportIssuesFromBCFCommand::ImportIssuesFromBCFCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ImportIssuesFromBCFCommand::GetName () const
{
    return "ImportIssuesFromBCF";
}

GS::Optional<GS::UniString> ImportIssuesFromBCFCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "importPath": {
                "type": "string",
                "description": "The os path to the bcf file, including it's name."
            },
            "alignBySurveyPoint": {
                "type": "boolean",
                "description": "Align BCF views by Archicad Survey Point or Archicad Project Origin."
            }
        },
        "additionalProperties": false,
        "required": [
            "importPath",
            "alignBySurveyPoint"
        ]
    })";
}

GS::Optional<GS::UniString> ImportIssuesFromBCFCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ImportIssuesFromBCFCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool alignBySurveyPoint = true;
    GS::UniString importPath;

    if (!parameters.Get ("importPath", importPath) || !parameters.Get ("alignBySurveyPoint", alignBySurveyPoint)) {
        return CreateFailedExecutionResult (Error, "Invalid input parameters.");
    }

    IO::Location bcfFilePath (importPath);
    GSErrCode err = ACAPI_CallUndoableCommand ("Import BCF Issues", [&]() -> GSErrCode {
        API_IFCRelationshipData ifcRelationshipData = GetCurrentProjectIFCRelationshipData ();
        return ACAPI_Markup_ImportFromBCF (bcfFilePath, true, &GetIFCRelationshipData, &ifcRelationshipData, false, alignBySurveyPoint);
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to import issues.");
    }

    return CreateSuccessfulExecutionResult ();
}