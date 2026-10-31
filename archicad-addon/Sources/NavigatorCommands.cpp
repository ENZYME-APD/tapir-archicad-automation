#include "NavigatorCommands.hpp"
#include "MigrationHelper.hpp"
#include "Base64Converter.hpp"
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

GS::Optional<GS::UniString> UpdateDrawingsCommand::GetRawResponseSchema() const
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

GetDrawingContentBoxesCommand::GetDrawingContentBoxesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDrawingContentBoxesCommand::GetName () const
{
    return "GetDrawingContentBoxes";
}

GS::Optional<GS::UniString> GetDrawingContentBoxesCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetDrawingContentBoxesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "contentBoxes": {
                "type": "array",
                "description": "One item per input element, in the same order: the full content box of the Drawing, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "contentBox": {
                                    "$ref": "#/BoundingBox2D",
                                    "description": "The full boundary box of the Drawing's content, independent of its frame, as returned by Archicad."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "contentBox"
                            ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "contentBoxes"
        ]
    })";
}

GS::ObjectState GetDrawingContentBoxesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& contentBoxes = response.AddList<GS::ObjectState> ("contentBoxes");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            contentBoxes (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            contentBoxes (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }
        if (GetElemTypeId (elemHead) != API_DrawingID) {
            contentBoxes (CreateErrorResponse (APIERR_BADID, "The given element is not a Drawing."));
            continue;
        }

        API_Box contentBox = {};
        err = ACAPI_Drawing_GetFullDrawingContentBox (&contentBox, &elemHead.guid);
        if (err != NoError) {
            contentBoxes (CreateErrorResponse (err, "Failed to get the content box of the Drawing."));
            continue;
        }

        contentBoxes (GS::ObjectState ("contentBox", GS::ObjectState (
            "xMin", contentBox.xMin,
            "yMin", contentBox.yMin,
            "xMax", contentBox.xMax,
            "yMax", contentBox.yMax)));
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "GetDrawingContentBoxes is not supported in Archicad versions earlier than 27.");
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

GS::Optional<GS::UniString> GetDatabaseIdFromNavigatorItemIdCommand::GetRawResponseSchema () const
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

struct DatabaseTypeData {
    const char*      name;
    API_WindowTypeID typeID;
};

static const DatabaseTypeData databaseTypes[] = {
    { "Section",           APIWind_SectionID },
    { "Elevation",         APIWind_ElevationID },
    { "InteriorElevation", APIWind_InteriorElevationID },
    { "Detail",            APIWind_DetailID },
    { "Worksheet",         APIWind_WorksheetID },
    { "DocumentFrom3D",    APIWind_DocumentFrom3DID },
    { "Layout",            APIWind_LayoutID },
    { "MasterLayout",      APIWind_MasterLayoutID }
};

static GSErrCode GetDatabaseUnIdsOfType (API_WindowTypeID typeID, GS::Array<API_DatabaseUnId>& databaseUnIds)
{
    switch (typeID) {
        case APIWind_SectionID:           return ACAPI_Database_GetSectionDatabases (nullptr, &databaseUnIds);
        case APIWind_ElevationID:         return ACAPI_Database_GetElevationDatabases (nullptr, &databaseUnIds);
        case APIWind_InteriorElevationID: return ACAPI_Database_GetInteriorElevationDatabases (nullptr, &databaseUnIds);
        case APIWind_DetailID:            return ACAPI_Database_GetDetailDatabases (nullptr, &databaseUnIds);
        case APIWind_WorksheetID:         return ACAPI_Database_GetWorksheetDatabases (nullptr, &databaseUnIds);
        case APIWind_DocumentFrom3DID:    return ACAPI_Database_GetDocumentFrom3DDatabases (nullptr, &databaseUnIds);
        case APIWind_LayoutID:            return ACAPI_Database_GetLayoutDatabases (nullptr, &databaseUnIds);
        case APIWind_MasterLayoutID:      return ACAPI_Database_GetMasterLayoutDatabases (nullptr, &databaseUnIds);
        default:                          return APIERR_BADPARS;
    }
}

GetDatabasesCommand::GetDatabasesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetDatabasesCommand::GetName () const
{
    return "GetDatabases";
}

GS::Optional<GS::UniString> GetDatabasesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "types": {
            "type": "array",
            "description": "The types of the databases to list. Optional, by default the databases of every type are listed.",
            "items": {
                "type": "string",
                "enum": [
                    "Section",
                    "Elevation",
                    "InteriorElevation",
                    "Detail",
                    "Worksheet",
                    "DocumentFrom3D",
                    "Layout",
                    "MasterLayout"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": []
})";
}

GS::Optional<GS::UniString> GetDatabasesCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "databases": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "type": {
                        "type": "string",
                        "description": "The type of the database."
                    },
                    "databaseId": {
                        "$ref": "#/DatabaseId"
                    },
                    "name": {
                        "type": "string",
                        "description": "The name of the database. Missing when the database info cannot be read."
                    },
                    "referenceId": {
                        "type": "string",
                        "description": "The reference id of the database. Missing when the database info cannot be read."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "type",
                    "databaseId"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "databases"
    ]
})";
}

GS::ObjectState GetDatabasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<const DatabaseTypeData*> requestedTypes;
    GS::Array<GS::UniString> typeNames;
    if (parameters.Get ("types", typeNames)) {
        for (const GS::UniString& typeName : typeNames) {
            const DatabaseTypeData* found = nullptr;
            for (const DatabaseTypeData& databaseType : databaseTypes) {
                if (typeName == databaseType.name) {
                    found = &databaseType;
                    break;
                }
            }
            if (found == nullptr) {
                return CreateErrorResponse (APIERR_BADPARS, "Unknown database type: " + typeName);
            }
            if (!requestedTypes.Contains (found)) {
                requestedTypes.Push (found);
            }
        }
    } else {
        for (const DatabaseTypeData& databaseType : databaseTypes) {
            requestedTypes.Push (&databaseType);
        }
    }

    GS::ObjectState response;
    const auto& databases = response.AddList<GS::ObjectState> ("databases");

    for (const DatabaseTypeData* databaseType : requestedTypes) {
        GS::Array<API_DatabaseUnId> databaseUnIds;
        // As with the layout lookups in DocumentCreationCommands, a type whose list
        // cannot be read is treated as having no databases, so one failing type does
        // not hide the others.
        if (GetDatabaseUnIdsOfType (databaseType->typeID, databaseUnIds) != NoError) {
            continue;
        }

        for (const API_DatabaseUnId& databaseUnId : databaseUnIds) {
            API_DatabaseInfo databaseInfo = {};
            databaseInfo.typeID = databaseType->typeID;
            databaseInfo.databaseUnId = databaseUnId;

            GS::ObjectState database (
                "type", GS::UniString (databaseType->name),
                "databaseId", CreateGuidObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (databaseInfo)));

            if (ACAPI_Window_GetDatabaseInfo (&databaseInfo) == NoError) {
                database.Add ("name", GS::UniString (databaseInfo.name));
                database.Add ("referenceId", GS::UniString (databaseInfo.ref));
            }

            databases (database);
        }
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

GS::Optional<GS::UniString> GetModelViewOptionsCommand::GetRawResponseSchema () const
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

FindModelViewOptionCommand::FindModelViewOptionCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String FindModelViewOptionCommand::GetName () const
{
    return "FindModelViewOption";
}

GS::Optional<GS::UniString> FindModelViewOptionCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "description": "The model view option combination to find, given by id or by name.",
        "properties": {
            "modelViewOptionId": {
                "type": "object",
                "description": "The identifier of the model view option combination.",
                "properties": {
                    "guid": {
                        "$ref": "#/Guid"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid"
                ]
            },
            "name": {
                "type": "string",
                "description": "The name of the model view option combination, as shown in Document > Model View > Model View Options."
            }
        },
        "additionalProperties": false,
        "oneOf": [
            {
                "required": [
                    "modelViewOptionId"
                ]
            },
            {
                "required": [
                    "name"
                ]
            }
        ]
    })";
}

GS::Optional<GS::UniString> FindModelViewOptionCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "description": "The model view option combination found.",
        "properties": {
            "index": {
                "type": "integer",
                "description": "The index of the model view option combination."
            },
            "modelViewOptionId": {
                "type": "object",
                "description": "The identifier of the model view option combination.",
                "properties": {
                    "guid": {
                        "$ref": "#/Guid"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid"
                ]
            },
            "name": {
                "type": "string",
                "description": "The name of the model view option combination."
            },
            "flags": {
                "type": "integer",
                "description": "The flags of the model view option combination, as Archicad returns them."
            }
        },
        "additionalProperties": false,
        "required": [
            "index",
            "modelViewOptionId",
            "name",
            "flags"
        ]
    })";
}

GS::ObjectState FindModelViewOptionCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_ModelViewOptions_Head searchHead = {};
    API_Guid guidToSearch = APINULLGuid;
    GS::UniString nameToSearch;

    const GS::ObjectState* modelViewOptionIdOS = parameters.Get ("modelViewOptionId");
    if (modelViewOptionIdOS != nullptr) {
        guidToSearch = GetGuidFromObjectState (*modelViewOptionIdOS);
        if (guidToSearch == APINULLGuid) {
            return CreateErrorResponse (APIERR_BADPARS, "modelViewOptionId is corrupt or missing.");
        }
        searchHead.guid = guidToSearch;
    } else if (parameters.Get ("name", nameToSearch)) {
        searchHead.uniStringNamePtr = &nameToSearch;
    } else {
        return CreateErrorResponse (APIERR_BADPARS, "Either modelViewOptionId or name is required.");
    }

    GSErrCode err = ACAPI_Navigator_ModelViewOptions_Search (&searchHead);

    // The search is documented with the name only; when it does not find the guid, the
    // combinations are walked through the way GetModelViewOptions reads them.
    if (guidToSearch != APINULLGuid && (err != NoError || searchHead.guid != guidToSearch)) {
        err = APIERR_BADID;

        UInt32 count = 0;
        ACAPI_Navigator_ModelViewOptions_GetNum (count);

        for (UInt32 i = 1; i <= count; ++i) {
            API_ModelViewOptionsType modelViewOption = {};
            modelViewOption.head.index = i;

            if (ACAPI_Navigator_ModelViewOptions_Get (&modelViewOption) == NoError && modelViewOption.head.guid == guidToSearch) {
                searchHead.index = modelViewOption.head.index;
                err = NoError;
                break;
            }
        }
    }

    if (err != NoError) {
        if (modelViewOptionIdOS != nullptr) {
            return CreateErrorResponse (err, "No model view option combination has the given modelViewOptionId.");
        }
        return CreateErrorResponse (err, "No model view option combination is named '" + nameToSearch + "'.");
    }

    // The combination is read back by its index, so the name is returned as Archicad carries it.
    GS::UniString name;
    API_ModelViewOptionsType modelViewOption = {};
    modelViewOption.head.index = searchHead.index;
    modelViewOption.head.uniStringNamePtr = &name;

    err = ACAPI_Navigator_ModelViewOptions_Get (&modelViewOption);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the model view option combination found.");
    }
    if (guidToSearch != APINULLGuid && modelViewOption.head.guid != guidToSearch) {
        return CreateErrorResponse (APIERR_BADID, "No model view option combination has the given modelViewOptionId.");
    }

    GS::ObjectState response;
    response.Add ("index", static_cast<Int32> (modelViewOption.head.index));
    response.Add ("modelViewOptionId", CreateGuidObjectState (modelViewOption.head.guid));
    response.Add ("name", name);
    response.Add ("flags", static_cast<Int32> (modelViewOption.head.flags));
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
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

GS::Optional<GS::UniString> GetViewSettingsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> SetViewSettingsCommand::GetRawResponseSchema () const
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

// The graphic override combination applied to the current (front) window, as
// opposed to the one stored in a saved view (SetViewSettings). The two API
// functions below are new in Archicad 30; the combinations themselves are
// reached through the ACAPI_GraphicalOverride_* functions of Archicad 27+.

