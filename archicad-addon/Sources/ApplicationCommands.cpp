#include "ApplicationCommands.hpp"
#include "ObjectState.hpp"
#include "FileSystem.hpp"
#include "AddOnVersion.hpp"
#include "MigrationHelper.hpp"
#include "DGModule.hpp"
#include "Folder.hpp"

#include <cstdlib>

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

#if defined (ServerMainVers_2800) && __has_include ("ACAPI/LicenseInfo/ProductVersionInfo.hpp")
#include "ACAPI/LicenseInfo/ProductVersionInfo.hpp"
#endif

#if defined (ServerMainVers_2900) && __has_include ("ACAPI/DomainAvailability.hpp")
#include "ACAPI/DomainAvailability.hpp"
#define TAPIR_HAS_DOMAIN_AVAILABILITY
#endif

#if defined (ServerMainVers_2900) && __has_include ("ACAPI/ProjectID.hpp")
#include "ACAPI/ProjectID.hpp"
#define TAPIR_HAS_CEIP_PROJECT_ID
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

GetPrinterAndPlotterInfoCommand::GetPrinterAndPlotterInfoCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetPrinterAndPlotterInfoCommand::GetName () const
{
    return "GetPrinterAndPlotterInfo";
}

GS::Optional<GS::UniString> GetPrinterAndPlotterInfoCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "printer": {
                "type": "object",
                "description": "The printer selected in Archicad, or an error when it cannot be read.",
                "oneOf": [
                    {
                        "type": "object",
                        "properties": {
                            "name": {
                                "type": "string",
                                "description": "The name of the selected printer."
                            },
                            "portrait": {
                                "type": "boolean",
                                "description": "The selected orientation: true for portrait, false for landscape."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "name",
                            "portrait"
                        ]
                    },
                    {
                        "$ref": "#/ErrorItem"
                    }
                ]
            },
            "plotter": {
                "type": "object",
                "description": "The plotter selected in Archicad, or an error when it cannot be read.",
                "oneOf": [
                    {
                        "type": "object",
                        "properties": {
                            "name": {
                                "type": "string",
                                "description": "The name of the selected plotter."
                            },
                            "model": {
                                "type": "string",
                                "description": "The model of the selected plotter."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "name",
                            "model"
                        ]
                    },
                    {
                        "$ref": "#/ErrorItem"
                    }
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "printer",
            "plotter"
        ]
    })";
}

GS::ObjectState GetPrinterAndPlotterInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState response;

    API_MiscPrinterInfo printerInfo = {};
    GSErrCode err = ACAPI_ProjectOperation_GetMiscPrinterInfo (&printerInfo);
    if (err == NoError) {
        response.Add ("printer", GS::ObjectState (
            "name", GS::UniString (printerInfo.name),
            "portrait", printerInfo.orientPortrait));
    } else {
        response.Add ("printer", CreateErrorResponse (err, "Failed to get the printer info."));
    }

    API_MiscPlotterInfo plotterInfo = {};
    err = ACAPI_ProjectOperation_GetMiscPlotterInfo (&plotterInfo);
    if (err == NoError) {
        response.Add ("plotter", GS::ObjectState (
            "name", GS::UniString (plotterInfo.name),
            "model", GS::UniString (plotterInfo.model)));
    } else {
        response.Add ("plotter", CreateErrorResponse (err, "Failed to get the plotter info."));
    }

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

        // GoToView rejects master layout items of the Layout Book with APIERR_GENERAL
        // (#1481), while switching to their database works. Layout Book items carry no
        // view settings to apply, so for them a plain database/window switch is the same
        // as opening the item in the Navigator.
        if (err != NoError && (navigatorItem.itemType == API_LayoutNavItem || navigatorItem.itemType == API_MasterLayoutNavItem)) {
            API_DatabaseInfo targetDatabase = navigatorItem.db;
            err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
            if (err == NoError) {
                err = ACAPI_Database_ChangeCurrentDatabase (&targetDatabase);
            }
            if (err == NoError) {
                API_WindowInfo windowInfo = targetDatabase;
                err = ACAPI_Window_ChangeWindow (&windowInfo);
            }
        }
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

CloseWindowsCommand::CloseWindowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CloseWindowsCommand::GetName () const
{
    return "CloseWindows";
}

