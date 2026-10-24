#include "ApplicationCommands.hpp"
#include "ObjectState.hpp"
#include "FileSystem.hpp"
#include "AddOnVersion.hpp"
#include "MigrationHelper.hpp"
#include "DGModule.hpp"
#include "Folder.hpp"

static ULong GenerateProjectLocationHashValue (const IO::Location& projectLocation)
{
    ULong hashValue = projectLocation.GenerateHashValue ();
    const GS::UniString username = projectLocation.GetUserName ();
    if (username.IsEmpty ()) {
        return hashValue;
    } else {
        return hashValue * 65599 + username.GenerateHashValue ();
    }
}

static IO::Location GetProjectPreviewsFolder (const IO::Location& projectLocation)
{
    const auto hashValue = GenerateProjectLocationHashValue (projectLocation);
    const auto projectHashFolderName = IO::Name (GS::UniString::Printf ("%lu", hashValue));

    API_SpecFolderID specFolderId = API_ApplicationPrefsFolderID;
    IO::Location applicationPrefSpecFolderLoc;
    ACAPI_ProjectSettings_GetSpecFolder (&specFolderId, &applicationPrefSpecFolderLoc);

	IO::Folder applicationPrefSpecFolder (applicationPrefSpecFolderLoc);
	if (applicationPrefSpecFolder.GetStatus () != NoError)
		return {};

    IO::Location previewFolderLoc = {};
	applicationPrefSpecFolder.Enumerate ([&] (const IO::Name& name, bool isFolder) {
		if (isFolder && previewFolderLoc.IsEmpty ()) {
            const IO::Folder folder (IO::Location (applicationPrefSpecFolderLoc, name));
            bool containsProjectHashFolder = false;
            if (folder.Contains (projectHashFolderName, &containsProjectHashFolder) == NoError && containsProjectHashFolder) {
                previewFolderLoc = folder.GetLocation ();
            }
		}
	});

	if (previewFolderLoc.IsEmpty ())
		return {};

	return IO::Location (previewFolderLoc, projectHashFolderName);
}

#if defined (ServerMainVers_2700) && __has_include ("ACAPI/GSID.hpp")
#include "ACAPI/GSID.hpp"
#define TAPIR_HAS_GSID
#endif

#if defined (ServerMainVers_2800) && __has_include ("ACAPI/LicenseInfo/LicenseInfoManager.hpp")
#include "ACAPI/LicenseInfo/LicenseInfoManager.hpp"
#define TAPIR_HAS_LICENSE_INFO
#endif

#if defined (ServerMainVers_2800) && __has_include ("ACAPI/CuttingPlane.hpp")
#include "ACAPI/CuttingPlane.hpp"
#define TAPIR_HAS_CUTTING_PLANE
#endif

GetAddOnVersionCommand::GetAddOnVersionCommand () :
    CommandBase (CommonSchema::NotUsed)
{

}

GS::String GetAddOnVersionCommand::GetName () const
{
    return "GetAddOnVersion";
}

GS::Optional<GS::UniString> GetAddOnVersionCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "version": {
                "type": "string",
                "description": "Version number in the form of \"1.1.1\".",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "version"
        ]
    })";
}

GS::ObjectState GetAddOnVersionCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    return GS::ObjectState ("version", ADDON_VERSION);
}

GetArchicadLocationCommand::GetArchicadLocationCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetArchicadLocationCommand::GetName () const
{
    return "GetArchicadLocation";
}

GS::Optional<GS::UniString> GetArchicadLocationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "archicadLocation": {
                "type": "string",
                "description": "The location of the Archicad executable in the filesystem.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "archicadLocation"
        ]
    })";
}

GS::ObjectState GetArchicadLocationCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    IO::Location applicationFileLocation;
    GSErrCode err = IO::fileSystem.GetSpecialLocation (IO::FileSystem::ApplicationFile, &applicationFileLocation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the location of the Archicad application!");
    }
    return GS::ObjectState ("archicadLocation", applicationFileLocation.ToDisplayText ());
}

GetArchicadReleaseInfoCommand::GetArchicadReleaseInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetArchicadReleaseInfoCommand::GetName () const
{
    return "GetArchicadReleaseInfo";
}

GS::Optional<GS::UniString> GetArchicadReleaseInfoCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "mainVersion": {
                "type": "integer",
                "description": "The main version of the running Archicad, e.g. 29."
            },
            "releaseVersion": {
                "type": "integer",
                "description": "The release (update) version of the running Archicad."
            },
            "buildNumber": {
                "type": "integer",
                "description": "The build number of the running Archicad."
            },
            "caption": {
                "type": "string",
                "description": "The caption of the Archicad application window. Missing when Archicad does not report it."
            }
        },
        "additionalProperties": false,
        "required": [
            "mainVersion",
            "releaseVersion",
            "buildNumber"
        ]
    })";
}

GS::ObjectState GetArchicadReleaseInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_ServerApplicationInfo serverApplicationInfo = {};
    ACAPI_GetReleaseNumber (&serverApplicationInfo);

    GS::ObjectState response;
    response.Add ("mainVersion", static_cast<Int32> (serverApplicationInfo.mainVersion));
    response.Add ("releaseVersion", static_cast<Int32> (serverApplicationInfo.releaseVersion));
    response.Add ("buildNumber", static_cast<Int32> (serverApplicationInfo.buildNum));

    // The window caption is an extra: the version numbers above are still worth
    // returning when it cannot be read.
    API_MiscAppInfo miscAppInfo = {};
    if (ACAPI_ProjectOperation_GetMiscAppInfo (&miscAppInfo) == NoError) {
        response.Add ("caption", GS::UniString (miscAppInfo.caption));
    }

    return response;
}

GetPrinterInfoCommand::GetPrinterInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetPrinterInfoCommand::GetName () const
{
    return "GetPrinterInfo";
}

