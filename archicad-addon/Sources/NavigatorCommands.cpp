#include "NavigatorCommands.hpp"
#include "MigrationHelper.hpp"
#include "Transformation2D.hpp"
#include "Matrix2.hpp"
#include "TM.h"

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

GetDatabaseIdFromNavigatorItemIdCommand::GetDatabaseIdFromNavigatorItemIdCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetDatabaseIdFromNavigatorItemIdCommand::GetName () const
{
    return "GetDatabaseIdFromNavigatorItemId";
}

GS::Optional<GS::UniString> GetDatabaseIdFromNavigatorItemIdCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "navigatorItemIds": {
            "$ref": "#/NavigatorItemIds"
        }
    },
    "additionalProperties": false,
    "required": [
        "navigatorItemIds"
    ]
})";
}

GS::Optional<GS::UniString> GetDatabaseIdFromNavigatorItemIdCommand::GetResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "databases": {
            "$ref": "#/Databases"
        }
    },
    "additionalProperties": false,
    "required": [
        "databases"
    ]
})";
}

GS::ObjectState GetDatabaseIdFromNavigatorItemIdCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> navigatorItemIds;
    parameters.Get ("navigatorItemIds", navigatorItemIds);

    GS::ObjectState response;
    const auto& databases = response.AddList<GS::ObjectState> ("databases");

    for (const GS::ObjectState& navigatorItemIdArrayItem : navigatorItemIds) {
        API_Guid navGuid = GetGuidFromNavigatorItemIdArrayItem (navigatorItemIdArrayItem);
        if (navGuid == APINULLGuid) {
            databases (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing"));
            continue;
        }

        API_NavigatorItem navigatorItem = {};
        GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&navGuid, &navigatorItem);

        if (err != NoError) {
            databases (CreateErrorResponse (err, "Failed to get navigator item from guid"));
            continue;
        }

        const API_Guid databaseGuid = DatabaseIdResolver::Instance ().GetIdOfDatabase (navigatorItem.db);

        if (databaseGuid == APINULLGuid) {
            databases (CreateErrorResponse (APIERR_BADPARS, "Navigator item {navigatorItem.itemType} has no associated database"));
            continue;
        }

        databases (CreateDatabaseIdObjectState (databaseGuid));
    }
    return response;
}

GetModelViewOptionsCommand::GetModelViewOptionsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetModelViewOptionsCommand::GetName () const
{
    return "GetModelViewOptions";
}

GS::Optional<GS::UniString> GetModelViewOptionsCommand::GetResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "modelViewOptions": {
            "type": "array",
            "items": {
                "type": "object",
                "description": "Represents the model view options.",
                "properties": {
                    "name": {
                        "type": "string"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "name"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "modelViewOptions"
    ]
})";
}

GS::ObjectState GetModelViewOptionsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState response;
    const auto& modelViewOptions = response.AddList<GS::ObjectState> ("modelViewOptions");

#ifdef ServerMainVers_2700
    UInt32 count = 0;
    ACAPI_Navigator_ModelViewOptions_GetNum (count);

    for (UInt32 i = 1; i <= count; ++i) {
        GS::UniString name;
        API_ModelViewOptionsType modelViewOption = {};
        modelViewOption.head.index = i;
        modelViewOption.head.uniStringNamePtr = &name;

        if (ACAPI_Navigator_ModelViewOptions_Get (&modelViewOption) == NoError) {
            modelViewOptions (GS::ObjectState ("name", name));
        }
    }
#else
    API_AttributeIndex count = 0;
    ACAPI_Attribute_GetNum (API_ModelViewOptionsID, &count);

    for (API_AttributeIndex i = 1; i <= count; ++i) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_ModelViewOptionsID;
        attr.header.index = i;
        attr.header.uniStringNamePtr = &name;

        if (ACAPI_Attribute_Get (&attr) == NoError) {
            modelViewOptions (GS::ObjectState ("name", name));
        }
    }
#endif

    return response;
}

GetViewSettingsCommand::GetViewSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetViewSettingsCommand::GetName () const
{
    return "GetViewSettings";
}

GS::Optional<GS::UniString> GetViewSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemIds": {
                "$ref": "#/NavigatorItemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "navigatorItemIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetViewSettingsCommand::GetResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "viewSettings": {
            "type": "array",
            "items": {
                "$ref": "#/ViewSettingsOrError"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "viewSettings"
    ]
})";
}

GS::ObjectState GetViewSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> navigatorItemIds;
    parameters.Get ("navigatorItemIds", navigatorItemIds);

    GS::ObjectState response;
    const auto& viewSettings = response.AddList<GS::ObjectState> ("viewSettings");

    for (const GS::ObjectState& navigatorItemIdArrayItem : navigatorItemIds) {
        API_NavigatorItem navigatorItem = {};
        navigatorItem.guid = GetGuidFromNavigatorItemIdArrayItem (navigatorItemIdArrayItem);
        if (navigatorItem.guid == APINULLGuid) {
            viewSettings (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing"));
            continue;
        }

        navigatorItem.mapId = API_PublicViewMap;

        API_NavigatorView navigatorView = {};
        GSErrCode err = ACAPI_Navigator_GetNavigatorView (&navigatorItem, &navigatorView);
        if (err != NoError) {
            viewSettings (CreateErrorResponse (err, "Failed to get view settings from the navigator item, probably it's not a view"));
            continue;
        }

        viewSettings (GS::ObjectState (
            "modelViewOptions", navigatorView.modelViewOptName,
            "layerCombination", navigatorView.layerCombination,
            "dimensionStyle", navigatorView.dimName,
            "penSetName", navigatorView.penSetName,
            "graphicOverrideCombination", navigatorView.overrideCombination));
    }

    return response;
}

