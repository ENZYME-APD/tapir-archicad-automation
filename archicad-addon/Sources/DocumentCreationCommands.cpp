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

// The id of a new worksheet or detail database, with the navigatorItemId of its Project Map item,
// so that the new item can be opened with ChangeWindow or placed with CreateDrawings without
// reading the whole Project Map (#1602). Without the item only the databaseId is returned.
GS::ObjectState CreateNewWorksheetOrDetailObjectState (const API_DatabaseInfo& dbInfo)
{
    GS::ObjectState result = CreateDatabaseIdObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (dbInfo));
    const GS::Optional<API_Guid> navigatorItemId = FindProjectMapItemOfWorksheetOrDetail (dbInfo);
    if (navigatorItemId.HasValue ()) {
        result.Add ("navigatorItemId", CreateGuidObjectState (navigatorItemId.Get ()));
    }
    return result;
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

// Finds the Layout Book navigator item of a master layout database. Used both to tell master
// layouts from layouts (the master layout of a layout is stored on its database, not in its
// API_LayoutInfo) and to report a layout's master the way the Navigator shows it.
GS::Optional<API_NavigatorItem> FindMasterLayoutNavigatorItem (const API_DatabaseUnId& masterLayoutUnId)
{
    API_NavigatorItem filterItem = {};
    filterItem.mapId           = API_LayoutMap;
    filterItem.itemType        = API_MasterLayoutNavItem;
    filterItem.db.databaseUnId = masterLayoutUnId;

    GS::Array<API_NavigatorItem> results;
    if (ACAPI_Navigator_SearchNavigatorItem (&filterItem, &results) != NoError) {
        return {};
    }

    // The search is already filtered by the database, so the first master layout item it
    // returns is the one asked for (the same way IsMasterLayoutDatabase has always worked).
    for (const auto& result : results) {
        if (result.itemType == API_MasterLayoutNavItem) {
            return result;
        }
    }
    return {};
}

// Adds the master layout of a layout database (masterLayoutDatabaseId, masterNavigatorItemId,
// masterLayoutName) to a GetLayoutSettings result. Master layouts have no master of their own,
// so nothing is added for them.
void AddMasterLayoutOfLayout (API_DatabaseInfo layoutDbInfo, GS::ObjectState& layoutResult)
{
    if (ACAPI_Window_GetDatabaseInfo (&layoutDbInfo) != NoError ||
        layoutDbInfo.typeID != APIWind_LayoutID ||
        layoutDbInfo.masterLayoutUnId.elemSetId == APINULLGuid) {
        return;
    }

    API_DatabaseInfo masterDbInfo = {};
    masterDbInfo.typeID       = APIWind_MasterLayoutID;
    masterDbInfo.databaseUnId = layoutDbInfo.masterLayoutUnId;
    layoutResult.Add ("masterLayoutDatabaseId", CreateGuidObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (masterDbInfo)));

    const auto masterNavItem = FindMasterLayoutNavigatorItem (layoutDbInfo.masterLayoutUnId);
    if (masterNavItem.HasValue ()) {
        layoutResult.Add ("masterNavigatorItemId", CreateGuidObjectState (masterNavItem.Get ().guid));
    }

    if (ACAPI_Window_GetDatabaseInfo (&masterDbInfo) == NoError) {
        layoutResult.Add ("masterLayoutName", GS::UniString (masterDbInfo.name));
    }
}

// Resolves the master layout a SetLayoutSettings item asks for, given as masterNavigatorItemId,
// masterLayoutDatabaseId or masterLayoutName (in this order of precedence). Returns false, with
// the failed execution result filled, when one is given but does not name a master layout.
bool ResolveRequestedMasterLayout (const GS::ObjectState& item, GS::Optional<API_DatabaseUnId>& masterLayoutUnId, GS::ObjectState& failedResult)
{
    const GS::ObjectState* masterNavItemOS = item.Get ("masterNavigatorItemId");
    const GS::ObjectState* masterDbIdOS    = item.Get ("masterLayoutDatabaseId");
    GS::UniString masterLayoutName;

    if (masterNavItemOS != nullptr) {
        API_Guid masterNavGuid = GetGuidFromObjectState (*masterNavItemOS);
        API_NavigatorItem masterNavItem = {};
        const GSErrCode navErr = ACAPI_Navigator_GetNavigatorItem (&masterNavGuid, &masterNavItem);
        if (navErr != NoError) {
            failedResult = CreateFailedExecutionResult (navErr, "Failed to get navigator item from masterNavigatorItemId.");
            return false;
        }
        if (masterNavItem.itemType != API_MasterLayoutNavItem) {
            failedResult = CreateFailedExecutionResult (APIERR_BADID, "masterNavigatorItemId is not a master layout.");
            return false;
        }
        masterLayoutUnId = masterNavItem.db.databaseUnId;
    } else if (masterDbIdOS != nullptr) {
        const API_DatabaseInfo masterDbInfo = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*masterDbIdOS));
        if (!FindMasterLayoutNavigatorItem (masterDbInfo.databaseUnId).HasValue ()) {
            failedResult = CreateFailedExecutionResult (APIERR_BADID, "masterLayoutDatabaseId is not a master layout.");
            return false;
        }
        masterLayoutUnId = masterDbInfo.databaseUnId;
    } else if (item.Get ("masterLayoutName", masterLayoutName)) {
        const auto masterDbInfo = FindMasterLayoutDatabaseByName (masterLayoutName);
        if (!masterDbInfo.HasValue ()) {
            failedResult = CreateFailedExecutionResult (APIERR_BADID, "No master layout found with the given masterLayoutName.");
            return false;
        }
        masterLayoutUnId = masterDbInfo.Get ().databaseUnId;
    }

    return true;
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