GS::Optional<GS::UniString> GetPrinterInfoCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "printArea": {
                "type": "string",
                "description": "The area Archicad prints, as set in the print settings. 'unknown' for a value this add-on does not name.",
                "enum": ["currentView", "entireDrawing", "marquee", "unknown"]
            },
            "grid": {
                "type": "boolean",
                "description": "Whether the grid is printed."
            },
            "fixText": {
                "type": "boolean",
                "description": "Whether text is printed in fixed size."
            },
            "scale": {
                "type": "integer",
                "description": "The print scale."
            }
        },
        "additionalProperties": false,
        "required": [
            "printArea",
            "grid",
            "fixText",
            "scale"
        ]
    })";
}

static GS::UniString ConvertPrintAreaToString (const API_PrintPars& printPars)
{
    switch (printPars.printArea) {
        case PrintArea_CurrentView: return "currentView";
        case PrintArea_EntireDrawing: return "entireDrawing";
        case PrintArea_Marquee: return "marquee";
        default: return "unknown";
    }
}

GS::ObjectState GetPrinterInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    // The same settings PrintView takes, read back from Archicad's print settings.
    API_PrintPars printPars = {};
    const GSErrCode err = ACAPI_ProjectOperation_GetPrinterPars (&printPars);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the print settings.");
    }

    GS::ObjectState response;
    response.Add ("printArea", ConvertPrintAreaToString (printPars));
    response.Add ("grid", static_cast<bool> (printPars.grid));
    response.Add ("fixText", static_cast<bool> (printPars.fixText));
    response.Add ("scale", static_cast<Int32> (printPars.scale));
    return response;
}

QuitArchicadCommand::QuitArchicadCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String QuitArchicadCommand::GetName () const
{
    return "QuitArchicad";
}

GS::Optional<GS::UniString> QuitArchicadCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState QuitArchicadCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#if defined (ServerMainVers_2800)
    GSErrCode err = ACAPI_ProjectOperation_Quit ();
#else
    Int32 magicCode = 1234;
    GSErrCode err = ACAPI_ProjectOperation_Quit (magicCode);
#endif
    if (err != NoError) {
        return CreateFailedExecutionResult (APIERR_COMMANDFAILED, "Failed to quit Archicad!");
    }

    return CreateSuccessfulExecutionResult ();
}

GetCurrentWindowTypeCommand::GetCurrentWindowTypeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentWindowTypeCommand::GetName () const
{
    return "GetCurrentWindowType";
}

GS::Optional<GS::UniString> GetCurrentWindowTypeCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "currentWindowType": {
                "$ref": "#/WindowType"
            }
        },
        "additionalProperties": false,
        "required": [
            "currentWindowType"
        ]
    })";
}

static GS::UniString ConvertWindowTypeToString (API_WindowTypeID type)
{
    switch (type) {
        case APIWind_FloorPlanID: return "FloorPlan";
        case APIWind_SectionID: return "Section";
        case APIWind_DetailID: return "Details";
        case APIWind_3DModelID: return "3DModel";
        case APIWind_LayoutID: return "Layout";
        case APIWind_DrawingID: return "Drawing";
        case APIWind_MyTextID: return "CustomText";
        case APIWind_MyDrawID: return "CustomDraw";
        case APIWind_MasterLayoutID: return "MasterLayout";
        case APIWind_ElevationID: return "Elevation";
        case APIWind_InteriorElevationID: return "InteriorElevation";
        case APIWind_WorksheetID: return "Worksheet";
        case APIWind_ReportID: return "Report";
        case APIWind_DocumentFrom3DID: return "3DDocument";
        case APIWind_External3DID: return "External3D";
        case APIWind_Movie3DID: return "Movie3D";
        case APIWind_MovieRenderingID: return "MovieRendering";
        case APIWind_RenderingID: return "Rendering";
        case APIWind_ModelCompareID: return "ModelCompare";
        case APIWind_IESCommonDrawingID: return "Interactive Schedule";
        default: return "Unknown";
    }
}

static API_WindowTypeID ConvertStringToWindowType (GS::UniString typeStr)
{
    if ("FloorPlan" == typeStr) {
        return APIWind_FloorPlanID;
    }
    if ("Section" == typeStr) {
        return APIWind_SectionID;
    }
    if ("Details" == typeStr) {
        return APIWind_DetailID;
    }
    if ("3DModel" == typeStr) {
        return APIWind_3DModelID;
    }
    if ("Layout" == typeStr) {
        return APIWind_LayoutID;
    }
    if ("Drawing" == typeStr) {
        return APIWind_DrawingID;
    }
    if ("CustomText" == typeStr) {
        return APIWind_MyTextID;
    }
    if ("CustomDraw" == typeStr) {
        return APIWind_MyDrawID;
    }
    if ("MasterLayout" == typeStr) {
        return APIWind_MasterLayoutID;
    }
    if ("Elevation" == typeStr) {
        return APIWind_ElevationID;
    }
    if ("InteriorElevation" == typeStr) {
        return APIWind_InteriorElevationID;
    }
    if ("Worksheet" == typeStr) {
        return APIWind_WorksheetID;
    }
    if ("Report" == typeStr) {
        return APIWind_ReportID;
    }
    if ("3DDocument" == typeStr) {
        return APIWind_DocumentFrom3DID;
    }
    if ("External3D" == typeStr) {
        return APIWind_External3DID;
    }
    if ("Movie3D" == typeStr) {
        return APIWind_Movie3DID;
    }
    if ("MovieRendering" == typeStr) {
        return APIWind_MovieRenderingID;
    }
    if ("Rendering" == typeStr) {
        return APIWind_RenderingID;
    }
    if ("ModelCompare" == typeStr) {
        return APIWind_ModelCompareID;
    }
    if ("Interactive Schedule" == typeStr) {
        return APIWind_IESCommonDrawingID;
    }
    return API_ZombieWindowID;
}

GS::ObjectState GetCurrentWindowTypeCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_WindowInfo windowInfo;
    GSErrCode err = ACAPI_Window_GetCurrentWindow (&windowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current window!");
    }
    return GS::ObjectState ("currentWindowType", ConvertWindowTypeToString (windowInfo.typeID));
}

GetCurrentWindowDrawingScaleCommand::GetCurrentWindowDrawingScaleCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetCurrentWindowDrawingScaleCommand::GetName () const
{
    return "GetCurrentWindowDrawingScale";
}

