#include "DocumentCreationCommands.hpp"

#include "MigrationHelper.hpp"

namespace {

GS::ObjectState CreateDatabasesResponse (const GS::Array<GS::ObjectState>& databases)
{
    GS::ObjectState response;
    const auto& databasesList = response.AddList<GS::ObjectState> ("databases");
    for (const auto& database : databases) {
        databasesList (database);
    }
    return response;
}

GS::ObjectState CreateExecutionResultsResponse (const GS::Array<GS::ObjectState>& executionResults)
{
    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("executionResults");
    for (const auto& result : executionResults) {
        results (result);
    }
    return response;
}

bool GetItems (const GS::ObjectState& parameters, const char* fieldName, GS::Array<GS::ObjectState>& outItems, GS::ObjectState& errorResponse)
{
    if (!parameters.Get (fieldName, outItems)) {
        errorResponse = CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Missing required array field '%s'.", fieldName));
        return false;
    }
    return true;
}

GS::Array<API_Guid> GetLayoutDatabaseGuids ()
{
    GS::Array<API_Guid> guids;
    API_DatabaseUnId* dbases = nullptr;
    if (ACAPI_Database_GetLayoutDatabases (&dbases) == NoError && dbases != nullptr) {
        const GSSize nDbases = BMpGetSize (reinterpret_cast<GSPtr> (dbases)) / Sizeof32 (API_DatabaseUnId);
        for (GSSize i = 0; i < nDbases; ++i) {
            API_DatabaseInfo info = {};
            info.typeID = APIWind_LayoutID;
            info.databaseUnId = dbases[i];
            guids.Push (DatabaseIdResolver::Instance ().GetIdOfDatabase (info));
        }
    }
    BMpKill (reinterpret_cast<GSPtr*> (&dbases));
    return guids;
}

GS::Optional<API_Guid> FindNewLayoutDatabaseGuid (const GS::Array<API_Guid>& before)
{
    const GS::Array<API_Guid> after = GetLayoutDatabaseGuids ();
    for (const auto& guid : after) {
        if (!before.Contains (guid)) {
            return guid;
        }
    }
    return {};
}

GS::Optional<API_DatabaseInfo> FindMasterLayoutDatabaseByName (const GS::UniString& masterLayoutName)
{
    API_DatabaseUnId* dbases = nullptr;
    GS::Optional<API_DatabaseInfo> foundMasterLayout;

    if (ACAPI_Database_GetMasterLayoutDatabases (&dbases) == NoError && dbases != nullptr) {
        const GSSize nDbases = BMpGetSize (reinterpret_cast<GSPtr> (dbases)) / Sizeof32 (API_DatabaseUnId);
        for (GSSize i = 0; i < nDbases; ++i) {
            API_DatabaseInfo candidate = {};
            candidate.typeID = APIWind_MasterLayoutID;
            candidate.databaseUnId = dbases[i];

            if (ACAPI_Window_GetDatabaseInfo (&candidate) == NoError && GS::UniString (candidate.name) == masterLayoutName) {
                foundMasterLayout = candidate;
                break;
            }
        }
    }

    BMpKill (reinterpret_cast<GSPtr*> (&dbases));
    return foundMasterLayout;
}

GS::Optional<API_Guid> FindLayoutDatabaseGuidByName (const GS::UniString& layoutName)
{
    API_DatabaseUnId* dbases = nullptr;
    GS::Optional<API_Guid> foundLayoutGuid;

    if (ACAPI_Database_GetLayoutDatabases (&dbases) == NoError && dbases != nullptr) {
        const GSSize nDbases = BMpGetSize (reinterpret_cast<GSPtr> (dbases)) / Sizeof32 (API_DatabaseUnId);
        for (GSSize i = 0; i < nDbases; ++i) {
            API_DatabaseInfo candidate = {};
            candidate.typeID = APIWind_LayoutID;
            candidate.databaseUnId = dbases[i];

            if (ACAPI_Window_GetDatabaseInfo (&candidate) == NoError && GS::UniString (candidate.name) == layoutName) {
                foundLayoutGuid = DatabaseIdResolver::Instance ().GetIdOfDatabase (candidate);
                break;
            }
        }
    }

    BMpKill (reinterpret_cast<GSPtr*> (&dbases));
    return foundLayoutGuid;
}

bool GetLayoutInfoForDatabase (const API_DatabaseUnId& databaseUnId, API_LayoutInfo& layoutInfo)
{
    BNZeroMemory (&layoutInfo, sizeof (layoutInfo));
    return ACAPI_Navigator_GetLayoutSets (&layoutInfo, const_cast<API_DatabaseUnId*> (&databaseUnId), nullptr) == NoError;
}

}