void CollectLayoutSubsetGuids (API_NavigatorItem item, GS::Array<API_Guid>& subsetGuids)
{
    item.mapId = API_LayoutMap;
    GS::Array<API_NavigatorItem> children;
    if (ACAPI_Navigator_GetNavigatorChildrenItems (&item, &children) != NoError) {
        return;
    }
    for (const API_NavigatorItem& child : children) {
        if (child.itemType == API_SubSetNavItem) {
            subsetGuids.Push (child.guid);
            CollectLayoutSubsetGuids (child, subsetGuids);
        }
    }
}

GS::Array<API_Guid> GetLayoutSubsetGuids ()
{
    GS::Array<API_Guid> subsetGuids;
    API_NavigatorSet navSet = {};
    navSet.mapId = API_LayoutMap;
    Int32 idx = 0;
    if (ACAPI_Navigator_GetNavigatorSet (&navSet, &idx) != NoError) {
        return subsetGuids;
    }
    API_NavigatorItem rootItem = {};
    if (ACAPI_Navigator_GetNavigatorItem (&navSet.rootGuid, &rootItem) != NoError) {
        return subsetGuids;
    }
    CollectLayoutSubsetGuids (rootItem, subsetGuids);
    return subsetGuids;
}

GS::Optional<API_Guid> FindNewLayoutSubsetGuid (const GS::Array<API_Guid>& before)
{
    const GS::Array<API_Guid> after = GetLayoutSubsetGuids ();
    for (const auto& guid : after) {
        if (!before.Contains (guid)) {
            return guid;
        }
    }
    return {};
}

GS::Optional<API_Guid> GetLayoutBookRootGuid ()
{
    API_NavigatorSet navSet = {};
    navSet.mapId = API_LayoutMap;
    Int32 idx = 0;
    if (ACAPI_Navigator_GetNavigatorSet (&navSet, &idx) != NoError) {
        return {};
    }
    return navSet.rootGuid;
}

GS::Optional<API_Guid> FindLayoutNavigatorItemGuid (const API_Guid& layoutDatabaseId)
{
    API_NavigatorItem filterItem = {};
    filterItem.mapId           = API_LayoutMap;
    filterItem.itemType        = API_LayoutNavItem;
    filterItem.db.databaseUnId = DatabaseIdResolver::Instance ().GetDatabaseWithId (layoutDatabaseId).databaseUnId;

    GS::Array<API_NavigatorItem> results;
    if (ACAPI_Navigator_SearchNavigatorItem (&filterItem, &results) != NoError) {
        return {};
    }
    for (const auto& result : results) {
        if (result.itemType == API_LayoutNavItem) {
            return result.guid;
        }
    }
    return {};
}

// The last child of the given Layout Book item, not counting the item to be
// placed and the Masters folder, which Archicad keeps last under the Layout
// Book root; empty when there is no other child
GS::Optional<API_Guid> FindLastLayoutBookChild (const API_Guid& parentGuid, const API_Guid& itemToPlace)
{
    API_NavigatorItem parentItem = {};
    if (ACAPI_Navigator_GetNavigatorItem (&parentGuid, &parentItem) != NoError) {
        return {};
    }
    parentItem.mapId = API_LayoutMap;
    GS::Array<API_NavigatorItem> children;
    if (ACAPI_Navigator_GetNavigatorChildrenItems (&parentItem, &children) != NoError) {
        return {};
    }
    for (UIndex i = children.GetSize (); i > 0; --i) {
        const API_NavigatorItem& child = children[i - 1];
        if (child.guid != itemToPlace && child.itemType != API_MasterFolderNavItem) {
            return child.guid;
        }
    }
    return {};
}

// Moves the freshly created layout where the caller asked for it: after
// previousNavigatorItemId, or to the first or last place under its parent
// (the Layout Book root when no parentNavigatorItemId was given). Archicad
// itself inserts new layouts at the first place, which renumbers every sheet
// after them, so most scripts want "last".
GSErrCode PlaceNewLayout (const API_Guid& layoutDatabaseId, const API_Guid& parentNavGuid,
                          const GS::ObjectState* previousOS, const GS::UniString& position, GS::UniString& errorMessage)
{
    const auto layoutNavGuid = FindLayoutNavigatorItemGuid (layoutDatabaseId);
    if (!layoutNavGuid.HasValue ()) {
        errorMessage = "Layout created but could not resolve its navigator item to position it.";
        return APIERR_GENERAL;
    }

    API_Guid parentGuid = parentNavGuid;
    if (parentGuid == APINULLGuid) {
        const auto rootGuid = GetLayoutBookRootGuid ();
        if (!rootGuid.HasValue ()) {
            errorMessage = "Layout created but could not resolve the Layout Book root to position it.";
            return APIERR_GENERAL;
        }
        parentGuid = rootGuid.Get ();
    }

    GS::Optional<API_Guid> previousGuid;
    if (previousOS != nullptr) {
        previousGuid = GetGuidFromObjectState (*previousOS);
    } else if (position == "last") {
        previousGuid = FindLastLayoutBookChild (parentGuid, layoutNavGuid.Get ());
    }

    const GS::Guid sourceGuid = APIGuid2GSGuid (layoutNavGuid.Get ());
    const GS::Guid parentGSGuid = APIGuid2GSGuid (parentGuid);
    GS::Guid  prevGuidStorage;
    GS::Guid* prevGuidPtr = nullptr;
    if (previousGuid.HasValue ()) {
        prevGuidStorage = APIGuid2GSGuid (previousGuid.Get ());
        prevGuidPtr = &prevGuidStorage;
    }

    GSErrCode err = NoError;
    ACAPI_CallUndoableCommand ("CreateLayoutCommand", [&]() -> GSErrCode {
        err = ACAPI_Navigator_SetNavigatorItemPosition (&sourceGuid, &parentGSGuid, prevGuidPtr);
        return err;
    });
    if (err != NoError) {
        errorMessage = "Layout created at Archicad's default position but could not be moved to the requested position.";
    }
    return err;
}