GS::Optional<GS::UniString> GetCurrentWindowDrawingScaleCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "scale": {
                "type": "number",
                "description": "The denominator of the drawing scale of the current window: 100.0 for 1:100, 20.0 for 1:20. Always 1.0 for layouts."
            }
        },
        "additionalProperties": false,
        "required": [
            "scale"
        ]
    })";
}

GS::ObjectState GetCurrentWindowDrawingScaleCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    // The scale belongs to the current database (every floor plan, section, elevation,
    // detail, worksheet and 3D document window has its own), which is the database of the
    // current window unless an earlier command switched it.
    double scale = 0.0;
    GSErrCode err = ACAPI_Drawing_GetDrawingScale (&scale);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the drawing scale of the current window!");
    }
    return GS::ObjectState ("scale", scale);
}

SetCurrentWindowDrawingScaleCommand::SetCurrentWindowDrawingScaleCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String SetCurrentWindowDrawingScaleCommand::GetName () const
{
    return "SetCurrentWindowDrawingScale";
}

GS::Optional<GS::UniString> SetCurrentWindowDrawingScaleCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "scale": {
                "type": "number",
                "description": "The denominator of the new drawing scale: 100 for 1:100, 20 for 1:20. Archicad accepts scales between 1 and 30000."
            },
            "rebuild": {
                "type": "boolean",
                "description": "Rebuild the window after the scale changed (the API's zoom flag). Default: false.",
                "default": false
            }
        },
        "additionalProperties": false,
        "required": [
            "scale"
        ]
    })";
}

GS::Optional<GS::UniString> SetCurrentWindowDrawingScaleCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetCurrentWindowDrawingScaleCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    double scale = 0.0;
    if (!parameters.Get ("scale", scale)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing parameter: scale.");
    }
    bool rebuild = false;
    parameters.Get ("rebuild", rebuild);

    // ACAPI_Drawing_ChangeDrawingScale documents the accepted scale as greater than 1 and
    // smaller than 30000 and answers anything else with a bare APIERR_BADPARS. The negated
    // comparison also rejects NaN.
    if (!(scale >= 1.0 && scale <= 30000.0)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: scale must be the denominator of a scale between 1:1 and 1:30000 (100 for 1:100).");
    }

    API_WindowInfo windowInfo = {};
    GSErrCode err = ACAPI_Window_GetCurrentWindow (&windowInfo);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current window!");
    }
    if (windowInfo.typeID == APIWind_LayoutID || windowInfo.typeID == APIWind_MasterLayoutID) {
        return CreateFailedExecutionResult (APIERR_BADDATABASE, "The current window is a layout: a layout is always 1:1 and its scale cannot be changed. Activate a floor plan, section, elevation, detail or worksheet window first (see ChangeWindow).");
    }

    err = ACAPI_CallUndoableCommand ("SetCurrentWindowDrawingScaleCommand", [&] () -> GSErrCode {
        return ACAPI_Drawing_ChangeDrawingScale (&scale, rebuild);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to change the drawing scale of the current window. Archicad accepts scales between 1 and 30000 in a floor plan, section, elevation, detail, worksheet or 3D document window.");
    }

    return CreateSuccessfulExecutionResult ();
}

ChangeWindowCommand::ChangeWindowCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeWindowCommand::GetName () const
{
    return "ChangeWindow";
}

GS::Optional<GS::UniString> ChangeWindowCommand::GetInputParametersSchema () const
{
    return R"({
        "$ref": "#/NavigatorItemIdOrDatabaseIdAndWindowType"
    })";
}

GS::Optional<GS::UniString> ChangeWindowCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ChangeWindowCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::ObjectState* navigatorItemId = parameters.Get ("navigatorItemId");
    if (navigatorItemId != nullptr) {
#if defined (ServerMainVers_2700)
        API_Guid navGuid = GetGuidFromObjectState (*navigatorItemId);
        if (navGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId is corrupt or missing");
        }

        API_NavigatorItem navigatorItem = {};
        GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&navGuid, &navigatorItem);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to get navigator item from guid");
        }

        switch (navigatorItem.itemType) {
            case API_UndefinedNavItem:
            case API_ProjectNavItem:
            case API_CameraSetNavItem:
            case API_InfoNavItem:
            case API_HelpNavItem:
            case API_BookNavItem:
            case API_MasterFolderNavItem:
            case API_SubSetNavItem:
            case API_FolderNavItem:
                return CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId must be a view or viewpoint; got a container or folder");
            default:
                break;
        }

        // ACAPI_View_GoToView applies the view's saved settings (layer combo,
        // scale, MVO, dim, zoom). The whole point of accepting navigatorItemId
        // is to apply those settings, so AC25/26 (without GoToView) rejects
        // rather than silently falling back to a plain database/window switch.
        const GS::UniString guidStr = APIGuidToString (navGuid);
        err = ACAPI_View_GoToView (guidStr.ToCStr ().Get ());
        return err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateErrorResponse (err, "Failed to activate the view");
