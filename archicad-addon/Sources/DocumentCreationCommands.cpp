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

GS::ObjectState CreateNavigatorItemsResponse (const GS::Array<GS::ObjectState>& navigatorItems)
{
    GS::ObjectState response;
    const auto& list = response.AddList<GS::ObjectState> ("navigatorItems");
    for (const auto& item : navigatorItems) {
        list (item);
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
    GS::Array<API_DatabaseUnId> dbases;
    if (ACAPI_Database_GetLayoutDatabases (nullptr, &dbases) == NoError) {
        for (const auto& db : dbases) {
            API_DatabaseInfo info = {};
            info.typeID = APIWind_LayoutID;
            info.databaseUnId = db;
            guids.Push (DatabaseIdResolver::Instance ().GetIdOfDatabase (info));
        }
    }
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
    GS::Optional<API_DatabaseInfo> foundMasterLayout;
    GS::Array<API_DatabaseUnId> dbases;
    if (ACAPI_Database_GetMasterLayoutDatabases (nullptr, &dbases) == NoError) {
        for (const auto& db : dbases) {
            API_DatabaseInfo candidate = {};
            candidate.typeID = APIWind_MasterLayoutID;
            candidate.databaseUnId = db;
            if (ACAPI_Window_GetDatabaseInfo (&candidate) == NoError && GS::UniString (candidate.name) == masterLayoutName) {
                foundMasterLayout = candidate;
                break;
            }
        }
    }

    return foundMasterLayout;
}

GS::Optional<API_Guid> FindLayoutDatabaseGuidByName (const GS::UniString& layoutName)
{
    GS::Optional<API_Guid> foundLayoutGuid;

    GS::Array<API_DatabaseUnId> dbases;
    if (ACAPI_Database_GetLayoutDatabases (nullptr, &dbases) == NoError) {
        for (const auto& db : dbases) {
            API_DatabaseInfo candidate = {};
            candidate.typeID = APIWind_LayoutID;
            candidate.databaseUnId = db;
            if (ACAPI_Window_GetDatabaseInfo (&candidate) == NoError && GS::UniString (candidate.name) == layoutName) {
                foundLayoutGuid = DatabaseIdResolver::Instance ().GetIdOfDatabase (candidate);
                break;
            }
        }
    }

    return foundLayoutGuid;
}

bool GetLayoutInfoForDatabase (const API_DatabaseUnId& databaseUnId, API_LayoutInfo& layoutInfo)
{
    BNZeroMemory (&layoutInfo, sizeof (layoutInfo));
    return ACAPI_Navigator_GetLayoutSets (&layoutInfo, const_cast<API_DatabaseUnId*> (&databaseUnId)) == NoError;
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

CreateLayoutCommand::CreateLayoutCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateLayoutCommand::GetName () const
{
    return "CreateLayout";
}

GS::Optional<GS::UniString> CreateLayoutCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "masterLayoutName":      { "type": "string", "minLength": 1 },
                        "masterNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutName":            { "type": "string", "minLength": 1 },
                        "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutParameters": {
                            "type": "object",
                            "properties": {
                                "horizontalSize":           { "type": "number" },
                                "verticalSize":             { "type": "number" },
                                "leftMargin":               { "type": "number" },
                                "topMargin":                { "type": "number" },
                                "rightMargin":              { "type": "number" },
                                "bottomMargin":             { "type": "number" },
                                "customLayoutNumber":       { "type": "string" },
                                "customLayoutNumbering":    { "type": "boolean" },
                                "doNotIncludeInNumbering":  { "type": "boolean" },
                                "displayMasterLayoutBelow": { "type": "boolean" }
                            },
                            "additionalProperties": false
                        }
                    },
                    "additionalProperties": false,
                    "required": ["layoutName"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutsData"]
    })";
}

GS::Optional<GS::UniString> CreateLayoutCommand::GetResponseSchema () const
{
    return CreateDetailsCommand ().GetResponseSchema ();
}