GetActiveGraphicalOverrideCombinationCommand::GetActiveGraphicalOverrideCombinationCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetActiveGraphicalOverrideCombinationCommand::GetName () const
{
    return "GetActiveGraphicalOverrideCombination";
}

GS::Optional<GS::UniString> GetActiveGraphicalOverrideCombinationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "description": "The graphic override combination applied to the current window. Empty when the current window has no combination applied.",
        "properties": {
            "combinationId": {
                "type": "object",
                "description": "The identifier of the graphic override combination.",
                "properties": {
                    "guid": {
                        "$ref": "#/Guid"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid"
                ]
            },
            "name": {
                "type": "string",
                "description": "The name of the graphic override combination. Missing when the combination cannot be read, e.g. it was deleted."
            }
        },
        "additionalProperties": false
    })";
}

GS::ObjectState GetActiveGraphicalOverrideCombinationCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    API_Guid combinationGuid = APINULLGuid;
    GSErrCode err = ACAPI_GraphicalOverride_GetActualOverrideCombination (combinationGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the graphic override combination of the current window. The current window must be a model window (floor plan, section, elevation, 3D, ...).");
    }

    GS::ObjectState response;
    if (combinationGuid == APINULLGuid) {
        return response;
    }

    response.Add ("combinationId", CreateGuidObjectState (combinationGuid));

    API_OverrideCombination combination = {};
    combination.guid = combinationGuid;
    if (ACAPI_GraphicalOverride_GetOverrideCombination (combination) == NoError) {
        response.Add ("name", combination.name);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 30.");
#endif
}

SetActiveGraphicalOverrideCombinationCommand::SetActiveGraphicalOverrideCombinationCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetActiveGraphicalOverrideCombinationCommand::GetName () const
{
    return "SetActiveGraphicalOverrideCombination";
}

GS::Optional<GS::UniString> SetActiveGraphicalOverrideCombinationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "description": "The graphic override combination to apply, given by id or by name.",
        "properties": {
            "combinationId": {
                "type": "object",
                "description": "The identifier of the graphic override combination, as returned by GetActiveGraphicalOverrideCombination.",
                "properties": {
                    "guid": {
                        "$ref": "#/Guid"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid"
                ]
            },
            "name": {
                "type": "string",
                "description": "The name of the graphic override combination, as shown in Document > Graphic Overrides > Graphic Override Combinations."
            }
        },
        "additionalProperties": false,
        "oneOf": [
            {
                "required": [
                    "combinationId"
                ]
            },
            {
                "required": [
                    "name"
                ]
            }
        ]
    })";
}

GS::Optional<GS::UniString> SetActiveGraphicalOverrideCombinationCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetActiveGraphicalOverrideCombinationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    API_Guid combinationGuid = APINULLGuid;

    const GS::ObjectState* combinationIdOS = parameters.Get ("combinationId");
    GS::UniString name;
    if (combinationIdOS != nullptr) {
        combinationGuid = GetGuidFromObjectState (*combinationIdOS);
        if (combinationGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "combinationId is corrupt or missing.");
        }
    } else if (parameters.Get ("name", name)) {
        GS::Array<API_Guid> combinationGuids;
        GSErrCode err = ACAPI_GraphicalOverride_GetOverrideCombinationList (combinationGuids);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to get the list of graphic override combinations.");
        }

        for (const API_Guid& guid : combinationGuids) {
            API_OverrideCombination combination = {};
            combination.guid = guid;
            if (ACAPI_GraphicalOverride_GetOverrideCombination (combination) == NoError && combination.name == name) {
                combinationGuid = guid;
                break;
            }
        }

        if (combinationGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADNAME, "No graphic override combination is named '" + name + "'.");
        }
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Either combinationId or name is required.");
    }

    GSErrCode err = ACAPI_GraphicalOverride_SetActualOverrideCombination (combinationGuid);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to apply the graphic override combination to the current window. The current window must be a model window (floor plan, section, elevation, 3D, ...) and the combination must exist.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    (void) parameters;
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Available from Archicad 30.");
#endif
}

// Trace & Reference of the floor plan. API_GhostStoryType (ACAPI_View_GetGhostStorySettings)
// is the read-only view of it: on, one story / the one above / the one below, which story,
// the color and the element type filter. The API has no setter for that struct: the ghost
// story is the ghost record of the floor plan database (API_GhostRecord), which
// ACAPI_View_ChangeGhostRecord changes and ACAPI_View_CreateGhostRecord creates when the
// floor plan has none yet. Both commands work on the floor plan database, whatever window
// is in front, and put the previous current database back afterwards.

static GS::ObjectState CreateGhostStorySettingsObjectState (const API_GhostStoryType& settings)
{
    GS::ObjectState os (
        "on", settings.on,
        "showOne", settings.showOne,
        "showAbove", settings.showAbove,
        "showBelow", settings.showBelow,
        "storyIndex", static_cast<Int32> (settings.storyInd),
        "ghostColor", GS::ObjectState (
            "red", settings.ghostColor.f_red,
            "green", settings.ghostColor.f_green,
            "blue", settings.ghostColor.f_blue));

    // API_ElemFilter is one flag per element type, indexed by API_ElemTypeID.
    const auto& filter = os.AddList<GS::UniString> ("filter");
    for (Int32 typeId = API_ZombieElemID + 1; typeId <= API_LastElemType; ++typeId) {
        if (!settings.filter[typeId]) {
            continue;
        }
        const GS::UniString typeName = GetElementTypeNonLocalizedName (static_cast<API_ElemTypeID> (typeId));
        if (typeName != "Unknown") {
            filter (typeName);
        }
    }

    return os;
}

GetGhostStorySettingsCommand::GetGhostStorySettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetGhostStorySettingsCommand::GetName () const
{
    return "GetGhostStorySettings";
}

GS::Optional<GS::UniString> GetGhostStorySettingsCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "on": {
            "type": "boolean",
            "description": "Whether the ghost story (Trace & Reference) is shown on the floor plan."
        },
        "showOne": {
            "type": "boolean",
            "description": "Whether the story at storyIndex is the ghost story."
        },
        "showAbove": {
            "type": "boolean",
            "description": "Whether the story above the current one is the ghost story."
        },
        "showBelow": {
            "type": "boolean",
            "description": "Whether the story below the current one is the ghost story."
        },
        "storyIndex": {
            "type": "integer",
            "description": "The index of the ghost story, as GetStories reports it. Meaningful when showOne is true."
        },
        "ghostColor": {
            "$ref": "#/ColorRGB",
            "description": "The color the ghost story is drawn in."
        },
        "filter": {
            "type": "array",
            "description": "The element types shown in the ghost story. Types without a Tapir element type name (for example MEP or external ones) are left out, so passing this list to SetGhostStorySettings hides them.",
            "items": {
                "$ref": "#/ElementType"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "on",
        "showOne",
        "showAbove",
        "showBelow",
        "storyIndex",
        "ghostColor",
        "filter"
    ]
})";
}

GS::ObjectState GetGhostStorySettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_DatabaseInfo previousDatabase = {};
    bool databaseSwitched = false;
    const GSErrCode databaseErr = SwitchCurrentDatabaseToFloorPlan (previousDatabase, databaseSwitched);
    const GS::OnExit restoreDatabase ([&]() {
        if (databaseSwitched) {
            ACAPI_Database_ChangeCurrentDatabase (&previousDatabase);
        }
    });
    if (databaseErr != NoError) {
        return CreateErrorResponse (databaseErr, "Failed to activate the floor plan database, which holds the ghost story settings.");
    }

    API_GhostStoryType settings = {};
    const GSErrCode err = ACAPI_View_GetGhostStorySettings (&settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the ghost story settings.");
    }

    return CreateGhostStorySettingsObjectState (settings);
}

SetGhostStorySettingsCommand::SetGhostStorySettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetGhostStorySettingsCommand::GetName () const
{
    return "SetGhostStorySettings";
}

GS::Optional<GS::UniString> SetGhostStorySettingsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "description": "Every field is optional; the ones left out keep their current value. A story choice (showOne, showAbove or showBelow true) replaces the current reference, whatever it is; without one the reference is kept, also when it is another view (a section, an elevation, a worksheet).",
    "properties": {
        "on": {
            "type": "boolean",
            "description": "Show (true) or hide (false) the ghost story on the floor plan."
        },
        "showOne": {
            "type": "boolean",
            "description": "Show the story at storyIndex as the ghost story."
        },
        "showAbove": {
            "type": "boolean",
            "description": "Show the story above the current one as the ghost story."
        },
        "showBelow": {
            "type": "boolean",
            "description": "Show the story below the current one as the ghost story."
        },
        "storyIndex": {
            "type": "integer",
            "description": "The index of the story to show when showOne is true, as GetStories reports it."
        },
        "ghostColor": {
            "$ref": "#/ColorRGB",
            "description": "The color to draw the ghost story in."
        },
        "filter": {
            "type": "array",
            "description": "The element types to show in the ghost story; the others are hidden.",
            "items": {
                "$ref": "#/ElementType"
            }
        }
    },
    "additionalProperties": false
})";
}