#else
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "navigatorItemId requires Archicad 27 or later; use databaseId instead.");
#endif
    }

    // windowType may be omitted when a databaseId is given: the database identifies its
    // window, and ACAPI_Window_GetDatabaseInfo fills in its typeID below (#1384).
    const GS::ObjectState* databaseId = parameters.Get ("databaseId");
    GS::UniString windowTypeStr;
    const bool hasWindowType = parameters.Get ("windowType", windowTypeStr);
    if (!hasWindowType && databaseId == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing parameter: windowType or databaseId.");
    }

    API_WindowInfo windowInfo = {};
    if (hasWindowType) {
        windowInfo.typeID = ConvertStringToWindowType (windowTypeStr);
        if (windowInfo.typeID == API_ZombieWindowID) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: windowType.");
        }
    }

    Int32 storyIndex = 0;
    const bool hasStoryIndex = parameters.Get ("storyIndex", storyIndex);
    if (hasStoryIndex) {
        if (windowInfo.typeID != APIWind_FloorPlanID) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "storyIndex is only valid when windowType is 'FloorPlan'.");
        }

        // The floor plan is one database for every story, so putting the story index into
        // API_WindowInfo::index and switching the database/window does not move the active
        // story: every call reports NoError and GetStories keeps returning the old actStory
        // (#748). The active story is changed with the story settings command instead
        // (APIStory_GoTo), the same way the Navigator does it. The window/database switch
        // stays, so the floor plan comes to the front before the story changes.
        API_StoryInfo storyInfo = {};
        GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
        BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to retrieve stories info.");
        }
        if (storyIndex < storyInfo.firstStory || storyIndex > storyInfo.lastStory) {
            return CreateFailedExecutionResult (APIERR_BADINDEX, "storyIndex is out of range; see GetStories for the valid story indices.");
        }

        // GetDatabaseInfo fills in the GUID and all fields from typeID,
        // which are needed to change the current database (used by ACAPI element creation).
        if (ACAPI_Window_GetDatabaseInfo (&windowInfo) == NoError) {
            ACAPI_Database_ChangeCurrentDatabase (&windowInfo);
        }
        err = ACAPI_Window_ChangeWindow (&windowInfo);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to change the window to the floor plan.");
        }

        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_GoTo;
        storyCmd.index  = static_cast<short> (storyIndex);
        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        return err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to change active story.");
    }

    if (databaseId != nullptr) {
        API_DatabaseInfo targetDatabase = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*databaseId));
        GSErrCode err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to resolve target database.");
        }

        if (hasWindowType && targetDatabase.typeID != windowInfo.typeID) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "databaseId does not belong to the requested windowType.");
        }

        err = ACAPI_Database_ChangeCurrentDatabase (&targetDatabase);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to change current database.");
        }

        windowInfo = targetDatabase;
    }

    GSErrCode err = ACAPI_Window_ChangeWindow (&windowInfo);
    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateErrorResponse (err, "Failed to change the window!");
}

GetContainingDatabasesCommand::GetContainingDatabasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetContainingDatabasesCommand::GetName () const
{
    return "GetContainingDatabases";
}

GS::Optional<GS::UniString> GetContainingDatabasesCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetContainingDatabasesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "databases": {
                "type": "array",
                "description": "The database of each element, in the order of the input elements. Pass windowType and databaseId to ChangeWindow to open it.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "databaseId": {
                                    "$ref": "#/DatabaseId"
                                },
                                "windowType": {
                                    "$ref": "#/WindowType"
                                },
                                "name": {
                                    "type": "string",
                                    "description": "The name of the database (for example the name of the layout). Empty for databases without a name, like the floor plan."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "databaseId",
                                "windowType",
                                "name"
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
            "databases"
        ]
    })";
}

GS::ObjectState GetContainingDatabasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& databases = response.AddList<GS::ObjectState> ("databases");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);
        if (elemGuid == APINULLGuid) {
            databases (CreateErrorResponse (APIERR_BADPARS, "elementId is corrupt or missing."));
            continue;
        }

        API_DatabaseInfo dbInfo = {};
        GSErrCode err = ACAPI_Database_GetContainingDatabase (&elemGuid, &dbInfo);
        if (err != NoError) {
            databases (CreateErrorResponse (err, "Failed to get the containing database of the element."));
            continue;
        }

        // Fills in the type and the name of the database from its id, the same
        // way ChangeWindow resolves a databaseId.
        API_DatabaseInfo fullDbInfo = dbInfo;
        if (ACAPI_Window_GetDatabaseInfo (&fullDbInfo) == NoError) {
            dbInfo = fullDbInfo;
        }

        GS::ObjectState database = CreateDatabaseIdObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (dbInfo));
        database.Add ("windowType", ConvertWindowTypeToString (dbInfo.typeID));
        database.Add ("name", GS::UniString (dbInfo.name));
        databases (database);
    }

    return response;
}



ShowAlertCommand::ShowAlertCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String ShowAlertCommand::GetName () const
{
    return "ShowAlert";
}

GS::Optional<GS::UniString> ShowAlertCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "alertType": {
                "type": "string",
                "enum": ["information", "warning", "error"],
                "description": "The type of the alert dialog."
            },
            "title": {
                "type": "string",
                "description": "The title of the alert dialog."
            },
            "message": {
                "type": "string",
                "description": "The main message text."
            },
            "subMessage": {
                "type": "string",
                "description": "Optional smaller sub-message text below the main message."
            },
            "button1": {
                "type": "string",
                "description": "Label for the first (default) button."
            },
            "button2": {
                "type": "string",
                "description": "Label for the second button (e.g. Cancel)."
            },
            "button3": {
                "type": "string",
                "description": "Label for the optional third button."
            }
        },
        "additionalProperties": false,
        "required": ["alertType", "title", "message", "button1"]
    })";
}

GS::Optional<GS::UniString> ShowAlertCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "clickedButton": {
                "type": "integer",
                "description": "Index of the button the user clicked: 1 = button1, 2 = button2, 3 = button3."
            }
        },
        "additionalProperties": false,
        "required": ["clickedButton"]
    })";
}

GS::ObjectState ShowAlertCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString alertTypeStr;
    parameters.Get ("alertType", alertTypeStr);

    short alertType = DG_INFORMATION;
    if (alertTypeStr == "warning") {
        alertType = DG_WARNING;
    } else if (alertTypeStr == "error") {
        alertType = DG_ERROR;
    }

    GS::UniString title, message, subMessage, button1, button2, button3;
    parameters.Get ("title", title);
    parameters.Get ("message", message);
    parameters.Get ("subMessage", subMessage);
    parameters.Get ("button1", button1);
    parameters.Get ("button2", button2);
    parameters.Get ("button3", button3);

    short clicked = DGAlert (alertType, title, message, subMessage, button1, button2, button3);

    return GS::ObjectState ("clickedButton", static_cast<Int32> (clicked));
}

GetSpecialFoldersCommand::GetSpecialFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSpecialFoldersCommand::GetName () const
{
    return "GetSpecialFolders";
}

GS::Optional<GS::UniString> GetSpecialFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folderTypes": {
                "type": "array",
                "description": "The types of the special folders to retrieve.",
                "items": {
                    "$ref": "#/SpecialFolderType"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "folderTypes"
        ]
    })";
}

