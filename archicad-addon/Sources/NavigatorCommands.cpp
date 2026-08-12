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
                "description": "Full local or LAN path for publishing. Optional, by default the path set in the settings of the publisher set will be used.",
                "minLength": 1
            },
            "selectedNavigatorItemIds": {
                "$ref": "#/NavigatorItemIds",
                "description": "Optional publisher-tree navigator items to publish instead of the whole publisher set."
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

    GS::Array<API_Guid> selectedLinks;
    const GS::Array<API_Guid>* selectedLinksPtr = nullptr;
    if (parameters.Contains("selectedNavigatorItemIds")) {
        GS::Array<GS::ObjectState> selectedNavigatorItemIds;
        parameters.Get("selectedNavigatorItemIds", selectedNavigatorItemIds);

        for (const GS::ObjectState& navigatorItemIdArrayItem : selectedNavigatorItemIds) {
            const API_Guid selectedGuid = GetGuidFromNavigatorItemIdArrayItem(navigatorItemIdArrayItem);
            if (selectedGuid == APINULLGuid) {
                delete publishPars.path;
                return CreateErrorResponse(APIERR_BADPARS, "selectedNavigatorItemId is corrupt or missing.");
            }
            selectedLinks.Push(selectedGuid);
        }

        if (!selectedLinks.IsEmpty()) {
            selectedLinksPtr = &selectedLinks;
        }
    }

    GSErrCode err = ACAPI_ProjectOperation_Publish(&publishPars, selectedLinksPtr);
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

        GS::UniString graphicOverrideCombinationName (navigatorView.overrideCombination);
        GS::ObjectState viewSetting (
            "modelViewOptions", navigatorView.modelViewOptName,
            "layerCombination", navigatorView.layerCombination,
            "dimensionStyle", navigatorView.dimName,
            "penSetName", navigatorView.penSetName,
            "graphicOverrideCombination", graphicOverrideCombinationName,
            "drawingScale", navigatorView.drawingScale,
            "saveZoom", navigatorView.saveZoom,
            "ignoreSavedZoom", navigatorView.ignoreSavedZoom);

        if (navigatorView.saveZoom) {
            viewSetting.Add ("zoom", GS::ObjectState (
                "xMin", navigatorView.zoom.xMin,
                "yMin", navigatorView.zoom.yMin,
                "xMax", navigatorView.zoom.xMax,
                "yMax", navigatorView.zoom.yMax));
        }

        {
            Vector2D vec2D1 (navigatorView.tr.tmx[0], navigatorView.tr.tmx[4]);
            Vector2D vec2D2 (navigatorView.tr.tmx[1], navigatorView.tr.tmx[5]);
            Vector2D offset (navigatorView.tr.tmx[3], navigatorView.tr.tmx[7]);
            Geometry::Matrix22 rotMatrix;
            Geometry::Matrix22::ColVectorsMatrix (vec2D1, vec2D2, rotMatrix);
            Geometry::Transformation2D trafo;
            trafo.SetMatrix (rotMatrix);
            trafo.SetOffset (offset);
            double rotAngle = 0.0;
            Geometry::TranAngle (trafo, &rotAngle);
            viewSetting.Add ("rotation", rotAngle);
        }

        const char* structureDisplayStr = "EntireStructure";
        switch (navigatorView.structureDisplay) {
            case API_CoreOnly:        structureDisplayStr = "CoreOnly";        break;
            case API_WithoutFinishes: structureDisplayStr = "WithoutFinishes"; break;
            case API_StructureOnly:   structureDisplayStr = "StructureOnly";   break;
            default: break;
        }
        viewSetting.Add ("structureDisplay", GS::UniString (structureDisplayStr));

        if (navigatorView.renovationFilterGuid != APINULLGuid) {
            viewSetting.Add ("renovationFilterGuid", CreateGuidObjectState (navigatorView.renovationFilterGuid));
        }

        GS::UniString d3styleName (navigatorView.d3styleName);
        viewSetting.Add ("d3styleName", d3styleName);

        GS::UniString renderingSceneName (navigatorView.renderingSceneName);
        viewSetting.Add ("renderingSceneName", renderingSceneName);

        viewSetting.Add ("usePhotoRendering", navigatorView.usePhotoRendering);

        viewSettings (viewSetting);
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
        if (viewSettingsOS->Get ("drawingScale", navigatorView.drawingScale)) {
            navigatorView.saveDScale = true;
        }
        viewSettingsOS->Get ("saveZoom", navigatorView.saveZoom);
        viewSettingsOS->Get ("ignoreSavedZoom", navigatorView.ignoreSavedZoom);
        const GS::ObjectState* zoomOS = viewSettingsOS->Get ("zoom");
        if (zoomOS != nullptr &&
            zoomOS->Get ("xMin", navigatorView.zoom.xMin) &&
            zoomOS->Get ("yMin", navigatorView.zoom.yMin) &&
            zoomOS->Get ("xMax", navigatorView.zoom.xMax) &&
            zoomOS->Get ("yMax", navigatorView.zoom.yMax)) {
            navigatorView.saveZoom = true;
        }

        double rotation = 0.0;
        if (viewSettingsOS->Get ("rotation", rotation)) {
            const double c = cos (rotation);
            const double s = sin (rotation);
            navigatorView.tr.tmx[0]  = c;   navigatorView.tr.tmx[1]  = -s;  navigatorView.tr.tmx[2]  = 0.0;
            navigatorView.tr.tmx[4]  = s;   navigatorView.tr.tmx[5]  = c;   navigatorView.tr.tmx[6]  = 0.0;
            navigatorView.tr.tmx[8]  = 0.0; navigatorView.tr.tmx[9]  = 0.0; navigatorView.tr.tmx[10] = 1.0;
            navigatorView.tr.tmx[11] = 0.0;
        }

        GS::UniString structureDisplayStr;
        if (viewSettingsOS->Get ("structureDisplay", structureDisplayStr)) {
            if      (structureDisplayStr == "CoreOnly")         navigatorView.structureDisplay = API_CoreOnly;
            else if (structureDisplayStr == "WithoutFinishes")  navigatorView.structureDisplay = API_WithoutFinishes;
            else if (structureDisplayStr == "StructureOnly")    navigatorView.structureDisplay = API_StructureOnly;
            else                                                navigatorView.structureDisplay = API_EntireStructure;
            navigatorView.saveStructureDisplay = true;
        }

        const GS::ObjectState* renovFilterOS = viewSettingsOS->Get ("renovationFilterGuid");
        if (renovFilterOS != nullptr) {
            navigatorView.renovationFilterGuid = GetGuidFromObjectState (*renovFilterOS);
        }

        SetUCharProperty (viewSettingsOS, "d3styleName", navigatorView.d3styleName);
        SetUCharProperty (viewSettingsOS, "renderingSceneName", navigatorView.renderingSceneName);
        viewSettingsOS->Get ("usePhotoRendering", navigatorView.usePhotoRendering);

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
            "navigatorItemIds": {
                "$ref": "#/NavigatorItemIds"
            },
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

    auto runForDatabaseIds = [&](const GS::Array<API_Guid>& databaseIds) -> GSErrCode {
        auto action = [&]() -> GSErrCode {
            return GetTransformationFromCurrentDatabase (transformations);
        };
        auto actionSuccess = [&]() -> void {};
        auto actionFailure = [&](GSErrCode err, const GS::UniString& errMsg) -> void {
            transformations (CreateErrorResponse (err, errMsg));
        };
        return ExecuteActionForEachDatabase (databaseIds, action, actionSuccess, actionFailure);
    };

    GS::Array<GS::ObjectState> navigatorItemIds;
    GS::Array<GS::ObjectState> databases;

    if (parameters.Get ("navigatorItemIds", navigatorItemIds)) {
        GS::Array<API_Guid> databaseIds;
        for (const GS::ObjectState& navItemOS : navigatorItemIds) {
            API_Guid navGuid = GetGuidFromNavigatorItemIdArrayItem (navItemOS);
            if (navGuid == APINULLGuid) {
                transformations (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing"));
                continue;
            }
            API_NavigatorItem navigatorItem = {};
            if (ACAPI_Navigator_GetNavigatorItem (&navGuid, &navigatorItem) != NoError) {
                transformations (CreateErrorResponse (APIERR_BADPARS, "Failed to get navigator item"));
                continue;
            }
            API_DatabaseInfo navDb = navigatorItem.db;
            GSErrCode dbErr = ACAPI_Window_GetDatabaseInfo (&navDb);
            if (dbErr != NoError) {
                transformations (CreateErrorResponse (dbErr, "Navigator item has no associated database"));
                continue;
            }
            const API_Guid dbGuid = DatabaseIdResolver::Instance ().GetIdOfDatabase (navDb);
            if (dbGuid == APINULLGuid) {
                transformations (CreateErrorResponse (APIERR_BADPARS, "Could not resolve database id for navigator item"));
                continue;
            }
            databaseIds.Push (dbGuid);
        }
        if (!databaseIds.IsEmpty ()) {
            GSErrCode err = runForDatabaseIds (databaseIds);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to switch database.");
            }
        }
    } else if (parameters.Get ("databases", databases)) {
        const GS::Array<API_Guid> databaseIds = databases.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);
        GSErrCode err = runForDatabaseIds (databaseIds);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to retrieve the starting database or to switch back to it after execution.");
        }
    } else {
        GetTransformationFromCurrentDatabase (transformations);
    }

    return response;
}