// Sets the paper size (in mm) of a master layout. Its margins are kept.
GSErrCode SetMasterLayoutSize (const API_DatabaseUnId& databaseUnId, double widthMm, double heightMm)
{
    API_LayoutInfo info = {};
    GSErrCode err = ACAPI_Navigator_GetLayoutSets (&info, const_cast<API_DatabaseUnId*> (&databaseUnId));
    if (err != NoError) {
        return err;
    }
    info.sizeX = widthMm;
    info.sizeY = heightMm;
    err = ACAPI_Navigator_ChangeLayoutSets (&info, const_cast<API_DatabaseUnId*> (&databaseUnId));
    delete info.customData;
    info.customData = nullptr;
    return err;
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

GS::Optional<GS::UniString> CreateDetailsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "databases": {
                "type": "array",
                "description": "One item per input item, in the same order.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "databaseId": { "$ref": "#/DatabaseId" },
                                "navigatorItemId": {
                                    "$ref": "#/NavigatorItemId",
                                    "description": "The Project Map item of the new database, for ChangeWindow or CreateDrawings. Missing when the item could not be found."
                                }
                            },
                            "additionalProperties": false,
                            "required": ["databaseId"]
                        },
                        { "$ref": "#/ErrorItem" }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["databases"]
    })";
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

        databases.Push (CreateNewWorksheetOrDetailObjectState (dbInfo));
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

GS::Optional<GS::UniString> CreateWorksheetsCommand::GetRawResponseSchema () const
{
    return CreateDetailsCommand ().GetRawResponseSchema ();
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

        databases.Push (CreateNewWorksheetOrDetailObjectState (dbInfo));
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
                        "previousNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "The sibling after which the new layout is placed, as in MoveNavigatorItem. It must be a child of parentNavigatorItemId (or of the Layout Book root when that is omitted). Cannot be combined with position."
                        },
                        "position": {
                            "type": "string",
                            "enum": ["first", "last"],
                            "description": "Where to place the new layout under its parent. Archicad inserts new layouts at the first place, which renumbers the sheets after them; \"last\" appends it after the existing children instead. Cannot be combined with previousNavigatorItemId."
                        },
                        "paperWidth":            { "type": "number", "description": "Optional paper width in mm, used together with paperHeight. Sets the size of the new layout, and of the master layout when this command creates it from masterLayoutName: a new master starts with the template's default size, which layoutParameters cannot change. An existing master keeps its size. Wins over layoutParameters.horizontalSize." },
                        "paperHeight":           { "type": "number", "description": "Optional paper height in mm, used together with paperWidth. See paperWidth. Wins over layoutParameters.verticalSize." },
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

GS::Optional<GS::UniString> CreateLayoutCommand::GetRawResponseSchema () const
{
    return R"({"type":"object","properties":{"databases":{"$ref":"#/DatabaseIdsOrErrors"}},"additionalProperties":false,"required":["databases"]})";
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

                // A master created here starts with the template's default size (A1 in the
                // Japanese template); with paperWidth/paperHeight it gets that size instead. An
                // existing master keeps its size, as the layouts already on it depend on it.
                double paperWidth = 0.0, paperHeight = 0.0;
                if (item.Get ("paperWidth", paperWidth) && item.Get ("paperHeight", paperHeight) &&
                        paperWidth > 0.0 && paperHeight > 0.0) {
                    // Reported per item, as the layout would otherwise get a size its new master does not have.
                    const GSErrCode sizeErr = SetMasterLayoutSize (masterLayoutDbInfo.databaseUnId, paperWidth, paperHeight);
                    if (sizeErr != NoError) {
                        databases.Push (CreateErrorResponse (sizeErr, "The master layout was created, but its paper size could not be set; no layout was created."));
                        continue;
                    }
                }

                // Best-effort flush of the navigator/database state just created above: a new
                // database followed soon after by a RenameNavigatorItem + DeleteNavigatorItems
                // has been seen to crash ArchiCAD internally (issue #1428). Error ignored.
                // ACAPI_Database_RedrawCurrentDatabase only exists from AC27 on.
#ifdef ServerMainVers_2700
                ACAPI_Database_RedrawCurrentDatabase ();
#endif
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

        // paperWidth/paperHeight are also set on the new layout directly. Reading the size back
        // from a master that was just created or resized in the same call can still return the
        // old size (measured: the A1 default), so it is not taken from the master here.
        double reqWidth = 0.0, reqHeight = 0.0;
        if (item.Get ("paperWidth", reqWidth) && item.Get ("paperHeight", reqHeight) &&
                reqWidth > 0.0 && reqHeight > 0.0) {
            layoutInfo.sizeX = reqWidth;
            layoutInfo.sizeY = reqHeight;
        }

        API_Guid parentNavGuid = APINULLGuid;
        const GS::ObjectState* parentOS = item.Get ("parentNavigatorItemId");
        if (parentOS != nullptr) {
            parentNavGuid = GetGuidFromObjectState (*parentOS);
        }

        const GS::ObjectState* previousOS = item.Get ("previousNavigatorItemId");
        GS::UniString position;
        item.Get ("position", position);
        if (previousOS != nullptr && !position.IsEmpty ()) {
            databases.Push (CreateErrorResponse (APIERR_BADPARS, "previousNavigatorItemId and position cannot be given together."));
            continue;
        }
        const bool placeLayout = previousOS != nullptr || !position.IsEmpty ();

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

        auto newLayoutGuid = FindNewLayoutDatabaseGuid (before);
        if (!newLayoutGuid.HasValue ()) {
            newLayoutGuid = FindLayoutDatabaseGuidByName (GS::UniString (layoutInfo.layoutName));
        }
        if (!newLayoutGuid.HasValue ()) {
            databases.Push (CreateErrorResponse (APIERR_GENERAL, "Layout created but could not resolve its database id."));
            continue;
        }

        if (placeLayout) {
            GS::UniString placeErrorMessage;
            const GSErrCode placeErr = PlaceNewLayout (newLayoutGuid.Get (), parentNavGuid, previousOS, position, placeErrorMessage);
            if (placeErr != NoError) {
                databases.Push (CreateErrorResponse (placeErr, placeErrorMessage));
                continue;
            }
        }

        databases.Push (CreateDatabaseIdObjectState (newLayoutGuid.Get ()));
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

GS::Optional<GS::UniString> CreateLayoutSubsetCommand::GetRawResponseSchema () const
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

        // includeToIDSequence is the positive form of the Subset Settings dialog's
        // "Do not include this Subset in ID sequence" checkbox, which API_SubSet
        // stores negated
        bool includeToIDSequence = false;
        if (item.Get ("includeToIDSequence", includeToIDSequence)) {
            subSet.doNotInclude = !includeToIDSequence;
        }

        const GS::ObjectState* parent = item.Get ("parentNavigatorItemId");
        const API_Guid* parentGuidPtr = nullptr;
        API_Guid parentGuid = APINULLGuid;
        if (parent != nullptr) {
            parentGuid = GetGuidFromObjectState (*parent);
            parentGuidPtr = &parentGuid;
        }

        // ACAPI_Navigator_CreateSubSet has no output guid, so the created subset's
        // navigator item is found the same way CreateLayout finds its new database:
        // by diffing the Layout Book's subset items around the creation
        const GS::Array<API_Guid> before = GetLayoutSubsetGuids ();
        err = ACAPI_Navigator_CreateSubSet (&subSet, parentGuidPtr);
        if (err != NoError) {
            navigatorItems.Push (CreateErrorResponse (err, "Failed to create subset."));
            continue;
        }

        const auto createdGuid = FindNewLayoutSubsetGuid (before);
        if (createdGuid.HasValue ()) {
            navigatorItems.Push (CreateIdObjectState ("navigatorItemId", createdGuid.Get ()));
        } else {
            navigatorItems.Push (CreateErrorResponse (APIERR_GENERAL, "Subset created but could not resolve its navigator item id."));
        }
    }
    return CreateNavigatorItemsResponse (navigatorItems);
}