GS::Optional<GS::UniString> SetGhostStorySettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetGhostStorySettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_DatabaseInfo previousDatabase = {};
    bool databaseSwitched = false;
    const GSErrCode databaseErr = SwitchCurrentDatabaseToFloorPlan (previousDatabase, databaseSwitched);
    const GS::OnExit restoreDatabase ([&]() {
        if (databaseSwitched) {
            ACAPI_Database_ChangeCurrentDatabase (&previousDatabase);
        }
    });
    if (databaseErr != NoError) {
        return CreateFailedExecutionResult (databaseErr, "Failed to activate the floor plan database, which holds the ghost story settings.");
    }

    // The fields left out keep their value: the current settings are the baseline.
    API_GhostStoryType current = {};
    GSErrCode err = ACAPI_View_GetGhostStorySettings (&current);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current ghost story settings.");
    }

    bool on = current.on;
    parameters.Get ("on", on);

    // A story choice in the input replaces the current one: the two flags not given are
    // cleared, so {"showBelow": true} alone switches from the story above to the one below.
    // Without a true flag in the input the flags not given keep their value.
    bool showOne = false;
    bool showAbove = false;
    bool showBelow = false;
    const bool hasShowOne = parameters.Get ("showOne", showOne);
    const bool hasShowAbove = parameters.Get ("showAbove", showAbove);
    const bool hasShowBelow = parameters.Get ("showBelow", showBelow);
    if (!showOne && !showAbove && !showBelow) {
        if (!hasShowOne) {
            showOne = current.showOne;
        }
        if (!hasShowAbove) {
            showAbove = current.showAbove;
        }
        if (!hasShowBelow) {
            showBelow = current.showBelow;
        }
    }
    if ((showOne ? 1 : 0) + (showAbove ? 1 : 0) + (showBelow ? 1 : 0) > 1) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Only one of showOne, showAbove and showBelow can be true.");
    }

    Int32 storyIndex = current.storyInd;
    if (parameters.Get ("storyIndex", storyIndex)) {
        const Stories stories = GetStories ();
        if (stories.find (static_cast<short> (storyIndex)) == stories.end ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "storyIndex is not the index of a story of the project.");
        }
    }

    API_RGBColor ghostColor = current.ghostColor;
    GetColor (parameters, "ghostColor", ghostColor);

    GS::Array<GS::UniString> filterNames;
    const bool hasFilter = parameters.Get ("filter", filterNames);
    GS::Array<API_ElemTypeID> filterTypes;
    for (const GS::UniString& typeName : filterNames) {
        const API_ElemTypeID typeId = GetElementTypeFromNonLocalizedName (typeName);
        if (typeId == API_ZombieElemID) {
            return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("Unknown element type in filter: ") + typeName);
        }
        filterTypes.Push (typeId);
    }

    API_DatabaseInfo floorPlanDatabase = {};
    err = ACAPI_Database_GetCurrentDatabase (&floorPlanDatabase);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the floor plan database.");
    }

    API_GhostRecord ghostRecord = {};
    const bool hasRecord = ACAPI_View_GetGhostRecord (&floorPlanDatabase.databaseUnId, &ghostRecord) == NoError;
    if (!hasRecord) {
        // The floor plan has no ghost record yet (Trace & Reference was never used in it).
        // A new one: the active view is drawn in its own colors, fills are kept in both,
        // and the element filter starts from what the floor plan reports now. The reference
        // is drawn in place: no offset, no rotation, and scale 1 (the value-initialised 0
        // would draw it degenerate).
        ghostRecord = {};
        ghostRecord.parentDatabase = floorPlanDatabase.databaseUnId;
        ghostRecord.viewOriginalColor = true;
        ghostRecord.viewBackgroundFill = true;
        ghostRecord.ghostBackgroundFill = true;
        ghostRecord.ghostColor.f_alpha = 1.0;
        ghostRecord.scale = 1.0;
        for (Int32 typeId = 0; typeId <= API_LastElemType; ++typeId) {
            ghostRecord.filter[typeId] = current.filter[typeId];
        }
    }

    ghostRecord.activated = on;
    ghostRecord.show = on;
    // A story choice makes a story of the floor plan the reference, whatever the record
    // referred to before. Without one an existing record keeps its reference, which may be
    // another view (a section, an elevation, a worksheet): a color-only or on/off call must
    // not replace it. A new record has nothing to keep.
    if (!hasRecord || showOne || showAbove || showBelow) {
        ghostRecord.ghostDatabase = floorPlanDatabase.databaseUnId;
        ghostRecord.ghostDBType = floorPlanDatabase.typeID;
        if (showOne) {
            ghostRecord.ghostType = APIGhost_ShowOneFloor;
        } else if (showAbove) {
            ghostRecord.ghostType = APIGhost_ShowAboveFloor;
        } else if (showBelow) {
            ghostRecord.ghostType = APIGhost_ShowBelowFloor;
        } else {
            ghostRecord.ghostType = APIGhost_NoFloor;
        }
        ghostRecord.floor = static_cast<short> (storyIndex);
    }
    // The record's color is an API_RGBAColor: its alpha (the ghost's intensity) is kept,
    // since the settings only carry the RGB part.
    ghostRecord.ghostColor.f_red = ghostColor.f_red;
    ghostRecord.ghostColor.f_green = ghostColor.f_green;
    ghostRecord.ghostColor.f_blue = ghostColor.f_blue;
    if (hasFilter) {
        for (Int32 typeId = 0; typeId <= API_LastElemType; ++typeId) {
            ghostRecord.filter[typeId] = false;
        }
        for (const API_ElemTypeID typeId : filterTypes) {
            ghostRecord.filter[typeId] = true;
        }
    }

    err = ACAPI_CallUndoableCommand ("SetGhostStorySettingsCommand", [&]() -> GSErrCode {
        return hasRecord
            ? ACAPI_View_ChangeGhostRecord (&ghostRecord)
            : ACAPI_View_CreateGhostRecord (&ghostRecord);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, hasRecord
            ? "Failed to change the ghost record of the floor plan."
            : "Failed to create the ghost record of the floor plan.");
    }

    return CreateSuccessfulExecutionResult ();
}

// The Partial Structure Display of the current (front) window, as opposed to
// the one stored in a saved view (Get/SetViewSettings "structureDisplay").

#ifdef ServerMainVers_2700
static GS::UniString StructureDisplayToString (API_StructureDisplay structureDisplay)
{
    switch (structureDisplay) {
        case API_CoreOnly:        return "CoreOnly";
        case API_WithoutFinishes: return "WithoutFinishes";
        case API_StructureOnly:   return "StructureOnly";
        default:                  return "EntireStructure";
    }
}

static bool StringToStructureDisplay (const GS::UniString& str, API_StructureDisplay& structureDisplay)
{
    if      (str == "EntireStructure")  structureDisplay = API_EntireStructure;
    else if (str == "CoreOnly")         structureDisplay = API_CoreOnly;
    else if (str == "WithoutFinishes")  structureDisplay = API_WithoutFinishes;
    else if (str == "StructureOnly")    structureDisplay = API_StructureOnly;
    else                                return false;
    return true;
}
#endif

GetStructureDisplayCommand::GetStructureDisplayCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String GetStructureDisplayCommand::GetName () const
{
    return "GetStructureDisplay";
}

GS::Optional<GS::UniString> GetStructureDisplayCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "description": "The Partial Structure Display of the current window.",
        "properties": {
            "structureDisplay": {
                "type": "string",
                "description": "The Partial Structure Display mode of the current window.",
                "enum": ["EntireStructure", "CoreOnly", "WithoutFinishes", "StructureOnly"]
            },
            "is3D": {
                "type": "boolean",
                "description": "True when the setting belongs to the 3D window."
            }
        },
        "additionalProperties": false,
        "required": [
            "structureDisplay",
            "is3D"
        ]
    })";
}

GS::ObjectState GetStructureDisplayCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_UIStructureDisplay uiStructureDisplay = {};
    GSErrCode err = ACAPI_Navigator_GetStructureDisplay (&uiStructureDisplay);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Partial Structure Display of the current window. The current window must be a floor plan, section, elevation, interior elevation, worksheet, detail or 3D window.");
    }

    return GS::ObjectState (
        "structureDisplay", StructureDisplayToString (uiStructureDisplay.structureDisplay),
        "is3D", uiStructureDisplay.is3D);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}

SetStructureDisplayCommand::SetStructureDisplayCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetStructureDisplayCommand::GetName () const
{
    return "SetStructureDisplay";
}

GS::Optional<GS::UniString> SetStructureDisplayCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "structureDisplay": {
                "type": "string",
                "description": "The Partial Structure Display mode to apply to the current window.",
                "enum": ["EntireStructure", "CoreOnly", "WithoutFinishes", "StructureOnly"]
            }
        },
        "additionalProperties": false,
        "required": [
            "structureDisplay"
        ]
    })";
}

GS::Optional<GS::UniString> SetStructureDisplayCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetStructureDisplayCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString structureDisplayStr;
    API_StructureDisplay structureDisplay = API_EntireStructure;
    if (!parameters.Get ("structureDisplay", structureDisplayStr) || !StringToStructureDisplay (structureDisplayStr, structureDisplay)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "structureDisplay must be one of EntireStructure, CoreOnly, WithoutFinishes, StructureOnly.");
    }

    // Read the current setting first, so the owner database stays the one of the current window.
    API_UIStructureDisplay uiStructureDisplay = {};
    GSErrCode err = ACAPI_Navigator_GetStructureDisplay (&uiStructureDisplay);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the Partial Structure Display of the current window. The current window must be a floor plan, section, elevation, interior elevation, worksheet, detail or 3D window.");
    }

    uiStructureDisplay.structureDisplay = structureDisplay;
    err = ACAPI_Navigator_ChangeStructureDisplay (&uiStructureDisplay);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to change the Partial Structure Display of the current window.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    (void) parameters;
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
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

GS::Optional<GS::UniString> GetView2DTransformationsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> SetViewRotationCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> FitInWindowCommand::GetRawResponseSchema () const
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

ZoomToElementsCommand::ZoomToElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ZoomToElementsCommand::GetName () const
{
    return "ZoomToElements";
}

GS::Optional<GS::UniString> ZoomToElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "keepSelection": {
                "type": "boolean",
                "description": "Restore the selection the window had before the command (default). With false the given elements stay selected."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> ZoomToElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

// The selection of the current window as neigs; an empty array when nothing is selected.
static GSErrCode GetSelectedNeigs (GS::Array<API_Neig>& selectedNeigs)
{
    selectedNeigs.Clear ();

    API_SelectionInfo selectionInfo = {};
    GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selectedNeigs, false);
    BMKillHandle ((GSHandle*) &selectionInfo.marquee.coords);

    if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty) {
        selectedNeigs.Clear ();
        return NoError;
    }

    return err;
}

GS::ObjectState ZoomToElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);
    if (elements.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No elements were given.");
    }

    bool keepSelection = true;
    parameters.Get ("keepSelection", keepSelection);

    // APIDo_ZoomToSelectedID is documented to zoom to the selection "even in 3D", unlike
    // the element list of ACAPI_View_ZoomToElements (FitInWindow): the given elements are
    // selected, the window zooms to the selection, then the previous selection is restored.
    GS::Array<API_Neig> previousSelection;
    GSErrCode err = GetSelectedNeigs (previousSelection);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to retrieve the current selection.");
    }

    if (!previousSelection.IsEmpty ()) {
        err = ACAPI_Selection_Select (previousSelection, false);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to clear the current selection.");
        }
    }

    const auto restorePreviousSelection = [&] (const GS::Array<API_Neig>& selectionToRemove) -> GSErrCode {
        if (!selectionToRemove.IsEmpty ()) {
            const GSErrCode removeErr = ACAPI_Selection_Select (selectionToRemove, false);
            if (removeErr != NoError) {
                return removeErr;
            }
        }
        if (previousSelection.IsEmpty ()) {
            return NoError;
        }
        return ACAPI_Selection_Select (previousSelection, true);
    };

    // Elements that cannot be selected in the current window (on another story, on a
    // hidden layer, ...) are skipped; the selection is read back to see what is left.
    for (const GS::ObjectState& element : elements) {
        ACAPI_Selection_Select ({ API_Neig (GetGuidFromElementsArrayItem (element)) }, true);
    }

    GS::Array<API_Neig> zoomSelection;
    err = GetSelectedNeigs (zoomSelection);
    if (err == NoError && zoomSelection.IsEmpty ()) {
        err = APIERR_NOSEL;
    }
    if (err != NoError) {
        restorePreviousSelection (zoomSelection);
        return CreateFailedExecutionResult (err, "None of the given elements could be selected in the current window. They may be on another story, on a hidden layer or not shown in this window.");
    }

    err = ACAPI_View_ZoomToSelected ();
    if (err != NoError) {
        if (keepSelection) {
            restorePreviousSelection (zoomSelection);
        }
        return CreateFailedExecutionResult (err, "Failed to zoom to the elements.");
    }

    if (keepSelection) {
        err = restorePreviousSelection (zoomSelection);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Zoomed to the elements, but failed to restore the previous selection.");
        }
    }

    return CreateSuccessfulExecutionResult ();
}

SetZoomCommand::SetZoomCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetZoomCommand::GetName () const
{
    return "SetZoom";
}

GS::Optional<GS::UniString> SetZoomCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "zoomBox": {
                "$ref": "#/Zoom",
                "description": "The area to zoom the current 2D window to, in model coordinates (meters). The window keeps its aspect ratio, so the shown area may be larger than the box. The rotation of the window is kept as it is."
            }
        },
        "additionalProperties": false,
        "required": [
            "zoomBox"
        ]
    })";
}

GS::Optional<GS::UniString> SetZoomCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetZoomCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Box zoomBox = {};
    const GS::ObjectState* zoomBoxOS = parameters.Get ("zoomBox");
    if (zoomBoxOS == nullptr ||
        !zoomBoxOS->Get ("xMin", zoomBox.xMin) ||
        !zoomBoxOS->Get ("yMin", zoomBox.yMin) ||
        !zoomBoxOS->Get ("xMax", zoomBox.xMax) ||
        !zoomBoxOS->Get ("yMax", zoomBox.yMax)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid zoomBox parameter.");
    }
    if (zoomBox.xMin >= zoomBox.xMax || zoomBox.yMin >= zoomBox.yMax) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "The zoomBox must have xMin < xMax and yMin < yMax.");
    }

    API_WindowInfo currentWindow = {};
    GSErrCode err = ACAPI_Window_GetCurrentWindow (&currentWindow);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current window.");
    }
    if (currentWindow.typeID == APIWind_3DModelID) {
        return CreateFailedExecutionResult (APIERR_BADDATABASE, "The current window is the 3D window; SetZoom works only in 2D windows.");
    }

    // Without a transformation matrix ACAPI_View_SetZoom resets the rotation of the
    // window (for example of a rotated floor plan) to zero, so the current one is
    // read and passed back to keep the rotation as it is (issue #1692).
    API_Box     currentZoomBox = {};
    API_Tranmat currentTranmat = {};
    err = ACAPI_View_GetZoom (&currentZoomBox, &currentTranmat);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current zoom and rotation of the current window.");
    }

    err = ACAPI_View_SetZoom (&zoomBox, &currentTranmat);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the zoom of the current window.");
    }

    return CreateSuccessfulExecutionResult ();
}