SetViewRotationCommand::SetViewRotationCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetViewRotationCommand::GetName () const
{
    return "SetViewRotation";
}

GS::Optional<GS::UniString> SetViewRotationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemIdsWithRotation": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId"
                        },
                        "rotation": {
                            "type": "number",
                            "description": "View rotation angle in radians."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "rotation"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIdsWithRotation"]
    })";
}

GS::Optional<GS::UniString> SetViewRotationCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState SetViewRotationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithRotation;
    parameters.Get ("navigatorItemIdsWithRotation", itemsWithRotation);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& item : itemsWithRotation) {
        double rotation = 0.0;
        if (!item.Get ("rotation", rotation)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "rotation is missing"));
            continue;
        }

        API_Guid navGuid = GetGuidFromNavigatorItemIdArrayItem (item);
        if (navGuid == APINULLGuid) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId is corrupt or missing"));
            continue;
        }

        API_NavigatorItem navigatorItem = {};
        GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&navGuid, &navigatorItem);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to get navigator item"));
            continue;
        }
        navigatorItem.mapId = API_PublicViewMap;

        API_NavigatorView navigatorView = {};
        err = ACAPI_Navigator_GetNavigatorView (&navigatorItem, &navigatorView);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to get navigator view"));
            continue;
        }

        // Obtain the axis-aligned model extent for this database without
        // modifying the database's live zoom state.
        // - ACAPI_Element_GetExtent returns the fit-in-window bounding box of
        //   all elements, always axis-aligned (independent of current rotation).
        // - We never call SetZoom/GetZoom, so the shared database zoom is
        //   untouched. Views without their own saveZoom keep using whatever
        //   the database currently shows.
        // ArchiCAD only applies tr (rotation) when navigating to a view whose
        // saveZoom flag is true.
        if (!navigatorView.saveZoom) {
            API_DatabaseInfo startDb  = {};
            API_DatabaseInfo targetDb = navigatorItem.db;
            if (ACAPI_Database_GetCurrentDatabase (&startDb) == NoError &&
                ACAPI_Window_GetDatabaseInfo      (&targetDb) == NoError &&
                ACAPI_Database_ChangeCurrentDatabase (&targetDb) == NoError)
            {
                API_Box extent = {};
                if (ACAPI_Element_GetExtent (&extent) == NoError &&
                    extent.xMax > extent.xMin)
                {
                    navigatorView.zoom     = extent;
                    navigatorView.saveZoom = true;
                }
                ACAPI_Database_ChangeCurrentDatabase (&startDb);
            }
        }

        const double c = cos (rotation);
        const double s = sin (rotation);
        navigatorView.tr.tmx[0]  = c;   navigatorView.tr.tmx[1]  = -s;  navigatorView.tr.tmx[2]  = 0.0;
        navigatorView.tr.tmx[4]  = s;   navigatorView.tr.tmx[5]  =  c;  navigatorView.tr.tmx[6]  = 0.0;
        navigatorView.tr.tmx[8]  = 0.0; navigatorView.tr.tmx[9]  = 0.0; navigatorView.tr.tmx[10] = 1.0;
        navigatorView.tr.tmx[11] = 0.0;

        err = ACAPI_Navigator_ChangeNavigatorView (&navigatorItem, &navigatorView);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to change navigator view rotation"));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