GS::Optional<GS::UniString> GetSpecialFoldersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folderPaths": {
                "$ref": "#/SpecialFolderPathsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "folderPaths"
        ]
    })";
}

static GS::Optional<API_SpecFolderID> ConvertStringToSpecFolderID (const GS::UniString& folderTypeStr)
{
    if (folderTypeStr == "ApplicationPrefs") {
        return API_ApplicationPrefsFolderID;
    }
    if (folderTypeStr == "GraphisoftPrefs") {
        return API_GraphisoftPrefsFolderID;
    }
    if (folderTypeStr == "GraphisoftHome") {
        return API_GraphisoftHomeFolderID;
    }
    if (folderTypeStr == "Cache") {
        return API_CacheFolderID;
    }
    if (folderTypeStr == "Data") {
        return API_DataFolderID;
    }
    if (folderTypeStr == "UserDocuments") {
        return API_UserDocumentsFolderID;
    }
    if (folderTypeStr == "Temporary") {
        return API_TemporaryFolderID;
    }
    if (folderTypeStr == "Application") {
        return API_ApplicationFolderID;
    }
    if (folderTypeStr == "Defaults") {
        return API_DefaultsFolderID;
    }
    if (folderTypeStr == "WebObjects") {
        return API_WebObjectsFolderID;
    }
    if (folderTypeStr == "Templates") {
        return API_TemplatesFolderID;
    }
    if (folderTypeStr == "Help") {
        return API_HelpFolderID;
    }
    if (folderTypeStr == "EmbeddedProjectLibrary") {
        return API_EmbeddedProjectLibraryFolderID;
    }
    if (folderTypeStr == "EmbeddedProjectLibraryHotlink") {
        return API_EmbeddedProjectLibraryHotlinkFolderID;
    }
    return GS::NoValue;
}

GS::ObjectState GetSpecialFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> folderTypes;
    parameters.Get ("folderTypes", folderTypes);

    GS::ObjectState response;
    const auto& folderPaths = response.AddList<GS::ObjectState> ("folderPaths");

    for (const GS::UniString& folderTypeStr : folderTypes) {
        if (folderTypeStr == "ProjectPreviews") {
            API_ProjectInfo projectInfo = {};
            const GSErrCode projectErr = ACAPI_ProjectOperation_Project (&projectInfo);
            const IO::Location* projectLocation = projectInfo.teamwork ? projectInfo.location_team : projectInfo.location;
            if (projectErr != NoError || projectInfo.untitled || projectLocation == nullptr) {
                folderPaths (CreateErrorResponse (APIERR_NOPLAN, "ProjectPreviews requires a saved project."));
                continue;
            }

            const IO::Location previewsFolder = GetProjectPreviewsFolder (*projectLocation);
            if (previewsFolder.IsEmpty ()) {
                folderPaths (CreateErrorResponse (APIERR_GENERAL, "Failed to find the previews folder of the current project."));
                continue;
            }

            folderPaths (GS::ObjectState ("path", previewsFolder.ToDisplayText ()));
            continue;
        }

        const GS::Optional<API_SpecFolderID> folderId = ConvertStringToSpecFolderID (folderTypeStr);
        if (!folderId.HasValue ()) {
            folderPaths (CreateErrorResponse (APIERR_BADPARS, "Invalid folder type: " + folderTypeStr));
            continue;
        }

        API_SpecFolderID specFolderId = folderId.Get ();
        IO::Location location;
        const GSErrCode err = ACAPI_ProjectSettings_GetSpecFolder (&specFolderId, &location);
        if (err != NoError) {
            folderPaths (CreateErrorResponse (err, "Failed to get the path of the special folder: " + folderTypeStr));
            continue;
        }

        folderPaths (GS::ObjectState ("path", location.ToDisplayText ()));
    }

    return response;
}

GetUserGSIDCommand::GetUserGSIDCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String GetUserGSIDCommand::GetName () const
{
    return "GetUserGSID";
}

GS::Optional<GS::UniString> GetUserGSIDCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "userId": {
                "type": "string",
                "description": "The stable GSID User ID of the logged-in user."
            },
            "organizationIds": {
                "type": "array",
                "items": {
                    "type": "string"
                },
                "description": "The list of organization IDs the user belongs to. Empty if not part of any organization or if the information cannot be retrieved."
            }
        },
        "additionalProperties": false,
        "required": [
            "userId"
        ]
    })";
}

GS::ObjectState GetUserGSIDCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef TAPIR_HAS_GSID
    auto gsid = ACAPI::CreateGSIDObject ();
    if (gsid.IsErr ()) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to create GSID object!");
    }

    auto userId = gsid->GetUserId ();
    if (userId.IsErr ()) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to get GSID user ID. User may not be signed in.");
    }
    GS::Array<GS::UniString> orgIdsArray;
    auto organizationIds = gsid->GetOrganizationIds ();
    if (organizationIds.IsOk ()) {
        for (const auto& orgId : *organizationIds) {
            orgIdsArray.Push (orgId);
        }
    }
    return GS::ObjectState ("userId", *userId, "organizationIds", orgIdsArray);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "GetUserGSID requires Archicad 27 or later.");
#endif
}

// ---------------------------------------------------------------------------
// Archicad visibility, Action Center checks, redraw
// ---------------------------------------------------------------------------

GetArchicadVisibilityCommand::GetArchicadVisibilityCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetArchicadVisibilityCommand::GetName () const
{
    return "GetArchicadVisibility";
}

GS::Optional<GS::UniString> GetArchicadVisibilityCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "isVisible": {
                "type": "boolean",
                "description": "True when the Archicad application is shown, false when it is hidden."
            }
        },
        "additionalProperties": false,
        "required": [
            "isVisible"
        ]
    })";
}

GS::ObjectState GetArchicadVisibilityCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#if defined (WINDOWS)
    bool isShown = true;
    const GSErrCode err = TAPIR_View_GetShowHideState (&isShown);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the visibility of Archicad.");
    }
    return GS::ObjectState ("isVisible", isShown);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Supported on Windows only.");
#endif
}