GS::ObjectState CreateLayoutCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "layoutsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> databases;
    for (const auto& item : items) {
        API_DatabaseInfo masterLayoutDbInfo = {};

        const GS::ObjectState* masterNavItemOS = item.Get ("masterNavigatorItemId");
        if (masterNavItemOS != nullptr) {
            API_Guid masterNavGuid = GetGuidFromObjectState (*masterNavItemOS);
            API_NavigatorItem masterNavItem = {};
            const GSErrCode navErr = ACAPI_Navigator_GetNavigatorItem (&masterNavGuid, &masterNavItem);
            if (navErr != NoError) {
                databases.Push (CreateErrorResponse (navErr, "Failed to get master layout from masterNavigatorItemId."));
                continue;
            }
            masterLayoutDbInfo = masterNavItem.db;
        } else {
            GS::UniString masterLayoutName;
            item.Get ("masterLayoutName", masterLayoutName);

            const auto existingMasterLayout = FindMasterLayoutDatabaseByName (masterLayoutName);
            if (existingMasterLayout.HasValue ()) {
                masterLayoutDbInfo = existingMasterLayout.Get ();
            } else {
                if (masterLayoutName.IsEmpty ()) {
                    databases.Push (CreateErrorResponse (APIERR_BADPARS, "Either masterLayoutName or masterNavigatorItemId must be provided."));
                    continue;
                }
                masterLayoutDbInfo.typeID = APIWind_MasterLayoutID;
                SetUCharProperty (&item, "masterLayoutName", masterLayoutDbInfo.name);

                const GSErrCode createMasterErr = ACAPI_Database_NewDatabase (&masterLayoutDbInfo);
                if (createMasterErr != NoError) {
                    databases.Push (CreateErrorResponse (createMasterErr, "Failed to create master layout."));
                    continue;
                }
            }
        }

        API_LayoutInfo layoutInfo = {};
#ifdef ServerMainVers_2600
        SetUCharProperty (&item, "layoutName", layoutInfo.layoutName);
#else
        SetCharProperty (&item, "layoutName", layoutInfo.layoutName);
#endif

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

        const GS::ObjectState* layoutParamsOS = item.Get ("layoutParameters");
        if (layoutParamsOS != nullptr) {
            layoutParamsOS->Get ("horizontalSize",           layoutInfo.sizeX);
            layoutParamsOS->Get ("verticalSize",             layoutInfo.sizeY);
            layoutParamsOS->Get ("leftMargin",               layoutInfo.leftMargin);
            layoutParamsOS->Get ("topMargin",                layoutInfo.topMargin);
            layoutParamsOS->Get ("rightMargin",              layoutInfo.rightMargin);
            layoutParamsOS->Get ("bottomMargin",             layoutInfo.bottomMargin);
            SetCharProperty (layoutParamsOS, "customLayoutNumber",      layoutInfo.customLayoutNumber);
            layoutParamsOS->Get ("customLayoutNumbering",    layoutInfo.customLayoutNumbering);
            layoutParamsOS->Get ("doNotIncludeInNumbering",  layoutInfo.doNotIncludeInNumbering);
            layoutParamsOS->Get ("displayMasterLayoutBelow", layoutInfo.showMasterBelow);
        }

        API_Guid parentNavGuid = APINULLGuid;
        const GS::ObjectState* parentOS = item.Get ("parentNavigatorItemId");
        if (parentOS != nullptr) {
            parentNavGuid = GetGuidFromObjectState (*parentOS);
        }

        const GS::Array<API_Guid> before = GetLayoutDatabaseGuids ();
#ifdef ServerMainVers_2700
        const GSErrCode err = ACAPI_Navigator_CreateLayout (&layoutInfo, &masterLayoutDbInfo.databaseUnId,
            (parentNavGuid != APINULLGuid) ? &parentNavGuid : nullptr);
#else
        // AC25/26: parent navigator item not supported by CreateLayout API
        const GSErrCode err = ACAPI_Navigator_CreateLayout (&layoutInfo, &masterLayoutDbInfo.databaseUnId);
#endif
        if (err != NoError) {
            databases.Push (CreateErrorResponse (err, "Failed to create layout."));
            continue;
        }

        const auto newLayoutGuid = FindNewLayoutDatabaseGuid (before);
        if (newLayoutGuid.HasValue ()) {
            databases.Push (CreateDatabaseIdObjectState (newLayoutGuid.Get ()));
        } else {
            const auto layoutGuid = FindLayoutDatabaseGuidByName (GS::UniString (layoutInfo.layoutName));
            if (layoutGuid.HasValue ()) {
                databases.Push (CreateDatabaseIdObjectState (layoutGuid.Get ()));
            } else {
                databases.Push (CreateErrorResponse (APIERR_GENERAL, "Layout created but could not resolve its database id."));
            }
        }
    }

    return CreateDatabasesResponse (databases);
}

