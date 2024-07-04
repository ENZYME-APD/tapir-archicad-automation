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


CreateIssueCommand::CreateIssueCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String CreateIssueCommand::GetName () const
{
    return "CreateIssue";
}

GS::ObjectState CreateIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    GS::UniString parentIdStr = "";
    GS::UniString tagText = "";

    parameters.Get ("parentId", parentIdStr);
    parameters.Get ("tagText", tagText);
    if (!parameters.Get ("name", name)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    API_MarkUpType issue (name);
    issue.tagText = tagText;
    if (parentIdStr != "")
        issue.parentGuid = APIGuidFromString (parentIdStr.ToCStr ());

    GSErrCode err = ACAPI_CallUndoableCommand ("Create issue", [&]() -> GSErrCode {
        err = ACAPI_Markup_Create (issue);
        return err;
    });
   
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create issue.");
    }

    return {};
}

DeleteIssueCommand::DeleteIssueCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String DeleteIssueCommand::GetName () const
{
    return "DeleteIssue";
}

GS::ObjectState DeleteIssueCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString issueIdStr;
    bool acceptAllElements = false;

    if (!parameters.Get ("issueId", issueIdStr)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    parameters.Get ("acceptAllElements", acceptAllElements);
    API_Guid guid = APIGuidFromString (issueIdStr.ToCStr());
    GSErrCode err = ACAPI_CallUndoableCommand ("Delete issue", [&]() -> GSErrCode {
        err = ACAPI_Markup_Delete (guid, acceptAllElements);
        return err;
    });

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to delete issue.");
    }

    return {};
}

GetIssuesCommand::GetIssuesCommand () :
    CommandBase (CommonSchema::NotUsed)
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
                        "guid": {
                            "type": "string",
                            "description": "Issue identifier"
                        },
                        "name": {
                            "type": "string",
                            "description": "Issue name"
                        },
                        "parentGuid": {
                            "type": "string",
                            "description": "The identifier of the parent issue"
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
                        "tagTextElemGuid": {
                            "type": "string",
                            "description": "The identifier of the attached tag text element"
                        },
                        "isTagTextElemVisible": {
                            "type": "boolean",
                            "description": "The visibility of the attached tag text element"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid",
                        "name",
                        "parentGuid",
                        "creaTime",
                        "modiTime",
                        "tagText",
                        "tagTextElemGuid",
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
        issueData.Add ("guid", APIGuidToString(i->guid));
        issueData.Add ("name", i->name);
        issueData.Add ("parentGuid", APIGuidToString (i->parentGuid));
        issueData.Add ("creaTime", i->creaTime);
        issueData.Add ("modiTime", i->modiTime);
        issueData.Add ("tagText", i->tagText);
        issueData.Add ("tagTextElemGuid", APIGuidToString (i->tagTextElemGuid));
        issueData.Add ("isTagTextElemVisible", i->isTagTextElemVisible);
        listAdder (issueData);
    }

    return response;
}

AddCommentCommand::AddCommentCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String AddCommentCommand::GetName () const
{
    return "AddComment";
}

GS::ObjectState AddCommentCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString issueIdStr;
    GS::UniString text;
    GS::UniString author = "API";
    int status;

    parameters.Get ("author", author);
    parameters.Get ("status", status);
    if (!parameters.Get ("issueId", issueIdStr) || !parameters.Get ("text", text)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }
    auto GetCommentStatus = [](int status) -> API_MarkUpCommentStatusID {
        if (status >= 0 && status <= 3) {
            return static_cast<API_MarkUpCommentStatusID>(status);
        } else {
            return APIComment_Unknown;
        }
    };

    API_Guid guid = APIGuidFromString (issueIdStr.ToCStr ());
    GSErrCode err = ACAPI_CallUndoableCommand ("Add comment", [&]() -> GSErrCode {
        API_MarkUpCommentType comment (author, text, GetCommentStatus (status));
        GSErrCode err = ACAPI_Markup_AddComment (guid, comment);
        return err;
    });

    if (err != NoError)
        return CreateErrorResponse (err, "Failed to create a comment.");

    return {};
}

GetCommentsCommand::GetCommentsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetCommentsCommand::GetName () const
{
    return "GetComments";
}

GS::Optional<GS::UniString> GetCommentsCommand::GetResponseSchema () const
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
                            "type": "string",
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
                            "type": "string",
                            "description": "Comment status"
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

GS::ObjectState GetCommentsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString issueIdStr;
    if (!parameters.Get ("issueId", issueIdStr)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    API_Guid issueId = APIGuidFromString (issueIdStr.ToCStr ());
    GS::Array<API_MarkUpCommentType> comments;
    ACAPI_Markup_GetComments (issueId, &comments);

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("comments");

    comments.Enumerate ([&listAdder](const API_MarkUpCommentType& comment) {
        auto GetCommentStatusStr = [](API_MarkUpCommentStatusID commentStatusID) -> const char* {
            switch (commentStatusID) {
                case APIComment_Error:		return "Error";
                case APIComment_Warning:	return "Warning";
                case APIComment_Info:		return "Info";
                case APIComment_Unknown:
                default:					return "Unknown";
            }
        };
        GS::ObjectState commentData;
        commentData.Add ("guid", APIGuidToString (comment.guid));
        commentData.Add ("author", comment.author);
        commentData.Add ("text", comment.text);
        commentData.Add ("status", GetCommentStatusStr (comment.status));
        commentData.Add ("creaTime", comment.creaTime);
        listAdder (commentData);
    });

    return response;
}