ResetZoomCommand::ResetZoomCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ResetZoomCommand::GetName () const
{
    return "ResetZoom";
}

GS::Optional<GS::UniString> ResetZoomCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "numberOfStepsBack": {
                "type": "integer",
                "description": "The number of steps to go back in the zoom history of the current window, like Archicad's Zoom Back. The default is 1.",
                "minimum": 1,
                "maximum": 32767
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> ResetZoomCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ResetZoomCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    Int32 numberOfStepsBack = 1;
    parameters.Get ("numberOfStepsBack", numberOfStepsBack);
    if (numberOfStepsBack < 1 || numberOfStepsBack > 32767) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "numberOfStepsBack must be between 1 and 32767.");
    }

    short numOfStepsBack = static_cast<short> (numberOfStepsBack);
    const GSErrCode err = ACAPI_View_ResetZoom (&numOfStepsBack);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to step back in the zoom history of the current window. The history might be shorter than the requested number of steps, or the current window has no zoom history.");
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
                            "description": "Navigator item ID of the View Map folder to place the clone in. Required - Archicad's CloneProjectMapItemToViewMap API leaves a clone created directly at the View Map root in a broken state (it cannot be deleted or moved afterwards, by API or by hand), so Tapir refuses to fall back to the root on your behalf. Create a View Map folder (CreateViewMapFolder) first if you don't already have one to target."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "parentNavigatorItemId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewsData"]
    })";
}

GS::Optional<GS::UniString> CloneProjectMapItemToViewMapCommand::GetRawResponseSchema () const
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

// Shared by CloneProjectMapItemToViewMapCommand and CreateViewsInViewMapCommand: a caller
// can also reach the undeletable-orphan state (#658) by passing the View Map root's own
// guid as parentNavigatorItemId explicitly - e.g. one read back from GetNavigatorItemTree,
// whose root item IS the View Map root. Requiring the field non-empty only closes the
// omission path; this closes the explicit one too.
static bool IsViewMapRootGuid (const API_Guid& guid)
{
    API_NavigatorSet viewMapSet = {};
    viewMapSet.mapId = API_PublicViewMap;
    Int32 idx = 0;
    return ACAPI_Navigator_GetNavigatorSet (&viewMapSet, &idx) == NoError && guid == viewMapSet.rootGuid;
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

        const GS::ObjectState* parentOS = item.Get ("parentNavigatorItemId");
        if (parentOS == nullptr) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "parentNavigatorItemId is missing. Cloning to the View Map root is refused because Archicad's API leaves such a clone impossible to delete or move afterwards - target a real View Map folder instead."));
            continue;
        }

        API_Guid parentGuid = GetGuidFromObjectState (*parentOS);
        if (parentGuid == APINULLGuid) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "parentNavigatorItemId is corrupt or missing."));
            continue;
        }
        if (IsViewMapRootGuid (parentGuid)) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "parentNavigatorItemId is the View Map root itself. Cloning directly onto it is refused because Archicad's API leaves such a clone impossible to delete or move afterwards - target a real folder instead."));
            continue;
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
                            "description": "View Map folder to place the new view in. Required - the underlying Archicad API used to create the item leaves it undeletable and unmovable afterwards when it is attached directly to the View Map root, so Tapir refuses to fall back to the root on your behalf. Create a View Map folder (CreateViewMapFolder) first if you don't already have one to target."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name for the new view. Optional; defaults to the source item name."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "parentNavigatorItemId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewsData"]
    })";
}

GS::Optional<GS::UniString> CreateViewsInViewMapCommand::GetRawResponseSchema () const
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

        const GS::ObjectState* parentOS = item.Get ("parentNavigatorItemId");
        if (parentOS == nullptr) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "parentNavigatorItemId is missing. Creating the view directly at the View Map root is refused because it becomes impossible to delete or move afterwards - target a real View Map folder instead."));
            continue;
        }

        API_Guid parentGuid = GetGuidFromObjectState (*parentOS);
        if (parentGuid == APINULLGuid) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "parentNavigatorItemId is corrupt or missing."));
            continue;
        }
        if (IsViewMapRootGuid (parentGuid)) {
            navigatorItems (CreateErrorResponse (APIERR_BADPARS, "parentNavigatorItemId is the View Map root itself. Creating a view directly at it is refused because it becomes impossible to delete or move afterwards - target a real folder instead."));
            continue;
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

GS::Optional<GS::UniString> CreateViewMapFolderCommand::GetRawResponseSchema () const
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

// ACAPI_Navigator_GetNavigatorItem does not tell which navigator map an item is in, and
// ACAPI_Navigator_GetNavigatorChildrenItems needs it, so the children of parentGuid are
// looked up in every map and the map whose children contain childGuid is returned.
static bool GetChildrenContaining (const API_Guid& parentGuid, const API_Guid& childGuid, API_NavigatorMapID& mapId, GS::Array<API_Guid>& childGuids)
{
    API_NavigatorItem parentItem = {};
    if (ACAPI_Navigator_GetNavigatorItem (&parentGuid, &parentItem) != NoError) {
        return false;
    }

    for (const API_NavigatorMapID candidateMapId : { API_PublicViewMap, API_LayoutMap, API_PublisherSets, API_ProjectMap }) {
        parentItem.mapId = candidateMapId;
        GS::Array<API_NavigatorItem> children;
        if (ACAPI_Navigator_GetNavigatorChildrenItems (&parentItem, &children) != NoError) {
            continue;
        }
        GS::Array<API_Guid> guids;
        for (const API_NavigatorItem& child : children) {
            guids.Push (child.guid);
        }
        if (guids.Contains (childGuid)) {
            mapId      = candidateMapId;
            childGuids = guids;
            return true;
        }
    }
    return false;
}

static bool FindNavigatorItemParent (API_NavigatorItem item, API_NavigatorMapID mapId, const API_Guid& targetGuid, API_Guid& parentGuid)
{
    item.mapId = mapId;
    GS::Array<API_NavigatorItem> children;
    if (ACAPI_Navigator_GetNavigatorChildrenItems (&item, &children) != NoError) {
        return false;
    }
    for (const API_NavigatorItem& child : children) {
        if (child.guid == targetGuid) {
            parentGuid = item.guid;
            return true;
        }
        if (FindNavigatorItemParent (child, mapId, targetGuid, parentGuid)) {
            return true;
        }
    }
    return false;
}

// Archicad leaves an item in place when it is moved to the parent it is already in, but
// an item moved in from another parent is placed where asked. So a folder to park the
// item in for a moment is needed: the parent of parentGuid, or when parentGuid is the
// root of its map, a folder in it other than the item itself.
static bool FindTemporaryParent (const API_Guid& parentGuid, const API_Guid& itemGuid, API_NavigatorMapID mapId, API_Guid& temporaryParentGuid)
{
    // A project has one set of each map, except the publisher sets
    Int32 numberOfSets = 1;
    if (mapId == API_PublisherSets) {
        ACAPI_Navigator_GetNavigatorSetNum (&numberOfSets);
    }
    GS::Array<API_Guid> rootGuids;
    for (Int32 ii = 0; ii < numberOfSets; ++ii) {
        API_NavigatorSet navSet = {};
        navSet.mapId = mapId;
        if (ACAPI_Navigator_GetNavigatorSet (&navSet, &ii) == NoError) {
            rootGuids.Push (navSet.rootGuid);
        }
    }

    for (const API_Guid& rootGuid : rootGuids) {
        API_NavigatorItem rootItem = {};
        if (ACAPI_Navigator_GetNavigatorItem (&rootGuid, &rootItem) != NoError) {
            continue;
        }
        if (rootGuid != parentGuid) {
            if (FindNavigatorItemParent (rootItem, mapId, parentGuid, temporaryParentGuid)) {
                return true;
            }
            continue;
        }
        rootItem.mapId = mapId;
        GS::Array<API_NavigatorItem> children;
        if (ACAPI_Navigator_GetNavigatorChildrenItems (&rootItem, &children) != NoError) {
            return false;
        }
        for (const API_NavigatorItem& child : children) {
            if (child.guid != itemGuid && (child.itemType == API_FolderNavItem || child.itemType == API_SubSetNavItem)) {
                temporaryParentGuid = child.guid;
                return true;
            }
        }
        return false;
    }
    return false;
}

static bool IsAtRequestedPosition (const GS::Array<API_Guid>& childGuids, const API_Guid& itemGuid, const API_Guid* previousGuid)
{
    const UIndex itemIndex = childGuids.FindFirst (itemGuid);
    if (itemIndex == MaxUIndex) {
        return false;
    }
    if (previousGuid == nullptr) {
        return true;
    }
    const UIndex previousIndex = childGuids.FindFirst (*previousGuid);
    return previousIndex != MaxUIndex && previousIndex + 1 == itemIndex;
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
            "parentNavigatorItemId": {
                "$ref": "#/NavigatorItemId",
                "description": "The new parent of the item. It can be the item's current parent, to change the order of the items in it."
            },
            "previousNavigatorItemId": {
                "$ref": "#/NavigatorItemId",
                "description": "Optional child of parentNavigatorItemId that the item is placed right after. Without it an item moved within its own parent goes to the end of it."
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIdToMove", "parentNavigatorItemId"]
    })";
}