CreateLayoutSubsetCommand::CreateLayoutSubsetCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateLayoutSubsetCommand::GetName () const
{
    return "CreateLayoutSubset";
}

GS::Optional<GS::UniString> CreateLayoutSubsetCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "subsetsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name":                  { "type": "string", "minLength": 1 },
                        "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "ownPrefix":             { "type": "string" },
                        "customNumber":          { "type": "string" },
                        "numberingStyle":        { "type": "string", "enum": ["Undefined", "abc", "ABC", "1", "01", "001", "0001", "noID"] },
                        "startAt":               { "type": "integer" },
                        "continueNumbering":     { "type": "boolean" },
                        "useUpperPrefix":        { "type": "boolean" },
                        "includeToIDSequence":   { "type": "boolean" },
                        "customNumbering":       { "type": "boolean" },
                        "addOwnPrefix":          { "type": "boolean" }
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

GS::Optional<GS::UniString> CreateLayoutSubsetCommand::GetResponseSchema () const
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

GS::ObjectState CreateLayoutSubsetCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "subsetsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> navigatorItems;
    for (const auto& item : items) {
        API_SubSet subSet = {};
        GSErrCode err = ACAPI_Navigator_GetSubSetDefault (&subSet);
        if (err != NoError) {
            navigatorItems.Push (CreateErrorResponse (err, "Failed to get subset defaults."));
            continue;
        }

        SetUCharProperty (&item, "name", subSet.name);

        // ownPrefix — backward compat: if ownPrefix string given without explicit addOwnPrefix, set it true
        if (item.Contains ("ownPrefix")) {
            subSet.addOwnPrefix = true;
            SetUCharProperty (&item, "ownPrefix", subSet.ownPrefix);
        }
        bool addOwnPrefixExplicit = false;
        if (item.Get ("addOwnPrefix", addOwnPrefixExplicit)) {
            subSet.addOwnPrefix = addOwnPrefixExplicit;
        }

        // customNumber — backward compat: if customNumber string given without explicit customNumbering, set it true
        if (item.Contains ("customNumber")) {
            subSet.customNumbering = true;
            SetUCharProperty (&item, "customNumber", subSet.customNumber);
        }
        bool customNumberingExplicit = false;
        if (item.Get ("customNumbering", customNumberingExplicit)) {
            subSet.customNumbering = customNumberingExplicit;
        }

        // numberingStyle
        GS::UniString numberingStyleStr;
        if (item.Get ("numberingStyle", numberingStyleStr)) {
            if      (numberingStyleStr == "abc")  subSet.numberingStyle = API_NS_abc;
            else if (numberingStyleStr == "ABC")  subSet.numberingStyle = API_NS_ABC;
            else if (numberingStyleStr == "1")    subSet.numberingStyle = API_NS_1;
            else if (numberingStyleStr == "01")   subSet.numberingStyle = API_NS_01;
            else if (numberingStyleStr == "001")  subSet.numberingStyle = API_NS_001;
            else if (numberingStyleStr == "0001") subSet.numberingStyle = API_NS_0001;
            else if (numberingStyleStr == "noID") subSet.numberingStyle = API_NS_noID;
            else                                  subSet.numberingStyle = API_NS_Undefined;
        }

        item.Get ("startAt",           subSet.startAt);
        item.Get ("continueNumbering", subSet.continueNumbering);
        item.Get ("useUpperPrefix",    subSet.useUpperPrefix);

        const GS::ObjectState* parent = item.Get ("parentNavigatorItemId");
        const API_Guid* parentGuidPtr = nullptr;
        API_Guid parentGuid = APINULLGuid;
        if (parent != nullptr) {
            parentGuid = GetGuidFromObjectState (*parent);
            parentGuidPtr = &parentGuid;
        }

        err = ACAPI_Navigator_CreateSubSet (&subSet, parentGuidPtr);
        if (err != NoError) {
            navigatorItems.Push (CreateErrorResponse (err, "Failed to create subset."));
            continue;
        }

        navigatorItems.Push (CreateSuccessfulExecutionResult ());
    }
    return CreateNavigatorItemsResponse (navigatorItems);
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
                        "scale": { "type": "number", "exclusiveMinimum": 0.0 },
                        "clipPolygon": { "type": "array", "items": { "$ref": "#/Coordinate2D" }, "minItems": 3 }
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
#ifdef ServerMainVers_2600
            element.header.type   = API_DrawingID;