// The inverse of the numberingStyle mapping in CreateLayoutSubsetCommand::Execute
static GS::UniString SubSetNumberingStyleToString (const API_SubSet& subSet)
{
    switch (subSet.numberingStyle) {
        case API_NS_abc:  return "abc";
        case API_NS_ABC:  return "ABC";
        case API_NS_1:    return "1";
        case API_NS_01:   return "01";
        case API_NS_001:  return "001";
        case API_NS_0001: return "0001";
        case API_NS_noID: return "noID";
        default:          return "Undefined";
    }
}

GetLayoutSubsetSettingsCommand::GetLayoutSubsetSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLayoutSubsetSettingsCommand::GetName () const
{
    return "GetLayoutSubsetSettings";
}

GS::Optional<GS::UniString> GetLayoutSubsetSettingsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetLayoutSubsetSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "subsetSettings": {
                "type": "array",
                "description": "One item per input navigator item, in the same order: the subset's settings with the field names of CreateLayoutSubset, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "name":                { "type": "string" },
                                "ownPrefix":           { "type": "string" },
                                "customNumber":        { "type": "string" },
                                "numberingStyle":      { "type": "string", "enum": ["Undefined", "abc", "ABC", "1", "01", "001", "0001", "noID"] },
                                "startAt":             { "type": "integer" },
                                "continueNumbering":   { "type": "boolean" },
                                "useUpperPrefix":      { "type": "boolean" },
                                "includeToIDSequence": { "type": "boolean" },
                                "customNumbering":     { "type": "boolean" },
                                "addOwnPrefix":        { "type": "boolean" }
                            },
                            "additionalProperties": false,
                            "required": ["name", "ownPrefix", "customNumber", "numberingStyle", "startAt", "continueNumbering", "useUpperPrefix", "includeToIDSequence", "customNumbering", "addOwnPrefix"]
                        },
                        { "$ref": "#/ErrorItem" }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["subsetSettings"]
    })";
}

GS::ObjectState GetLayoutSubsetSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> navigatorItemIds;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "navigatorItemIds", navigatorItemIds, errorResponse)) {
        return errorResponse;
    }

    GS::ObjectState response;
    const auto& subsetSettings = response.AddList<GS::ObjectState> ("subsetSettings");

    for (const auto& navigatorItemId : navigatorItemIds) {
        const API_Guid guid = GetGuidFromNavigatorItemIdArrayItem (navigatorItemId);
        if (guid == APINULLGuid) {
            subsetSettings (CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing."));
            continue;
        }

        // Checked first so that any other navigator item gets a clear error
        // instead of whatever ACAPI_Navigator_GetSubSet returns for it
        API_NavigatorItem navItem = {};
        GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&guid, &navItem);
        if (err != NoError) {
            subsetSettings (CreateErrorResponse (err, "Failed to get navigator item."));
            continue;
        }
        if (navItem.itemType != API_SubSetNavItem) {
            subsetSettings (CreateErrorResponse (APIERR_BADID, "The navigator item is not a Layout Book subset."));
            continue;
        }

        API_SubSet subSet = {};
        err = ACAPI_Navigator_GetSubSet (&guid, &subSet);
        if (err != NoError) {
            subsetSettings (CreateErrorResponse (err, "Failed to get subset settings."));
            continue;
        }

        subsetSettings (GS::ObjectState (
            "name",                GS::UniString (subSet.name),
            "ownPrefix",           GS::UniString (subSet.ownPrefix),
            "customNumber",        GS::UniString (subSet.customNumber),
            "numberingStyle",      SubSetNumberingStyleToString (subSet),
            "startAt",             subSet.startAt,
            "continueNumbering",   subSet.continueNumbering,
            "useUpperPrefix",      subSet.useUpperPrefix,
            // API_SubSet stores the dialog's "Do not include this Subset in ID sequence" negated
            "includeToIDSequence", !subSet.doNotInclude,
            "customNumbering",     subSet.customNumbering,
            "addOwnPrefix",        subSet.addOwnPrefix));
    }

    return response;
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
                        "name": {
                            "type": "string",
                            "minLength": 1,
                            "description": "Custom title name of the new Drawing. Giving a name implies nameType CustomName unless nameType is set explicitly."
                        },
                        "nameType": {
                            "$ref": "#/DrawingNameType",
                            "description": "Defaults to CustomName when name is given, otherwise to the Drawing tool's current default."
                        },
                        "position": { "$ref": "#/Coordinate2D" },
                        "scale": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Scale ratio applied to the drawing relative to its source view (API_DrawingType::ratio). Defaults to 1.0."
                        },
                        "angle": {
                            "type": "number",
                            "description": "Rotation angle of the drawing in radians. Defaults to the Drawing tool's current default."
                        },
                        "drawingScale": {
                            "type": "number",
                            "description": "The nominal scale of the drawing. Defaults to the Drawing tool's current default."
                        },
                        "modelOffset": {
                            "$ref": "#/Coordinate2D",
                            "description": "Offset of the model origin within the drawing. Defaults to the Drawing tool's current default."
                        },
                        "clipPolygon": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3,
                            "description": "Optional polygon to clip the drawing with, in layout coordinates (meters on the layout, the same coordinate system as position) - not the coordinates of the source view's model. Stored as given: position, angle, scale and modelOffset are not applied to it."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "position"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["drawingsData"]
    })";
}