GS::Optional<GS::UniString> MoveNavigatorItemCommand::GetRawResponseSchema () const
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

    const API_Guid sourceApiGuid = GetGuidFromObjectState (*itemToMoveOS);
    const API_Guid parentApiGuid = GetGuidFromObjectState (*parentOS);
    const GS::Guid sourceGuid    = APIGuid2GSGuid (sourceApiGuid);
    const GS::Guid parentGuid    = APIGuid2GSGuid (parentApiGuid);

    const GS::ObjectState* prevOS = parameters.Get ("previousNavigatorItemId");
    API_Guid  prevApiGuid = APINULLGuid;
    GS::Guid  prevGuidStorage;
    GS::Guid* prevGuidPtr = nullptr;
    if (prevOS != nullptr) {
        prevApiGuid     = GetGuidFromObjectState (*prevOS);
        prevGuidStorage = APIGuid2GSGuid (prevApiGuid);
        prevGuidPtr     = &prevGuidStorage;
        if (prevApiGuid == sourceApiGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "previousNavigatorItemId cannot be the item to move.");
        }
    }
    const API_Guid* prevApiGuidPtr = prevOS != nullptr ? &prevApiGuid : nullptr;

    API_NavigatorMapID  mapId = API_PublicViewMap;
    GS::Array<API_Guid> childGuidsBefore;
    const bool alreadyInParent = GetChildrenContaining (parentApiGuid, sourceApiGuid, mapId, childGuidsBefore);

    // Checked before anything is moved, so that a wrong previousNavigatorItemId does not
    // leave the item in another folder
    if (prevApiGuidPtr != nullptr) {
        API_NavigatorMapID  prevMapId = API_PublicViewMap;
        GS::Array<API_Guid> prevSiblingGuids;
        const bool prevInParent = alreadyInParent ?
            childGuidsBefore.Contains (prevApiGuid) :
            GetChildrenContaining (parentApiGuid, prevApiGuid, prevMapId, prevSiblingGuids);
        if (!prevInParent) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "previousNavigatorItemId must be a child of parentNavigatorItemId.");
        }
    }

    GSErrCode     err = NoError;
    GS::UniString errorMessage;
    ACAPI_CallUndoableCommand ("MoveNavigatorItemCommand", [&]() -> GSErrCode {
        err = ACAPI_Navigator_SetNavigatorItemPosition (&sourceGuid, &parentGuid, prevGuidPtr);
        if (err != NoError) {
            errorMessage = "Failed to move navigator item.";
            return err;
        }

        // ACAPI_Navigator_SetNavigatorItemPosition returns NoError also when it leaves the
        // item where it was, so read the parent back. Without previousNavigatorItemId an
        // item that already is in the parent goes to the end of it.
        GS::Array<API_Guid> childGuids;
        bool moved = GetChildrenContaining (parentApiGuid, sourceApiGuid, mapId, childGuids) &&
                     IsAtRequestedPosition (childGuids, sourceApiGuid, prevApiGuidPtr) &&
                     (!alreadyInParent || prevApiGuidPtr != nullptr || childGuids.GetLast () == sourceApiGuid);

        // An item moved within its own parent stays in place, so move it out and back in
        if (!moved && alreadyInParent) {
            API_Guid temporaryParentApiGuid = APINULLGuid;
            if (!FindTemporaryParent (parentApiGuid, sourceApiGuid, mapId, temporaryParentApiGuid)) {
                err          = APIERR_GENERAL;
                errorMessage = "Archicad did not move the navigator item within its parent, and there is no other folder to move it through.";
                return err;
            }
            const GS::Guid temporaryParentGuid = APIGuid2GSGuid (temporaryParentApiGuid);
            err = ACAPI_Navigator_SetNavigatorItemPosition (&sourceGuid, &temporaryParentGuid, nullptr);
            if (err != NoError) {
                errorMessage = "Failed to move navigator item within its parent.";
                return err;
            }
            err = ACAPI_Navigator_SetNavigatorItemPosition (&sourceGuid, &parentGuid, prevGuidPtr);
            const bool backInParent = GetChildrenContaining (parentApiGuid, sourceApiGuid, mapId, childGuids);
            if (!backInParent) {
                // Put the item back where it was, after its previous sibling, instead of
                // leaving it in the temporary folder
                const UIndex originalIndex = childGuidsBefore.FindFirst (sourceApiGuid);
                GS::Guid  originalPrevGuid;
                GS::Guid* originalPrevGuidPtr = nullptr;
                if (originalIndex != MaxUIndex && originalIndex > 0) {
                    originalPrevGuid    = APIGuid2GSGuid (childGuidsBefore[originalIndex - 1]);
                    originalPrevGuidPtr = &originalPrevGuid;
                }
                ACAPI_Navigator_SetNavigatorItemPosition (&sourceGuid, &parentGuid, originalPrevGuidPtr);
            }
            if (err != NoError || !backInParent) {
                if (err == NoError) {
                    err = APIERR_REFUSEDPAR;
                }
                errorMessage = "Failed to move navigator item within its parent.";
                return err;
            }
            moved = IsAtRequestedPosition (childGuids, sourceApiGuid, prevApiGuidPtr);
        }

        if (!moved) {
            err          = APIERR_REFUSEDPAR;
            errorMessage = prevApiGuidPtr != nullptr ?
                "Archicad did not move the navigator item to the requested position." :
                "Archicad did not move the navigator item to the requested parent.";
            return err;
        }
        return NoError;
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, errorMessage);
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
            "newId": {
                "type": "string",
                "description": "The new ID of the item. On a layout it is the custom layout number (customLayoutNumbering is switched on); on a Project Map section, elevation, interior elevation, detail or worksheet it is the reference ID of the element behind the item; on any other item it is the custom ID of the navigator item (customUiId becomes true)."
            },
            "customName": {
                "type": "boolean",
                "description": "Set to false to drop the custom name of the item, so that it follows the name of its Project Map source again. Cannot be combined with newName."
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemId"]
    })";
}

GS::Optional<GS::UniString> RenameNavigatorItemCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

template <USize N>
static void CopyToFixedLengthString (GS::uchar_t (&target)[N], const GS::UniString& source)
{
    GS::ucsncpy (target, source.ToUStr (), N);
    target[N - 1] = 0;
}

static void SetSectionSegmentNameAndId (API_SectionSegment& segment, const GS::UniString* newName, const GS::UniString* newId)
{
    if (newName != nullptr) {
        CopyToFixedLengthString (segment.cutPlName, *newName);
    }
    if (newId != nullptr) {
        CopyToFixedLengthString (segment.cutPlIdStr, *newId);
    }
}

// A Project Map interior elevation item is one segment of the element: the
// viewpoint's database links to the element and names the segment by its index in
// the memo (intElevSegments), and the segment carries its own name and ID there.
// The element-level segment only holds the defaults the segments were created
// with, so it is left alone.
static GSErrCode RenameInteriorElevationSegment (API_Element& element, Int32 segmentMemoIndex, const GS::UniString* newName, const GS::UniString* newId, GS::UniString& failureMessage)
{
    API_ElementMemo memo = {};
    const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    GSErrCode err = ACAPI_Element_GetMemo (element.header.guid, &memo, APIMemoMask_All);
    if (err != NoError) {
        failureMessage = "Failed to read the segments of the interior elevation behind the navigator item.";
        return err;
    }

    const GSSize nSegments = memo.intElevSegments != nullptr
        ? BMGetPtrSize (reinterpret_cast<GSPtr> (memo.intElevSegments)) / sizeof (API_SectionSegment)
        : 0;
    if (segmentMemoIndex < 0 || segmentMemoIndex >= nSegments) {
        failureMessage = "The interior elevation segment behind the navigator item was not found.";
        return APIERR_NOTSUPPORTED;
    }

    SetSectionSegmentNameAndId (memo.intElevSegments[segmentMemoIndex], newName, newId);

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_All, true);
    if (err != NoError) {
        failureMessage = "Failed to rename the interior elevation segment behind the navigator item.";
    }
    return err;
}

// Sections, elevations, interior elevations, details and worksheets are elements
// first: their Project Map viewpoint only mirrors the name and ID stored on the
// element. ACAPI_Navigator_ChangeNavigatorItem is a View Map call - it accepts such
// an item and returns NoError without changing anything (#466, #755) - so the
// element behind the viewpoint is what has to be changed. The element is found
// through the viewpoint's database, which links back to it.
static GSErrCode RenameViewpointElement (const API_DatabaseInfo& dbInfo, const GS::UniString* newName, const GS::UniString* newId, GS::UniString& failureMessage)
{
    API_Element element = {};
    element.header.guid = dbInfo.linkedElement;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        failureMessage = "Failed to read the element behind the navigator item.";
        return err;
    }

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    switch (GetElemTypeId (element.header)) {
        case API_CutPlaneID:
            if (newName != nullptr) {
                CopyToFixedLengthString (element.cutPlane.segment.cutPlName, *newName);
                ACAPI_ELEMENT_MASK_SET (mask, API_CutPlaneType, segment.cutPlName);
            }
            if (newId != nullptr) {
                CopyToFixedLengthString (element.cutPlane.segment.cutPlIdStr, *newId);
                ACAPI_ELEMENT_MASK_SET (mask, API_CutPlaneType, segment.cutPlIdStr);
            }
            break;
        case API_ElevationID:
            if (newName != nullptr) {
                CopyToFixedLengthString (element.elevation.segment.cutPlName, *newName);
                ACAPI_ELEMENT_MASK_SET (mask, API_ElevationType, segment.cutPlName);
            }
            if (newId != nullptr) {
                CopyToFixedLengthString (element.elevation.segment.cutPlIdStr, *newId);
                ACAPI_ELEMENT_MASK_SET (mask, API_ElevationType, segment.cutPlIdStr);
            }
            break;
        case API_InteriorElevationID:
            return RenameInteriorElevationSegment (element, dbInfo.linkedSegmentMemoIndex, newName, newId, failureMessage);
        case API_DetailID:
            if (newName != nullptr) {
                CopyToFixedLengthString (element.detail.detailName, *newName);
                ACAPI_ELEMENT_MASK_SET (mask, API_DetailType, detailName);
            }
            if (newId != nullptr) {
                CopyToFixedLengthString (element.detail.detailIdStr, *newId);
                ACAPI_ELEMENT_MASK_SET (mask, API_DetailType, detailIdStr);
            }
            break;
        case API_WorksheetID:
            if (newName != nullptr) {
                CopyToFixedLengthString (element.worksheet.detailName, *newName);
                ACAPI_ELEMENT_MASK_SET (mask, API_WorksheetType, detailName);
            }
            if (newId != nullptr) {
                CopyToFixedLengthString (element.worksheet.detailIdStr, *newId);
                ACAPI_ELEMENT_MASK_SET (mask, API_WorksheetType, detailIdStr);
            }
            break;
        default:
            failureMessage = "Renaming this kind of Project Map item is not supported.";
            return APIERR_NOTSUPPORTED;
    }

    err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
    if (err != NoError) {
        failureMessage = "Failed to rename the element behind the navigator item.";
    }
    return err;
}

static bool ContainsNavigatorItem (API_NavigatorItem item, API_NavigatorMapID mapId, const API_Guid& guid)
{
    if (item.guid == guid) {
        return true;
    }
    item.mapId = mapId;
    GS::Array<API_NavigatorItem> children;
    if (ACAPI_Navigator_GetNavigatorChildrenItems (&item, &children) != NoError) {
        return false;
    }
    for (const API_NavigatorItem& child : children) {
        if (ContainsNavigatorItem (child, mapId, guid)) {
            return true;
        }
    }
    return false;
}