#else
            element.header.typeID = API_DrawingID;
#endif
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
            if (!item.Get ("scale", element.drawing.ratio)) {
                element.drawing.ratio = 1.0;
            }

            // Optional clip polygon — drop closing vertex if present, then build memo
            GS::Array<GS::ObjectState> clipCoords;
            item.Get ("clipPolygon", clipCoords);
            if (clipCoords.GetSize () > 1) {
                API_Coord first = Get2DCoordinateFromObjectState (clipCoords.GetFirst ());
                API_Coord last  = Get2DCoordinateFromObjectState (clipCoords.GetLast ());
                if (IsSame2DCoordinate (first, last))
                    clipCoords.Pop ();
            }
            const Int32 nClip = (Int32) clipCoords.GetSize ();

            API_ElementMemo memo = {};
            if (nClip >= 3) {
                element.drawing.isCutWithFrame = true;
                element.drawing.poly.nSubPolys = 1;
                element.drawing.poly.nCoords   = nClip + 1;
                element.drawing.poly.nArcs     = 0;
                memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nClip + 2) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
                memo.pends  = reinterpret_cast<Int32**>     (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
                if (memo.coords != nullptr && memo.pends != nullptr) {
                    for (Int32 i = 0; i < nClip; ++i)
                        (*memo.coords)[i + 1] = Get2DCoordinateFromObjectState (clipCoords[i]);
                    (*memo.coords)[nClip + 1] = (*memo.coords)[1];
                    (*memo.pends)[1] = nClip + 1;
                }
            }
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

// Returns a GUID-string → field-name map built from the Layout Book custom scheme.
static GS::HashTable<GS::UniString, GS::UniString> GetLayoutSchemeByGuidString ()
{
    GS::HashTable<GS::UniString, GS::UniString> result;
    API_LayoutBook book = {};
    if (ACAPI_Navigator_GetLayoutBook (&book) != NoError) {
        return result;
    }
    for (const auto& kv : book.customScheme) {
#ifdef ServerMainVers_2800
        result.Add (APIGuidToString (kv.key), kv.value);
#else
        result.Add (APIGuidToString (*kv.key), *kv.value);
#endif
    }
    return result;
}

GetLayoutSettingsCommand::GetLayoutSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLayoutSettingsCommand::GetName () const
{
    return "GetLayoutSettings";
}

GS::Optional<GS::UniString> GetLayoutSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutDatabaseIds": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "databaseId":      { "$ref": "#/DatabaseId" },
                        "navigatorItemId": { "$ref": "#/NavigatorItemId" }
                    },
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutDatabaseIds"]
    })";
}

GS::Optional<GS::UniString> GetLayoutSettingsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutSettings": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "layoutName":               { "type": "string" },
                        "horizontalSize":           { "type": "number" },
                        "verticalSize":             { "type": "number" },
                        "leftMargin":               { "type": "number" },
                        "topMargin":                { "type": "number" },
                        "rightMargin":              { "type": "number" },
                        "bottomMargin":             { "type": "number" },
                        "customLayoutNumber":       { "type": "string" },
                        "customLayoutNumbering":    { "type": "boolean" },
                        "doNotIncludeInNumbering":  { "type": "boolean" },
                        "displayMasterLayoutBelow": { "type": "boolean" },
                        "customData": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "customSchemeKey":   { "type": "string" },
                                    "customSchemeName":  { "type": "string" },
                                    "customSchemeValue": { "type": "string" }
                                },
                                "required": ["customSchemeKey", "customSchemeValue"],
                                "additionalProperties": false
                            }
                        }
                    },
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutSettings"]
    })";
}