FitInWindowCommand::FitInWindowCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String FitInWindowCommand::GetName () const
{
    return "FitInWindow";
}

GS::Optional<GS::UniString> FitInWindowCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> FitInWindowCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState FitInWindowCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    GSErrCode err = NoError;

    if (parameters.Get ("elements", elements) && !elements.IsEmpty ()) {
        const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);
        err = ACAPI_View_ZoomToElements (&elemIds);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to zoom to elements.");
        }
    } else {
        err = ACAPI_View_Zoom ();
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to fit in window. There might be no project open.");
        }
    }

    return CreateSuccessfulExecutionResult ();
}

CloneProjectMapItemToViewMapCommand::CloneProjectMapItemToViewMapCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String CloneProjectMapItemToViewMapCommand::GetName () const
{
    return "CloneProjectMapItemToViewMap";
}

GS::Optional<GS::UniString> CloneProjectMapItemToViewMapCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewsData": {
                "type": "array",
                "description": "Array of views to clone from the Project Map to the View Map.",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "Navigator item ID of the Project Map viewpoint to clone."
                        },
                        "parentNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "Navigator item ID of the View Map folder to place the clone in. Optional; defaults to the View Map root."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewsData"]
    })";
}

GS::Optional<GS::UniString> CloneProjectMapItemToViewMapCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItems": {
                "type": "array",
                "items": {
                    "$ref": "#/NavigatorItemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItems"]
    })";
}

GS::ObjectState CloneProjectMapItemToViewMapCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> viewsData;
    parameters.Get ("viewsData", viewsData);

    GS::ObjectState response;
    const auto& navigatorItems = response.AddList<GS::ObjectState> ("navigatorItems");

    for (const GS::ObjectState& item : viewsData) {
        const GS::ObjectState* navigatorItemIdOS = item.Get ("navigatorItemId");
        if (navigatorItemIdOS == nullptr) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is missing."));
            continue;
        }

        API_Guid sourceGuid = GetGuidFromObjectState (*navigatorItemIdOS);
        if (sourceGuid == APINULLGuid) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing."));
            continue;
        }

        API_Guid parentGuid = APINULLGuid;
        const GS::ObjectState* parentOS = item.Get ("parentNavigatorItemId");
        if (parentOS != nullptr) {
            parentGuid = GetGuidFromObjectState (*parentOS);
        }

        if (parentGuid == APINULLGuid) {
            API_NavigatorSet viewMapSet = {};
            viewMapSet.mapId = API_PublicViewMap;
            Int32 idx = 0;
            if (ACAPI_Navigator_GetNavigatorSet (&viewMapSet, &idx) == NoError) {
                parentGuid = viewMapSet.rootGuid;
            }
        }

        API_Guid createdGuid = APINULLGuid;
        const GSErrCode err = ACAPI_Navigator_CloneProjectMapItemToViewMap (&sourceGuid, &parentGuid, &createdGuid);
        if (err != NoError) {
            navigatorItems (CreateErrorResponse (err, "Failed to clone navigator item to view map."));
            continue;
        }

        navigatorItems (CreateIdObjectState ("navigatorItemId", createdGuid));
    }

    return response;
}