static bool IsProjectMapItem (const API_Guid& guid)
{
    API_NavigatorSet projectMapSet = {};
    projectMapSet.mapId = API_ProjectMap;
    Int32 idx = 0;
    if (ACAPI_Navigator_GetNavigatorSet (&projectMapSet, &idx) != NoError) {
        return false;
    }
    API_NavigatorItem rootItem = {};
    if (ACAPI_Navigator_GetNavigatorItem (&projectMapSet.rootGuid, &rootItem) != NoError) {
        return false;
    }
    return ContainsNavigatorItem (rootItem, API_ProjectMap, guid);
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
    const bool hasNewName = parameters.Get ("newName", newName) && !newName.IsEmpty ();

    bool customName = true;
    const bool resetCustomName = parameters.Get ("customName", customName) && !customName;
    if (hasNewName && resetCustomName) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "newName cannot be combined with customName false.");
    }

    GS::UniString newId;
    const bool hasNewId = parameters.Get ("newId", newId) && !newId.IsEmpty ();

    // The ID of a layout is its layout number, which lives in the layout settings
    // (API_LayoutInfo::customLayoutNumber) and not on the navigator item, so it is
    // changed through ACAPI_Navigator_ChangeLayoutSets below. The ID of every other
    // item is the uiId of the navigator item, changed the same way as its name.
    const bool isLayout = navItem.itemType == API_LayoutNavItem || navItem.itemType == API_MasterLayoutNavItem;
    const bool hasNewItemId = hasNewId && !isLayout;

    // Archicad does not let the name or the ID of a Project Map item (story, section,
    // elevation, schedule, ...) be changed through the navigator item:
    // ACAPI_Navigator_ChangeNavigatorItem returns NoError for them and
    // ACAPI_Navigator_GetNavigatorItem then reads back the value that was passed in,
    // so the read-back below cannot catch it. Sections, elevations, interior
    // elevations, details and worksheets are elements first, and their Project Map
    // viewpoint only mirrors the name and ID stored on the element, so for them the
    // element behind the viewpoint is renamed instead (#466, #755). It is found
    // through the viewpoint's database, which links back to it. Every other Project
    // Map item is refused here.
    const bool isProjectMapItem = (hasNewName || resetCustomName || hasNewId) && IsProjectMapItem (guid);
    API_DatabaseInfo dbInfo = navItem.db;
    const bool renameLinkedElement = isProjectMapItem &&
                                     !resetCustomName &&
                                     ACAPI_Window_GetDatabaseInfo (&dbInfo) == NoError &&
                                     dbInfo.linkedElement != APINULLGuid;
    if (isProjectMapItem && !renameLinkedElement) {
        return CreateFailedExecutionResult (APIERR_REFUSEDPAR,
            "The name and the ID of this Project Map item cannot be changed (only those of sections, elevations, interior elevations, details and worksheets can). Rename its view in the View Map instead.");
    }

    if (hasNewName) {
        GS::ucsncpy (navItem.uName, newName.ToUStr (), GS::ArraySize (navItem.uName));
        navItem.uName[GS::ArraySize (navItem.uName) - 1] = 0;
        navItem.customName = true;
    } else if (resetCustomName) {
        navItem.customName = false;
    }

    if (renameLinkedElement) {
        // The read-back runs inside the undoable command: when the navigator item
        // does not show the new name, the error rolls the element change back, so a
        // failed rename never leaves the element renamed. navItem.uName holds the
        // name truncated the way the item stores it, so a long name is not reported
        // as a failed rename. The item is not passed to ChangeNavigatorItem on this
        // path, so the read-back shows what the item really carries.
        GS::UniString failureMessage;
        const GSErrCode undoErr = ACAPI_CallUndoableCommand ("RenameNavigatorItemCommand", [&]() -> GSErrCode {
            err = RenameViewpointElement (dbInfo,
                                          hasNewName ? &newName : nullptr,
                                          hasNewId ? &newId : nullptr,
                                          failureMessage);
            if (err != NoError) {
                return err;
            }
            if (hasNewName) {
                API_NavigatorItem changedItem = {};
                err = ACAPI_Navigator_GetNavigatorItem (&guid, &changedItem);
                if (err != NoError) {
                    failureMessage = "Failed to read back the navigator item after the change.";
                    return err;
                }
                if (GS::UniString (changedItem.uName) != GS::UniString (navItem.uName)) {
                    failureMessage = "Archicad did not apply the new name. The Project Map item kept its old name after the element behind it was renamed, so the change was undone.";
                    err = APIERR_REFUSEDPAR;
                    return err;
                }
            }
            return NoError;
        });
        if (err == NoError && undoErr != NoError) {
            // The undo scope did not open (nested undoable command, teamwork state): the lambda never ran.
            err = undoErr;
            failureMessage = "Failed to rename the element behind the navigator item.";
        }
        if (err != NoError) {
            return CreateFailedExecutionResult (err, failureMessage);
        }
        return CreateSuccessfulExecutionResult ();
    }

    if (hasNewItemId) {
        // uiId is a char array (unlike uName), so it is copied like customLayoutNumber below.
        CHTruncate (newId.ToCStr ().Get (), navItem.uiId, GS::ArraySize (navItem.uiId));
        navItem.customUiId = true;
    }

    if (hasNewName || resetCustomName || hasNewItemId) {
        err = ACAPI_Navigator_ChangeNavigatorItem (&navItem);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to rename navigator item.");
        }

        // ACAPI_Navigator_ChangeNavigatorItem returns NoError even when it leaves the name
        // or the ID untouched (e.g. Project Map schedule items, which are named by their
        // scheme), so read the item back and report a value that was not applied as an error.
        API_NavigatorItem changedItem = {};
        err = ACAPI_Navigator_GetNavigatorItem (&guid, &changedItem);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to read back the navigator item after the change.");
        }
        if (hasNewName && GS::UniString (changedItem.uName) != GS::UniString (navItem.uName)) {
            return CreateFailedExecutionResult (APIERR_REFUSEDPAR,
                "Archicad did not apply the new name. The name of this navigator item cannot be changed by this command (e.g. Project Map schedule items are named by their scheme).");
        }
        if (resetCustomName && changedItem.customName) {
            return CreateFailedExecutionResult (APIERR_REFUSEDPAR,
                "Archicad did not drop the custom name of this navigator item.");
        }
        if (hasNewItemId && GS::UniString (changedItem.uiId) != GS::UniString (navItem.uiId)) {
            return CreateFailedExecutionResult (APIERR_REFUSEDPAR,
                "Archicad did not apply the new ID. The ID of this navigator item cannot be changed by this command.");
        }
    }

    if (hasNewId && isLayout) {
        API_LayoutInfo layoutInfo = {};
        err = ACAPI_Navigator_GetLayoutSets (&layoutInfo, &navItem.db.databaseUnId);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to read the layout settings of the layout.");
        }
        CHTruncate (newId.ToCStr ().Get (), layoutInfo.customLayoutNumber,
                    GS::ArraySize (layoutInfo.customLayoutNumber));
        layoutInfo.customLayoutNumbering = true;
        err = ACAPI_Navigator_ChangeLayoutSets (&layoutInfo, &navItem.db.databaseUnId);
        delete layoutInfo.customData;
        layoutInfo.customData = nullptr;
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to change the ID of the layout.");
        }

        API_LayoutInfo changedLayoutInfo = {};
        err = ACAPI_Navigator_GetLayoutSets (&changedLayoutInfo, &navItem.db.databaseUnId);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to read back the layout settings after the change.");
        }
        const bool layoutIdApplied = changedLayoutInfo.customLayoutNumbering &&
                                     GS::UniString (changedLayoutInfo.customLayoutNumber) == GS::UniString (layoutInfo.customLayoutNumber);
        delete changedLayoutInfo.customData;
        changedLayoutInfo.customData = nullptr;
        if (!layoutIdApplied) {
            return CreateFailedExecutionResult (APIERR_REFUSEDPAR,
                "Archicad did not apply the new ID of the layout.");
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

GS::Optional<GS::UniString> DeleteNavigatorItemsCommand::GetRawResponseSchema () const
{
    return R"({"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]})";
}

// A worksheet or detail of the Project Map is a database, like a layout, and
// ACAPI_Navigator_DeleteNavigatorView deletes View Map views only. The item is
// looked up in the Project Map by its database instead of trusting the mapId
// returned with it, so a View Map view of the same worksheet - another item on
// the same database - is not matched and still deletes only the view.
static bool IsProjectMapWorksheetOrDetail (const API_Guid& guid, const API_NavigatorItem& navItem)
{
    if (navItem.db.typeID != APIWind_WorksheetID && navItem.db.typeID != APIWind_DetailID) {
        return false;
    }

    API_NavigatorItem filterItem = {};
    filterItem.mapId           = API_ProjectMap;
    filterItem.itemType        = navItem.itemType;
    filterItem.db.databaseUnId = navItem.db.databaseUnId;

    GS::Array<API_NavigatorItem> results;
    if (ACAPI_Navigator_SearchNavigatorItem (&filterItem, &results) != NoError) {
        return false;
    }

    for (const API_NavigatorItem& result : results) {
        if (result.guid == guid) {
            return true;
        }
    }
    return false;
}

GS::ObjectState DeleteNavigatorItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> navigatorItemIds;
    parameters.Get ("navigatorItemIds", navigatorItemIds);

    API_ProjectInfo projectInfo = {};
    const bool isTeamwork = ACAPI_ProjectOperation_Project (&projectInfo) == NoError && projectInfo.teamwork;

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
        GS::UniString failureMessage = "Failed to delete navigator item.";
        if (navItem.itemType == API_LayoutNavItem || navItem.itemType == API_MasterLayoutNavItem) {
            API_DatabaseInfo dbInfo = navItem.db;
            err = ACAPI_Database_DeleteDatabase (&dbInfo);
        } else if (IsProjectMapWorksheetOrDetail (guid, navItem)) {
            // Only independent worksheets and details are databases of their
            // own; one placed by a marker belongs to its marker and cannot be
            // deleted this way.
            API_DatabaseInfo dbInfo = navItem.db;
            err = ACAPI_Database_DeleteDatabase (&dbInfo);
            failureMessage = "Failed to delete the worksheet or detail. Only independent worksheets and details can be deleted; delete a marker-linked one by deleting its marker.";
        } else if (navItem.itemType == API_SubSetNavItem) {
            err = APIERR_NOTSUPPORTED;
        } else {
            bool silentMode = true;
            err = ACAPI_Navigator_DeleteNavigatorView (&guid, &silentMode);
        }

        if (err == APIERR_NOTSUPPORTED) {
            executionResults (CreateFailedExecutionResult (err, "Deleting layout subsets is not supported by the ArchiCAD API."));
            continue;
        }
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, failureMessage));
            continue;
        }

        // The delete call reports NoError in silent mode even when Archicad refuses
        // the delete (in Teamwork: the item is not reserved or the user has no right
        // to delete it), so success is decided by whether the item is really gone
        // afterwards, as DeleteElements does.
        API_NavigatorItem remainingItem = {};
        if (ACAPI_Navigator_GetNavigatorItem (&guid, &remainingItem) != NoError) {
            executionResults (CreateSuccessfulExecutionResult ());
        } else if (isTeamwork) {
            executionResults (CreateFailedExecutionResult (APIERR_NOACCESSRIGHT, "The navigator item was not deleted. In a Teamwork project it must be reserved and the user needs the right to delete it."));
        } else {
            executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The navigator item was not deleted, because Archicad refused to delete it."));
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

static GS::ObjectState NavigatorItemToObjectState (API_NavigatorItem item, API_NavigatorMapID mapId, bool includeChildCount)
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

    // The ID shown next to the name on the navigator, and the two flags telling
    // whether the item's ID and name are hand-written or inherited from the
    // Project Map source - the View Settings "Custom" vs "By Project Map"
    // radio buttons. Only the flags can tell the two apart: a custom ID typed
    // identical to the source's is indistinguishable by comparing the strings.
    itemOS.Add ("uiId",          GS::UniString (item.uiId));
    itemOS.Add ("customUiId",    item.customUiId);
    itemOS.Add ("customName",    item.customName);
    itemOS.Add ("isIndependent", item.isIndependent);

    // The navigator item this one was created from: the Project Map item
    // behind a view, or the source view behind a placed Drawing's Layout Book
    // entry. Only present when the item has such a source; Project Map items
    // never do.
    if (item.sourceGuid != APINULLGuid) {
        itemOS.Add ("sourceNavigatorItemId", CreateGuidObjectState (item.sourceGuid));
    }

    item.mapId = mapId;

    // The number of direct children as Archicad counts them, without fetching
    // the children themselves. Missing when Archicad cannot count them.
    if (includeChildCount) {
        Int32 childCount = 0;
        if (ACAPI_Navigator_GetNavigatorChildNum (&item, &childCount) == NoError) {
            itemOS.Add ("childCount", childCount);
        }
    }

    GS::Array<API_NavigatorItem> children;
    if (ACAPI_Navigator_GetNavigatorChildrenItems (&item, &children) == NoError && !children.IsEmpty ()) {
        const auto& childList = itemOS.AddList<GS::ObjectState> ("children");
        for (const API_NavigatorItem& child : children) {
            GS::ObjectState wrapper;
            wrapper.Add ("navigatorItem", NavigatorItemToObjectState (child, mapId, includeChildCount));
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
            },
            "publisherSetName": {
                "type": "string",
                "minLength": 1,
                "description": "The name of the publisher set to retrieve. Used only when navigatorMapId is PublisherSets. Without it the first publisher set is returned."
            },
            "includeChildCount": {
                "type": "boolean",
                "description": "Set to true to add childCount, the number of direct children Archicad reports, to every item. Defaults to false."
            }
        },
        "additionalProperties": false,
        "required": ["navigatorMapId"]
    })";
}