CreateDetailsCommand::CreateDetailsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateDetailsCommand::GetName () const
{
    return "CreateDetails";
}

GS::Optional<GS::UniString> CreateDetailsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "detailsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string", "minLength": 1 },
                        "referenceId": { "type": "string", "minLength": 1 }
                    },
                    "additionalProperties": false,
                    "required": ["name", "referenceId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["detailsData"]
    })";
}

GS::Optional<GS::UniString> CreateDetailsCommand::GetResponseSchema () const
{
    return R"({"type":"object","properties":{"databases":{"$ref":"#/Databases"}},"additionalProperties":false,"required":["databases"]})";
}

GS::ObjectState CreateDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "detailsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> databases;
    for (const auto& item : items) {
        API_DatabaseInfo dbInfo = {};
        dbInfo.typeID = APIWind_DetailID;
        SetUCharProperty (&item, "name", dbInfo.name);
        SetUCharProperty (&item, "referenceId", dbInfo.ref);

        const GSErrCode err = ACAPI_Database_NewDatabase (&dbInfo);
        if (err != NoError) {
            databases.Push (CreateErrorResponse (err, "Failed to create detail database."));
            continue;
        }

        databases.Push (CreateDatabaseIdObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (dbInfo)));
    }
    return CreateDatabasesResponse (databases);
}

CreateWorksheetsCommand::CreateWorksheetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateWorksheetsCommand::GetName () const
{
    return "CreateWorksheets";
}

GS::Optional<GS::UniString> CreateWorksheetsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "worksheetsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string", "minLength": 1 },
                        "referenceId": { "type": "string", "minLength": 1 }
                    },
                    "additionalProperties": false,
                    "required": ["name", "referenceId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["worksheetsData"]
    })";
}

GS::Optional<GS::UniString> CreateWorksheetsCommand::GetResponseSchema () const
{
    return CreateDetailsCommand ().GetResponseSchema ();
}

GS::ObjectState CreateWorksheetsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "worksheetsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> databases;
    for (const auto& item : items) {
        API_DatabaseInfo dbInfo = {};
        dbInfo.typeID = APIWind_WorksheetID;
        SetUCharProperty (&item, "name", dbInfo.name);
        SetUCharProperty (&item, "referenceId", dbInfo.ref);

        const GSErrCode err = ACAPI_Database_NewDatabase (&dbInfo);
        if (err != NoError) {
            databases.Push (CreateErrorResponse (err, "Failed to create worksheet database."));
            continue;
        }

        databases.Push (CreateDatabaseIdObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (dbInfo)));
    }
    return CreateDatabasesResponse (databases);
}

CreateLayoutsCommand::CreateLayoutsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateLayoutsCommand::GetName () const
{
    return "CreateLayouts";
}

GS::Optional<GS::UniString> CreateLayoutsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "masterLayoutName": { "type": "string", "minLength": 1 },
                        "layoutName": { "type": "string", "minLength": 1 }
                    },
                    "additionalProperties": false,
                    "required": ["masterLayoutName", "layoutName"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutsData"]
    })";
}

GS::Optional<GS::UniString> CreateLayoutsCommand::GetResponseSchema () const
{
    return CreateDetailsCommand ().GetResponseSchema ();
}