CreateViewsInViewMapCommand::CreateViewsInViewMapCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String CreateViewsInViewMapCommand::GetName () const
{
    return "CreateViewsInViewMap";
}

GS::Optional<GS::UniString> CreateViewsInViewMapCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewsData": {
                "type": "array",
                "description": "Array of views to create as independent (non-clone) items in the View Map.",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "Source navigator item whose database and settings are copied."
                        },
                        "parentNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "View Map folder to place the new view in. Optional; defaults to View Map root."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name for the new view. Optional; defaults to the source item name."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewsData"]
    })";
}

GS::Optional<GS::UniString> CreateViewsInViewMapCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItems": {
                "type": "array",
                "items": {
                    "$ref": "#/NavigatorItemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItems"]
    })";
}

GS::ObjectState CreateViewsInViewMapCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> viewsData;
    parameters.Get ("viewsData", viewsData);

    GS::ObjectState response;
    const auto& navigatorItems = response.AddList<GS::ObjectState> ("navigatorItems");

    for (const GS::ObjectState& item : viewsData) {
        const GS::ObjectState* navigatorItemIdOS = item.Get ("navigatorItemId");
        if (navigatorItemIdOS == nullptr) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is missing."));
            continue;
        }

        API_Guid sourceGuid = GetGuidFromObjectState (*navigatorItemIdOS);
        if (sourceGuid == APINULLGuid) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing."));
            continue;
        }

        API_Guid parentGuid = APINULLGuid;
        const GS::ObjectState* parentOS = item.Get ("parentNavigatorItemId");
        if (parentOS != nullptr) {
            parentGuid = GetGuidFromObjectState (*parentOS);
        }
        if (parentGuid == APINULLGuid) {
            API_NavigatorSet viewMapSet = {};
            viewMapSet.mapId = API_PublicViewMap;
            Int32 idx = 0;
            if (ACAPI_Navigator_GetNavigatorSet (&viewMapSet, &idx) == NoError) {
                parentGuid = viewMapSet.rootGuid;
            }
        }

        // Step 1: clone from Project Map (only reliable way to create a
        // floor-plan View Map item)
        API_Guid createdGuid = APINULLGuid;
        GSErrCode err = ACAPI_Navigator_CloneProjectMapItemToViewMap (&sourceGuid, &parentGuid, &createdGuid);
        if (err != NoError) {
            navigatorItems (CreateErrorResponse (err, "Failed to clone navigator item to view map."));
            continue;
        }

        // Step 2: read the newly created navigator item
        API_NavigatorItem createdItem = {};
        err = ACAPI_Navigator_GetNavigatorItem (&createdGuid, &createdItem);
        if (err != NoError) {
            navigatorItems (CreateErrorResponse (err, "Failed to read created navigator item."));
            continue;
        }

        // Step 3: make it independent (break the link to the Project Map) and
        // apply optional custom name
        createdItem.mapId         = API_PublicViewMap;
        createdItem.isIndependent = true;

        GS::UniString newName;
        if (item.Get ("name", newName) && !newName.IsEmpty ()) {
            GS::ucscpy (createdItem.uName, newName.ToUStr ());
            createdItem.customName = true;
        }

        ACAPI_Navigator_ChangeNavigatorItem (&createdItem);

        navigatorItems (CreateIdObjectState ("navigatorItemId", createdGuid));
    }

    return response;
}

CreateViewMapFolderCommand::CreateViewMapFolderCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String CreateViewMapFolderCommand::GetName () const
{
    return "CreateViewMapFolder";
}

GS::Optional<GS::UniString> CreateViewMapFolderCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folderName": {
                "type": "string",
                "description": "Name of the new folder to create in the View Map."
            },
            "parentNavigatorItemId": {
                "$ref": "#/NavigatorItemId",
                "description": "Navigator item ID of the parent View Map folder. Optional; defaults to the View Map root."
            }
        },
        "additionalProperties": false,
        "required": ["folderName"]
    })";
}

GS::Optional<GS::UniString> CreateViewMapFolderCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemId": {
                "$ref": "#/NavigatorItemId"
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemId"]
    })";
}