AttachElementsCommand::AttachElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String AttachElementsCommand::GetName () const
{
    return "AttachElements";
}

GS::ObjectState AttachElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& ) const
{
    int type;
    GS::UniString issueIdStr;
    API_Guid issueId;
    GS::Array<GS::UniString> elemIdsStr;
    GS::Array<API_Guid> elemIds;

    parameters.Get ("type", type);
    if (!parameters.Get ("issueId", issueIdStr) || !parameters.Get ("guids", elemIdsStr) || !(type >= 0 && type <= 3)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    } else {
        issueId = APIGuidFromString (issueIdStr.ToCStr ());
        for (ULong i = 0; i < elemIdsStr.GetSize (); ++i) {
            elemIds.Push (APIGuidFromString (elemIdsStr[i].ToCStr ()));
        }
    }

    GSErrCode err = ACAPI_CallUndoableCommand ("Attach elements", [&]() -> GSErrCode {
        err = TAPIR_MarkUp_AttachElements (issueId, elemIds, type);
        return err;
    });

    if (err != NoError) {
        return CreateErrorResponse (Error, "Failed to attach elements.");
    }

    return {};
}

DetachElementsCommand::DetachElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String DetachElementsCommand::GetName () const
{
    return "DetachElements";
}

GS::ObjectState DetachElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString issueIdStr;
    API_Guid issueId;
    GS::Array<GS::UniString> elemIdsStr;
    GS::Array<API_Guid> elemIds;

    if (!parameters.Get ("issueId", issueIdStr) || !parameters.Get ("guids", elemIdsStr)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    } 
    else {
        issueId = APIGuidFromString (issueIdStr.ToCStr ());
        for (ULong i = 0; i < elemIdsStr.GetSize (); ++i) {
            elemIds.Push (APIGuidFromString (elemIdsStr[i].ToCStr ()));
        }
    }

    GSErrCode err = ACAPI_CallUndoableCommand ("Detach elements", [&]() -> GSErrCode {
        err = ACAPI_Markup_DetachElements (issueId, elemIds);
        return err;
    });

    return {};
}

GetAttachedElementsCommand::GetAttachedElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetAttachedElementsCommand::GetName () const
{
    return "GetAttachedElements";
}

GS::Optional<GS::UniString> GetAttachedElementsCommand::GetResponseSchema () const
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

GS::ObjectState GetAttachedElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    int type;
    GS::ObjectState response;
    GS::Array<API_Guid> elemIds;
    GS::UniString issueIdStr;

    parameters.Get ("type", type);
    if (!parameters.Get ("issueId", issueIdStr) || !(type >= 0 && type <= 3)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    API_Guid issueId = APIGuidFromString (issueIdStr.ToCStr ());
    GSErrCode err = TAPIR_MarkUp_GetAttachedElements (issueId, type, elemIds);
    const auto& elemObj = response.AddList<GS::ObjectState> ("elements");

    if (err == NoError) {
        elemIds.Enumerate ([&elemObj](const API_Guid& guid) {
            GS::ObjectState elements;
            elements.Add ("guid", APIGuidToString (guid));
            elemObj (elements);
        });
    } else {
        return CreateErrorResponse (err, "Failed to retrieve attached elements.");
    }

   return response;
}

ExportToBCFCommand::ExportToBCFCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ExportToBCFCommand::GetName () const
{
    return "ExportToBCF";
}

GS::ObjectState ExportToBCFCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString exportPath;
    GS::Array<GS::UniString> issueIdsStr;
    GS::Array<API_Guid> issueIds;
    GS::Array<API_MarkUpType> issues;
    bool useExternalId = false;
    bool alignBySurveyPoint = true;

    if (!parameters.Get ("exportPath", exportPath) || !parameters.Get ("useExternalId", useExternalId) || !parameters.Get ("alignBySurveyPoint", alignBySurveyPoint)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    parameters.Get ("issueIds", issueIdsStr);
    if (issueIdsStr.IsEmpty ()) {
        GSErrCode err = ACAPI_Markup_GetList (APINULLGuid, &issues);
        if (err == NoError) {
        	for (const auto& issues : issues) {
                issueIds.Push (issues.guid);
        	}
        }
    } else {
        for (ULong i = 0; i < issueIdsStr.GetSize (); ++i) {
            issueIds.Push (APIGuidFromString (issueIdsStr[i].ToCStr ()));
        }
    }

    IO::Location bcfFilePath(exportPath);
    GSErrCode err = ACAPI_Markup_ExportToBCF (bcfFilePath, issueIds, useExternalId, alignBySurveyPoint);

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to export issues.");
    }

    return {};
}

ImportFromBCFCommand::ImportFromBCFCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ImportFromBCFCommand::GetName () const
{
    return "ImportFromBCF";
}

GS::ObjectState ImportFromBCFCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool alignBySurveyPoint = true;
    GS::UniString importPath;

    if (!parameters.Get ("importPath", importPath) || !parameters.Get ("alignBySurveyPoint", alignBySurveyPoint)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    IO::Location bcfFilePath (importPath);
    GSErrCode err = ACAPI_CallUndoableCommand ("Import BCF Issues", [&] () -> GSErrCode {
        API_IFCRelationshipData ifcRelationshipData = GetCurrentProjectIFCRelationshipData ();
        err = ACAPI_Markup_ImportFromBCF (bcfFilePath, true, &GetIFCRelationshipData, &ifcRelationshipData, false, alignBySurveyPoint);
        return err;
    });

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to import issues.");
    }

    return {};
}