GS::ObjectState CreateLayoutsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "layoutsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> databases;
    for (const auto& item : items) {
        GS::UniString masterLayoutName;
        item.Get ("masterLayoutName", masterLayoutName);

        API_DatabaseInfo masterLayoutDbInfo = {};
        if (const auto existingMasterLayout = FindMasterLayoutDatabaseByName (masterLayoutName); existingMasterLayout.HasValue ()) {
            masterLayoutDbInfo = existingMasterLayout.Get ();
        } else {
            masterLayoutDbInfo.typeID = APIWind_MasterLayoutID;
            SetUCharProperty (&item, "masterLayoutName", masterLayoutDbInfo.name);

            const GSErrCode createMasterErr = ACAPI_Database_NewDatabase (&masterLayoutDbInfo);
            if (createMasterErr != NoError) {
                databases.Push (CreateErrorResponse (createMasterErr, "Failed to create master layout."));
                continue;
            }
        }

        API_LayoutInfo layoutInfo = {};
        SetUCharProperty (&item, "layoutName", layoutInfo.layoutName);

        API_LayoutInfo masterLayoutInfo = {};
        if (GetLayoutInfoForDatabase (masterLayoutDbInfo.databaseUnId, masterLayoutInfo)) {
            layoutInfo.sizeX = masterLayoutInfo.sizeX;
            layoutInfo.sizeY = masterLayoutInfo.sizeY;
            layoutInfo.leftMargin = masterLayoutInfo.leftMargin;
            layoutInfo.topMargin = masterLayoutInfo.topMargin;
            layoutInfo.rightMargin = masterLayoutInfo.rightMargin;
            layoutInfo.bottomMargin = masterLayoutInfo.bottomMargin;
            layoutInfo.showMasterBelow = masterLayoutInfo.showMasterBelow;
        }

        const GS::Array<API_Guid> before = GetLayoutDatabaseGuids ();
        GSErrCode err = ACAPI_Navigator_CreateLayout (&layoutInfo, &masterLayoutDbInfo.databaseUnId);
        if (err != NoError) {
            databases.Push (CreateErrorResponse (err, "Failed to create layout."));
            continue;
        }

        const auto newLayoutGuid = FindNewLayoutDatabaseGuid (before);
        if (newLayoutGuid.HasValue ()) {
            databases.Push (CreateDatabaseIdObjectState (newLayoutGuid.Get ()));
        } else if (const auto layoutGuid = FindLayoutDatabaseGuidByName (GS::UniString (layoutInfo.layoutName)); layoutGuid.HasValue ()) {
            databases.Push (CreateDatabaseIdObjectState (layoutGuid.Get ()));
        } else {
            databases.Push (CreateErrorResponse (APIERR_GENERAL, "Layout created but could not resolve its database id."));
        }
    }

    return CreateDatabasesResponse (databases);
}

CreateSubsetsCommand::CreateSubsetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateSubsetsCommand::GetName () const
{
    return "CreateSubsets";
}

GS::Optional<GS::UniString> CreateSubsetsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "subsetsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string", "minLength": 1 },
                        "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "ownPrefix": { "type": "string" },
                        "customNumber": { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": ["name"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["subsetsData"]
    })";
}

GS::Optional<GS::UniString> CreateSubsetsCommand::GetResponseSchema () const
{
    return R"({"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]})";
}

GS::ObjectState CreateSubsetsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "subsetsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> executionResults;
    for (const auto& item : items) {
        API_SubSet subSet = {};
        GSErrCode err = ACAPI_Navigator_GetSubSetDefault (&subSet);
        if (err != NoError) {
            executionResults.Push (CreateFailedExecutionResult (err, "Failed to get subset defaults."));
            continue;
        }

        SetUCharProperty (&item, "name", subSet.name);
        if (item.Contains ("ownPrefix")) {
            subSet.addOwnPrefix = true;
            SetUCharProperty (&item, "ownPrefix", subSet.ownPrefix);
        }
        if (item.Contains ("customNumber")) {
            subSet.customNumbering = true;
            SetUCharProperty (&item, "customNumber", subSet.customNumber);
        }

        const GS::ObjectState* parent = item.Get ("parentNavigatorItemId");
        const API_Guid* parentGuidPtr = nullptr;
        API_Guid parentGuid = APINULLGuid;
        if (parent != nullptr) {
            parentGuid = GetGuidFromObjectState (*parent);
            parentGuidPtr = &parentGuid;
        }

        err = ACAPI_Navigator_CreateSubSet (&subSet, parentGuidPtr);
        executionResults.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to create subset."));
    }
    return CreateExecutionResultsResponse (executionResults);
}