GS::ObjectState CreateViewMapFolderCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString folderName;
    if (!parameters.Get ("folderName", folderName) || folderName.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "folderName is required and must not be empty.");
    }

    GS::Guid  parentGuidStorage;
    GS::Guid* parentGuidPtr = nullptr;

    const GS::ObjectState* parentOS = parameters.Get ("parentNavigatorItemId");
    if (parentOS != nullptr) {
        parentGuidStorage = APIGuid2GSGuid (GetGuidFromObjectState (*parentOS));
        parentGuidPtr     = &parentGuidStorage;
    }
    // If no parent specified, pass nullptr → folder is created at the View Map root.

    API_NavigatorItem folderItem = {};
    folderItem.itemType   = API_FolderNavItem;
    folderItem.mapId      = API_PublicViewMap;
    folderItem.customName = true;
    GS::ucscpy (folderItem.uName, folderName.ToUStr ());

    const GSErrCode err = ACAPI_Navigator_NewNavigatorView (&folderItem, nullptr, parentGuidPtr, nullptr);
    if (err != NoError) {
        return CreateErrorResponse (err, GS::UniString::Printf ("Failed to create View Map folder (code %d).", (int) err));
    }

    GS::ObjectState response;
    response.Add ("navigatorItemId", CreateGuidObjectState (folderItem.guid));
    return response;
}

MoveNavigatorItemCommand::MoveNavigatorItemCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String MoveNavigatorItemCommand::GetName () const
{
    return "MoveNavigatorItem";
}

GS::Optional<GS::UniString> MoveNavigatorItemCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemIdToMove": { "$ref": "#/NavigatorItemId" },
            "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
            "previousNavigatorItemId": { "$ref": "#/NavigatorItemId" }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIdToMove", "parentNavigatorItemId"]
    })";
}

GS::Optional<GS::UniString> MoveNavigatorItemCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState MoveNavigatorItemCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::ObjectState* itemToMoveOS = parameters.Get ("navigatorItemIdToMove");
    const GS::ObjectState* parentOS     = parameters.Get ("parentNavigatorItemId");

    if (itemToMoveOS == nullptr || parentOS == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemIdToMove and parentNavigatorItemId are required.");
    }

    const GS::Guid sourceGuid = APIGuid2GSGuid (GetGuidFromObjectState (*itemToMoveOS));
    const GS::Guid parentGuid = APIGuid2GSGuid (GetGuidFromObjectState (*parentOS));

    const GS::ObjectState* prevOS = parameters.Get ("previousNavigatorItemId");
    GS::Guid  prevGuidStorage;
    GS::Guid* prevGuidPtr = nullptr;
    if (prevOS != nullptr) {
        prevGuidStorage = APIGuid2GSGuid (GetGuidFromObjectState (*prevOS));
        prevGuidPtr = &prevGuidStorage;
    }

    GSErrCode err = NoError;
    ACAPI_CallUndoableCommand ("MoveNavigatorItemCommand", [&]() -> GSErrCode {
        err = ACAPI_Navigator_SetNavigatorItemPosition (&sourceGuid, &parentGuid, prevGuidPtr);
        return err;
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to move navigator item.");
    }

    return CreateSuccessfulExecutionResult ();
}

RenameNavigatorItemCommand::RenameNavigatorItemCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String RenameNavigatorItemCommand::GetName () const
{
    return "RenameNavigatorItem";
}

GS::Optional<GS::UniString> RenameNavigatorItemCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemId": { "$ref": "#/NavigatorItemId" },
            "newName":         { "type": "string" },
            "newId":           { "type": "string" }
        },
        "additionalProperties": false,
        "required": ["navigatorItemId"]
    })";
}