GS::Optional<GS::UniString> CreateDrawingsCommand::GetRawResponseSchema () const
{
    return R"({"type":"object","properties":{"elements":{"$ref":"#/ElementIdsOrErrors"}},"additionalProperties":false,"required":["elements"]})";
}

// Tells whether a navigator item can be the source of a Drawing. Only viewpoints and the views
// saved from them can be placed - containers (folders, subsets, books, the project root), the
// Layout Book's own items (layouts, master layouts) and the navigator item of an already placed
// Drawing cannot. Views carry the item type of the viewpoint they were saved from, so the same
// check covers both the Project Map and the View Map.
static bool IsPlaceableAsDrawing (API_NavigatorItemTypeID itemType)
{
    switch (itemType) {
        case API_StoryNavItem:
        case API_SectionNavItem:
        case API_ElevationNavItem:
        case API_InteriorElevationNavItem:
        case API_DetailDrawingNavItem:
        case API_WorksheetDrawingNavItem:
        case API_DocumentFrom3DNavItem:
        case API_PerspectiveNavItem:
        case API_AxonometryNavItem:
        case API_ScheduleNavItem:
        case API_ListNavItem:
        case API_TextListNavItem:
        case API_TocNavItem:
            return true;
        default:
            return false;
    }
}

// Creates a single Drawing from a "drawingsData"-shaped item (navigatorItemId, position, optional
// name/nameType/scale/angle/drawingScale/modelOffset/clipPolygon). Shared by CreateDrawingsCommand
// and ChangeDrawingLinkCommand, which synthesizes the same item shape from an existing Drawing's
// own current appearance.
static GS::ObjectState CreateOneDrawing (const GS::ObjectState& item)
{
    // The source has to be resolved and checked here, before anything is handed to
    // ACAPI_Element_Create: a drawingGuid that is not a placeable viewpoint - one that resolves
    // to nothing, or to a folder, a layout, or an already placed Drawing's own navigator item -
    // terminates Archicad inside element creation instead of returning an error.
    const GS::ObjectState* navigatorItemIdState = item.Get ("navigatorItemId");
    if (navigatorItemIdState == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing required field 'navigatorItemId'.");
    }

    API_Guid sourceGuid = GetGuidFromObjectState (*navigatorItemIdState);
    if (sourceGuid == APINULLGuid) {
        return CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is corrupt or missing.");
    }

    API_NavigatorItem sourceItem = {};
    const GSErrCode navErr = ACAPI_Navigator_GetNavigatorItem (&sourceGuid, &sourceItem);
    if (navErr != NoError) {
        return CreateErrorResponse (navErr, "Failed to get navigator item from navigatorItemId.");
    }
    if (!IsPlaceableAsDrawing (sourceItem.itemType)) {
        return CreateErrorResponse (APIERR_BADID, "navigatorItemId is not a view or viewpoint that can be placed as a Drawing.");
    }

    API_Element element = {};
#ifdef ServerMainVers_2600
    element.header.type   = API_DrawingID;
#else
    element.header.typeID = API_DrawingID;
#endif
    GSErrCode err = ACAPI_Element_GetDefaults (&element, nullptr);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get drawing defaults.");
    }

    element.drawing.drawingGuid = sourceGuid;
    // An explicit nameType wins; a name alone means CustomName (the historic behavior of this
    // command, when name was required); with neither, the Drawing tool's default is kept.
    const bool hasName = SetCharProperty (&item, "name", element.drawing.name);
    GS::UniString nameTypeStr;
    if (item.Get ("nameType", nameTypeStr)) {
        element.drawing.nameType = DrawingNameTypeFromString (nameTypeStr, element.drawing.nameType);
    } else if (hasName) {
        element.drawing.nameType = APIName_CustomName;
    }
    element.drawing.anchorPoint = APIAnc_MM;
    element.drawing.pos = Get2DCoordinateFromObjectState (*item.Get ("position"));
    if (!item.Get ("scale", element.drawing.ratio)) {
        element.drawing.ratio = 1.0;
    }
    // Optional; these must be set at creation time rather than via a follow-up
    // ACAPI_Element_Change (changing an element immediately after creating it, within the same
    // undoable command, is unreliable) - which is also why ChangeDrawingLink passes them here.
    item.Get ("angle", element.drawing.angle);
    item.Get ("drawingScale", element.drawing.drawingScale);
    const GS::ObjectState* modelOffsetState = item.Get ("modelOffset");
    if (modelOffsetState != nullptr) {
        element.drawing.modelOffset = Get2DCoordinateFromObjectState (*modelOffsetState);
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
    if (nClip < 3) {
        // No clip polygon requested. ACAPI_Element_GetDefaults filled isCutWithFrame from the
        // Drawing tool defaults, which carry over the crop of the last manually placed Drawing -
        // clear it explicitly so the new Drawing always shows its full, unclipped extent (#651).
        element.drawing.isCutWithFrame = false;
        element.drawing.poly.nSubPolys = 0;
        element.drawing.poly.nCoords   = 0;
        element.drawing.poly.nArcs     = 0;
    } else {
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
        return CreateErrorResponse (err, "Failed to create drawing.");
    }
    return CreateElementIdObjectState (element.header.guid);
}