GS::Optional<GS::UniString> CloseWindowsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "windows": {
                "type": "array",
                "description": "The windows to close.",
                "items": {
                    "type": "object",
                    "properties": {
                        "databaseId": {
                            "$ref": "#/DatabaseId",
                            "description": "The database shown in the window, as GetDatabases, GetContainingDatabases or GetDatabaseIdFromNavigatorItemId return it."
                        },
                        "windowType": {
                            "$ref": "#/WindowType",
                            "description": "Optional. When given, the window is only closed when its database is of this type."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "databaseId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "windows"
        ]
    })";
}

GS::Optional<GS::UniString> CloseWindowsCommand::GetRawResponseSchema () const
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

GS::ObjectState CloseWindowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> windows;
    parameters.Get ("windows", windows);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& window : windows) {
        const GS::ObjectState* databaseId = window.Get ("databaseId");
        if (databaseId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "databaseId is missing."));
            continue;
        }

        // Resolves the database the same way ChangeWindow does.
        API_WindowInfo windowInfo = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*databaseId));
        GSErrCode err = ACAPI_Window_GetDatabaseInfo (&windowInfo);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to resolve the database."));
            continue;
        }

        GS::UniString windowTypeStr;
        if (window.Get ("windowType", windowTypeStr) && ConvertStringToWindowType (windowTypeStr) != windowInfo.typeID) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "databaseId does not belong to the requested windowType."));
            continue;
        }

        err = ACAPI_Window_CloseWindow (&windowInfo);
        executionResults (err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to close the window."));
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "CloseWindows requires Archicad 27 or later.");
#endif
}

RebuildCurrentDatabaseCommand::RebuildCurrentDatabaseCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RebuildCurrentDatabaseCommand::GetName () const
{
    return "RebuildCurrentDatabase";
}

GS::Optional<GS::UniString> RebuildCurrentDatabaseCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState RebuildCurrentDatabaseCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Database_RebuildCurrentDatabase ();
    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to rebuild the current database.");
#else
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "RebuildCurrentDatabase requires Archicad 27 or later.");
#endif
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
// GetEditModeState
// ---------------------------------------------------------------------------

GetEditModeStateCommand::GetEditModeStateCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetEditModeStateCommand::GetName () const
{
    return "GetEditModeState";
}

GS::Optional<GS::UniString> GetEditModeStateCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "inEditMode": {
                "type": "boolean",
                "description": "True when Archicad is in any hierarchical editing mode (curtain wall, stair, railing, beam, column, MEP elements, etc.). Only returned from Archicad 29."
            },
            "curtainWall": {
                "$ref": "#/EditModeStateItem",
                "description": "Curtain wall editing mode."
            },
            "stair": {
                "$ref": "#/EditModeStateItem",
                "description": "Stair editing mode."
            },
            "beam": {
                "$ref": "#/EditModeStateItem",
                "description": "Beam editing mode."
            },
            "column": {
                "$ref": "#/EditModeStateItem",
                "description": "Column editing mode."
            }
        },
        "additionalProperties": false,
        "required": [
            "curtainWall",
            "stair",
            "beam",
            "column"
        ]
    })";
}

#if defined (ServerMainVers_2700)
static GS::ObjectState CreateEditModeStateItem (bool inEditMode, const API_Guid& editedElement)
{
    GS::ObjectState item ("inEditMode", inEditMode);
    if (inEditMode && editedElement != APINULLGuid) {
        item.Add ("elementId", CreateGuidObjectState (editedElement));
    }
    return item;
}

// The ACAPI_HierarchicalEditing_IsIn*EditMode functions take the flag as bool* before Archicad 29 and as bool& from Archicad 29.
#if defined (ServerMainVers_2900)
#define EDIT_MODE_FLAG(flag) (flag)
#else
#define EDIT_MODE_FLAG(flag) (&(flag))
#endif
#endif

GS::ObjectState GetEditModeStateCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#if defined (ServerMainVers_2700)
    // Read only: the edit mode is only queried, never entered, left or cancelled.
    GS::ObjectState response;
    GSErrCode err = NoError;

#if defined (ServerMainVers_2900)
    bool inAnyEditMode = false;
    err = ACAPI_HierarchicalEditing_IsInEditMode (inAnyEditMode);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to query the hierarchical editing mode.");
    }
    response.Add ("inEditMode", inAnyEditMode);
