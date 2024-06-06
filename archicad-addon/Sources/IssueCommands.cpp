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


GetIssueListCommand::GetIssueListCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetIssueListCommand::GetName () const
{
    return "GetIssueList";
}

GS::Optional<GS::UniString> GetIssueListCommand::GetResponseSchema () const
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

GS::ObjectState GetIssueListCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_MarkUpType> issueList;
    GSErrCode err = ACAPI_MarkUp_GetList (APINULLGuid, &issueList);
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
            "highlighted": {
                "type": "array",
                "description": "A list of highlighted elements in the current issue.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "type": "string",
                            "description": "Element identifier"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid"
                    ]
                }
            },
            "modified": {
                "type": "array",
                "description": "A list of modified elements in the current issue.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "type": "string",
                            "description": "Element identifier"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid"
                    ]
                }
            },
            "created": {
                "type": "array",
                "description": "A list of created elements in the current issue.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "type": "string",
                            "description": "Element identifier"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid"
                    ]
                }
            },
            "deleted": {
                "type": "array",
                "description": "A list of created deleted in the current issue.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "type": "string",
                            "description": "Element identifier"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "highlighted",
            "modified",
            "created",
            "deleted"
        ]
    })";
}

GS::ObjectState GetAttachedElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString guidStr;
    if (!parameters.Get ("issueGuid", guidStr)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }
    API_Guid guid = APIGuidFromString (guidStr.ToCStr ());
    GS::ObjectState response;
    GSErrCode err;

    const auto& highlightedObj = response.AddList<GS::ObjectState> ("highlighted");
    GS::Array<API_Guid> highlightedElems;
    err = ACAPI_MarkUp_GetAttachedElements (guid, APIMarkUpComponent_Highlight, highlightedElems);

    const auto& modifiedObj = response.AddList<GS::ObjectState> ("modified");
    GS::Array<API_Guid> modifiedElems;
    err = ACAPI_MarkUp_GetAttachedElements (guid, APIMarkUpComponent_Modification, modifiedElems);

    const auto& createdObj = response.AddList<GS::ObjectState> ("created");
    GS::Array<API_Guid> createdElems;
    err = ACAPI_MarkUp_GetAttachedElements (guid, APIMarkUpComponent_Creation, createdElems);

    const auto& deletedObj = response.AddList<GS::ObjectState> ("deleted");
    GS::Array<API_Guid> deletedElems;
    err = ACAPI_MarkUp_GetAttachedElements (guid, APIMarkUpComponent_Deletion, deletedElems);

    if (err == NoError) {
        highlightedElems.Enumerate ([&highlightedObj](const API_Guid& guid) {
            GS::ObjectState highlighted;
            highlighted.Add ("guid", APIGuidToString (guid));
            highlightedObj (highlighted);
        });
        modifiedElems.Enumerate ([&modifiedObj](const API_Guid& guid) {
            GS::ObjectState modified;
            modified.Add ("guid", APIGuidToString (guid));
            modifiedObj (modified);
        });
        createdElems.Enumerate ([&createdObj](const API_Guid& guid) {
            GS::ObjectState created;
            created.Add ("guid", APIGuidToString (guid));
            createdObj (created);
        });
        deletedElems.Enumerate ([&deletedObj](const API_Guid& guid) {
            GS::ObjectState deleted;
            deleted.Add ("guid", APIGuidToString (guid));
            deletedObj (deleted);
        });
    }

    if (err != NoError)
        return CreateErrorResponse (err, "Failed to retrieve attached elements.");

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
        GSErrCode err = ACAPI_MarkUp_GetList (APINULLGuid, &issues);
        if (err == NoError) {
        	for (const auto& issues : issues) {
                issueIds.Push (issues.guid);
        	}
        }
    }
    else {
        for (ULong i = 0; i < issueIdsStr.GetSize (); ++i) {
            issueIds.Push (APIGuidFromString (issueIdsStr[i].ToCStr ()));
        }
    }

    IO::Location bcfFilePath(exportPath);
    GSErrCode err = ACAPI_MarkUp_ExportToBCF (bcfFilePath, issueIds, useExternalId, alignBySurveyPoint);

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
        err = ACAPI_MarkUp_ImportFromBCF (bcfFilePath, true, &GetIFCRelationshipData, &ifcRelationshipData, false, alignBySurveyPoint);
        return err;
    });

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to import issues.");
    }

    return {};
}