// NavigatorItem refers back to itself through NavigatorItemArrayItem, which is
// what the children recursion needs. Only the documentation reads this schema -
// see the comment on CommandBase::GetResponseSchema - and its renderer stops at
// a reference it is already resolving, so the cycle is safe here.
GS::Optional<GS::UniString> GetNavigatorItemTreeCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemTree": {
                "$ref": "#/NavigatorItem"
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemTree"]
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

    API_Guid rootGuid = APINULLGuid;
    if (mapId == API_PublisherSets && parameters.Contains ("publisherSetName")) {
        // A project can hold several publisher sets, and the index passed to
        // ACAPI_Navigator_GetNavigatorSet selects between them. Looking the
        // set up by name lets the caller reach any of them, not just the first.
        GS::UniString publisherSetName;
        parameters.Get ("publisherSetName", publisherSetName);

        const auto publisherSetNameGuidTable = GetPublisherSetNameGuidTable ();
        if (!publisherSetNameGuidTable.ContainsKey (publisherSetName)) {
            return CreateErrorResponse (Error, "Not valid publisher set name.");
        }
        rootGuid = publisherSetNameGuidTable.Get (publisherSetName);
    } else {
        API_NavigatorSet navSet = {};
        navSet.mapId = mapId;
        Int32 idx = 0;
        GSErrCode err = ACAPI_Navigator_GetNavigatorSet (&navSet, &idx);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to get navigator set.");
        }
        rootGuid = navSet.rootGuid;
    }

    API_NavigatorItem rootItem = {};
    GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&rootGuid, &rootItem);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get root navigator item.");
    }

    bool includeChildCount = false;
    parameters.Get ("includeChildCount", includeChildCount);

    GS::ObjectState response;
    response.Add ("navigatorItemTree", NavigatorItemToObjectState (rootItem, mapId, includeChildCount));
    return response;
}

// ─── GetNavigatorParentItems ──────────────────────────────────────────────────

GetNavigatorParentItemsCommand::GetNavigatorParentItemsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetNavigatorParentItemsCommand::GetName () const
{
    return "GetNavigatorParentItems";
}

GS::Optional<GS::UniString> GetNavigatorParentItemsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetNavigatorParentItemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "parentItems": {
                "type": "array",
                "description": "The parent of each requested navigator item, in the order of the input, or an error for an item whose parent cannot be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "navigatorItemId": { "$ref": "#/NavigatorItemId" },
                                "type":            { "$ref": "#/NavigatorItemType" },
                                "name":            { "type": "string", "description": "The name of the parent navigator item." }
                            },
                            "additionalProperties": false,
                            "required": ["navigatorItemId", "type", "name"]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["parentItems"]
    })";
}

GS::ObjectState GetNavigatorParentItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> navigatorItemIds;
    parameters.Get ("navigatorItemIds", navigatorItemIds);

    GS::ObjectState response;
    const auto& parentItems = response.AddList<GS::ObjectState> ("parentItems");

    for (const GS::ObjectState& navItemIdOS : navigatorItemIds) {
        const API_Guid guid = GetGuidFromNavigatorItemIdArrayItem (navItemIdOS);
        if (guid == APINULLGuid) {
            parentItems (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing."));
            continue;
        }

        API_NavigatorItem parent = {};
        const GSErrCode err = ACAPI_Navigator_GetNavigatorParentItem (&guid, &parent);
        if (err != NoError) {
            parentItems (CreateErrorResponse (err, "Failed to get the parent of the navigator item."));
            continue;
        }

        // The root of a map has no parent. Report it as an error rather than
        // an item with a null id, whichever way Archicad signals it.
        if (parent.guid == APINULLGuid) {
            parentItems (CreateErrorResponse (APIERR_BADPARS, "The navigator item has no parent."));
            continue;
        }

        GS::ObjectState parentOS;
        parentOS.Add ("navigatorItemId", CreateGuidObjectState (parent.guid));
        parentOS.Add ("type",            NavigatorItemTypeToString (parent.itemType));
        parentOS.Add ("name",            GS::UniString (parent.uName));
        parentItems (parentOS);
    }

    return response;
}

// ─── SetMasterLayoutContext ───────────────────────────────────────────────────

SetMasterLayoutContextCommand::SetMasterLayoutContextCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetMasterLayoutContextCommand::GetName () const
{
    return "SetMasterLayoutContext";
}

GS::Optional<GS::UniString> SetMasterLayoutContextCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutDatabaseId": {
                "$ref": "#/DatabaseId"
            }
        },
        "additionalProperties": false,
        "required": ["layoutDatabaseId"]
    })";
}

GS::Optional<GS::UniString> SetMasterLayoutContextCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

// ACAPI_Navigator_SetMasterLayoutOnLayout does not assign a master layout to a
// layout. Its DevKit remark: when the current database is a master layout, it
// sets the layout whose values (autotexts, etc.) the master layout shows.
GS::ObjectState SetMasterLayoutContextCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    const API_Guid layoutDatabaseGuid = GetGuidFromArrayItem ("layoutDatabaseId", parameters);
    if (layoutDatabaseGuid == APINULLGuid) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "layoutDatabaseId is corrupt or missing.");
    }

    API_DatabaseInfo currentDb = {};
    GSErrCode err = ACAPI_Database_GetCurrentDatabase (&currentDb);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current database.");
    }
    if (currentDb.typeID != APIWind_MasterLayoutID) {
        return CreateFailedExecutionResult (APIERR_BADDATABASE, "The current window must be a master layout.");
    }

    API_DatabaseInfo layoutDb = DatabaseIdResolver::Instance ().GetDatabaseWithId (layoutDatabaseGuid);
    err = ACAPI_Window_GetDatabaseInfo (&layoutDb);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the database of layoutDatabaseId.");
    }
    if (layoutDb.typeID != APIWind_LayoutID) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "layoutDatabaseId is not the database of a layout.");
    }

    API_DatabaseUnId layoutDatabaseUnId = layoutDb.databaseUnId;
    err = ACAPI_Navigator_SetMasterLayoutOnLayout (&layoutDatabaseUnId);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the layout context of the master layout.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    (void) parameters;
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}

// ─── Add-on viewpoints ────────────────────────────────────────────────────────

// The Project Map items an add-on creates through the API (roots, groups and
// leaf nodes), read with ACAPI_Navigator_GetNavigatorVP*. They are identified
// by their guid, given as {"viewpointId": {"guid": ...}}.

static const char* AddOnViewpointIdsSchema = R"({
    "type": "array",
    "items": {
        "type": "object",
        "properties": {
            "viewpointId": {
                "type": "object",
                "description": "The identifier of an add-on viewpoint item.",
                "properties": {
                    "guid": { "$ref": "#/Guid" }
                },
                "additionalProperties": false,
                "required": ["guid"]
            }
        },
        "additionalProperties": false,
        "required": ["viewpointId"]
    }
})";

#ifdef ServerMainVers_2700
static GS::ObjectState CreateViewpointIdArrayItem (const API_Guid& guid)
{
    return GS::ObjectState ("viewpointId", CreateGuidObjectState (guid));
}

static GS::UniString AddOnViewpointTypeToString (API_NavigatorAddOnViewPointTypeID itemType)
{
    switch (itemType) {
        case API_NavigatorAddOnViewPointRootID:  return "Root";
        case API_NavigatorAddOnViewPointGroupID: return "Group";
        case API_NavigatorAddOnViewPointNodeID:  return "Leaf";
        default:                                 return "Undefined";
    }
}
#endif

GetAddOnViewpointRootsCommand::GetAddOnViewpointRootsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetAddOnViewpointRootsCommand::GetName () const
{
    return "GetAddOnViewpointRoots";
}

GS::Optional<GS::UniString> GetAddOnViewpointRootsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "viewpointIds": )") + AddOnViewpointIdsSchema + R"(
        },
        "additionalProperties": false,
        "required": ["viewpointIds"]
    })";
}

GS::ObjectState GetAddOnViewpointRootsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<API_Guid> rootGuids;
    const GSErrCode err = ACAPI_Navigator_GetNavigatorVPRootGroups (&rootGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the add-on viewpoint roots.");
    }

    GS::ObjectState response;
    const auto& viewpointIds = response.AddList<GS::ObjectState> ("viewpointIds");
    for (const API_Guid& rootGuid : rootGuids) {
        viewpointIds (CreateViewpointIdArrayItem (rootGuid));
    }
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}

GetAddOnViewpointChildrenCommand::GetAddOnViewpointChildrenCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetAddOnViewpointChildrenCommand::GetName () const
{
    return "GetAddOnViewpointChildren";
}

GS::Optional<GS::UniString> GetAddOnViewpointChildrenCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewpointId": {
                "type": "object",
                "description": "The add-on viewpoint root or group to list the children of.",
                "properties": {
                    "guid": { "$ref": "#/Guid" }
                },
                "additionalProperties": false,
                "required": ["guid"]
            }
        },
        "additionalProperties": false,
        "required": ["viewpointId"]
    })";
}

GS::Optional<GS::UniString> GetAddOnViewpointChildrenCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "viewpointIds": )") + AddOnViewpointIdsSchema + R"(
        },
        "additionalProperties": false,
        "required": ["viewpointIds"]
    })";
}

GS::ObjectState GetAddOnViewpointChildrenCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    const API_Guid guid = GetGuidFromArrayItem ("viewpointId", parameters);
    if (guid == APINULLGuid) {
        return CreateErrorResponse (APIERR_BADPARS, "viewpointId is corrupt or missing.");
    }

    GS::Array<API_Guid> childrenGuids;
    const GSErrCode err = ACAPI_Navigator_GetNavigatorVPItemChildren (&guid, &childrenGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the children of the add-on viewpoint.");
    }

    GS::ObjectState response;
    const auto& viewpointIds = response.AddList<GS::ObjectState> ("viewpointIds");
    for (const API_Guid& childGuid : childrenGuids) {
        viewpointIds (CreateViewpointIdArrayItem (childGuid));
    }
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}

GetAddOnViewpointsCommand::GetAddOnViewpointsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetAddOnViewpointsCommand::GetName () const
{
    return "GetAddOnViewpoints";
}

GS::Optional<GS::UniString> GetAddOnViewpointsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "viewpointIds": )") + AddOnViewpointIdsSchema + R"(
        },
        "additionalProperties": false,
        "required": ["viewpointIds"]
    })";
}

GS::Optional<GS::UniString> GetAddOnViewpointsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewpoints": {
                "type": "array",
                "description": "The details of each given add-on viewpoint item, in the order of the input, or an error for an item that cannot be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "viewpointId": {
                                    "type": "object",
                                    "description": "The identifier of the add-on viewpoint item.",
                                    "properties": {
                                        "guid": { "$ref": "#/Guid" }
                                    },
                                    "additionalProperties": false,
                                    "required": ["guid"]
                                },
                                "parentViewpointId": {
                                    "type": "object",
                                    "description": "The identifier of the root or group the item is in. Missing when Archicad reports no parent, as for a root.",
                                    "properties": {
                                        "guid": { "$ref": "#/Guid" }
                                    },
                                    "additionalProperties": false,
                                    "required": ["guid"]
                                },
                                "itemType": {
                                    "type": "string",
                                    "description": "Whether the item is a root, a group or a leaf node.",
                                    "enum": ["Root", "Group", "Leaf", "Undefined"]
                                },
                                "displayId": {
                                    "type": "string",
                                    "description": "The ID shown for the item in the Project Map."
                                },
                                "displayName": {
                                    "type": "string",
                                    "description": "The name shown for the item in the Project Map."
                                },
                                "viewSettingsFlags": {
                                    "type": "integer",
                                    "description": "The view settings flags of the item, as Archicad stores them (a bit field)."
                                },
                                "data": {
                                    "type": "string",
                                    "description": "The data the add-on attached to the item, base64 encoded. Empty when there is none."
                                }
                            },
                            "additionalProperties": false,
                            "required": ["viewpointId", "itemType", "displayId", "displayName", "viewSettingsFlags", "data"]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewpoints"]
    })";
}