GS::ObjectState GetLayoutSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "layoutDatabaseIds", items, errorResponse)) {
        return errorResponse;
    }

    const auto schemeByGuid = GetLayoutSchemeByGuidString ();

    GS::ObjectState response;
    const auto& layoutSettingsList = response.AddList<GS::ObjectState> ("layoutSettings");

    for (const auto& item : items) {
        API_DatabaseInfo dbInfo = {};

        const GS::ObjectState* navIdOS = item.Get ("navigatorItemId");
        const GS::ObjectState* dbIdOS  = item.Get ("databaseId");

        if (navIdOS != nullptr) {
            API_Guid navGuid = GetGuidFromObjectState (*navIdOS);
            API_NavigatorItem navItem = {};
            const GSErrCode navErr = ACAPI_Navigator_GetNavigatorItem (&navGuid, &navItem);
            if (navErr != NoError) {
                layoutSettingsList (CreateErrorResponse (navErr, "Failed to get navigator item from navigatorItemId."));
                continue;
            }
            dbInfo = navItem.db;
        } else if (dbIdOS != nullptr) {
            dbInfo = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*dbIdOS));
        } else {
            layoutSettingsList (CreateErrorResponse (APIERR_BADPARS, "Missing databaseId or navigatorItemId."));
            continue;
        }

        API_LayoutInfo layoutInfo = {};
        const GSErrCode err = ACAPI_Navigator_GetLayoutSets (&layoutInfo, &dbInfo.databaseUnId);
        if (err != NoError) {
            layoutSettingsList (CreateErrorResponse (err, "Failed to get layout settings."));
            continue;
        }

        GS::ObjectState layoutResult (
            "layoutName",               GS::UniString (layoutInfo.layoutName),
            "horizontalSize",           layoutInfo.sizeX,
            "verticalSize",             layoutInfo.sizeY,
            "leftMargin",               layoutInfo.leftMargin,
            "topMargin",                layoutInfo.topMargin,
            "rightMargin",              layoutInfo.rightMargin,
            "bottomMargin",             layoutInfo.bottomMargin,
            "customLayoutNumber",       GS::UniString (layoutInfo.customLayoutNumber),
            "customLayoutNumbering",    layoutInfo.customLayoutNumbering,
            "doNotIncludeInNumbering",  layoutInfo.doNotIncludeInNumbering,
            "displayMasterLayoutBelow", layoutInfo.showMasterBelow);

        if (layoutInfo.customData != nullptr && !layoutInfo.customData->IsEmpty ()) {
            const auto& customDataList = layoutResult.AddList<GS::ObjectState> ("customData");
            for (auto& kv : *layoutInfo.customData) {
#ifdef ServerMainVers_2800
                const GS::UniString guidStr  = APIGuidToString (kv.key);
                const GS::UniString& value   = kv.value;
#else
                const GS::UniString guidStr  = APIGuidToString (*kv.key);
                const GS::UniString& value   = *kv.value;
#endif
                GS::UniString name;
                if (schemeByGuid.ContainsKey (guidStr)) {
                    schemeByGuid.Get (guidStr, &name);
                }
                GS::ObjectState entry;
                entry.Add ("customSchemeKey",   guidStr);
                entry.Add ("customSchemeValue", value);
                if (!name.IsEmpty ()) {
                    entry.Add ("customSchemeName", name);
                }
                customDataList (entry);
            }
        }

        delete layoutInfo.customData;
        layoutInfo.customData = nullptr;

        layoutSettingsList (layoutResult);
    }

    return response;
}

SetLayoutSettingsCommand::SetLayoutSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLayoutSettingsCommand::GetName () const
{
    return "SetLayoutSettings";
}

GS::Optional<GS::UniString> SetLayoutSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layoutsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "layoutDatabaseId":         { "$ref": "#/DatabaseId" },
                        "layoutNavigatorItemId":    { "$ref": "#/NavigatorItemId" },
                        "layoutName":               { "type": "string" },
                        "horizontalSize":           { "type": "number" },
                        "verticalSize":             { "type": "number" },
                        "leftMargin":               { "type": "number" },
                        "topMargin":                { "type": "number" },
                        "rightMargin":              { "type": "number" },
                        "bottomMargin":             { "type": "number" },
                        "customLayoutNumber":       { "type": "string" },
                        "customLayoutNumbering":    { "type": "boolean" },
                        "doNotIncludeInNumbering":  { "type": "boolean" },
                        "showMasterBelow":          { "type": "boolean" },
                        "customData": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "customSchemeKey":   { "type": "string" },
                                    "customSchemeName":  { "type": "string" },
                                    "customSchemeValue": { "type": "string" }
                                },
                                "required": ["customSchemeValue"],
                                "additionalProperties": false
                            }
                        }
                    },
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutsData"]
    })";
}