// Switches to the layout database identified by layoutDatabaseId (the same database-activation
// dance CreateDrawingsCommand and ChangeDrawingLinkCommand both need before creating a Drawing).
static GSErrCode ActivateLayoutDatabase (const API_Guid& layoutDatabaseGuid)
{
    API_DatabaseInfo targetDatabase = DatabaseIdResolver::Instance ().GetDatabaseWithId (layoutDatabaseGuid);
    GSErrCode err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
    if (err != NoError) {
        return err;
    }
    err = ACAPI_Database_ChangeCurrentDatabase (&targetDatabase);
    if (err != NoError) {
        return err;
    }
    API_WindowInfo windowInfo = targetDatabase;
    return ACAPI_Window_ChangeWindow (&windowInfo);
}

// The active window, captured so that a command which has to switch windows can switch back
// afterwards. The active story is kept too: it is a project-wide setting, not a property of the
// front window, so a view switch that lands on a story view changes it whatever window was in
// front (the Floor Plan is one window for every story).
struct ActiveWindowState {
    bool            valid = false;
    API_WindowInfo  window = {};
    bool            hasStory = false;
    short           actStory = 0;
};

static bool GetActiveStory (short& actStory)
{
    API_StoryInfo storyInfo = {};
    const GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
    if (err != NoError) {
        return false;
    }
    actStory = storyInfo.actStory;
    return true;
}

static ActiveWindowState GetActiveWindowState ()
{
    ActiveWindowState state;
    state.valid = ACAPI_Window_GetCurrentWindow (&state.window) == NoError;
    state.hasStory = GetActiveStory (state.actStory);
    return state;
}

static void RestoreActiveWindow (const ActiveWindowState& state)
{
    if (state.valid) {
        API_WindowInfo windowInfo = state.window;
        ACAPI_Window_ChangeWindow (&windowInfo);
    }
    // Putting the story into API_WindowInfo::index does not move the active story (#748):
    // the story is changed with the story settings command instead, the same way
    // ChangeWindowCommand and CreateZonesCommand do it, and only when the switches left
    // another story active. This does not depend on the window switch: the story is
    // project-wide, so it is restored even when the window could not be.
    short currentStory = 0;
    if (state.hasStory && GetActiveStory (currentStory) && currentStory != state.actStory) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_GoTo;
        storyCmd.index  = state.actStory;
        ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
    }
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
                    const GSErrCode err = ActivateLayoutDatabase (GetGuidFromObjectState (*layoutDatabaseId));
                    if (err != NoError) {
                        elementResults.Push (CreateErrorResponse (err, "Failed to activate the layout database."));
                        continue;
                    }
                }
            }

            elementResults.Push (CreateOneDrawing (item));
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

// ============================================================================
// ChangeDrawingLink — simulates relinking a Drawing to a different source.
//
// Archicad has no API to change a Drawing's source link (API_DrawingType::drawingGuid)
// in place: confirmed live that ACAPI_Element_Change silently ignores any change to that
// field once the Drawing already exists, even though other fields (pos, angle, ...) change
// correctly through the same call. The only way to relink is what Graphisoft's own support
// engineers recommend on the forums for this exact problem: capture the old Drawing's
// appearance, create a brand new Drawing against the new source, reapply the appearance,
// then delete the old one. The new Drawing necessarily gets a new guid - any external
// references to the old guid (dimensions, markers, IDs) will need updating separately.
//
// All of the old Drawing's appearance (pos, angle, ratio, drawingScale, modelOffset,
// nameType, clipPolygon) is set on the new element BEFORE creation (via CreateOneDrawing), not through
// a follow-up ACAPI_Element_Change - calling Change on an element immediately after creating
// it, within the same undoable command, crashed Archicad's command layer in testing.
//
// Known gap: the Drawing Title marker's own position (a "Neig" sub-element,
// APINeig_DrawingTitle) is not carried over. Neither the ACAPI documentation nor
// Graphisoft's own support responses address this - it appears to be an unresolved,
// undocumented corner of the API rather than something this command works around.
// ============================================================================

ChangeDrawingLinkCommand::ChangeDrawingLinkCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeDrawingLinkCommand::GetName () const
{
    return "ChangeDrawingLink";
}

GS::Optional<GS::UniString> ChangeDrawingLinkCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "drawingsWithNewLinks": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "An existing Drawing and the navigator item it should be relinked to.",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "navigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutDatabaseId": { "$ref": "#/DatabaseId" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId", "navigatorItemId", "layoutDatabaseId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["drawingsWithNewLinks"]
    })";
}

