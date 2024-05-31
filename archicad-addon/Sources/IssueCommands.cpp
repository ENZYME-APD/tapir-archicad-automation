#include "IssueCommands.hpp"
#include "MigrationHelper.hpp"

GetIssueCommand::GetIssueCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetIssueCommand::GetName () const
{
    return "GetIssue";
}

GS::Optional<GS::UniString> GetIssueCommand::GetResponseSchema () const
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
                        },
                        "highlightedElems": {
                            "type": "array",
                            "description": "A list of highlighted elements.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "guid": {
                                        "type": "string",
                                        "description": "Identifier of highlighted element"
                                    }
                                }
                            }
                        },
                        "modifiedElems": {
                            "type": "array",
                            "description": "A list of modified elements.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "guid": {
                                        "type": "string",
                                        "description": "Identifier of modified element"
                                    }
                                }
                            }
                        },
                        "createdElems": {
                            "type": "array",
                            "description": "A list of created elements.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "guid": {
                                        "type": "string",
                                        "description": "Identifier of created element"
                                    }
                                }
                            }
                        },
                        "deletedElems": {
                            "type": "array",
                            "description": "A list of deleted elements.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "guid": {
                                        "type": "string",
                                        "description": "Identifier of deleted element"
                                    }
                                }
                            }
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

GS::ObjectState GetIssueCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
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

        GS::Array<API_Guid> highlightedElems;
        ACAPI_MarkUp_GetAttachedElements (i->guid, APIMarkUpComponent_Highlight, highlightedElems);
        const auto& highlightedObj = issueData.AddList<GS::ObjectState> ("highlightedElems");

        highlightedElems.Enumerate ([&highlightedObj](const API_Guid& guid) {
            GS::ObjectState highlightedData;
            highlightedData.Add ("guid", APIGuidToString (guid));
            highlightedObj (highlightedData);
        });

        GS::Array<API_Guid> modifiedElems;
        ACAPI_MarkUp_GetAttachedElements (i->guid, APIMarkUpComponent_Modification, modifiedElems);
        const auto& modifiedObj = issueData.AddList<GS::ObjectState> ("modifiedElems");

        modifiedElems.Enumerate ([&modifiedObj](const API_Guid& guid) {
            GS::ObjectState modifiedData;
            modifiedData.Add ("guid", APIGuidToString (guid));
            modifiedObj (modifiedData);
        });

        GS::Array<API_Guid> createdElems;
        ACAPI_MarkUp_GetAttachedElements (i->guid, APIMarkUpComponent_Creation, createdElems);
        const auto& createdObj = issueData.AddList<GS::ObjectState> ("createdElems");

        createdElems.Enumerate ([&createdObj](const API_Guid& guid) {
            GS::ObjectState createdData;
            createdData.Add ("guid", APIGuidToString (guid));
            createdObj (createdData);
        });

        GS::Array<API_Guid> deletedElems;
        ACAPI_MarkUp_GetAttachedElements (i->guid, APIMarkUpComponent_Deletion, deletedElems);
        const auto& deletedObj = issueData.AddList<GS::ObjectState> ("deletedElems");

        deletedElems.Enumerate ([&deletedObj](const API_Guid& guid) {
            GS::ObjectState deletedData;
            deletedData.Add ("guid", APIGuidToString (guid));
            deletedObj (deletedData);
        });

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

GS::ObjectState ExportToBCFCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    IO::Location bcfFilePath("C:\\Users\\i.yurasov\\Desktop\\dev\\issues.bcf");

    GS::Array<API_Guid> issueEntryIds;
	GS::Array<API_MarkUpType> issues;

	GSErrCode err = ACAPI_MarkUp_GetList (APINULLGuid, &issues);

	if (err == NoError) {
		for (const auto& issues : issues) {
			issueEntryIds.Push (issues.guid);
		}
	}

    if (err == NoError)
        err = ACAPI_MarkUp_ExportToBCF (bcfFilePath, issueEntryIds, false, true);

    if (err != NoError)
        return CreateErrorResponse (err, "Failed to export issues.");

    return {};
}