GS::Optional<GS::UniString> RenameNavigatorItemCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState RenameNavigatorItemCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::ObjectState* navIdOS = parameters.Get ("navigatorItemId");
    if (navIdOS == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId is required.");
    }

    const API_Guid guid = GetGuidFromObjectState (*navIdOS);

    API_NavigatorItem navItem = {};
    GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&guid, &navItem);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get navigator item.");
    }

    GS::UniString newName;
    if (parameters.Get ("newName", newName) && !newName.IsEmpty ()) {
        GS::ucsncpy (navItem.uName, newName.ToUStr (), GS::ArraySize (navItem.uName));
        navItem.uName[GS::ArraySize (navItem.uName) - 1] = 0;
        navItem.customName = true;
    }

    err = ACAPI_Navigator_ChangeNavigatorItem (&navItem);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to rename navigator item.");
    }

    // For layouts: set custom layout number (ID) via ChangeLayoutSets
    GS::UniString newId;
    if (parameters.Get ("newId", newId)) {
        API_LayoutInfo layoutInfo = {};
        BNZeroMemory (&layoutInfo, sizeof (layoutInfo));
        if (ACAPI_Navigator_GetLayoutSets (&layoutInfo, &navItem.db.databaseUnId) == NoError) {
            CHTruncate (newId.ToCStr ().Get (), layoutInfo.customLayoutNumber,
                        GS::ArraySize (layoutInfo.customLayoutNumber));
            layoutInfo.customLayoutNumbering = true;
            ACAPI_Navigator_ChangeLayoutSets (&layoutInfo, &navItem.db.databaseUnId);
            delete layoutInfo.customData;
            layoutInfo.customData = nullptr;
        }
    }

    return CreateSuccessfulExecutionResult ();
}

DeleteNavigatorItemsCommand::DeleteNavigatorItemsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String DeleteNavigatorItemsCommand::GetName () const
{
    return "DeleteNavigatorItems";
}

GS::Optional<GS::UniString> DeleteNavigatorItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemIds": { "$ref": "#/NavigatorItemIds" }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIds"]
    })";
}

GS::Optional<GS::UniString> DeleteNavigatorItemsCommand::GetResponseSchema () const
{
    return R"({"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]})";
}

GS::ObjectState DeleteNavigatorItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> navigatorItemIds;
    parameters.Get ("navigatorItemIds", navigatorItemIds);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& navItemIdOS : navigatorItemIds) {
        const API_Guid guid = GetGuidFromNavigatorItemIdArrayItem (navItemIdOS);
        if (guid == APINULLGuid) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId is corrupt or missing."));
            continue;
        }

        API_NavigatorItem navItem = {};
        const GSErrCode getErr = ACAPI_Navigator_GetNavigatorItem (&guid, &navItem);
        if (getErr != NoError) {
            executionResults (CreateFailedExecutionResult (getErr, "Failed to get navigator item."));
            continue;
        }

        GSErrCode err = NoError;
        if (navItem.itemType == API_LayoutNavItem || navItem.itemType == API_MasterLayoutNavItem) {
            API_DatabaseInfo dbInfo = navItem.db;
            err = ACAPI_Database_DeleteDatabase (&dbInfo);
        } else if (navItem.itemType == API_SubSetNavItem) {
            err = APIERR_NOTSUPPORTED;
        } else {
            bool silentMode = true;
            err = ACAPI_Navigator_DeleteNavigatorView (&guid, &silentMode);
        }

        if (err == APIERR_NOTSUPPORTED) {
            executionResults (CreateFailedExecutionResult (err, "Deleting layout subsets is not supported by the ArchiCAD API."));
        } else if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to delete navigator item."));
        } else {
            executionResults (CreateSuccessfulExecutionResult ());
        }
    }

    return response;
}

// ─── GetNavigatorItemTree ─────────────────────────────────────────────────────