GS::ObjectState GetAddOnViewpointsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> viewpointIds;
    parameters.Get ("viewpointIds", viewpointIds);

    GS::ObjectState response;
    const auto& viewpoints = response.AddList<GS::ObjectState> ("viewpoints");

    for (const GS::ObjectState& viewpointIdOS : viewpointIds) {
        const API_Guid guid = GetGuidFromArrayItem ("viewpointId", viewpointIdOS);
        if (guid == APINULLGuid) {
            viewpoints (CreateErrorResponse (APIERR_BADPARS, "viewpointId is corrupt or missing."));
            continue;
        }

        // The struct owns its data handle and disposes of it.
        API_NavigatorAddOnViewPointData viewPointData;
        viewPointData.guid = guid;
        const GSErrCode err = ACAPI_Navigator_GetNavigatorVPItem (&viewPointData);
        if (err != NoError) {
            viewpoints (CreateErrorResponse (err, "Failed to get the add-on viewpoint."));
            continue;
        }

        GS::UniString data;
        if (viewPointData.data != nullptr) {
            data = Base64Converter::Encode (*viewPointData.data, BMGetHandleSize (viewPointData.data));
            data.DeleteAll (GS::UniChar (char ('\n')));
        }

        GS::ObjectState viewpointOS;
        viewpointOS.Add ("viewpointId", CreateGuidObjectState (guid));
        if (viewPointData.parentGuid != APINULLGuid) {
            viewpointOS.Add ("parentViewpointId", CreateGuidObjectState (viewPointData.parentGuid));
        }
        viewpointOS.Add ("itemType",          AddOnViewpointTypeToString (viewPointData.itemType));
        viewpointOS.Add ("displayId",         viewPointData.displayId);
        viewpointOS.Add ("displayName",       viewPointData.displayName);
        viewpointOS.Add ("viewSettingsFlags", static_cast<Int64> (viewPointData.viewSettingsFlags));
        viewpointOS.Add ("data",              data);
        viewpoints (viewpointOS);
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}

// ─── Creating and changing add-on viewpoints ──────────────────────────────────

#ifdef ServerMainVers_2700
static bool AddOnViewpointTypeFromString (const GS::UniString& itemTypeStr, API_NavigatorAddOnViewPointTypeID& itemType)
{
    if (itemTypeStr == "Root") {
        itemType = API_NavigatorAddOnViewPointRootID;
    } else if (itemTypeStr == "Group") {
        itemType = API_NavigatorAddOnViewPointGroupID;
    } else if (itemTypeStr == "Leaf") {
        itemType = API_NavigatorAddOnViewPointNodeID;
    } else {
        return false;
    }
    return true;
}

// Decodes standard base64 (padding optional, whitespace ignored) into a new
// handle; an empty string gives nullptr. The caller owns the handle.
static bool CreateHandleFromBase64 (const GS::UniString& base64, GSHandle& handle)
{
    handle = nullptr;

    const auto base64CStr = base64.ToCStr (CC_UTF8);
    const char* text = base64CStr.Get ();

    GS::Array<char> bytes;
    UInt32 buffer = 0;
    Int32 bitCount = 0;
    bool paddingReached = false;
    for (const char* c = text; *c != '\0'; ++c) {
        const char ch = *c;
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            continue;
        }
        if (ch == '=') {
            paddingReached = true;
            continue;
        }
        if (paddingReached) {
            return false;
        }

        UInt32 value = 0;
        if (ch >= 'A' && ch <= 'Z') {
            value = static_cast<UInt32> (ch - 'A');
        } else if (ch >= 'a' && ch <= 'z') {
            value = static_cast<UInt32> (ch - 'a' + 26);
        } else if (ch >= '0' && ch <= '9') {
            value = static_cast<UInt32> (ch - '0' + 52);
        } else if (ch == '+') {
            value = 62;
        } else if (ch == '/') {
            value = 63;
        } else {
            return false;
        }

        buffer = ((buffer << 6) | value) & 0xFFFFFF;
        bitCount += 6;
        if (bitCount >= 8) {
            bitCount -= 8;
            bytes.Push (static_cast<char> ((buffer >> bitCount) & 0xFF));
        }
    }
    // A single leftover character carries fewer than 8 bits: not valid base64.
    if (bitCount >= 6) {
        return false;
    }

    if (bytes.IsEmpty ()) {
        return true;
    }

    handle = BMAllocateHandle (static_cast<GSSize> (bytes.GetSize ()), ALLOCATE_CLEAR, 0);
    if (handle == nullptr) {
        return false;
    }
    for (UIndex i = 0; i < bytes.GetSize (); ++i) {
        (*handle)[i] = bytes[i];
    }
    return true;
}

// Sets the fields given in the item (displayId, displayName, viewSettingsFlags,
// data) on viewPointData and leaves the others. Returns an error message for an
// invalid value, an empty string otherwise.
static GS::UniString ApplyAddOnViewpointFields (const GS::ObjectState& item, API_NavigatorAddOnViewPointData& viewPointData)
{
    GS::UniString displayId;
    if (item.Get ("displayId", displayId)) {
        viewPointData.displayId = displayId;
    }

    GS::UniString displayName;
    if (item.Get ("displayName", displayName)) {
        viewPointData.displayName = displayName;
    }

    Int64 viewSettingsFlags = 0;
    if (item.Get ("viewSettingsFlags", viewSettingsFlags)) {
        if (viewSettingsFlags < 0 || viewSettingsFlags > static_cast<Int64> (0xFFFFFFFFu)) {
            return "viewSettingsFlags must be between 0 and 4294967295.";
        }
        viewPointData.viewSettingsFlags = static_cast<UInt32> (viewSettingsFlags);
    }

    GS::UniString data;
    if (item.Get ("data", data)) {
        GSHandle dataHandle = nullptr;
        if (!CreateHandleFromBase64 (data, dataHandle)) {
            return "data is not valid base64.";
        }
        // The struct owns its data handle and disposes of it.
        BMKillHandle (&viewPointData.data);
        viewPointData.data = dataHandle;
    }

    return GS::UniString ();
}
#endif

CreateAddOnViewpointsCommand::CreateAddOnViewpointsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String CreateAddOnViewpointsCommand::GetName () const
{
    return "CreateAddOnViewpoints";
}

GS::Optional<GS::UniString> CreateAddOnViewpointsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewpoints": {
                "type": "array",
                "description": "The add-on viewpoint items to create. The identifier of a new root or group is only known from the response, so create the parents in an earlier call.",
                "items": {
                    "type": "object",
                    "properties": {
                        "itemType": {
                            "type": "string",
                            "description": "Whether to create a root, a group or a leaf node. Roots and groups can't be created in a Teamwork project.",
                            "enum": ["Root", "Group", "Leaf"]
                        },
                        "parentViewpointId": {
                            "type": "object",
                            "description": "The root or group to create the item in. Required for a group or a leaf node, ignored for a root.",
                            "properties": {
                                "guid": { "$ref": "#/Guid" }
                            },
                            "additionalProperties": false,
                            "required": ["guid"]
                        },
                        "displayId": {
                            "type": "string",
                            "description": "The ID shown for the item in the Project Map."
                        },
                        "displayName": {
                            "type": "string",
                            "description": "The name shown for the item in the Project Map."
                        },
                        "viewSettingsFlags": {
                            "type": "integer",
                            "description": "The view settings flags of a leaf node, as Archicad stores them (a bit field). Ignored for a root or a group.",
                            "minimum": 0,
                            "maximum": 4294967295
                        },
                        "data": {
                            "type": "string",
                            "description": "Data to attach to the item, base64 encoded."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["itemType"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewpoints"]
    })";
}

GS::Optional<GS::UniString> CreateAddOnViewpointsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewpoints": {
                "type": "array",
                "description": "The identifier of each created item, in the order of the input, or an error for an item that could not be created.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "viewpointId": {
                                    "type": "object",
                                    "description": "The identifier of the created add-on viewpoint item.",
                                    "properties": {
                                        "guid": { "$ref": "#/Guid" }
                                    },
                                    "additionalProperties": false,
                                    "required": ["guid"]
                                }
                            },
                            "additionalProperties": false,
                            "required": ["viewpointId"]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewpoints"]
    })";
}

GS::ObjectState CreateAddOnViewpointsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> items;
    parameters.Get ("viewpoints", items);

    GS::ObjectState response;
    const auto& viewpoints = response.AddList<GS::ObjectState> ("viewpoints");

    for (const GS::ObjectState& item : items) {
        GS::UniString itemTypeStr;
        item.Get ("itemType", itemTypeStr);

        // The struct owns its data handle and disposes of it. Its guid stays
        // APINULLGuid, so Archicad generates the guid of the new item.
        API_NavigatorAddOnViewPointData viewPointData;
        if (!AddOnViewpointTypeFromString (itemTypeStr, viewPointData.itemType)) {
            viewpoints (CreateErrorResponse (APIERR_BADPARS, "itemType must be Root, Group or Leaf."));
            continue;
        }

        if (viewPointData.itemType != API_NavigatorAddOnViewPointRootID) {
            viewPointData.parentGuid = GetGuidFromArrayItem ("parentViewpointId", item);
            if (viewPointData.parentGuid == APINULLGuid) {
                viewpoints (CreateErrorResponse (APIERR_BADPARS, "parentViewpointId is corrupt or missing; a group or a leaf node needs it."));
                continue;
            }
        }

        const GS::UniString fieldError = ApplyAddOnViewpointFields (item, viewPointData);
        if (!fieldError.IsEmpty ()) {
            viewpoints (CreateErrorResponse (APIERR_BADPARS, fieldError));
            continue;
        }

        const GSErrCode err = ACAPI_Navigator_CreateNavigatorVPItem (&viewPointData);
        if (err == APIERR_NOACCESSRIGHT) {
            viewpoints (CreateErrorResponse (err, "Roots and groups can't be created in a Teamwork project."));
            continue;
        }
        if (err != NoError) {
            viewpoints (CreateErrorResponse (err, "Failed to create the add-on viewpoint."));
            continue;
        }

        viewpoints (CreateViewpointIdArrayItem (viewPointData.guid));
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}

ChangeAddOnViewpointsCommand::ChangeAddOnViewpointsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ChangeAddOnViewpointsCommand::GetName () const
{
    return "ChangeAddOnViewpoints";
}

GS::Optional<GS::UniString> ChangeAddOnViewpointsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "viewpoints": {
                "type": "array",
                "description": "The add-on viewpoint items to change. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "viewpointId": {
                            "type": "object",
                            "description": "The identifier of the add-on viewpoint item to change.",
                            "properties": {
                                "guid": { "$ref": "#/Guid" }
                            },
                            "additionalProperties": false,
                            "required": ["guid"]
                        },
                        "displayId": {
                            "type": "string",
                            "description": "The ID shown for the item in the Project Map."
                        },
                        "displayName": {
                            "type": "string",
                            "description": "The name shown for the item in the Project Map."
                        },
                        "viewSettingsFlags": {
                            "type": "integer",
                            "description": "The view settings flags of the item, as Archicad stores them (a bit field).",
                            "minimum": 0,
                            "maximum": 4294967295
                        },
                        "data": {
                            "type": "string",
                            "description": "Data to attach to the item, base64 encoded. It replaces the attached data; an empty string removes it."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["viewpointId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewpoints"]
    })";
}

GS::Optional<GS::UniString> ChangeAddOnViewpointsCommand::GetRawResponseSchema () const
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

GS::ObjectState ChangeAddOnViewpointsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> items;
    parameters.Get ("viewpoints", items);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& item : items) {
        const API_Guid guid = GetGuidFromArrayItem ("viewpointId", item);
        if (guid == APINULLGuid) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "viewpointId is corrupt or missing."));
            continue;
        }

        // Read the item first, so the fields that are not given keep their
        // values. The struct owns its data handle and disposes of it.
        API_NavigatorAddOnViewPointData viewPointData;
        viewPointData.guid = guid;
        GSErrCode err = ACAPI_Navigator_GetNavigatorVPItem (&viewPointData);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to get the add-on viewpoint."));
            continue;
        }

        const GS::UniString fieldError = ApplyAddOnViewpointFields (item, viewPointData);
        if (!fieldError.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, fieldError));
            continue;
        }

        err = ACAPI_Navigator_ChangeNavigatorVPItem (&viewPointData);
        if (err == APIERR_NOACCESSRIGHT) {
            executionResults (CreateFailedExecutionResult (err, "Roots and groups can't be changed in a Teamwork project."));
            continue;
        }
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to change the add-on viewpoint."));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 27.");
#endif
}