CreateDrawingsCommand::CreateDrawingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateDrawingsCommand::GetName () const
{
    return "CreateDrawings";
}

GS::Optional<GS::UniString> CreateDrawingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "drawingsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutDatabaseId": { "$ref": "#/DatabaseId" },
                        "name": { "type": "string", "minLength": 1 },
                        "position": { "$ref": "#/Coordinate2D" },
                        "scale": { "type": "number", "exclusiveMinimum": 0.0 }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "name", "position"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["drawingsData"]
    })";
}

GS::Optional<GS::UniString> CreateDrawingsCommand::GetResponseSchema () const
{
    return R"({"type":"object","properties":{"elements":{"$ref":"#/Elements"}},"additionalProperties":false,"required":["elements"]})";
}

GS::ObjectState CreateDrawingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "drawingsData", items, errorResponse)) {
        return errorResponse;
    }

    API_DatabaseInfo startingDatabase = {};
    const GSErrCode startingDatabaseErr = ACAPI_Database_GetCurrentDatabase (&startingDatabase);

    GS::Array<GS::ObjectState> elementResults;
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("CreateDrawingsCommand", [&]() -> GSErrCode {
        for (const auto& item : items) {
            if (startingDatabaseErr == NoError) {
                const GS::ObjectState* layoutDatabaseId = item.Get ("layoutDatabaseId");
                if (layoutDatabaseId != nullptr) {
                    API_DatabaseInfo targetDatabase = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*layoutDatabaseId));
                    GSErrCode err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
                    if (err != NoError) {
                        elementResults.Push (CreateErrorResponse (err, "Failed to resolve layout database."));
                        continue;
                    }

                    err = ACAPI_Database_ChangeCurrentDatabase (&targetDatabase);
                    if (err != NoError) {
                        elementResults.Push (CreateErrorResponse (err, "Failed to activate layout database."));
                        continue;
                    }

                    API_WindowInfo windowInfo = targetDatabase;
                    err = ACAPI_Window_ChangeWindow (&windowInfo);
                    if (err != NoError) {
                        elementResults.Push (CreateErrorResponse (err, "Failed to activate layout window."));
                        continue;
                    }
                }
            }

            API_Element element = {};
            element.header.type = API_DrawingID;
            GSErrCode err = ACAPI_Element_GetDefaults (&element, nullptr);
            if (err != NoError) {
                elementResults.Push (CreateErrorResponse (err, "Failed to get drawing defaults."));
                continue;
            }

            element.drawing.drawingGuid = GetGuidFromObjectState (*item.Get ("navigatorItemId"));
            SetCharProperty (&item, "name", element.drawing.name);
            element.drawing.nameType = APIName_CustomName;
            element.drawing.anchorPoint = APIAnc_MM;
            element.drawing.pos = Get2DCoordinateFromObjectState (*item.Get ("position"));
            if (item.Contains ("scale")) {
                item.Get ("scale", element.drawing.ratio);
            } else {
                element.drawing.ratio = 1.0;
            }

            API_ElementMemo memo = {};
            err = ACAPI_Element_Create (&element, &memo);
            ACAPI_DisposeElemMemoHdls (&memo);

            if (err != NoError) {
                elementResults.Push (CreateErrorResponse (err, "Failed to create drawing."));
                continue;
            }
            elementResults.Push (CreateElementIdObjectState (element.header.guid));
        }

        return NoError;
    });

    if (startingDatabaseErr == NoError) {
        ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    }
    if (undoErr != NoError) {
        elementResults.Push (CreateErrorResponse (undoErr, "Failed to execute command in undo scope."));
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    for (const auto& elementResult : elementResults) {
        elements (elementResult);
    }
    return response;
}