GS::Optional<GS::UniString> ChangeDrawingLinkCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "One result per input item. On success, elementId is the NEW Drawing's identifier - relinking necessarily replaces the element, it cannot keep the original guid.",
                "items": {
                    "$ref": "#/ElementIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState ChangeDrawingLinkCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    GS::ObjectState errorResponse;
    if (!GetItems (parameters, "drawingsWithNewLinks", items, errorResponse)) {
        return errorResponse;
    }

    API_DatabaseInfo startingDatabase = {};
    const GSErrCode startingDatabaseErr = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
    const ActiveWindowState startingWindow = GetActiveWindowState ();
    bool windowSwitched = false;

    GS::Array<GS::ObjectState> results;
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("ChangeDrawingLinkCommand", [&]() -> GSErrCode {
        for (const auto& item : items) {
            const GS::ObjectState* elementIdState        = item.Get ("elementId");
            const GS::ObjectState* navigatorItemIdState  = item.Get ("navigatorItemId");
            const GS::ObjectState* layoutDatabaseIdState = item.Get ("layoutDatabaseId");
            if (elementIdState == nullptr || navigatorItemIdState == nullptr || layoutDatabaseIdState == nullptr) {
                results.Push (CreateErrorResponse (APIERR_BADPARS, "elementId, navigatorItemId and layoutDatabaseId are all required."));
                continue;
            }

            const API_Guid oldGuid = GetGuidFromObjectState (*elementIdState);

            API_Element oldElement = {};
            oldElement.header.guid = oldGuid;
            GSErrCode err = ACAPI_Element_Get (&oldElement);
            if (err != NoError) {
                results.Push (CreateErrorResponse (err, "Failed to find the Drawing to relink."));
                continue;
            }
#ifdef ServerMainVers_2600
            if (oldElement.header.type != API_DrawingID) {
#else
            if (oldElement.header.typeID != API_DrawingID) {
#endif
                results.Push (CreateErrorResponse (APIERR_BADID, "The given element is not a Drawing."));
                continue;
            }

            // Capture the old Drawing's clip polygon (if any) before it's deleted.
            GS::Array<GS::ObjectState> oldClipCoords;
            if (oldElement.drawing.isCutWithFrame) {
                GS::ObjectState oldClip;
                AddPolygonFromMemoCoords (oldGuid, oldClip, "clipPolygon");
                oldClip.Get ("clipPolygon", oldClipCoords);
            }

#if defined (ServerMainVers_2700)
            // Make the new source view current before creating the Drawing from it. A Drawing
            // created while its source view has never been made current inherits a stale 2D
            // transform instead of the one saved via SetViewSettings - ACAPI_View_GoToView
            // resyncs it, the same way ChangeWindowCommand does. The view is made current
            // even when its window is already the active one: the stale transform is the
            // view's, not the window's (two views of the same story with different settings
            // is the case of #645), and GoToView switches no window then, it only applies
            // the view's settings to the window in front. The window and story that were
            // active when the command started are restored at the end. Best effort: GoToView
            // only works on views, and some placeable sources (schedules, lists, viewpoints
            // from the project map) may reject it - that must not fail the whole relink.
            {
                const API_Guid newSourceGuid = GetGuidFromObjectState (*navigatorItemIdState);
                if (newSourceGuid != APINULLGuid) {
                    const GS::UniString newSourceGuidStr = APIGuidToString (newSourceGuid);
                    if (ACAPI_View_GoToView (newSourceGuidStr.ToCStr ().Get ()) == NoError) {
                        windowSwitched = true;
                    }
                }
            }
#endif

            if (startingDatabaseErr == NoError) {
                // ActivateLayoutDatabase also changes the active window to the layout.
                windowSwitched = true;
                err = ActivateLayoutDatabase (GetGuidFromObjectState (*layoutDatabaseIdState));
                if (err != NoError) {
                    results.Push (CreateErrorResponse (err, "Failed to activate the layout database."));
                    continue;
                }
            }

            GS::ObjectState newDrawingItem;
            newDrawingItem.Add ("navigatorItemId", *navigatorItemIdState);
            newDrawingItem.Add ("name", GS::UniString (oldElement.drawing.name));
            newDrawingItem.Add ("nameType", DrawingNameTypeToString (oldElement.drawing.nameType));
            newDrawingItem.Add ("position", Create2DCoordinateObjectState (oldElement.drawing.pos));
            newDrawingItem.Add ("scale", oldElement.drawing.ratio);
            newDrawingItem.Add ("angle", oldElement.drawing.angle);
            newDrawingItem.Add ("drawingScale", oldElement.drawing.drawingScale);
            newDrawingItem.Add ("modelOffset", Create2DCoordinateObjectState (oldElement.drawing.modelOffset));
            if (!oldClipCoords.IsEmpty ()) {
                const auto& clipList = newDrawingItem.AddList<GS::ObjectState> ("clipPolygon");
                for (const auto& coord : oldClipCoords) {
                    clipList (coord);
                }
            }

            const GS::ObjectState createResult = CreateOneDrawing (newDrawingItem);
            const GS::ObjectState* newElementIdState = createResult.Get ("elementId");
            if (newElementIdState == nullptr) {
                results.Push (createResult);
                continue;
            }
            const API_Guid newGuid = GetGuidFromObjectState (*newElementIdState);

            GS::Array<API_Guid> toDelete;
            toDelete.Push (oldGuid);
            err = ACAPI_Element_Delete (toDelete);
            if (err != NoError) {
                results.Push (CreateErrorResponse (err, "The relinked Drawing was created, but the old one could not be deleted."));
                continue;
            }

            results.Push (CreateElementIdObjectState (newGuid));
        }

        return NoError;
    });

    if (windowSwitched) {
        RestoreActiveWindow (startingWindow);
    }
    if (startingDatabaseErr == NoError) {
        ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    }
    if (undoErr != NoError) {
        results.Push (CreateErrorResponse (undoErr, "Failed to execute command in undo scope."));
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    for (const auto& result : results) {
        elements (result);
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

GS::Optional<GS::UniString> GetLayoutSettingsCommand::GetRawResponseSchema () const
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
                        "masterLayoutDatabaseId": {
                            "$ref": "#/DatabaseId",
                            "description": "The database of the master layout of the layout. Not present for master layouts."
                        },
                        "masterNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "The Layout Book navigator item of the master layout of the layout. Not present for master layouts."
                        },
                        "masterLayoutName": {
                            "type": "string",
                            "description": "The name of the master layout of the layout. Not present for master layouts."
                        },
                        "customData": {
                            "type": "array",
                            "items": {
                                "$ref": "#/LayoutCustomData"
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

        AddMasterLayoutOfLayout (dbInfo, layoutResult);

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
                        "masterNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "The master layout to assign to the layout, as its Layout Book navigator item. Only layouts can be given a master layout, master layouts cannot."
                        },
                        "masterLayoutDatabaseId": {
                            "$ref": "#/DatabaseId",
                            "description": "The master layout to assign to the layout, as its database. Used when masterNavigatorItemId is not given."
                        },
                        "masterLayoutName": {
                            "type": "string",
                            "minLength": 1,
                            "description": "The master layout to assign to the layout, by name. Used when neither masterNavigatorItemId nor masterLayoutDatabaseId is given."
                        },
                        "customData": {
                            "type": "array",
                            "items": {
                                "$ref": "#/LayoutCustomDataToSet"
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

GS::Optional<GS::UniString> SetLayoutSettingsCommand::GetRawResponseSchema () const
{
    return R"({"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]})";
}

bool IsMasterLayoutDatabase (const API_DatabaseUnId& targetUnId)
{
    return FindMasterLayoutNavigatorItem (targetUnId).HasValue ();
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

        GS::Optional<API_DatabaseUnId> requestedMasterLayoutUnId;
        GS::ObjectState masterLayoutFailedResult;
        if (!ResolveRequestedMasterLayout (item, requestedMasterLayoutUnId, masterLayoutFailedResult)) {
            executionResults.Push (masterLayoutFailedResult);
            continue;
        }
        if (requestedMasterLayoutUnId.HasValue () && isMasterLayout) {
            executionResults.Push (CreateFailedExecutionResult (APIERR_NOTSUPPORTED,
                "The master layout can only be changed on layouts, not on master layouts."));
            continue;
        }

        GSErrCode err = NoError;

        // The master layout is not part of API_LayoutInfo but of the layout's database, so it is
        // changed first, through the database. The layout settings are read afterwards, so the
        // values the new master layout gives the layout are kept unless the item overrides them.
        if (requestedMasterLayoutUnId.HasValue ()) {
            API_DatabaseInfo layoutDbInfo = dbInfo;
            err = ACAPI_Window_GetDatabaseInfo (&layoutDbInfo);
            if (err != NoError) {
                executionResults.Push (CreateFailedExecutionResult (err, "Failed to get the database of the layout."));
                continue;
            }
            layoutDbInfo.masterLayoutUnId = requestedMasterLayoutUnId.Get ();
            err = ACAPI_Database_ModifyDatabase (&layoutDbInfo);
            if (err != NoError) {
                executionResults.Push (CreateFailedExecutionResult (err, "Failed to change the master layout of the layout."));
                continue;
            }
        }

        API_LayoutInfo layoutInfo = {};
        err = ACAPI_Navigator_GetLayoutSets (&layoutInfo, &dbInfo.databaseUnId);
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

GS::Optional<GS::UniString> GetLayoutCustomSchemeCommand::GetRawResponseSchema () const
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

GetDrawingNumberingGridLinesCommand::GetDrawingNumberingGridLinesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDrawingNumberingGridLinesCommand::GetName () const
{
    return "GetDrawingNumberingGridLines";
}

GS::Optional<GS::UniString> GetDrawingNumberingGridLinesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "lines": {
                "type": "array",
                "description": "The lines of the grid of the grid-based drawing numbering of the current layout. Only a layout that uses grid-based drawing numbering has such a grid.",
                "items": {
                    "type": "object",
                    "properties": {
                        "begC": {
                            "$ref": "#/Coordinate2D",
                            "description": "The start point of the line, in layout coordinates."
                        },
                        "endC": {
                            "$ref": "#/Coordinate2D",
                            "description": "The end point of the line, in layout coordinates."
                        },
                        "penIndex": {
                            "type": "integer",
                            "description": "The index of the pen of the line."
                        },
                        "lineTypeIndex": {
                            "type": "integer",
                            "description": "The index of the line type of the line."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "begC",
                        "endC",
                        "penIndex",
                        "lineTypeIndex"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "lines"
        ]
    })";
}

// ACAPI_Drawing_GetNumberingGridLines calls back with only the line, without a user pointer,
// so the lines of one call are collected here.
static GS::Array<API_NumberingGridLine> collectedNumberingGridLines;

static GSErrCode CollectNumberingGridLine (const API_NumberingGridLine* gridLine)
{
    if (gridLine != nullptr) {
        collectedNumberingGridLines.Push (*gridLine);
    }
    return NoError;
}

GS::ObjectState GetDrawingNumberingGridLinesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    collectedNumberingGridLines.Clear ();
    const GSErrCode err = ACAPI_Drawing_GetNumberingGridLines (CollectNumberingGridLine);
    const GS::Array<API_NumberingGridLine> gridLines = collectedNumberingGridLines;
    collectedNumberingGridLines.Clear ();

    if (err == APIERR_BADDATABASE) {
        return CreateErrorResponse (err, "The current window is not a layout. The drawing numbering grid belongs to a layout: activate a layout first (see ChangeWindow).");
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the lines of the drawing numbering grid.");
    }

    GS::ObjectState response;
    const auto& lines = response.AddList<GS::ObjectState> ("lines");
    for (const API_NumberingGridLine& gridLine : gridLines) {
        lines (GS::ObjectState (
            "begC",          Create2DCoordinateObjectState (gridLine.begC),
            "endC",          Create2DCoordinateObjectState (gridLine.endC),
            "penIndex",      gridLine.penInd,
            "lineTypeIndex", GetAttributeIndex (gridLine.ltypeInd)));
    }

    return response;
}