#endif

    bool inEditMode = false;
    API_Guid editedElement = APINULLGuid;
    err = ACAPI_HierarchicalEditing_IsInCurtainWallEditMode (EDIT_MODE_FLAG (inEditMode), &editedElement);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to query the curtain wall editing mode.");
    }
    response.Add ("curtainWall", CreateEditModeStateItem (inEditMode, editedElement));

    inEditMode = false;
    editedElement = APINULLGuid;
    err = ACAPI_HierarchicalEditing_IsInStairEditMode (EDIT_MODE_FLAG (inEditMode), &editedElement);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to query the stair editing mode.");
    }
    response.Add ("stair", CreateEditModeStateItem (inEditMode, editedElement));

    inEditMode = false;
    editedElement = APINULLGuid;
    err = ACAPI_HierarchicalEditing_IsInBeamEditMode (EDIT_MODE_FLAG (inEditMode), &editedElement);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to query the beam editing mode.");
    }
    response.Add ("beam", CreateEditModeStateItem (inEditMode, editedElement));

    inEditMode = false;
    editedElement = APINULLGuid;
    err = ACAPI_HierarchicalEditing_IsInColumnEditMode (EDIT_MODE_FLAG (inEditMode), &editedElement);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to query the column editing mode.");
    }
    response.Add ("column", CreateEditModeStateItem (inEditMode, editedElement));

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "GetEditModeState requires Archicad 27 or later.");
#endif
}

// ---------------------------------------------------------------------------
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
            },
            "availableDomains": {
                "type": "array",
                "items": {
                    "type": "string",
                    "enum": ["Architecture", "MEP"]
                },
                "description": "The domains available in the running Archicad. Archicad 29 or later."
            },
            "ceipProjectId": {
                "type": "string",
                "description": "The CEIP project identifier of the open project. Left out when no saved project is open. Archicad 29 or later."
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

    const ACAPI::LicenseInfo::ProductVersionInfo versionInfo = licenseInfoManager->GetProductVersionInfo ();

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
#ifdef TAPIR_HAS_DOMAIN_AVAILABILITY
    const auto& availableDomains = response.AddList<GS::UniString> ("availableDomains");
    if (ACAPI::IsDomainAvailable (ACAPI::Domain::Architecture)) {
        availableDomains (GS::UniString ("Architecture"));
    }
    if (ACAPI::IsDomainAvailable (ACAPI::Domain::MEP)) {
        availableDomains (GS::UniString ("MEP"));
    }
#endif
#ifdef TAPIR_HAS_CEIP_PROJECT_ID
    API_ProjectInfo projectInfo = {};
    if (ACAPI_ProjectOperation_Project (&projectInfo) == NoError && !projectInfo.untitled) {
        const IO::Location* projectLocation = projectInfo.teamwork ? projectInfo.location_team : projectInfo.location;
        const GS::UniString ceipProjectId = projectLocation != nullptr ? ACAPI::GetCEIPProjectID (*projectLocation) : GS::UniString ();
        if (!ceipProjectId.IsEmpty ()) {
            response.Add ("ceipProjectId", ceipProjectId);
        }
    }
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

SetDialogAutoCloseCommand::SetDialogAutoCloseCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetDialogAutoCloseCommand::GetName () const
{
    return "SetDialogAutoClose";
}

GS::Optional<GS::UniString> SetDialogAutoCloseCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "enabled": {
                "type": "boolean",
                "description": "True turns the dialog auto close on: Archicad closes the dialogs it would show with their default button instead of waiting for the user. False turns it off."
            }
        },
        "additionalProperties": false,
        "required": [
            "enabled"
        ]
    })";
}

GS::Optional<GS::UniString> SetDialogAutoCloseCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetDialogAutoCloseCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool enabled = false;
    parameters.Get ("enabled", enabled);

    if (enabled) {
        DGEnableAutoClose ();
    } else {
        DGDisableAutoClose ();
    }

    if (DGIsAutoCloseEnabled () != enabled) {
        return CreateFailedExecutionResult (APIERR_GENERAL, enabled ? "Failed to turn the dialog auto close on." : "Failed to turn the dialog auto close off.");
    }

    return CreateSuccessfulExecutionResult ();
}

void ApplyDialogAutoCloseFromEnvironment ()
{
    const char* value = std::getenv ("TAPIR_DIALOG_AUTOCLOSE");
    if (value == nullptr) {
        return;
    }

    if (GS::UniString (value) == "true") {
        DGEnableAutoClose ();
    }
}