GS::Optional<GS::UniString> SetLayoutSettingsCommand::GetResponseSchema () const
{
    return R"({"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]})";
}

bool IsMasterLayoutDatabase (const API_DatabaseUnId& targetUnId)
{
    API_NavigatorItem filterItem = {};
    filterItem.mapId           = API_LayoutMap;
    filterItem.itemType        = API_MasterLayoutNavItem;
    filterItem.db.databaseUnId = targetUnId;

    GS::Array<API_NavigatorItem> results;
    if (ACAPI_Navigator_SearchNavigatorItem (&filterItem, &results) != NoError) {
        return false;
    }

    for (const auto& result : results) {
        if (result.itemType == API_MasterLayoutNavItem) {
            return true;
        }
    }
    return false;
}

GS::ObjectState SetLayoutSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "layoutsData", items, errorResponse)) {
        return errorResponse;
    }

    GS::Array<GS::ObjectState> executionResults;

    for (const auto& item : items) {
        API_DatabaseInfo dbInfo = {};
        bool isMasterLayout = false;

        const GS::ObjectState* navIdOS = item.Get ("layoutNavigatorItemId");
        const GS::ObjectState* dbIdOS  = item.Get ("layoutDatabaseId");

        if (navIdOS != nullptr) {
            API_Guid navGuid = GetGuidFromObjectState (*navIdOS);
            API_NavigatorItem navItem = {};
            const GSErrCode navErr = ACAPI_Navigator_GetNavigatorItem (&navGuid, &navItem);
            if (navErr != NoError) {
                executionResults.Push (CreateFailedExecutionResult (navErr, "Failed to get navigator item from layoutNavigatorItemId."));
                continue;
            }
            dbInfo = navItem.db;
            isMasterLayout = (navItem.itemType == API_MasterLayoutNavItem);
        } else if (dbIdOS != nullptr) {
            dbInfo = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*dbIdOS));
            isMasterLayout = IsMasterLayoutDatabase (dbInfo.databaseUnId);
        } else {
            executionResults.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing layoutDatabaseId or layoutNavigatorItemId."));
            continue;
        }

        double dummyD = 0.0;
        bool sizeProvided = false;
        sizeProvided |= static_cast<bool> (item.Get ("horizontalSize", dummyD));
        sizeProvided |= static_cast<bool> (item.Get ("verticalSize",   dummyD));

        if (sizeProvided && !isMasterLayout) {
            executionResults.Push (CreateFailedExecutionResult (APIERR_NOTSUPPORTED,
                "Size can only be changed on master layouts."));
            continue;
        }

        API_LayoutInfo layoutInfo = {};
        GSErrCode err = ACAPI_Navigator_GetLayoutSets (&layoutInfo, &dbInfo.databaseUnId);
        if (err != NoError) {
            executionResults.Push (CreateFailedExecutionResult (err, "Failed to read current layout settings."));
            continue;
        }

#ifdef ServerMainVers_2600
        SetUCharProperty (&item, "layoutName", layoutInfo.layoutName);
#else
        SetCharProperty (&item, "layoutName", layoutInfo.layoutName);