SetArchicadVisibilityCommand::SetArchicadVisibilityCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetArchicadVisibilityCommand::GetName () const
{
    return "SetArchicadVisibility";
}

GS::Optional<GS::UniString> SetArchicadVisibilityCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "isVisible": {
                "type": "boolean",
                "description": "True shows the Archicad application, false hides it."
            }
        },
        "additionalProperties": false,
        "required": [
            "isVisible"
        ]
    })";
}

GS::Optional<GS::UniString> SetArchicadVisibilityCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetArchicadVisibilityCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#if defined (WINDOWS)
    bool isVisible = true;
    if (!parameters.Get ("isVisible", isVisible)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing parameter: isVisible.");
    }

    const GSErrCode err = TAPIR_View_ChangeShowHideState (&isVisible);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, isVisible ? "Failed to show Archicad." : "Failed to hide Archicad.");
    }
    return CreateSuccessfulExecutionResult ();
#else
    (void) parameters;
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Supported on Windows only.");
#endif
}

RunActionCenterChecksCommand::RunActionCenterChecksCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RunActionCenterChecksCommand::GetName () const
{
    return "RunActionCenterChecks";
}

GS::Optional<GS::UniString> RunActionCenterChecksCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "checks": {
                "type": "array",
                "description": "The Action Center checks to run, in this order. Omitted means [\"Hotlinks\"]. \"Hotlinks\" is available on every Archicad version; \"All\", \"Libraries\", \"Drawings\" and \"Licenses\" only on Archicad 25, later versions return an error for them in their execution result.",
                "items": {
                    "type": "string",
                    "enum": ["Hotlinks", "All", "Libraries", "Drawings", "Licenses"]
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> RunActionCenterChecksCommand::GetRawResponseSchema () const
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

static GS::Optional<API_ActionCenterItemID> ConvertStringToActionCenterItemID (const GS::UniString& checkName)
{
    // Always the named enumerator: Hotlinks is 1 on AC25 (after All) and 0 from AC26 on.
    if (checkName == "Hotlinks") {
        return APIActionCenterItem_Hotlinks;
    }
#ifndef ServerMainVers_2600
    if (checkName == "All") {
        return APIActionCenterItem_All;
    }
    if (checkName == "Libraries") {
        return APIActionCenterItem_Libraries;
    }
    if (checkName == "Drawings") {
        return APIActionCenterItem_Drawings;
    }
    if (checkName == "Licenses") {
        return APIActionCenterItem_Licenses;
    }
#endif
    return GS::NoValue;
}

GS::ObjectState RunActionCenterChecksCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> checks;
    if (!parameters.Get ("checks", checks)) {
        checks.Push ("Hotlinks");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::UniString& checkName : checks) {
        const GS::Optional<API_ActionCenterItemID> itemId = ConvertStringToActionCenterItemID (checkName);
        if (!itemId.HasValue ()) {
            if (checkName == "All" || checkName == "Libraries" || checkName == "Drawings" || checkName == "Licenses") {
                executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Not available on this Archicad version: " + checkName));
            } else {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid check: " + checkName));
            }
            continue;
        }

        // Returns only whether the check could run: the Action Center tab appears
        // in Archicad when the check found an issue, the caller is not told.
        const GSErrCode err = TAPIR_Window_ActionCenterCheck (itemId.Get ());
        if (err == NoError) {
            executionResults (CreateSuccessfulExecutionResult ());
        } else if (err == APIERR_NOPLAN) {
            executionResults (CreateFailedExecutionResult (err, "There is no open project."));
        } else if (err == APIERR_REFUSEDCMD) {
            executionResults (CreateFailedExecutionResult (err, "Refused to run the check because a modal dialog is open."));
        } else {
            executionResults (CreateFailedExecutionResult (err, "Failed to run the check: " + checkName));
        }
    }

    return response;
}

RedrawCurrentDatabaseCommand::RedrawCurrentDatabaseCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RedrawCurrentDatabaseCommand::GetName () const
{
    return "RedrawCurrentDatabase";
}

GS::Optional<GS::UniString> RedrawCurrentDatabaseCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState RedrawCurrentDatabaseCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    const GSErrCode err = TAPIR_Database_RedrawCurrentDatabase ();
    if (err == APIERR_NOPLAN) {
        return CreateFailedExecutionResult (err, "There is no open project.");
    }
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to redraw the current database.");
    }
    return CreateSuccessfulExecutionResult ();
// GetProductVersionInfo
// ---------------------------------------------------------------------------

GetProductVersionInfoCommand::GetProductVersionInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetProductVersionInfoCommand::GetName () const
{
    return "GetProductVersionInfo";
}

GS::Optional<GS::UniString> GetProductVersionInfoCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "productName": {
                "type": "string",
                "description": "The name of the product. Archicad 29 or later."
            },
            "productType": {
                "type": "string",
                "description": "The Graphisoft product type. Archicad 29 or later."
            },
            "productFamilyId": {
                "type": "string",
                "description": "The identifier of the product family."
            },
            "languageCode": {
                "type": "string",
                "description": "The Graphisoft language code of the running Archicad (for example INT, GER)."
            },
            "buildNumber": {
                "type": "integer",
                "description": "The build number of the running Archicad."
            },
            "versionString": {
                "type": "string",
                "description": "The full version of the running Archicad as text."
            },
            "mainVersion": {
                "type": "string",
                "description": "The main version, as text. Archicad 29 or later."
            },
            "updateVersion": {
                "type": "string",
                "description": "The update version, as text. Archicad 29 or later."
            },
            "hotfixVersion": {
                "type": "string",
                "description": "The hotfix version, as text. Archicad 29 or later."
            },
            "versionNumbers": {
                "type": "array",
                "items": {
                    "type": "integer"
                },
                "description": "The three parts of the version number. Archicad 28 only."
            }
        },
        "additionalProperties": false,
        "required": [
            "productFamilyId",
            "languageCode",
            "buildNumber",
            "versionString"
        ]
    })";
}

