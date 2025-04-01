#include "NavigatorCommands.hpp"
#include "MigrationHelper.hpp"

static GS::HashTable<GS::UniString, API_Guid> GetPublisherSetNameGuidTable()
{
    GS::HashTable<GS::UniString, API_Guid> table;

    Int32 numberOfPublisherSets = 0;
    ACAPI_Navigator_GetNavigatorSetNum(&numberOfPublisherSets);

    API_NavigatorSet set = {};
    for (Int32 ii = 0; ii < numberOfPublisherSets; ++ii) {
        set.mapId = API_PublisherSets;
        GSErrCode err = ACAPI_Navigator_GetNavigatorSet(&set, &ii);
        if (err == NoError) {
            table.Add(set.name, set.rootGuid);
        }
    }

    return table;
}

PublishPublisherSetCommand::PublishPublisherSetCommand() :
    CommandBase(CommonSchema::NotUsed)
{
}

GS::String PublishPublisherSetCommand::GetName() const
{
    return "PublishPublisherSet";
}

GS::Optional<GS::UniString> PublishPublisherSetCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "publisherSetName": {
                "type": "string",
                "description": "The name of the publisher set.",
                "minLength": 1
            },
            "outputPath": {
                "type": "string",
                "description": "Full local or LAN path for publishing. Optional, by default the path set in the settings of the publiser set will be used.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "publisherSetName"
        ]
    })";
}

GS::ObjectState PublishPublisherSetCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString publisherSetName;
    parameters.Get("publisherSetName", publisherSetName);

    const auto publisherSetNameGuidTable = GetPublisherSetNameGuidTable();
    if (!publisherSetNameGuidTable.ContainsKey(publisherSetName)) {
        return CreateErrorResponse(Error, "Not valid publisher set name.");
    }

    API_PublishPars publishPars = {};
    publishPars.guid = publisherSetNameGuidTable.Get(publisherSetName);

    if (parameters.Contains("outputPath")) {
        GS::UniString outputPath;
        parameters.Get("outputPath", outputPath);
        publishPars.path = new IO::Location(outputPath);
    }

    GSErrCode err = ACAPI_ProjectOperation_Publish(&publishPars);
    delete publishPars.path;

    if (err != NoError) {
        return CreateErrorResponse(err, "Publishing failed. Check output path!");
    }

    return {};
}

UpdateDrawingsCommand::UpdateDrawingsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String UpdateDrawingsCommand::GetName() const
{
    return "UpdateDrawings";
}

GS::Optional<GS::UniString> UpdateDrawingsCommand::GetInputParametersSchema() const
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

GS::Optional<GS::UniString> UpdateDrawingsCommand::GetResponseSchema() const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState UpdateDrawingsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> elements;
    parameters.Get("elements", elements);

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid>(GetGuidFromElementsArrayItem);
    GSErrCode err = ACAPI_Drawing_Update_Drawings(elemIds);

    return err == NoError
        ? CreateSuccessfulExecutionResult()
        : CreateFailedExecutionResult(err, "Failed to update drawings.");
#else
    (void) parameters;
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "DrawingUpdateCommand is not supported in Archicad versions earlier than 27.");
#endif
}