#endif
        item.Get ("horizontalSize",          layoutInfo.sizeX);
        item.Get ("verticalSize",            layoutInfo.sizeY);
        item.Get ("leftMargin",              layoutInfo.leftMargin);
        item.Get ("topMargin",               layoutInfo.topMargin);
        item.Get ("rightMargin",             layoutInfo.rightMargin);
        item.Get ("bottomMargin",            layoutInfo.bottomMargin);
        SetCharProperty (&item, "customLayoutNumber", layoutInfo.customLayoutNumber);
        item.Get ("customLayoutNumbering",   layoutInfo.customLayoutNumbering);
        item.Get ("doNotIncludeInNumbering", layoutInfo.doNotIncludeInNumbering);
        bool showMasterBelowRequested = layoutInfo.showMasterBelow;
        const bool showMasterBelowProvided = item.Get ("showMasterBelow", showMasterBelowRequested);
        layoutInfo.showMasterBelow = showMasterBelowRequested;

        GS::Array<GS::ObjectState> customDataItems;
        if (item.Get ("customData", customDataItems)) {
            const auto schemeByGuid = GetLayoutSchemeByGuidString ();

            // Build reverse map: field name → GUID string (for name-based lookup)
            GS::HashTable<GS::UniString, GS::UniString> schemeByName;
            for (const auto& sk : schemeByGuid) {
#ifdef ServerMainVers_2800
                schemeByName.Add (sk.value, sk.key);
#else
                schemeByName.Add (*sk.value, *sk.key);
#endif
            }

            delete layoutInfo.customData;
            layoutInfo.customData = new GS::HashTable<API_Guid, GS::UniString> ();
            for (const auto& cd : customDataItems) {
                GS::UniString keyStr, value;
                if (!cd.Get ("customSchemeKey", keyStr)) {
                    GS::UniString nameStr;
                    if (cd.Get ("customSchemeName", nameStr) && schemeByName.ContainsKey (nameStr)) {
                        schemeByName.Get (nameStr, &keyStr);
                    }
                }
                if (!keyStr.IsEmpty () && cd.Get ("customSchemeValue", value)) {
                    layoutInfo.customData->Put (APIGuidFromString (keyStr.ToCStr ()), value);
                }
            }
        }

        err = ACAPI_Navigator_ChangeLayoutSets (&layoutInfo, &dbInfo.databaseUnId);
        delete layoutInfo.customData;
        layoutInfo.customData = nullptr;

        if (err != NoError) {
            executionResults.Push (CreateFailedExecutionResult (err, "Failed to change layout settings."));
        } else if (showMasterBelowProvided) {
            API_LayoutInfo verifyInfo = {};
            const GSErrCode verifyErr = ACAPI_Navigator_GetLayoutSets (&verifyInfo, &dbInfo.databaseUnId);
            if (verifyErr == NoError && verifyInfo.showMasterBelow != showMasterBelowRequested) {
                executionResults.Push (CreateFailedExecutionResult (APIERR_NOTSUPPORTED,
                    "showMasterBelow cannot be changed for this layout type via the API."));
            } else {
                executionResults.Push (CreateSuccessfulExecutionResult ());
            }
        } else {
            executionResults.Push (CreateSuccessfulExecutionResult ());
        }
    }

    return CreateExecutionResultsResponse (executionResults);
}

GetLayoutCustomSchemeCommand::GetLayoutCustomSchemeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLayoutCustomSchemeCommand::GetName () const
{
    return "GetLayoutCustomScheme";
}

GS::Optional<GS::UniString> GetLayoutCustomSchemeCommand::GetInputParametersSchema () const
{
    return GS::NoValue;
}

GS::Optional<GS::UniString> GetLayoutCustomSchemeCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "customScheme": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "customSchemeKey":  { "type": "string" },
                        "customSchemeName": { "type": "string" }
                    },
                    "required": ["customSchemeKey", "customSchemeName"],
                    "additionalProperties": false
                }
            }
        },
        "required": ["customScheme"],
        "additionalProperties": false
    })";
}

GS::ObjectState GetLayoutCustomSchemeCommand::Execute (const GS::ObjectState&, GS::ProcessControl&) const
{
    GS::ObjectState response;
    const auto& schemeList = response.AddList<GS::ObjectState> ("customScheme");

    API_LayoutBook book = {};
    if (ACAPI_Navigator_GetLayoutBook (&book) == NoError) {
        for (const auto& kv : book.customScheme) {
            schemeList (GS::ObjectState (
#ifdef ServerMainVers_2800
                "customSchemeKey",  APIGuidToString (kv.key),
                "customSchemeName", kv.value));
#else
                "customSchemeKey",  APIGuidToString (*kv.key),
                "customSchemeName", *kv.value));
#endif
        }
    }

    return response;
}