GS::ObjectState GetProductVersionInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef TAPIR_HAS_LICENSE_INFO
    auto licenseInfoManager = ACAPI::LicenseInfo::GetLicenseInfoManager ();
    if (licenseInfoManager.IsErr ()) {
        return CreateErrorResponse (licenseInfoManager.UnwrapErr ().kind, "Failed to get the license info manager.");
    }

    const auto versionInfo = licenseInfoManager->GetProductVersionInfo ();

    GS::ObjectState response;
    response.Add ("productFamilyId", versionInfo.GetProductFamilyId ());
    response.Add ("languageCode", versionInfo.GetGSLanguageCode ());
    response.Add ("buildNumber", versionInfo.GetBuildNum ());
#if defined (ServerMainVers_2900)
    response.Add ("productName", versionInfo.GetProductName ());
    response.Add ("productType", versionInfo.GetGSProductType ());
    response.Add ("versionString", versionInfo.GetAppVersionString ());
    response.Add ("mainVersion", versionInfo.GetMainVersionString ());
    response.Add ("updateVersion", versionInfo.GetUpdateVersionString ());
    response.Add ("hotfixVersion", versionInfo.GetHotfixVersionString ());
#else
    response.Add ("versionString", versionInfo.GetVersionString ());
    GS::Array<Int32> versionNumbers;
    versionNumbers.Push (versionInfo.GetVersionNum1 ());
    versionNumbers.Push (versionInfo.GetVersionNum2 ());
    versionNumbers.Push (versionInfo.GetVersionNum3 ());
    response.Add ("versionNumbers", versionNumbers);
#endif
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "GetProductVersionInfo requires Archicad 28 or later.");
#endif
}

// ---------------------------------------------------------------------------
// Get3DCutawayStatus
// ---------------------------------------------------------------------------

Get3DCutawayStatusCommand::Get3DCutawayStatusCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String Get3DCutawayStatusCommand::GetName () const
{
    return "Get3DCutawayStatus";
}

GS::Optional<GS::UniString> Get3DCutawayStatusCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cutawayEnabled": {
                "type": "boolean",
                "description": "Whether 3D Cutaway is on in the 3D window."
            },
            "cuttingPlanesVisible": {
                "type": "boolean",
                "description": "Whether the cutting planes are shown in the 3D window."
            }
        },
        "additionalProperties": false,
        "required": [
            "cutawayEnabled",
            "cuttingPlanesVisible"
        ]
    })";
}

GS::ObjectState Get3DCutawayStatusCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef TAPIR_HAS_CUTTING_PLANE
    auto cuttingPlane = ACAPI::Cutaway::GetCuttingPlane ();
    if (cuttingPlane.IsErr ()) {
        return CreateErrorResponse (cuttingPlane.UnwrapErr ().kind, "Failed to get the cutting plane.");
    }

    auto cutawayEnabled = cuttingPlane->Is3DCutawayEnabled ();
    if (cutawayEnabled.IsErr ()) {
        return CreateErrorResponse (cutawayEnabled.UnwrapErr ().kind, "Failed to get whether 3D Cutaway is on.");
    }

    auto cuttingPlanesVisible = cuttingPlane->AreCuttingPlanesVisible ();
    if (cuttingPlanesVisible.IsErr ()) {
        return CreateErrorResponse (cuttingPlanesVisible.UnwrapErr ().kind, "Failed to get whether the cutting planes are shown.");
    }

    return GS::ObjectState ("cutawayEnabled", *cutawayEnabled, "cuttingPlanesVisible", *cuttingPlanesVisible);
#elif !defined (ServerMainVers_2800)
    // Archicad 25-27: the C API (AC25/26 through MigrationHelper.hpp).
    bool cutawayEnabled = false;
    GSErrCode err = ACAPI_CuttingPlane_Is3DCutawayEnabled (&cutawayEnabled);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get whether 3D Cutaway is on. Is the 3D window open?");
    }

    bool cuttingPlanesVisible = false;
    err = ACAPI_CuttingPlane_IsCuttingPlanesVisible (&cuttingPlanesVisible);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get whether the cutting planes are shown. Is the 3D window open?");
    }

    return GS::ObjectState ("cutawayEnabled", cutawayEnabled, "cuttingPlanesVisible", cuttingPlanesVisible);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Get3DCutawayStatus is not available in this build.");
#endif
}

// ---------------------------------------------------------------------------
// Set3DCutawayStatus
// ---------------------------------------------------------------------------

Set3DCutawayStatusCommand::Set3DCutawayStatusCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String Set3DCutawayStatusCommand::GetName () const
{
    return "Set3DCutawayStatus";
}

GS::Optional<GS::UniString> Set3DCutawayStatusCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cutawayEnabled": {
                "type": "boolean",
                "description": "Turns 3D Cutaway on or off. Left unchanged when omitted."
            },
            "cuttingPlanesVisible": {
                "type": "boolean",
                "description": "Shows or hides the cutting planes. Left unchanged when omitted."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> Set3DCutawayStatusCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState Set3DCutawayStatusCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef TAPIR_HAS_CUTTING_PLANE
    auto cuttingPlane = ACAPI::Cutaway::GetCuttingPlane ();
    if (cuttingPlane.IsErr ()) {
        return CreateFailedExecutionResult (cuttingPlane.UnwrapErr ().kind, "Failed to get the cutting plane.");
    }

    bool cutawayEnabled = false;
    if (parameters.Get ("cutawayEnabled", cutawayEnabled)) {
        auto result = cuttingPlane->Set3DCutawayStatus (cutawayEnabled);
        if (result.IsErr ()) {
            return CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to turn 3D Cutaway on or off.");
        }
    }

    bool cuttingPlanesVisible = false;
    if (parameters.Get ("cuttingPlanesVisible", cuttingPlanesVisible)) {
        auto result = cuttingPlane->SetCuttingPlanesVisibility (cuttingPlanesVisible);
        if (result.IsErr ()) {
            return CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to show or hide the cutting planes.");
        }
    }

    return CreateSuccessfulExecutionResult ();
#elif !defined (ServerMainVers_2800)
    // Archicad 25-27: the C API (AC25/26 through MigrationHelper.hpp).
    bool cutawayEnabled = false;
    if (parameters.Get ("cutawayEnabled", cutawayEnabled)) {
        GSErrCode err = ACAPI_CuttingPlane_Set3DCutawayStatus (&cutawayEnabled);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to turn 3D Cutaway on or off. Is the 3D window open?");
        }
    }

    bool cuttingPlanesVisible = false;
    if (parameters.Get ("cuttingPlanesVisible", cuttingPlanesVisible)) {
        GSErrCode err = ACAPI_CuttingPlane_SetCuttingPlanesVisibility (&cuttingPlanesVisible);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to show or hide the cutting planes. Is the 3D window open?");
        }
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Set3DCutawayStatus is not available in this build.");
#endif
}