SetViewSettingsCommand::SetViewSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetViewSettingsCommand::GetName () const
{
    return "SetViewSettings";
}

GS::Optional<GS::UniString> SetViewSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemIdsWithViewSettings": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId"
                        },
                        "viewSettings": {
                            "$ref": "#/ViewSettings"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "navigatorItemId",
                        "viewSettings"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "navigatorItemIdsWithViewSettings"
        ]
    })";
}

GS::Optional<GS::UniString> SetViewSettingsCommand::GetResponseSchema () const
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

GS::ObjectState SetViewSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> navigatorItemIdsWithViewSettings;
    parameters.Get ("navigatorItemIdsWithViewSettings", navigatorItemIdsWithViewSettings);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& navigatorItemIdWithViewSetting : navigatorItemIdsWithViewSettings) {
        const GS::ObjectState* viewSettingsOS = navigatorItemIdWithViewSetting.Get ("viewSettings");
        if (viewSettingsOS == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "viewSettings input is missing"));
            continue;
        }
    
        API_Guid guid = GetGuidFromNavigatorItemIdArrayItem (navigatorItemIdWithViewSetting);
        if (guid == APINULLGuid) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId is corrupt or missing"));
            continue;
        }

        API_NavigatorItem navigatorItem = {};
        navigatorItem.mapId = API_PublicViewMap;
        GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&guid, &navigatorItem);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to get navigator item from guid"));
            continue;
        }


        API_NavigatorView navigatorView = {};
        err = ACAPI_Navigator_GetNavigatorView (&navigatorItem, &navigatorView);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to get navigator view settings from the navigator item"));
            continue;
        }

        SetCharProperty (viewSettingsOS, "modelViewOptions", navigatorView.modelViewOptName);
        SetCharProperty (viewSettingsOS, "layerCombination", navigatorView.layerCombination);
        SetCharProperty (viewSettingsOS, "dimensionStyle", navigatorView.dimName);
        SetCharProperty (viewSettingsOS, "penSetName", navigatorView.penSetName);
        SetUCharProperty (viewSettingsOS, "graphicOverrideCombination", navigatorView.overrideCombination);


        err = ACAPI_Navigator_ChangeNavigatorView (&navigatorItem, &navigatorView);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to set navigator item view settings"));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

GetView2DTransformationsCommand::GetView2DTransformationsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetView2DTransformationsCommand::GetName () const
{
    return "GetView2DTransformations";
}

GS::Optional<GS::UniString> GetView2DTransformationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "databases": {
                 "$ref": "#/Databases"
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetView2DTransformationsCommand::GetResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "transformations": {
            "type": "array",
            "items": {
                "$ref": "#/ViewTransformationsOrError"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "transformations"
    ]
})";
}

template <typename ListProxyType>
static GSErrCode GetTransformationFromCurrentDatabase (ListProxyType& transformationsListProxy)
{
    API_Box     zoomBox = {};
    API_Tranmat tranmat = {};
    GSErrCode err = ACAPI_View_GetZoom (&zoomBox, &tranmat);
    if (err != NoError) {
        return err;
    }

	Vector2D vec2D1 (tranmat.tmx[0], tranmat.tmx[4]);
	Vector2D vec2D2 (tranmat.tmx[1], tranmat.tmx[5]);
	Vector2D offset (tranmat.tmx[3], tranmat.tmx[7]);
	Geometry::Matrix22 matrix;
	Geometry::Matrix22::ColVectorsMatrix (vec2D1, vec2D2, matrix);
	Geometry::Transformation2D trafo;
	trafo.SetMatrix (matrix);
	trafo.SetOffset (offset);

    double tranAngle = 0;
    Geometry::TranAngle (trafo, &tranAngle);

    transformationsListProxy (
        GS::ObjectState (
            "zoom", GS::ObjectState (
                "xMin", zoomBox.xMin,
                "yMin", zoomBox.yMin,
                "xMax", zoomBox.xMax,
                "yMax", zoomBox.yMax),
            "rotation", tranAngle));

    return err;
}

GS::ObjectState GetView2DTransformationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState response;
    const auto& transformations = response.AddList<GS::ObjectState> ("transformations");

    GS::Array<GS::ObjectState> databases;
    if (!parameters.Get ("databases", databases)) {
        GetTransformationFromCurrentDatabase (transformations);
    } else {
        const GS::Array<API_Guid> databaseIds = databases.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

        auto action = [&]() -> GSErrCode {
            return GetTransformationFromCurrentDatabase (transformations);
        };
        auto actionSuccess = [&]() -> void {};
        auto actionFailure = [&](GSErrCode err, const GS::UniString& errMsg) -> void {
            transformations (CreateErrorResponse (err, errMsg));
        };

        GSErrCode err = ExecuteActionForEachDatabase (databaseIds, action, actionSuccess, actionFailure);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to retrieve the starting database or to switch back to it after execution.");
        }
    }

    return response;
}