static GS::UniString NavigatorItemTypeToString (API_NavigatorItemTypeID t)
{
    switch (t) {
        case API_UndefinedNavItem:           return "UndefinedItem";
        case API_ProjectNavItem:             return "ProjectItem";
        case API_StoryNavItem:               return "StoryItem";
        case API_SectionNavItem:             return "SectionItem";
        case API_DetailDrawingNavItem:       return "DetailDrawingItem";
        case API_PerspectiveNavItem:         return "PerspectiveItem";
        case API_AxonometryNavItem:          return "AxonometryItem";
        case API_ListNavItem:                return "ListItem";
        case API_ScheduleNavItem:            return "ScheduleItem";
        case API_TocNavItem:                 return "TocItem";
        case API_CameraNavItem:              return "CameraItem";
        case API_CameraSetNavItem:           return "CameraSetItem";
        case API_InfoNavItem:                return "InfoItem";
        case API_HelpNavItem:                return "HelpItem";
        case API_LayoutNavItem:              return "LayoutItem";
        case API_MasterLayoutNavItem:        return "MasterLayoutItem";
        case API_BookNavItem:                return "BookItem";
        case API_MasterFolderNavItem:        return "MasterFolderItem";
        case API_SubSetNavItem:              return "SubSetItem";
        case API_TextListNavItem:            return "TextListItem";
        case API_ElevationNavItem:           return "ElevationItem";
        case API_InteriorElevationNavItem:   return "InteriorElevationItem";
        case API_WorksheetDrawingNavItem:    return "WorksheetDrawingItem";
        case API_DocumentFrom3DNavItem:      return "DocumentFrom3DItem";
        case API_FolderNavItem:              return "FolderItem";
        case API_DrawingNavItem:             return "DrawingItem";
        default:                             return "UnknownItem";
    }
}

static GS::ObjectState NavigatorItemToObjectState (API_NavigatorItem item, API_NavigatorMapID mapId)
{
    GS::ObjectState itemOS;
    itemOS.Add ("type",            NavigatorItemTypeToString (item.itemType));
    itemOS.Add ("name",            GS::UniString (item.uName));
    itemOS.Add ("navigatorItemId", CreateGuidObjectState (item.guid));

    GS::UniString prefix;
    if (item.itemType == API_StoryNavItem) {
        prefix = GS::UniString::Printf ("%d", (int) item.floorNum);
    }
    itemOS.Add ("prefix", prefix);

    item.mapId = mapId;
    GS::Array<API_NavigatorItem> children;
    if (ACAPI_Navigator_GetNavigatorChildrenItems (&item, &children) == NoError && !children.IsEmpty ()) {
        const auto& childList = itemOS.AddList<GS::ObjectState> ("children");
        for (const API_NavigatorItem& child : children) {
            GS::ObjectState wrapper;
            wrapper.Add ("navigatorItem", NavigatorItemToObjectState (child, mapId));
            childList (wrapper);
        }
    }

    return itemOS;
}

GetNavigatorItemTreeCommand::GetNavigatorItemTreeCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetNavigatorItemTreeCommand::GetName () const
{
    return "GetNavigatorItemTree";
}

GS::Optional<GS::UniString> GetNavigatorItemTreeCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorMapId": {
                "type": "string",
                "enum": ["PublicViewMap", "ProjectMap", "LayoutBook", "PublisherSets"],
                "description": "The navigator map to retrieve."
            }
        },
        "additionalProperties": false,
        "required": ["navigatorMapId"]
    })";
}

GS::ObjectState GetNavigatorItemTreeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString navigatorMapIdStr;
    parameters.Get ("navigatorMapId", navigatorMapIdStr);

    API_NavigatorMapID mapId = API_PublicViewMap;
    if (navigatorMapIdStr == "ProjectMap")         mapId = API_ProjectMap;
    else if (navigatorMapIdStr == "LayoutBook")    mapId = API_LayoutMap;
    else if (navigatorMapIdStr == "PublisherSets") mapId = API_PublisherSets;

    API_NavigatorSet navSet = {};
    navSet.mapId = mapId;
    Int32 idx = 0;
    GSErrCode err = ACAPI_Navigator_GetNavigatorSet (&navSet, &idx);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get navigator set.");
    }

    API_NavigatorItem rootItem = {};
    err = ACAPI_Navigator_GetNavigatorItem (&navSet.rootGuid, &rootItem);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get root navigator item.");
    }

    GS::ObjectState response;
    response.Add ("navigatorItemTree", NavigatorItemToObjectState (rootItem, mapId));
    return response;
}