// ---------------------------------------------------------------------------
// GetPointFromUser
// ---------------------------------------------------------------------------

GetPointFromUserCommand::GetPointFromUserCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetPointFromUserCommand::GetName () const
{
    return "GetPointFromUser";
}

GS::Optional<GS::UniString> GetPointFromUserCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "prompt": {
                "type": "string",
                "description": "Shown in the control box while Archicad waits for the click. Single-byte text: the box takes a char field. Archicad's main thread waits for the click or for Escape, and every other JSON command queues behind this one until then."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetPointFromUserCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "position": {
                "$ref": "#/Coordinate3D",
                "description": "The clicked point in the project's coordinates."
            }
        },
        "additionalProperties": false,
        "required": [
            "position"
        ]
    })";
}

GS::ObjectState GetPointFromUserCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // Hands the input to the designer: Archicad waits for a click in the current
    // window (snapping to element hotspots), and the command returns with the point,
    // or with an error when the input is cancelled with Escape.
    GS::UniString prompt = "Click a point";
    parameters.Get ("prompt", prompt);

    API_GetPointType pointInfo = {};
    CHTruncate (prompt.ToCStr ().Get (), pointInfo.prompt, sizeof (pointInfo.prompt));
    pointInfo.enableQuickSelection = true;

    const GSErrCode err = ACAPI_UserInput_GetPoint (&pointInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "No point was given - the input was cancelled or could not start (a modal dialog or a non-model window).");
    }

    GS::ObjectState response;
    response.Add ("position", Create3DCoordinateObjectState (pointInfo.pos));
    return response;
}

// ---------------------------------------------------------------------------
// GetElementFromUser
// ---------------------------------------------------------------------------

GetElementFromUserCommand::GetElementFromUserCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementFromUserCommand::GetName () const
{
    return "GetElementFromUser";
}

GS::Optional<GS::UniString> GetElementFromUserCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "prompt": {
                "type": "string",
                "description": "Shown in the control box while Archicad waits for the click. Single-byte text: the box takes a char field. Archicad's main thread waits for the click or for Escape, and every other JSON command queues behind this one until then."
            },
            "elementTypes": {
                "type": "array",
                "description": "Only an element of one of these types is accepted. A click on empty space or on an element of another type asks for the click again. Default: any element.",
                "items": {
                    "$ref": "#/ElementType"
                }
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetElementFromUserCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementId": {
                "$ref": "#/ElementId",
                "description": "The clicked element. In a section or elevation window it is the model element the section element was cut from."
            },
            "elementType": {
                "type": "string",
                "description": "The type of the clicked element (one of the ElementType values, or Unknown)."
            },
            "position": {
                "$ref": "#/Coordinate3D",
                "description": "The clicked point in the project's coordinates, as GetPointFromUser returns it."
            },
            "elementPosition": {
                "$ref": "#/Coordinate3D",
                "description": "The coordinate of the clicked part of the element (e.g. its node or hotspot), when Archicad gives one for it."
            },
            "cancelled": {
                "type": "boolean",
                "description": "True when the input was cancelled with Escape; no other field is returned then."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::ObjectState GetElementFromUserCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // Same input as GetPointFromUser, but the element under the click is returned too.
    // A click that hits no element, or an element of a type not asked for, asks again.
    GS::UniString prompt = "Click an element";
    parameters.Get ("prompt", prompt);

    GS::Array<GS::UniString> elementTypeNames;
    parameters.Get ("elementTypes", elementTypeNames);
    GS::Array<API_ElemTypeID> elementTypes;
    for (const GS::UniString& elementTypeName : elementTypeNames) {
        const API_ElemTypeID elementType = GetElementTypeFromNonLocalizedName (elementTypeName);
        if (elementType == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown element type: ") + elementTypeName);
        }
        elementTypes.Push (elementType);
    }

    for (;;) {
        API_GetPointType pointInfo = {};
        CHTruncate (prompt.ToCStr ().Get (), pointInfo.prompt, sizeof (pointInfo.prompt));
        pointInfo.enableQuickSelection = true;

        const GSErrCode err = ACAPI_UserInput_GetPoint (&pointInfo);
        if (err == APIERR_CANCEL) {
            return GS::ObjectState ("cancelled", true);
        }
        if (err != NoError) {
            return CreateErrorResponse (err, "No element was given - the input could not start (a modal dialog or a non-model window).");
        }

        API_Guid elemGuid = APINULLGuid;
        if (ACAPI_Selection_GetSelectedElement (&pointInfo.neig, &elemGuid) != NoError || elemGuid == APINULLGuid) {
            elemGuid = pointInfo.neig.guid;
        }
        if (elemGuid == APINULLGuid) {
            continue;
        }
        elemGuid = GetParentElemOfSectElem (elemGuid);

        API_Elem_Head elemHead = {};
        elemHead.guid = elemGuid;
        if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
            continue;
        }
        const API_ElemTypeID elementType = GetElemTypeId (elemHead);
        if (!elementTypes.IsEmpty () && !elementTypes.Contains (elementType)) {
            continue;
        }

        GS::ObjectState response;
        response.Add ("elementId", CreateGuidObjectState (elemGuid));
        response.Add ("elementType", GetElementTypeNonLocalizedName (elementType));
        response.Add ("position", Create3DCoordinateObjectState (pointInfo.pos));
        API_Coord3D elementPosition = {};
        if (ACAPI_Element_NeigToCoord (&pointInfo.neig, &elementPosition) == NoError) {
            response.Add ("elementPosition", Create3DCoordinateObjectState (elementPosition));
        }
        return response;
    }
}

