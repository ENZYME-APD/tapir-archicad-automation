#include "ApplicationCommands.hpp"
#include "ObjectState.hpp"
#include "FileSystem.hpp"
#include "AddOnVersion.hpp"
#include "MigrationHelper.hpp"
#include "DGModule.hpp"
#include "Folder.hpp"
#include "Base64Converter.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>

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

#if defined (ServerMainVers_2900) && __has_include ("ACAPI/UI/NotificationBubble/NotificationBubbleManager.hpp")
#include "ACAPI/UI/NotificationBubble/NotificationBubbleManager.hpp"
#include "ACAPI/UI/NotificationBubble/NotificationBubble.hpp"
#include "DGUtility.hpp"
#define TAPIR_HAS_NOTIFICATION_BUBBLE
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

GetArchicadLicenseInfoCommand::GetArchicadLicenseInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetArchicadLicenseInfoCommand::GetName () const
{
    return "GetArchicadLicenseInfo";
}

GS::Optional<GS::UniString> GetArchicadLicenseInfoCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "licenseType": {
                "type": "string",
                "enum": [
                    "Full",
                    "Education",
                    "Trial",
                    "Demo",
                    "Start",
                    "BIMcloudTeamClient"
                ],
                "description": "The kind of license Archicad runs on, read from protectionMode: Trial, Demo, Education (an education license with or without a key), BIMcloudTeamClient, Start (the Start edition), otherwise Full."
            },
            "protectionMode": {
                "type": "integer",
                "description": "The raw protection mode bit mask reported by Archicad (the APIPROT_*_MASK bits of the Archicad API)."
            },
            "licenseFlags": {
                "type": "array",
                "items": {
                    "type": "string",
                    "enum": [
                        "Site",
                        "NotForResale",
                        "School",
                        "PayPerUse",
                        "Network",
                        "Teamwork",
                        "Education",
                        "Start",
                        "Solo",
                        "LiveSubscription",
                        "OEM",
                        "SSA",
                        "EducationWithoutKey",
                        "Demo",
                        "Trial",
                        "BIMcloudTeamClient"
                    ]
                },
                "description": "The bits set in protectionMode, by name: Site (more licenses), NotForResale, School, PayPerUse, Network (network licensing), Teamwork (Teamwork functionality is on), Education (education key), Start (Start edition), Solo (SOLO edition), LiveSubscription, OEM (Archicad 27 or later), SSA (SSA license, Archicad 25 only), and without a hardware or software key EducationWithoutKey, Demo, Trial and BIMcloudTeamClient."
            },
            "ssaLevel": {
                "type": "string",
                "enum": [
                    "NoSSA",
                    "SSA",
                    "Forward"
                ],
                "description": "The highest Software Service Agreement (SSA) level the license has: NoSSA, SSA or Forward. A script needing a level checks that this is at least that level, in this order. Missing in Archicad 25, which cannot report it."
            }
        },
        "additionalProperties": false,
        "required": [
            "licenseType",
            "protectionMode",
            "licenseFlags"
        ]
    })";
}

static UInt32 GetArchicadProtectionMode ()
{
#if defined (ServerMainVers_2700)
    return ACAPI_Licensing_GetProtectionMode ();
#elif defined (ServerMainVers_2600)
    return ACAPI_Protection_GetProtectionMode ();
#else
    API_ProtectionInfo protectionInfo;
    GS::UniString configurationNumber;
    if (ACAPI_Environment (APIEnv_ProtectionDataID, &protectionInfo, &configurationNumber) != NoError) {
        return 0;
    }
    return protectionInfo.protMode;
#endif
}

#if defined (ServerMainVers_2600)
static bool IsArchicadSSALevelAtLeast (API_SSALevel level)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Licensing_IsSSALevelAtLeast (level);
#else
    return ACAPI_Protection_IsSSALevelAtLeast (level);
#endif
}
#endif

GS::ObjectState GetArchicadLicenseInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    const UInt32 protectionMode = GetArchicadProtectionMode ();

    GS::UniString licenseType = "Full";
    if ((protectionMode & APIPROT_TRIAL_MASK) != 0) {
        licenseType = "Trial";
    } else if ((protectionMode & APIPROT_DEMO_MASK) != 0) {
        licenseType = "Demo";
    } else if ((protectionMode & (APIPROT_EDU_MASK | APIPROT_PROTEDU_MASK)) != 0) {
        licenseType = "Education";
    } else if ((protectionMode & APIPROT_BTC_MASK) != 0) {
        licenseType = "BIMcloudTeamClient";
    } else if ((protectionMode & APIPROT_START_MASK) != 0) {
        licenseType = "Start";
    }

    const struct {
        UInt32      mask;
        const char* name;
    } flagNames[] = {
        { APIPROT_SITE_MASK, "Site" },
        { APIPROT_NFR_MASK, "NotForResale" },
        { APIPROT_SCHOOL_MASK, "School" },
        { APIPROT_PPU_MASK, "PayPerUse" },
        { APIPROT_NET_MASK, "Network" },
        { APIPROT_TW_MASK, "Teamwork" },
        { APIPROT_PROTEDU_MASK, "Education" },
        { APIPROT_START_MASK, "Start" },
        { APIPROT_SOLO_MASK, "Solo" },
        { APIPROT_LIVE_MASK, "LiveSubscription" },
#ifdef APIPROT_OEM_MASK
        { APIPROT_OEM_MASK, "OEM" },
#endif
#ifdef APIPROT_SSA_MASK
        { APIPROT_SSA_MASK, "SSA" },
#endif
        { APIPROT_EDU_MASK, "EducationWithoutKey" },
        { APIPROT_DEMO_MASK, "Demo" },
        { APIPROT_TRIAL_MASK, "Trial" },
        { APIPROT_BTC_MASK, "BIMcloudTeamClient" },
    };

    GS::Array<GS::UniString> licenseFlags;
    for (const auto& flag : flagNames) {
        if ((protectionMode & flag.mask) != 0) {
            licenseFlags.Push (flag.name);
        }
    }

    GS::ObjectState response;
    response.Add ("licenseType", licenseType);
    response.Add ("protectionMode", static_cast<Int64> (protectionMode));
    response.Add ("licenseFlags", licenseFlags);

#if defined (ServerMainVers_2600)
    GS::UniString ssaLevel = "NoSSA";
    if (IsArchicadSSALevelAtLeast (API_SSALevels::Forward)) {
        ssaLevel = "Forward";
    } else if (IsArchicadSSALevelAtLeast (API_SSALevels::SSA)) {
        ssaLevel = "SSA";
    }
    response.Add ("ssaLevel", ssaLevel);
#endif

    return response;
}

GetPrinterInfoCommand::GetPrinterInfoCommand () :
    CommandBase (CommonSchema::Used)
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
                "$ref": "#/PrinterPrintArea"
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
                        "$ref": "#/PrinterInfo"
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
                        "$ref": "#/PlotterInfo"
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
            },
            "databaseId": {
                "$ref": "#/DatabaseId",
                "description": "The database shown in the current (active) window, the same id GetDatabases and GetContainingDatabases return. Pass it to ChangeWindow to bring this window back. Missing when the window has no database of its own that can be identified, for example a report, a rendering or a custom add-on window. The saved view (View Map item) the window was opened from is not returned: Archicad does not tell which one it was, and several saved views can show the same database."
            }
        },
        "additionalProperties": false,
        "required": [
            "currentWindowType"
        ]
    })";
}

GS::UniString ConvertWindowTypeToString (API_WindowTypeID type)
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
    API_WindowInfo windowInfo = {};
    GSErrCode err = ACAPI_Window_GetCurrentWindow (&windowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current window!");
    }
    GS::ObjectState response ("currentWindowType", ConvertWindowTypeToString (windowInfo.typeID));

    // The database of the active window, not the current database, which an earlier
    // command may have switched (#1708). The floor plan and the 3D model are identified
    // by their type, the other databases by their unique id; custom add-on windows only
    // by an index, so they, and windows without a database (reports, renderings, ...),
    // get no databaseId instead of a made-up one.
    if (windowInfo.typeID != APIWind_MyDrawID && windowInfo.typeID != APIWind_MyTextID) {
        const API_Guid databaseId = DatabaseIdResolver::Instance ().GetIdOfDatabase (windowInfo);
        if (databaseId != APINULLGuid) {
            response.Add ("databaseId", CreateGuidObjectState (databaseId));
        }
    }
    return response;
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
                return CreateFailedExecutionResult (APIERR_BADPARS, "navigatorItemId must be a view or viewpoint; got a container or folder. To open the Session Report, pass windowType 'Report' instead.");
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

    // The Session Report is not a database, so ACAPI_Window_ChangeWindow fails on it, even
    // when its window is already open (#1707). It is opened with its own API function.
    if (windowInfo.typeID == APIWind_ReportID && databaseId == nullptr && !parameters.Contains ("storyIndex")) {
#if defined (ServerMainVers_2700)
        const GSErrCode err = ACAPI_Dialog_ActivateSessionReport ();
#elif defined (ServerMainVers_2600)
        const GSErrCode err = ACAPI_Goodies (APIAny_ActivateSessionReportID);
#else
        const GSErrCode err = APIERR_NOTSUPPORTED;
#endif
        return err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateErrorResponse (err, "Failed to activate the Session Report window.");
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
        const API_DatabaseInfo resolvedDatabase = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*databaseId));
        API_DatabaseInfo targetDatabase = resolvedDatabase;
        GSErrCode err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
        if (err != NoError && hasWindowType && resolvedDatabase.typeID == API_ZombieWindowID) {
            // GetDatabaseWithId only recognizes the FloorPlan/3DModel sentinel guids; for
            // every other database kind it leaves typeID unset and only fills
            // databaseUnId.elemSetId, with which ACAPI_Window_GetDatabaseInfo can fail with
            // APIERR_BADDATABASE (seen for Sections and Elevations). Retry with the type
            // windowType gives, the same way DocumentCreationCommands does for
            // Layout/MasterLayout. Only as a retry, so the windowType check below still
            // catches a wrong windowType whenever the database resolves on its own.
            targetDatabase = resolvedDatabase;
            targetDatabase.typeID = windowInfo.typeID;
            err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
        }
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to resolve target database.");
        }

        if (hasWindowType && targetDatabase.typeID != windowInfo.typeID) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "databaseId does not belong to the requested windowType.");
        }

        err = ACAPI_Database_ChangeCurrentDatabase (&targetDatabase);
        if (err != NoError) {
#if defined (ServerMainVers_2700)
            // A worksheet or detail whose window was never opened, e.g. one just made by
            // CreateWorksheets or CreateDetails, cannot be made the current database
            // (APIERR_BADDATABASE), while activating its Project Map item works (#1602). So that
            // item is opened instead, the same way as with navigatorItemId.
            const GS::Optional<API_Guid> projectMapItem = FindProjectMapItemOfWorksheetOrDetail (targetDatabase);
            if (projectMapItem.HasValue ()) {
                const GS::UniString guidStr = APIGuidToString (projectMapItem.Get ());
                const GSErrCode goToViewErr = ACAPI_View_GoToView (guidStr.ToCStr ().Get ());
                return goToViewErr == NoError
                    ? CreateSuccessfulExecutionResult ()
                    : CreateErrorResponse (goToViewErr, "Failed to change current database, and failed to open the database's Project Map item.");
            }
#endif
            // Any other viewpoint whose window was never opened (e.g. an Elevation just created
            // with CreateElevations) has no database to switch to yet: ChangeCurrentDatabase
            // fails with APIERR_BADDATABASE until the window exists. Open the window first,
            // then retry.
            if (ACAPI_Window_ChangeWindow (&targetDatabase) == NoError) {
                err = ACAPI_Database_ChangeCurrentDatabase (&targetDatabase);
            }
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to change current database.");
            }
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
                            "$ref": "#/ContainingDatabase"
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
                    "$ref": "#/WindowToClose"
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

DeleteGhostRecordsCommand::DeleteGhostRecordsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteGhostRecordsCommand::GetName () const
{
    return "DeleteGhostRecords";
}

GS::Optional<GS::UniString> DeleteGhostRecordsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "databases": {
                "$ref": "#/Databases",
                "description": "The parent databases whose trace (ghost) record is deleted, as returned by GetDatabaseIdFromNavigatorItemId or GetContainingDatabases."
            }
        },
        "additionalProperties": false,
        "required": [
            "databases"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteGhostRecordsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteGhostRecordsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> databases;
    parameters.Get ("databases", databases);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& database : databases) {
        const API_Guid databaseId = GetGuidFromDatabaseArrayItem (database);
        if (databaseId == APINULLGuid) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "databaseId is corrupt or missing."));
            continue;
        }

        API_DatabaseInfo dbInfo = DatabaseIdResolver::Instance ().GetDatabaseWithId (databaseId);
        GSErrCode err = ACAPI_Window_GetDatabaseInfo (&dbInfo);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to resolve the database."));
            continue;
        }

        API_GhostRecord ghostRecord {};
        ghostRecord.parentDatabase = dbInfo.databaseUnId;
        err = ACAPI_View_GetGhostRecord (&dbInfo.databaseUnId, &ghostRecord);
        if (err == APIERR_WINDNOTEXIST) {
            executionResults (CreateFailedExecutionResult (err, "The database has no trace reference."));
            continue;
        }
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to get the trace record of the database."));
            continue;
        }

        // Deleting the ghost record is not undoable, so it is not wrapped in ACAPI_CallUndoableCommand.
        err = ACAPI_View_DeleteGhostRecord (&ghostRecord);
        executionResults (err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to delete the trace record of the database."));
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

ShowNotificationBubbleCommand::ShowNotificationBubbleCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ShowNotificationBubbleCommand::GetName () const
{
    return "ShowNotificationBubble";
}

GS::Optional<GS::UniString> ShowNotificationBubbleCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "message": {
                "type": "string",
                "description": "The text of the notification bubble.",
                "minLength": 1
            },
            "icon": {
                "type": "string",
                "enum": ["none", "information", "warning", "error"],
                "description": "The icon shown in the bubble. Default: information."
            },
            "timeoutMilliseconds": {
                "type": "integer",
                "description": "How long the bubble stays, in milliseconds. Default: Archicad's default, 1000 (1 second).",
                "minimum": 1,
                "maximum": 4294967295
            },
            "coordinate": {
                "$ref": "#/Coordinate3D",
                "description": "The model coordinate in the current database (meters) where the bubble is shown, converted like ModelToScreen does: in a 3D window it is projected with the current camera, in other windows z is disregarded. Do not give it together with screenPoint. When neither is given, the bubble is shown at the center of the current view in 2D windows and at the project's local origin otherwise."
            },
            "screenPoint": {
                "type": "object",
                "description": "The pixel position local to the current Archicad view window (not global desktop coordinates) where the bubble is shown, e.g. a point returned by ModelToScreen. Do not give it together with coordinate.",
                "properties": {
                    "h": {
                        "type": "integer",
                        "description": "Horizontal pixel position from the left edge of the view window.",
                        "minimum": -32768,
                        "maximum": 32767
                    },
                    "v": {
                        "type": "integer",
                        "description": "Vertical pixel position from the top edge of the view window.",
                        "minimum": -32768,
                        "maximum": 32767
                    }
                },
                "additionalProperties": false,
                "required": [
                    "h",
                    "v"
                ]
            },
            "offset": {
                "type": "object",
                "description": "Moves the bubble from its position by this many pixels.",
                "properties": {
                    "h": {
                        "type": "integer",
                        "description": "Horizontal offset in pixels, positive to the right.",
                        "minimum": -32768,
                        "maximum": 32767
                    },
                    "v": {
                        "type": "integer",
                        "description": "Vertical offset in pixels, positive downwards.",
                        "minimum": -32768,
                        "maximum": 32767
                    }
                },
                "additionalProperties": false,
                "required": [
                    "h",
                    "v"
                ]
            },
            "largeIcon": {
                "type": "string",
                "enum": ["information", "warning", "error"],
                "description": "The icon shown instead of icon when the \"Use large alert cues\" option is on in Work Environment. Not set by default."
            },
            "redrawCountLimit": {
                "type": "integer",
                "description": "Shows the bubble at most this many times. After that it does not appear again, for any later ShowNotificationBubble call, until the user runs the 'Enable all informational messages' command in Archicad. Not set by default.",
                "minimum": 1,
                "maximum": 4294967295
            },
            "disabled": {
                "type": "boolean",
                "description": "True turns the bubble off instead of showing it: it does not appear again, for any later ShowNotificationBubble call, until the user runs the 'Enable all informational messages' command in Archicad. False (the default) changes nothing."
            }
        },
        "additionalProperties": false,
        "required": [
            "message"
        ]
    })";
}

GS::Optional<GS::UniString> ShowNotificationBubbleCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "description": "The settings of the bubble after the call, read back from Archicad.",
        "properties": {
            "text": {
                "type": "string",
                "description": "The text of the bubble. Missing when it has none."
            },
            "icon": {
                "type": "string",
                "enum": ["none", "information", "warning", "error", "other"],
                "description": "The icon of the bubble."
            },
            "largeIcon": {
                "type": "string",
                "enum": ["none", "information", "warning", "error", "other"],
                "description": "The icon shown when the \"Use large alert cues\" option is on in Work Environment."
            },
            "offset": {
                "type": "object",
                "description": "The offset of the bubble from its position, in pixels.",
                "properties": {
                    "h": {
                        "type": "integer"
                    },
                    "v": {
                        "type": "integer"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "h",
                    "v"
                ]
            },
            "timeoutMilliseconds": {
                "type": "integer",
                "description": "How long the bubble stays, in milliseconds."
            },
            "redrawCountLimit": {
                "type": "integer",
                "description": "How many times the bubble is shown at most. Missing when it has no limit."
            }
        },
        "additionalProperties": false,
        "required": [
            "icon",
            "largeIcon",
            "offset",
            "timeoutMilliseconds"
        ]
    })";
}

#ifdef TAPIR_HAS_NOTIFICATION_BUBBLE
// One bubble of Tapir's own; showing it again replaces its text and settings.
static constexpr const char* TapirNotificationBubbleGuid = "8D3B6F1E-5C2A-4E7B-9A41-2F6C0D8E7B53";

static GSErrCode GetIconIdFromString (const GS::UniString& iconStr, std::optional<short>& iconId)
{
    if (iconStr == "information") {
        iconId = DG::Icon::Information;
    } else if (iconStr == "warning") {
        iconId = DG::Icon::Warning;
    } else if (iconStr == "error") {
        iconId = DG::Icon::Error;
    } else if (iconStr == "none") {
        iconId.reset ();
    } else {
        return APIERR_BADPARS;
    }
    return NoError;
}

static GS::UniString GetIconString (const std::optional<DG::Icon>& icon)
{
    if (!icon.has_value () || icon->IsEmpty ()) {
        return "none";
    }
    // ShowNotificationBubble only sets these system icons.
    switch (icon->GetResourceId ()) {
        case DG::Icon::Information: return "information";
        case DG::Icon::Warning:     return "warning";
        case DG::Icon::Error:       return "error";
        default:                    return "other";
    }
}

static API_Point GetPointFromObjectState (const GS::ObjectState& os)
{
    Int32 h = 0;
    Int32 v = 0;
    os.Get ("h", h);
    os.Get ("v", v);
    API_Point point = {};
    point.h = static_cast<short> (h);
    point.v = static_cast<short> (v);
    return point;
}

static GSErrCode GetModelCoordinateScreenPoint (const API_Coord3D& coord3D, API_Point& point)
{
    API_Coord coord = {};
    GSErrCode err = ACAPI_View_ModelToScreen (coord3D, coord);
    if (err != NoError) {
        return err;
    }
    return ACAPI_View_CoordToPoint (&coord, &point);
}
#endif

GS::ObjectState ShowNotificationBubbleCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef TAPIR_HAS_NOTIFICATION_BUBBLE
    GS::UniString message;
    parameters.Get ("message", message);
    if (message.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "message must not be empty.");
    }

    GS::UniString iconStr = "information";
    parameters.Get ("icon", iconStr);
    std::optional<short> iconId;
    if (GetIconIdFromString (iconStr, iconId) != NoError) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown icon: ") + iconStr + ".");
    }

    std::optional<short> largeIconId;
    GS::UniString largeIconStr;
    if (parameters.Get ("largeIcon", largeIconStr)) {
        if (largeIconStr == "none" || GetIconIdFromString (largeIconStr, largeIconId) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown largeIcon: ") + largeIconStr + ".");
        }
    }

    GS::Int64 redrawCountLimit = 0;
    const bool hasRedrawCountLimit = parameters.Get ("redrawCountLimit", redrawCountLimit);
    if (hasRedrawCountLimit && (redrawCountLimit < 1 || redrawCountLimit > 4294967295LL)) {
        return CreateErrorResponse (APIERR_BADPARS, "redrawCountLimit must be between 1 and 4294967295.");
    }

    bool disabled = false;
    parameters.Get ("disabled", disabled);

    const GS::ObjectState* coordinateOS = parameters.Get ("coordinate");
    const GS::ObjectState* screenPointOS = parameters.Get ("screenPoint");
    if (coordinateOS != nullptr && screenPointOS != nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Give either coordinate or screenPoint, not both.");
    }

    API_Point point = {};
    if (screenPointOS != nullptr) {
        point = GetPointFromObjectState (*screenPointOS);
    } else if (coordinateOS != nullptr) {
        const GSErrCode err = GetModelCoordinateScreenPoint (Get3DCoordinateFromObjectState (*coordinateOS), point);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to convert the coordinate to a screen point of the current view.");
        }
    } else {
        // The center of the visible area of a 2D window; ACAPI_View_GetZoom fails in
        // other windows (e.g. 3D), where the local origin is used as in the DevKit example.
        API_Box zoomBox = {};
        GSErrCode err = ACAPI_View_GetZoom (&zoomBox);
        if (err == NoError) {
            API_Coord center = {};
            center.x = (zoomBox.xMin + zoomBox.xMax) / 2.0;
            center.y = (zoomBox.yMin + zoomBox.yMax) / 2.0;
            err = ACAPI_View_CoordToPoint (&center, &point);
        } else {
            API_Coord3D localOrigin = {};
            err = ACAPI_Database_GetLocOrigo (&localOrigin);
            if (err == NoError) {
                err = GetModelCoordinateScreenPoint (localOrigin, point);
            }
        }
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to find a position in the current view; give coordinate or screenPoint.");
        }
    }

    auto notificationBubble = ACAPI::UI::Notification::CreateNotificationBubbleManager ().CreateNotificationBubble (APIGuidFromString (TapirNotificationBubbleGuid));

    notificationBubble.SetText (message);
    if (iconId.has_value ()) {
        notificationBubble.SetIcon (DG::Icon (SysResModule, *iconId));
    }

    GS::Int64 timeoutMilliseconds = 0;
    if (parameters.Get ("timeoutMilliseconds", timeoutMilliseconds) && timeoutMilliseconds > 0) {
        notificationBubble.SetTimeout (static_cast<UInt32> (timeoutMilliseconds));
    }

    const GS::ObjectState* offsetOS = parameters.Get ("offset");
    if (offsetOS != nullptr) {
        notificationBubble.SetOffset (GetPointFromObjectState (*offsetOS));
    }

    if (largeIconId.has_value ()) {
        notificationBubble.SetLargeIcon (DG::Icon (SysResModule, *largeIconId));
    }

    if (hasRedrawCountLimit) {
        notificationBubble.SetRedrawCountLimit (static_cast<UInt32> (redrawCountLimit));
    }

    if (disabled) {
        notificationBubble.SetDisabled ();
    } else {
        notificationBubble.Show (point);
    }

    GS::ObjectState response;
    const std::optional<GS::UniString> text = notificationBubble.GetText ();
    if (text.has_value ()) {
        response.Add ("text", *text);
    }
    response.Add ("icon", GetIconString (notificationBubble.GetIcon ()));
    response.Add ("largeIcon", GetIconString (notificationBubble.GetLargeIcon ()));
    const API_Point offset = notificationBubble.GetOffset ();
    GS::ObjectState offsetResponse;
    offsetResponse.Add ("h", static_cast<Int32> (offset.h));
    offsetResponse.Add ("v", static_cast<Int32> (offset.v));
    response.Add ("offset", offsetResponse);
    response.Add ("timeoutMilliseconds", static_cast<GS::Int64> (notificationBubble.GetTimeout ()));
    const std::optional<UInt32> effectiveRedrawCountLimit = notificationBubble.GetRedrawCountLimit ();
    if (effectiveRedrawCountLimit.has_value ()) {
        response.Add ("redrawCountLimit", static_cast<GS::Int64> (*effectiveRedrawCountLimit));
    }
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "ShowNotificationBubble requires Archicad 29 or later.");
#endif
}

void RegisterTapirNotificationBubble ()
{
#ifdef TAPIR_HAS_NOTIFICATION_BUBBLE
    ACAPI::UI::Notification::CreateNotificationBubbleManager ().RegisterNotificationBubble (APIGuidFromString (TapirNotificationBubbleGuid));
#endif
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
                    "$ref": "#/ActionCenterCheck"
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
}

// ---------------------------------------------------------------------------
// GetProductVersionInfo
// ---------------------------------------------------------------------------

GetProductVersionInfoCommand::GetProductVersionInfoCommand () :
    CommandBase (CommonSchema::Used)
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
                    "$ref": "#/ProductDomain"
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
// Snap filter of the Get...FromUser commands (#/SnapFilter)
// ---------------------------------------------------------------------------

// The #/SnapType names. Every API_NeigID here exists in Archicad 25 to 30.
static const std::pair<const char*, API_NeigID> SnapTypeNames[] = {
    { "Wall",          APINeig_Wall },
    { "WallOn",        APINeig_WallOn },
    { "Column",        APINeig_Colu },
    { "ColumnOn",      APINeig_ColuOn },
    { "Beam",          APINeig_Beam },
    { "BeamOn",        APINeig_BeamOn },
    { "Window",        APINeig_Wind },
    { "WindowOn",      APINeig_WindOn },
    { "Door",          APINeig_Door },
    { "DoorOn",        APINeig_DoorOn },
    { "Object",        APINeig_Symb },
    { "ObjectOn",      APINeig_SymbOn },
    { "Slab",          APINeig_Ceil },
    { "SlabOn",        APINeig_CeilOn },
    { "Roof",          APINeig_Roof },
    { "RoofOn",        APINeig_RoofOn },
    { "Mesh",          APINeig_Mesh },
    { "MeshOn",        APINeig_MeshOn },
    { "Shell",         APINeig_Shell },
    { "ShellOn",       APINeig_ShellOn },
    { "Morph",         APINeig_Morph },
    { "MorphOn",       APINeig_MorphOn },
    { "Skylight",      APINeig_Skylight },
    { "SkylightOn",    APINeig_SkylightOn },
    { "CurtainWall",   APINeig_CurtainWall },
    { "CurtainWallOn", APINeig_CurtainWallOn },
    { "Stair",         APINeig_Stair },
    { "StairOn",       APINeig_StairOn },
    { "Railing",       APINeig_Railing },
    { "RailingOn",     APINeig_RailingOn },
    { "Zone",          APINeig_Room },
    { "ZoneOn",        APINeig_RoomOn },
    { "Hatch",         APINeig_Hatch },
    { "HatchOn",       APINeig_HatchOn },
    { "Line",          APINeig_Line },
    { "LineOn",        APINeig_LineOn },
    { "PolyLine",      APINeig_PolyLine },
    { "PolyLineOn",    APINeig_PolyLineOn },
    { "Arc",           APINeig_Arc },
    { "ArcOn",         APINeig_ArcOn },
    { "Circle",        APINeig_Circ },
    { "CircleOn",      APINeig_CircOn },
    { "Spline",        APINeig_Spline },
    { "SplineOn",      APINeig_SplineOn },
    { "Hotspot",       APINeig_Hot },
    { "Text",          APINeig_Word },
    { "Label",         APINeig_Label },
    { "Dimension",     APINeig_DimPt },
    { "DimensionOn",   APINeig_DimOn }
};

static bool GetSnapTypeFromName (const GS::UniString& name, API_NeigID& neigID)
{
    for (const auto& snapType : SnapTypeNames) {
        if (name == snapType.first) {
            neigID = snapType.second;
            return true;
        }
    }
    return false;
}

static GSErrCode ReadSnapTypes (const GS::ObjectState& snapFilter, const char* key, GS::Array<API_NeigID>& neigIDs, GS::UniString& errorMessage)
{
    GS::Array<GS::UniString> names;
    snapFilter.Get (key, names);
    for (const GS::UniString& name : names) {
        API_NeigID neigID = APINeig_None;
        if (!GetSnapTypeFromName (name, neigID)) {
            errorMessage = GS::UniString ("Unknown snap type in snapFilter.") + GS::UniString (key) + GS::UniString (": ") + name;
            return APIERR_BADPARS;
        }
        neigIDs.Push (neigID);
    }
    return NoError;
}

// Reads the optional snapFilter parameter into the filter of a point or line input.
// changeFilter stays false when the parameter is not given, so Archicad snaps as usual.
static GSErrCode ReadSnapFilter (const GS::ObjectState& parameters, bool& changeFilter, API_NeigFilter& filter, GS::UniString& errorMessage)
{
    changeFilter = false;
    const GS::ObjectState* snapFilter = parameters.Get ("snapFilter");
    if (snapFilter == nullptr) {
        return NoError;
    }

    GS::Array<API_NeigID> include;
    GS::Array<API_NeigID> exclude;
    GSErrCode err = ReadSnapTypes (*snapFilter, "include", include, errorMessage);
    if (err == NoError) {
        err = ReadSnapTypes (*snapFilter, "exclude", exclude, errorMessage);
    }
    if (err != NoError) {
        return err;
    }

    GS::UniString base = include.IsEmpty () ? "All" : "None";
    snapFilter->Get ("base", base);
    API_InitNeigFilterID initNeigFilterID = APIInitNeigFilter_Full;
    if (base == "All") {
        initNeigFilterID = APIInitNeigFilter_Full;
    } else if (base == "None") {
        initNeigFilterID = APIInitNeigFilter_Empty;
    } else if (base == "Edges") {
        initNeigFilterID = APIInitNeigFilter_On;
    } else if (base == "Nodes") {
        initNeigFilterID = APIInitNeigFilter_Node;
    } else {
        errorMessage = GS::UniString ("Unknown snapFilter.base: ") + base;
        return APIERR_BADPARS;
    }

    err = ACAPI_UserInput_InitNeigFilter (&filter, initNeigFilterID);
    for (UIndex i = 0; i < include.GetSize () && err == NoError; ++i) {
        err = ACAPI_UserInput_SetNeigFilter (&filter, include[i]);
    }
    for (UIndex i = 0; i < exclude.GetSize () && err == NoError; ++i) {
        err = ACAPI_UserInput_ClearNeigFilter (&filter, exclude[i]);
    }
    if (err != NoError) {
        errorMessage = "Failed to set up the snap filter.";
        return err;
    }

    changeFilter = true;
    return NoError;
}

// ---------------------------------------------------------------------------
// Drawing constraint of the Get...FromUser commands (#/InputConstraint)
// ---------------------------------------------------------------------------

// Reads the optional constraint parameter. hasConstraint stays false when it is not given,
// so the input is not constrained.
static GSErrCode ReadInputConstraint (const GS::ObjectState& parameters, bool& hasConstraint, API_ConstraintPars& constraintPars, GS::UniString& errorMessage)
{
    hasConstraint = false;
    const GS::ObjectState* constraint = parameters.Get ("constraint");
    if (constraint == nullptr) {
        return NoError;
    }

    GS::UniString type;
    double value = 0.0;
    if (!constraint->Get ("type", type) || !constraint->Get ("value", value)) {
        errorMessage = "The constraint needs a type and a value.";
        return APIERR_BADPARS;
    }

    constraintPars = {};
    if (type == "X") {
        constraintPars.constType = APIConst_X;
    } else if (type == "Y") {
        constraintPars.constType = APIConst_Y;
    } else if (type == "Z") {
        constraintPars.constType = APIConst_Z;
    } else if (type == "Angle") {
        constraintPars.constType = APIConst_A;
    } else if (type == "Distance") {
        constraintPars.constType = APIConst_R;
    } else {
        errorMessage = GS::UniString ("Unknown constraint.type: ") + type;
        return APIERR_BADPARS;
    }
    constraintPars.constValue = value;

    bool fromLastPoint = false;
    constraint->Get ("fromLastPoint", fromLastPoint);
    constraintPars.useLocOrigo = fromLastPoint;

    hasConstraint = true;
    return NoError;
}

// Sets Archicad's drawing constraint for the input that follows, and dissolves it when it goes
// out of scope, so the constraint never outlives the command (also on Escape and on errors).
class InputConstraintScope {
public:
    InputConstraintScope (bool hasConstraintIn, const API_ConstraintPars& constraintParsIn) :
        hasConstraint (hasConstraintIn),
        constraintPars (constraintParsIn),
        isSet (false)
    {
    }

    InputConstraintScope (const InputConstraintScope&) = delete;
    InputConstraintScope& operator= (const InputConstraintScope&) = delete;

    ~InputConstraintScope ()
    {
        if (isSet) {
            ACAPI_UserInput_ResetConstraint ();
        }
    }

    // Called just before each ACAPI_UserInput_Get... call it should constrain.
    GSErrCode Set ()
    {
        if (!hasConstraint) {
            return NoError;
        }
        API_ConstraintPars pars = constraintPars;
        const GSErrCode err = ACAPI_UserInput_SetConstraint (&pars);
        if (err == NoError) {
            isSet = true;
        }
        return err;
    }

private:
    const bool hasConstraint;
    const API_ConstraintPars constraintPars;
    bool isSet;
};

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
            },
            "snapFilter": {
                "$ref": "#/SnapFilter",
                "description": "Which points and lines of the elements the cursor snaps to. Default: Archicad's usual snapping."
            },
            "constraint": {
                "$ref": "#/InputConstraint",
                "description": "Constrains the click, for example to a fixed X coordinate or to a direction. Default: no constraint."
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

    GS::UniString snapFilterError;
    GSErrCode err = ReadSnapFilter (parameters, pointInfo.changeFilter, pointInfo.filter, snapFilterError);
    if (err != NoError) {
        return CreateErrorResponse (err, snapFilterError);
    }

    bool hasConstraint = false;
    API_ConstraintPars constraintPars = {};
    GS::UniString constraintError;
    err = ReadInputConstraint (parameters, hasConstraint, constraintPars, constraintError);
    if (err != NoError) {
        return CreateErrorResponse (err, constraintError);
    }

    InputConstraintScope constraintScope (hasConstraint, constraintPars);
    err = constraintScope.Set ();
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the constraint.");
    }

    err = ACAPI_UserInput_GetPoint (&pointInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "No point was given - the input was cancelled or could not start (a modal dialog or a non-model window).");
    }

    GS::ObjectState response;
    response.Add ("position", Create3DCoordinateObjectState (pointInfo.pos));
    return response;
}

// ---------------------------------------------------------------------------
// GetLineFromUser
// ---------------------------------------------------------------------------

static void RubberLineCallBack (const API_RubberLineInfo* info)
{
    API_RubberLineType rubberLine = {};
    rubberLine.lineThick = APIRubberLine_Simple;

    // Draw a unit square around the current cursor position.
    rubberLine.c2.x = info->actCoord.x - 0.5;
    rubberLine.c2.y = info->actCoord.y - 0.5;
    rubberLine.c2.z = info->actCoord.z;

    rubberLine.c1 = rubberLine.c2;
    rubberLine.c2.y += 1.0;
    ACAPI_UserInput_DrawRubberLine (&rubberLine);

    rubberLine.c1 = rubberLine.c2;
    rubberLine.c2.x += 1.0;
    ACAPI_UserInput_DrawRubberLine (&rubberLine);

    rubberLine.c1 = rubberLine.c2;
    rubberLine.c2.y -= 1.0;
    ACAPI_UserInput_DrawRubberLine (&rubberLine);

    rubberLine.c1 = rubberLine.c2;
    rubberLine.c2.x -= 1.0;
    ACAPI_UserInput_DrawRubberLine (&rubberLine);
}

GetLineFromUserCommand::GetLineFromUserCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetLineFromUserCommand::GetName () const
{
    return "GetLineFromUser";
}

GS::Optional<GS::UniString> GetLineFromUserCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "startPrompt": {
                "type": "string",
                "description": "Prompt for selecting the line start point.",
                "default": "Click the start point of the line"
            },
            "endPrompt": {
                "type": "string",
                "description": "Prompt for selecting the line end point.",
                "default": "Click the end point of the line"
            },
            "showRubberLineFeedback": {
                "type": "boolean",
                "description": "If true, draws a unit-square rubber feedback around the cursor while input is active.",
                "default": true
            },
            "snapFilter": {
                "$ref": "#/SnapFilter",
                "description": "Which points and lines of the elements the cursor snaps to, for both points. Default: Archicad's usual snapping."
            },
            "constraint": {
                "$ref": "#/InputConstraint",
                "description": "Constrains the end point, for example to a direction from the start point (type Angle with fromLastPoint). The start point is not constrained. Default: no constraint."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetLineFromUserCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True if the user cancelled the input (ESC)."
            },
            "startPoint": {
                "$ref": "#/Coordinate3D",
                "description": "The start point of the line."
            },
            "endPoint": {
                "$ref": "#/Coordinate3D",
                "description": "The end point of the line. It is only valid if cancelled is false."
            }
        }
    })";
}

GS::ObjectState GetLineFromUserCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString startPromptStr = "Click the start point of the line";
    parameters.Get ("startPrompt", startPromptStr);

    GS::UniString endPromptStr = "Click the end point of the line";
    parameters.Get ("endPrompt", endPromptStr);

    bool showRubberLineFeedback = true;
    parameters.Get ("showRubberLineFeedback", showRubberLineFeedback);

    API_GetPointType pointInfo = {};
    CHTruncate (startPromptStr.ToCStr ().Get (), pointInfo.prompt, sizeof (pointInfo.prompt));

    GS::UniString snapFilterError;
    GSErrCode err = ReadSnapFilter (parameters, pointInfo.changeFilter, pointInfo.filter, snapFilterError);
    if (err != NoError) {
        return CreateErrorResponse (err, snapFilterError);
    }

    bool hasConstraint = false;
    API_ConstraintPars constraintPars = {};
    GS::UniString constraintError;
    err = ReadInputConstraint (parameters, hasConstraint, constraintPars, constraintError);
    if (err != NoError) {
        return CreateErrorResponse (err, constraintError);
    }
    InputConstraintScope constraintScope (hasConstraint, constraintPars);

    err = ACAPI_UserInput_GetPoint (&pointInfo);

    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve start point");
    }

    API_GetLineType lineInfo = {};
    lineInfo.startCoord = pointInfo.pos;
    CHTruncate (endPromptStr.ToCStr ().Get (), lineInfo.prompt, sizeof (lineInfo.prompt));
    lineInfo.changeFilter = pointInfo.changeFilter;
    std::memcpy (lineInfo.filter, pointInfo.filter, sizeof (lineInfo.filter));

    // The constraint applies to the end point only, so a direction measured from
    // the last clicked point (fromLastPoint) is measured from the start point.
    err = constraintScope.Set ();
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the constraint.");
    }

    err = showRubberLineFeedback
        ? ACAPI_UserInput_GetLine (&lineInfo, RubberLineCallBack)
        : ACAPI_UserInput_GetLine (&lineInfo);

    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve end point");
    }

    return GS::ObjectState (
        "cancelled", false,
        "startPoint", Create3DCoordinateObjectState (lineInfo.startCoord),
        "endPoint", Create3DCoordinateObjectState (lineInfo.pos)
    );
}

// ---------------------------------------------------------------------------
// GetArcFromUser
// ---------------------------------------------------------------------------

static constexpr double ArcInputTwoPi = 6.283185307179586;

// The rubber callbacks are plain function pointers, so the arc input keeps its
// center (and, once the start point is given, its radius) here. The input is modal
// on Archicad's main thread, so only one arc input uses them at a time.
static API_Coord3D arcInputCenter = {};
static double arcInputRadius = 0.0;
static bool arcInputRadiusFixed = false;

static void RubberArcCallBack (const API_RubberLineInfo* info)
{
    // Draws the whole circle the arc lies on: while the start point is chosen its
    // radius follows the cursor, while the end point is chosen it stays fixed.
    double radius = arcInputRadius;
    if (!arcInputRadiusFixed) {
        radius = std::hypot (info->actCoord.x - arcInputCenter.x, info->actCoord.y - arcInputCenter.y);
    }
    if (radius < 1e-6) {
        return;
    }

    API_RubberArcType rubberArc = {};
    rubberArc.origC = arcInputCenter;
    rubberArc.radii = false;
    rubberArc.lineThick = APIRubberLine_Simple;
    rubberArc.arcType = APIRubberArc_OrigoAngles;
    rubberArc.arc2D.r = radius;
    rubberArc.arc2D.angle = 0.0;
    rubberArc.arc2D.ratio = 1.0;
    rubberArc.arc2D.begAng = 0.0;
    rubberArc.arc2D.endAng = ArcInputTwoPi;
    ACAPI_UserInput_DrawRubberArc (&rubberArc);
}

GetArcFromUserCommand::GetArcFromUserCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetArcFromUserCommand::GetName () const
{
    return "GetArcFromUser";
}

GS::Optional<GS::UniString> GetArcFromUserCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "centerPrompt": {
                "type": "string",
                "description": "Prompt for clicking the center of the arc. Single-byte text: the control box takes a char field.",
                "default": "Click the center of the arc"
            },
            "startPrompt": {
                "type": "string",
                "description": "Prompt for clicking the start point of the arc, which also sets its radius.",
                "default": "Click the start point of the arc"
            },
            "endPrompt": {
                "type": "string",
                "description": "Prompt for clicking the end point of the arc. Archicad draws the arc while the cursor moves; the direction the cursor goes around the center decides whether the arc is clockwise.",
                "default": "Click the end point of the arc"
            },
            "showRubberArcFeedback": {
                "type": "boolean",
                "description": "If true, also draws the whole circle of the arc while the start and end points are chosen: its radius follows the cursor until the start point is clicked. Drawn in 2D windows only; the 3D window shows Archicad's default feedback.",
                "default": true
            },
            "snapFilter": {
                "$ref": "#/SnapFilter",
                "description": "Which points and lines of the elements the cursor snaps to for the center and the start point. Archicad's arc input has no snap filter, so the end point snaps as usual. Default: Archicad's usual snapping."
            },
            "constraint": {
                "$ref": "#/InputConstraint",
                "description": "Constrains every click: the center, the start point and the end point. With fromLastPoint the start point is measured from the center (type Distance fixes the radius, type Angle the direction of the start point) and the end point from the start point. Default: no constraint."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetArcFromUserCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True if the user cancelled the input (Escape); no other field is returned then."
            },
            "center": {
                "$ref": "#/Coordinate3D",
                "description": "The center of the arc."
            },
            "radius": {
                "type": "number",
                "description": "The radius of the arc: the distance of the start point from the center in the x-y plane."
            },
            "startAngle": {
                "type": "number",
                "description": "The direction of the start point from the center, in radians from the x axis, counterclockwise, between -pi and pi."
            },
            "endAngle": {
                "type": "number",
                "description": "The direction of the end point from the center, in radians from the x axis, counterclockwise, between -pi and pi."
            },
            "arcAngle": {
                "type": "number",
                "description": "The angle the arc turns through from its start to its end, in radians: positive (up to 2*pi) for a counterclockwise arc, negative (down to -2*pi) for a clockwise one."
            },
            "clockwise": {
                "type": "boolean",
                "description": "True if the arc was drawn clockwise from its start to its end."
            },
            "startPoint": {
                "$ref": "#/Coordinate3D",
                "description": "The start point of the arc, as clicked."
            },
            "endPoint": {
                "$ref": "#/Coordinate3D",
                "description": "The end point of the arc: the point of its circle in the direction of the third click (its z is the clicked z)."
            }
        },
        "required": ["cancelled"]
    })";
}

GS::ObjectState GetArcFromUserCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // The usual Archicad arc input: a click for the center, a line from it to the
    // start point (the radius), then the arc to its end point.
    GS::UniString centerPromptStr = "Click the center of the arc";
    parameters.Get ("centerPrompt", centerPromptStr);

    GS::UniString startPromptStr = "Click the start point of the arc";
    parameters.Get ("startPrompt", startPromptStr);

    GS::UniString endPromptStr = "Click the end point of the arc";
    parameters.Get ("endPrompt", endPromptStr);

    bool showRubberArcFeedback = true;
    parameters.Get ("showRubberArcFeedback", showRubberArcFeedback);

    // The rubber arc is defined by angles, which the API documents for 2D only.
    API_WindowInfo windowInfo = {};
    if (ACAPI_Window_GetCurrentWindow (&windowInfo) != NoError || windowInfo.typeID == APIWind_3DModelID) {
        showRubberArcFeedback = false;
    }

    API_GetPointType pointInfo = {};
    CHTruncate (centerPromptStr.ToCStr ().Get (), pointInfo.prompt, sizeof (pointInfo.prompt));

    GS::UniString snapFilterError;
    GSErrCode err = ReadSnapFilter (parameters, pointInfo.changeFilter, pointInfo.filter, snapFilterError);
    if (err != NoError) {
        return CreateErrorResponse (err, snapFilterError);
    }

    bool hasConstraint = false;
    API_ConstraintPars constraintPars = {};
    GS::UniString constraintError;
    err = ReadInputConstraint (parameters, hasConstraint, constraintPars, constraintError);
    if (err != NoError) {
        return CreateErrorResponse (err, constraintError);
    }

    // The constraint applies to every click, so it is set before each input.
    InputConstraintScope constraintScope (hasConstraint, constraintPars);
    err = constraintScope.Set ();
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the constraint.");
    }

    err = ACAPI_UserInput_GetPoint (&pointInfo);
    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the center of the arc");
    }

    arcInputCenter = pointInfo.pos;
    arcInputRadius = 0.0;
    arcInputRadiusFixed = false;

    API_GetLineType lineInfo = {};
    lineInfo.startCoord = pointInfo.pos;
    CHTruncate (startPromptStr.ToCStr ().Get (), lineInfo.prompt, sizeof (lineInfo.prompt));
    lineInfo.changeFilter = pointInfo.changeFilter;
    std::memcpy (lineInfo.filter, pointInfo.filter, sizeof (lineInfo.filter));

    err = constraintScope.Set ();
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the constraint.");
    }

    err = showRubberArcFeedback
        ? ACAPI_UserInput_GetLine (&lineInfo, RubberArcCallBack)
        : ACAPI_UserInput_GetLine (&lineInfo);
    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the start point of the arc");
    }

    const API_Coord3D center = lineInfo.startCoord;
    const double radius = std::hypot (lineInfo.pos.x - center.x, lineInfo.pos.y - center.y);
    if (radius < 1e-6) {
        return CreateErrorResponse (APIERR_BADPARS, "The start point of the arc is at its center: the radius is zero.");
    }

    arcInputCenter = center;
    arcInputRadius = radius;
    arcInputRadiusFixed = true;

    API_GetArcType arcInfo = {};
    arcInfo.origo = center;
    arcInfo.startCoord = lineInfo.pos;
    arcInfo.startCoordGiven = true;
    CHTruncate (endPromptStr.ToCStr ().Get (), arcInfo.prompt, sizeof (arcInfo.prompt));

    // API_GetArcType has no snap filter, so only the constraint applies to the end point.
    err = constraintScope.Set ();
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the constraint.");
    }

    err = showRubberArcFeedback
        ? ACAPI_UserInput_GetArc (&arcInfo, RubberArcCallBack)
        : ACAPI_UserInput_GetArc (&arcInfo);
    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the end point of the arc");
    }

    const API_Coord3D startPoint = arcInfo.startCoord;
    const double startAngle = std::atan2 (startPoint.y - center.y, startPoint.x - center.x);
    const double endAngle = std::atan2 (arcInfo.pos.y - center.y, arcInfo.pos.x - center.x);
    const bool clockwise = arcInfo.negArc;

    double arcAngle = endAngle - startAngle;
    if (clockwise) {
        while (arcAngle >= 0.0) {
            arcAngle -= ArcInputTwoPi;
        }
    } else {
        while (arcAngle <= 0.0) {
            arcAngle += ArcInputTwoPi;
        }
    }

    API_Coord3D endPoint = arcInfo.pos;
    endPoint.x = center.x + radius * std::cos (endAngle);
    endPoint.y = center.y + radius * std::sin (endAngle);

    GS::ObjectState response ("cancelled", false);
    response.Add ("center", Create3DCoordinateObjectState (center));
    response.Add ("radius", radius);
    response.Add ("startAngle", startAngle);
    response.Add ("endAngle", endAngle);
    response.Add ("arcAngle", arcAngle);
    response.Add ("clockwise", clockwise);
    response.Add ("startPoint", Create3DCoordinateObjectState (startPoint));
    response.Add ("endPoint", Create3DCoordinateObjectState (endPoint));
    return response;
}

// ---------------------------------------------------------------------------
// GetPolygonFromUser
// ---------------------------------------------------------------------------

template <class ItemType>
static bool TryGetHandleItemCount (GSHandle handle, Int32& itemCount)
{
    if (handle == nullptr) {
        return false;
    }

    const GSSize handleSize = BMhGetSize (handle);
    if (handleSize <= 0 || handleSize % sizeof (ItemType) != 0) {
        return false;
    }

    itemCount = static_cast<Int32> (handleSize / sizeof (ItemType));
    return true;
}

GetPolygonFromUserCommand::GetPolygonFromUserCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetPolygonFromUserCommand::GetName () const
{
    return "GetPolygonFromUser";
}

GS::Optional<GS::UniString> GetPolygonFromUserCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "startPrompt": {
                "type": "string",
                "description": "Prompt for selecting the polygon start point.",
                "default": "Click the start point of the polygon"
            },
            "prompt": {
                "type": "string",
                "description": "Prompt shown while placing the remaining polygon nodes.",
                "default": "Click polygon nodes and finish the input"
            },
            "method": {
                "$ref": "#/PolygonInputMethod"
            },
            "snapFilter": {
                "$ref": "#/SnapFilter",
                "description": "Which points and lines of the elements the cursor snaps to for the start point. Archicad's polygon input has no snap filter, so the other nodes snap as usual. Default: Archicad's usual snapping."
            },
            "constraint": {
                "$ref": "#/InputConstraint",
                "description": "Constrains the nodes after the start point, for example to a direction from the previous node (type Angle with fromLastPoint). The start point is not constrained. Default: no constraint."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetPolygonFromUserCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "cancelled": {
            "type": "boolean",
            "description": "True if the user cancelled the input (ESC)."
        },
        "points": {
            "type": "array",
            "items": {
                "$ref": "#/Coordinate2D"
            }
        },
        "arcs": {
            "type": "array",
            "items": {
                "$ref": "#/PolyArc"
            }
        }
    },
    "required": ["cancelled"]
})";
}

GS::ObjectState GetPolygonFromUserCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    try {
        GS::UniString startPromptStr = "Click the start point of the polygon";
        parameters.Get ("startPrompt", startPromptStr);

        GS::UniString promptStr = "Click polygon nodes and finish the input";
        parameters.Get ("prompt", promptStr);

        GS::UniString methodStr = "General";
        parameters.Get ("method", methodStr);
        API_PolyGetMethod polyMethod = APIPolyGetMethod_General;
        if (methodStr == "Rect") {
            polyMethod = APIPolyGetMethod_Rect;
        } else if (methodStr == "RotRect") {
            polyMethod = APIPolyGetMethod_RotRect;
        } else if (methodStr == "Polyline") {
            polyMethod = APIPolyGetMethod_Polyline;
        }

        API_GetPointType startPointInfo = {};
        CHTruncate (startPromptStr.ToCStr ().Get (), startPointInfo.prompt, sizeof (startPointInfo.prompt));

        GS::UniString snapFilterError;
        GSErrCode err = ReadSnapFilter (parameters, startPointInfo.changeFilter, startPointInfo.filter, snapFilterError);
        if (err != NoError) {
            return CreateErrorResponse (err, snapFilterError);
        }

        bool hasConstraint = false;
        API_ConstraintPars constraintPars = {};
        GS::UniString constraintError;
        err = ReadInputConstraint (parameters, hasConstraint, constraintPars, constraintError);
        if (err != NoError) {
            return CreateErrorResponse (err, constraintError);
        }
        InputConstraintScope constraintScope (hasConstraint, constraintPars);

        err = ACAPI_UserInput_GetPoint (&startPointInfo);
        if (err == APIERR_CANCEL) {
            return GS::ObjectState ("cancelled", true);
        }
        if (err != NoError) {
            return CreateErrorResponse (err,"Failed to retrieve start point");
        }

        API_GetPolyType polyInfo = {};
        polyInfo.startCoord = startPointInfo.pos;
        polyInfo.method = polyMethod;
        CHTruncate (promptStr.ToCStr ().Get (), polyInfo.prompt, sizeof (polyInfo.prompt));

        // The constraint applies to the nodes after the start point.
        err = constraintScope.Set ();
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to set the constraint.");
        }

        err = ACAPI_UserInput_GetPoly (&polyInfo);

        const GS::OnExit disposePolyHandles ([&polyInfo]() {
            if (polyInfo.coords != nullptr) {
                BMKillHandle (reinterpret_cast<GSHandle*> (&polyInfo.coords));
            }
            if (polyInfo.parcs != nullptr) {
                BMKillHandle (reinterpret_cast<GSHandle*> (&polyInfo.parcs));
            }
        });

        if (err == APIERR_CANCEL) {
            return GS::ObjectState ("cancelled", true);
        }
        if (err != NoError) {
            return CreateErrorResponse (err,"Failed to retrieve polygon points");
        }
        if (polyInfo.coords == nullptr) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed: Polygon input did not return coordinate handle.");
        }

        Int32 coordCountInHandle = 0;
        if (!TryGetHandleItemCount<API_Coord> (reinterpret_cast<GSHandle> (polyInfo.coords), coordCountInHandle)) {
            return CreateErrorResponse (APIERR_GENERAL, "Polygon input returned an invalid coordinate handle.");
        }

        if (coordCountInHandle <= 1) {
            return CreateErrorResponse (APIERR_GENERAL, "Polygon input returned an empty coordinate handle.");
        }

        Int32 coordCountToSerialize = polyInfo.nCoords;
        if (coordCountToSerialize <= 0 || coordCountToSerialize >= coordCountInHandle) {
            coordCountToSerialize = coordCountInHandle - 1;
        }

        Int32 arcCountToSerialize = 0;
        if (polyInfo.parcs != nullptr && polyInfo.nArcs > 0) {
            Int32 arcCountInHandle = 0;
            if (!TryGetHandleItemCount<API_PolyArc> (reinterpret_cast<GSHandle> (polyInfo.parcs), arcCountInHandle)) {
                return CreateErrorResponse (APIERR_GENERAL, "Polygon input returned an invalid arc handle.");
            }

            arcCountToSerialize = std::min (polyInfo.nArcs, arcCountInHandle);
        }

        GS::ObjectState response (
            "cancelled", false
        );

        const auto& points = response.AddList<GS::ObjectState> ("points");
        for (Int32 i = 1; i <= coordCountToSerialize; ++i) {
            const API_Coord& c = (*polyInfo.coords)[i];
            points (Create2DCoordinateObjectState (c));
        }

        if (polyInfo.parcs != nullptr && arcCountToSerialize > 0) {
            const auto& arcs = response.AddList<GS::ObjectState> ("arcs");
            for (Int32 i = 0; i < arcCountToSerialize; ++i) {
                arcs (CreatePolyArcObjectState ((*polyInfo.parcs)[i]));
            }
        }

        return response;

    } catch (...) {
        return CreateErrorResponse (APIERR_GENERAL, "Unexpected exception while processing polygon input.");
    }
}

// ---------------------------------------------------------------------------
// GetMagicWandSettings
// ---------------------------------------------------------------------------

GetMagicWandSettingsCommand::GetMagicWandSettingsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetMagicWandSettingsCommand::GetName () const
{
    return "GetMagicWandSettings";
}

GS::Optional<GS::UniString> GetMagicWandSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "curveApproximation": {
                "type": "string",
                "enum": [
                    "SegmentsAlongArcs",
                    "SegmentsAlongCircles",
                    "DeviationFromCurves",
                    "SegmentLength",
                    "Unknown"
                ],
                "description": "How the Magic Wand approximates curves with straight segments: a fixed number of segments along arcs or along full circles (segmentCount), segments that deviate from the curve by at most maxDeviation, or segments of segmentLength. Unknown for a mode this version of Tapir does not know."
            },
            "segmentCount": {
                "type": "integer",
                "description": "The number of segments along an arc or circle, used by SegmentsAlongArcs and SegmentsAlongCircles."
            },
            "maxDeviation": {
                "type": "number",
                "description": "The largest distance of the segments from the curve, in meters, used by DeviationFromCurves."
            },
            "segmentLength": {
                "type": "number",
                "description": "The length of the segments, in meters, used by SegmentLength."
            },
            "traceMethod": {
                "type": "string",
                "enum": [
                    "Linear",
                    "BestMatch",
                    "Unknown"
                ],
                "description": "How curves are traced: Linear traces them with straight segments as set above, BestMatch with the curves that match them best. Unknown for a method this version of Tapir does not know."
            }
        },
        "additionalProperties": false,
        "required": [
            "curveApproximation",
            "segmentCount",
            "maxDeviation",
            "segmentLength",
            "traceMethod"
        ]
    })";
}

static GS::UniString ConvertMagicWandDivArcModeToString (short divArcMode)
{
    switch (divArcMode) {
        case API_ArcDiv_DivArc:     return "SegmentsAlongArcs";
        case API_ArcDiv_DivCircle:  return "SegmentsAlongCircles";
        case API_ArcDiv_DiffArc:    return "DeviationFromCurves";
        case API_ArcDiv_PartLength: return "SegmentLength";
        default:                    return "Unknown";
    }
}

static GS::UniString ConvertMagicWandTraceMethodToString (short traceMethod)
{
    switch (traceMethod) {
        case API_ArcTrace_Linear:    return "Linear";
        case API_ArcTrace_BestMatch: return "BestMatch";
        default:                     return "Unknown";
    }
}

GS::ObjectState GetMagicWandSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    // The settings of the Magic Wand Settings dialog, read only.
    API_MagicWandInfo magicWandInfo = {};
    const GSErrCode err = ACAPI_UserInput_GetMagicWandSets (&magicWandInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Magic Wand settings.");
    }

    GS::ObjectState response;
    response.Add ("curveApproximation", ConvertMagicWandDivArcModeToString (magicWandInfo.divArcMode));
    response.Add ("segmentCount", static_cast<Int32> (magicWandInfo.arcSegNumb));
    response.Add ("maxDeviation", magicWandInfo.arcDiff);
    response.Add ("segmentLength", magicWandInfo.partLength);
    response.Add ("traceMethod", ConvertMagicWandTraceMethodToString (magicWandInfo.traceMethod));
    return response;
}


// ---------------------------------------------------------------------------
// ModelToScreen / ScreenToModel
// ---------------------------------------------------------------------------

ModelToScreenCommand::ModelToScreenCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModelToScreenCommand::GetName () const
{
    return "ModelToScreen";
}

GS::Optional<GS::UniString> ModelToScreenCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "coordinates": {
                "type": "array",
                "description": "Model coordinates in the current database (meters). In a 3D window they are projected with the current camera; in other windows z is disregarded.",
                "items": {
                    "$ref": "#/Coordinate3D"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "coordinates"
        ]
    })";
}

GS::Optional<GS::UniString> ModelToScreenCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "screenPoints": {
                "type": "array",
                "description": "One item per input coordinate, in the same order: the pixel position local to the current Archicad view window (not global desktop coordinates), or an error.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/ScreenPoint"
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
            "screenPoints"
        ]
    })";
}

GS::ObjectState ModelToScreenCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    // The documented route: ACAPI_View_ModelToScreen projects the model point into
    // the current view, then ACAPI_View_CoordToPoint turns it into a pixel position.
    // Both depend on the current window, its size, drawing scale and zoom.
    GS::Array<GS::ObjectState> coordinates;
    parameters.Get ("coordinates", coordinates);

    GS::ObjectState response;
    const auto& screenPoints = response.AddList<GS::ObjectState> ("screenPoints");
    for (const GS::ObjectState& coordinateOS : coordinates) {
        const API_Coord3D coord3D = Get3DCoordinateFromObjectState (coordinateOS);
        API_Coord coord = {};
        GSErrCode err = ACAPI_View_ModelToScreen (coord3D, coord);
        if (err != NoError) {
            screenPoints (CreateErrorResponse (err, "Failed to project the coordinate to the current view."));
            continue;
        }

        API_Point point = {};
        err = ACAPI_View_CoordToPoint (&coord, &point);
        if (err != NoError) {
            screenPoints (CreateErrorResponse (err, "Failed to convert the coordinate to a screen point."));
            continue;
        }

        screenPoints (GS::ObjectState ("h", (Int32) point.h, "v", (Int32) point.v));
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Available from Archicad 29.");
#endif
}

ScreenToModelCommand::ScreenToModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ScreenToModelCommand::GetName () const
{
    return "ScreenToModel";
}

GS::Optional<GS::UniString> ScreenToModelCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "screenPoints": {
                "type": "array",
                "description": "Pixel positions local to the current Archicad view window (not global desktop coordinates).",
                "items": {
                    "$ref": "#/ScreenPointInput"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "screenPoints"
        ]
    })";
}

GS::Optional<GS::UniString> ScreenToModelCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "coordinates": {
                "type": "array",
                "description": "One item per input screen point, in the same order: the 2D model coordinate in the current database (meters), or an error. This is a 2D mapping: it does not reconstruct a 3D point, and in a 3D window the result is not a point of the model.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/Coordinate2D"
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
            "coordinates"
        ]
    })";
}

GS::ObjectState ScreenToModelCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // Depends on the current window, its size, drawing scale and zoom; several
    // model coordinates can map to the same pixel.
    GS::Array<GS::ObjectState> screenPointOSs;
    parameters.Get ("screenPoints", screenPointOSs);

    GS::ObjectState response;
    const auto& coordinates = response.AddList<GS::ObjectState> ("coordinates");
    for (const GS::ObjectState& screenPointOS : screenPointOSs) {
        Int32 h = 0;
        Int32 v = 0;
        if (!screenPointOS.Get ("h", h) || !screenPointOS.Get ("v", v) ||
            h < -32768 || h > 32767 || v < -32768 || v > 32767) {
            coordinates (CreateErrorResponse (APIERR_BADPARS, "The screen point must have integer h and v between -32768 and 32767."));
            continue;
        }

        API_Point point = {};
        point.h = (short) h;
        point.v = (short) v;
        API_Coord coord = {};
        const GSErrCode err = ACAPI_View_PointToCoord (&point, &coord);
        if (err != NoError) {
            coordinates (CreateErrorResponse (err, "Failed to convert the screen point to a model coordinate."));
            continue;
        }

        coordinates (Create2DCoordinateObjectState (coord));
    }

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
            },
            "snapFilter": {
                "$ref": "#/SnapFilter",
                "description": "Which points and lines of the elements the cursor snaps to, for example only the points and edges of the element types asked for. Default: Archicad's usual snapping."
            },
            "constraint": {
                "$ref": "#/InputConstraint",
                "description": "Constrains every click, for example to a fixed X coordinate or to a direction. Default: no constraint."
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

    bool changeFilter = false;
    API_NeigFilter filter = {};
    GS::UniString snapFilterError;
    const GSErrCode snapFilterErr = ReadSnapFilter (parameters, changeFilter, filter, snapFilterError);
    if (snapFilterErr != NoError) {
        return CreateErrorResponse (snapFilterErr, snapFilterError);
    }

    bool hasConstraint = false;
    API_ConstraintPars constraintPars = {};
    GS::UniString constraintError;
    const GSErrCode constraintErr = ReadInputConstraint (parameters, hasConstraint, constraintPars, constraintError);
    if (constraintErr != NoError) {
        return CreateErrorResponse (constraintErr, constraintError);
    }
    InputConstraintScope constraintScope (hasConstraint, constraintPars);

    for (;;) {
        API_GetPointType pointInfo = {};
        CHTruncate (prompt.ToCStr ().Get (), pointInfo.prompt, sizeof (pointInfo.prompt));
        pointInfo.enableQuickSelection = true;
        pointInfo.changeFilter = changeFilter;
        std::memcpy (pointInfo.filter, filter, sizeof (pointInfo.filter));

        // Set again before every click, also the ones repeated after a miss.
        const GSErrCode constraintSetErr = constraintScope.Set ();
        if (constraintSetErr != NoError) {
            return CreateErrorResponse (constraintSetErr, "Failed to set the constraint.");
        }

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

// ---------------------------------------------------------------------------
// Toolbox: GetCurrentTool, SetCurrentTool, GetToolboxItems
// ---------------------------------------------------------------------------

// The JSON form of a Toolbox item, shared by GetCurrentTool, SetCurrentTool and GetToolboxItems:
// elementType, plus variation (the four-character subtype, e.g. DOOR, SKYL or the signature of an
// add-on's library part subtype) when it is not generic, plus classId for the tools of add-ons
// (Archicad 26 and later). The Arrow and the Marquee have no element type and give an empty object.
static const char* ToolDescriptionPropertiesSchema = R"(
            "elementType": {
                "$ref": "#/ElementType",
                "description": "The element type of the tool. Unknown for the tools of add-ons, which classId identifies."
            },
            "variation": {
                "type": "string",
                "description": "The four-character subtype of the tool, e.g. DOOR, SKYL, CRWD, or the signature of a library part subtype registered by an add-on. Missing for the generic tool of the element type.",
                "minLength": 1,
                "maxLength": 4
            },
            "classId": {
                "type": "string",
                "description": "The class id (a guid) of the tool of an add-on, whose elementType is Unknown. Archicad 26 and later.",
                "format": "uuid",
                "pattern": "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
            })";

static GS::UniString ToolVariationToString (API_ElemVariationID variationID)
{
    const UInt32 v = static_cast<UInt32> (variationID);
    char s[5] = {
        (char) ((v >> 24) & 0xFF),
        (char) ((v >> 16) & 0xFF),
        (char) ((v >> 8) & 0xFF),
        (char) (v & 0xFF),
        0
    };
    return GS::UniString (s);
}

// The reverse of ToolVariationToString: 'DOOR' is "DOOR"; a shorter string fills the high bytes.
static bool ToolVariationFromString (const GS::UniString& str, API_ElemVariationID& variationID)
{
    if (str.IsEmpty () || str.GetLength () > 4) {
        return false;
    }
    UInt32 v = 0;
    for (UIndex i = 0; i < 4; ++i) {
        UInt32 c = 0;
        if (i < str.GetLength ()) {
            const GS::UniChar::Layout ch = static_cast<GS::UniChar::Layout> (str[i]);
            if (ch < 0x20 || ch > 0x7E) {
                return false;
            }
            c = static_cast<UInt32> (ch);
        }
        v = (v << 8) | c;
    }
    variationID = static_cast<API_ElemVariationID> (v);
    return true;
}

static GS::ObjectState ToolBoxItemToObjectState (const API_ToolBoxItem& toolBoxItem)
{
#ifdef ServerMainVers_2600
    const API_ElemTypeID typeID = toolBoxItem.type.typeID;
    const API_ElemVariationID variationID = toolBoxItem.type.variationID;
#else
    const API_ElemTypeID typeID = toolBoxItem.typeID;
    const API_ElemVariationID variationID = toolBoxItem.variationID;
#endif

    GS::ObjectState os;
    // The Arrow and the Marquee have no element type (API_ZombieElemID).
    if (typeID == API_ZombieElemID) {
        return os;
    }
    os.Add ("elementType", GetElementTypeNonLocalizedName (typeID));
    if (variationID != APIVarId_Generic) {
        os.Add ("variation", ToolVariationToString (variationID));
    }
#ifdef ServerMainVers_2600
    if (typeID == API_ExternalElemID && toolBoxItem.type.classID != APINULLGuid) {
        os.Add ("classId", APIGuidToString (toolBoxItem.type.classID));
    }
#endif
    return os;
}

// Reads a tool description into toolBoxItem. An empty description is the Arrow.
static GS::Optional<GS::UniString> ToolBoxItemFromObjectState (const GS::ObjectState& os, API_ToolBoxItem& toolBoxItem)
{
    API_ElemTypeID typeID = API_ZombieElemID;
    API_ElemVariationID variationID = APIVarId_Generic;

    GS::UniString variationStr;
    if (os.Get ("variation", variationStr) && !ToolVariationFromString (variationStr, variationID)) {
        return GS::UniString::Printf ("Invalid variation '%T': it must be 1 to 4 ASCII characters.", variationStr.ToPrintf ());
    }

    GS::UniString classIdStr;
    if (os.Get ("classId", classIdStr)) {
#ifdef ServerMainVers_2600
        const API_Guid classID = APIGuidFromString (classIdStr.ToCStr ());
        if (classID == APINULLGuid) {
            return GS::UniString::Printf ("Invalid classId '%T'.", classIdStr.ToPrintf ());
        }
        toolBoxItem = {};
        toolBoxItem.type = API_ElemType (classID);
        toolBoxItem.type.variationID = variationID;
        return {};
#else
        return GS::UniString ("classId needs Archicad 26 or later.");
#endif
    }

    GS::UniString elementTypeStr;
    if (os.Get ("elementType", elementTypeStr)) {
        typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
        // SetToolBoxMode silently switches to the Arrow for an unknown type, so refuse it here.
        if (typeID == API_ZombieElemID) {
            return GS::UniString::Printf ("Invalid elementType '%T'. The tools of add-ons are given by classId.", elementTypeStr.ToPrintf ());
        }
    } else if (variationID != APIVarId_Generic) {
        return GS::UniString ("A variation needs an elementType.");
    }

    toolBoxItem = {};
#ifdef ServerMainVers_2600
    toolBoxItem.type = API_ElemType (typeID, variationID);
#else
    toolBoxItem.typeID = typeID;
    toolBoxItem.variationID = variationID;
#endif
    return {};
}

GetCurrentToolCommand::GetCurrentToolCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetCurrentToolCommand::GetName () const
{
    return "GetCurrentTool";
}

GS::Optional<GS::UniString> GetCurrentToolCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "description": "The tool active in the Toolbox, in the form SetCurrentTool accepts. Empty when the active tool makes no element, e.g. the Arrow or the Marquee.",
        "properties": {)") + ToolDescriptionPropertiesSchema + R"(
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::ObjectState GetCurrentToolCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_ToolBoxItem toolBoxItem = {};
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Toolbox_GetToolBoxMode (&toolBoxItem);
#else
    const GSErrCode err = ACAPI_Environment (APIEnv_GetToolBoxModeID, &toolBoxItem);
#endif
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the active tool.");
    }

    return ToolBoxItemToObjectState (toolBoxItem);
}

// ---------------------------------------------------------------------------
// SetCurrentTool
// ---------------------------------------------------------------------------

SetCurrentToolCommand::SetCurrentToolCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetCurrentToolCommand::GetName () const
{
    return "SetCurrentTool";
}

GS::Optional<GS::UniString> SetCurrentToolCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "description": "The tool to activate, in the form GetCurrentTool and GetToolboxItems return. An empty object activates the Arrow.",
        "properties": {)") + ToolDescriptionPropertiesSchema + R"(
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> SetCurrentToolCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetCurrentToolCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_ToolBoxItem toolBoxItem = {};
    const GS::Optional<GS::UniString> error = ToolBoxItemFromObjectState (parameters, toolBoxItem);
    if (error.HasValue ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, *error);
    }

#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Toolbox_SetToolBoxMode (&toolBoxItem);
#else
    const GSErrCode err = ACAPI_Environment (APIEnv_SetToolBoxModeID, &toolBoxItem);
#endif
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to activate the tool. Check that the variation belongs to the element type (GetToolboxItems lists the tools).");
    }

    return CreateSuccessfulExecutionResult ();
}

// ---------------------------------------------------------------------------
// GetToolboxItems
// ---------------------------------------------------------------------------

GetToolboxItemsCommand::GetToolboxItemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetToolboxItemsCommand::GetName () const
{
    return "GetToolboxItems";
}

GS::Optional<GS::UniString> GetToolboxItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "includeHidden": {
                "type": "boolean",
                "description": "Also list the tools that have no icon in the Toolbox but can be active, e.g. the Slab Accessory. Default: false."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetToolboxItemsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "toolboxItems": {
                "type": "array",
                "description": "The tools installed in the Toolbox, the tools of add-ons included, each in the form SetCurrentTool accepts.",
                "items": {
                    "type": "object",
                    "properties": {)") + ToolDescriptionPropertiesSchema + R"(
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementType"
                    ]
                }
            },
            "currentTool": {
                "type": "object",
                "description": "The tool active in the Toolbox, as GetCurrentTool returns it. Empty for the Arrow and the Marquee.",
                "properties": {)" + ToolDescriptionPropertiesSchema + R"(
                },
                "additionalProperties": false,
                "required": []
            }
        },
        "additionalProperties": false,
        "required": [
            "toolboxItems",
            "currentTool"
        ]
    })";
}

GS::ObjectState GetToolboxItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool includeHidden = false;
    parameters.Get ("includeHidden", includeHidden);

    API_ToolBoxInfo toolBoxInfo = {};
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Toolbox_GetToolBoxInfo (&toolBoxInfo, includeHidden);
#else
    const GSErrCode err = ACAPI_Environment (APIEnv_GetToolBoxInfoID, &toolBoxInfo, reinterpret_cast<void*> (static_cast<GS::IntPtr> (includeHidden)));
#endif
    if (err != NoError) {
        BMKillHandle ((GSHandle*) &toolBoxInfo.data);
        return CreateErrorResponse (err, "Failed to get the Toolbox items.");
    }

    GS::ObjectState response;
    const auto& toolboxItems = response.AddList<GS::ObjectState> ("toolboxItems");
    if (toolBoxInfo.data != nullptr) {
        for (Int32 i = 0; i < toolBoxInfo.nTools; ++i) {
            const GS::ObjectState item = ToolBoxItemToObjectState ((*toolBoxInfo.data)[i]);
            // An item without an element type would mean the Arrow, which SetCurrentTool gets from {}.
            if (item.Contains ("elementType")) {
                toolboxItems (item);
            }
        }
    }
    response.Add ("currentTool", ToolBoxItemToObjectState (toolBoxInfo.activeTool));

    BMKillHandle ((GSHandle*) &toolBoxInfo.data);
    return response;
}

GetDialogAutoCloseCommand::GetDialogAutoCloseCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetDialogAutoCloseCommand::GetName () const
{
    return "GetDialogAutoClose";
}

GS::Optional<GS::UniString> GetDialogAutoCloseCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "enabled": {
                "type": "boolean",
                "description": "Whether the dialog auto close is on: true while Archicad closes the dialogs it would show with their default button instead of waiting for the user. This is the same state SetDialogAutoClose writes."
            }
        },
        "additionalProperties": false,
        "required": [
            "enabled"
        ]
    })";
}

GS::ObjectState GetDialogAutoCloseCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    return GS::ObjectState ("enabled", DGIsAutoCloseEnabled ());
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

ConvertImagesCommand::ConvertImagesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ConvertImagesCommand::GetName () const
{
    return "ConvertImages";
}

GS::Optional<GS::UniString> ConvertImagesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "images": {
                "type": "array",
                "description": "The images to convert.",
                "items": {
                    "type": "object",
                    "properties": {
                        "image": {
                            "type": "string",
                            "description": "The base64 encoded image, for example a previewImage returned by GetElementPreviewImage."
                        },
                        "inputFormat": {
                            "type": "string",
                            "description": "The format of the image: Bitmap, GIF, TIFF, JPEG or PNG, or the MIME type of another format Archicad can read (for example image/x-targa)."
                        },
                        "outputFormat": {
                            "type": "string",
                            "description": "The format to convert the image to: Bitmap, GIF, TIFF, JPEG or PNG, or the MIME type of another format Archicad can write."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "image",
                        "inputFormat",
                        "outputFormat"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "images"
        ]
    })";
}

GS::Optional<GS::UniString> ConvertImagesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "images": {
                "type": "array",
                "description": "The converted images in the order of the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "image": {
                                    "type": "string",
                                    "description": "The base64 encoded converted image."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "image"
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
            "images"
        ]
    })";
}

// The image formats of API_PictureFormat, by the names ConvertImages takes, with their MIME types.
struct NamedPictureFormat {
    const char*       name;
    API_PictureFormat format;
    const char*       mimeType;
};

static const NamedPictureFormat NamedPictureFormats[] = {
    { "Bitmap", APIPictForm_Bitmap, "image/x-bmp" },
    { "GIF",    APIPictForm_GIF,    "image/gif" },
    { "TIFF",   APIPictForm_TIFF,   "image/tiff" },
    { "JPEG",   APIPictForm_JPEG,   "image/jpeg" },
    { "PNG",    APIPictForm_PNG,    "image/png" }
};

static const NamedPictureFormat* FindNamedPictureFormat (const GS::UniString& name)
{
    for (const NamedPictureFormat& namedFormat : NamedPictureFormats) {
        if (name == namedFormat.name) {
            return &namedFormat;
        }
    }
    return nullptr;
}

// Two named formats are converted with ACAPI_Conversion_ConvertPicture; when either format is a
// MIME type, both are passed as MIME types to ACAPI_Conversion_ConvertMimePicture.
static GS::ObjectState ConvertImage (const GS::ObjectState& imageOS)
{
    GS::UniString image;
    GS::UniString inputFormat;
    GS::UniString outputFormat;
    if (!imageOS.Get ("image", image) || !imageOS.Get ("inputFormat", inputFormat) || !imageOS.Get ("outputFormat", outputFormat)) {
        return CreateErrorResponse (APIERR_BADPARS, "image, inputFormat and outputFormat are required.");
    }

    const GS::Array<char> bytes = Base64Converter::Decode (image);
    if (bytes.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "image is empty or not base64 encoded.");
    }

    GSHandle inputHdl = BMAllocateHandle ((GSSize) bytes.GetSize (), ALLOCATE_CLEAR, 0);
    if (inputHdl == nullptr) {
        return CreateErrorResponse (APIERR_MEMFULL, "Failed to allocate the image.");
    }
    std::memcpy (*inputHdl, bytes.GetContent (), bytes.GetSize ());

    const NamedPictureFormat* namedInput = FindNamedPictureFormat (inputFormat);
    const NamedPictureFormat* namedOutput = FindNamedPictureFormat (outputFormat);

    GSErrCode err = NoError;
    GSHandle outputHdl = nullptr;
    if (namedInput != nullptr && namedOutput != nullptr) {
        API_PictureHandle oldPict = {};
        oldPict.storageFormat = namedInput->format;
        oldPict.pictHdl = inputHdl;
        API_PictureHandle newPict = {};
        newPict.storageFormat = namedOutput->format;
        err = ACAPI_Conversion_ConvertPicture (&oldPict, &newPict);
        outputHdl = newPict.pictHdl;
    } else {
        const GS::UniString mimeInType = namedInput != nullptr ? GS::UniString (namedInput->mimeType) : inputFormat;
        const GS::UniString mimeOutType = namedOutput != nullptr ? GS::UniString (namedOutput->mimeType) : outputFormat;
        const auto mimeIn = mimeInType.ToCStr (CC_UTF8);
        const auto mimeOut = mimeOutType.ToCStr (CC_UTF8);
        API_MimePicture mimePicture = {};
        mimePicture.mimeIn = mimeIn.Get ();
        mimePicture.inputHdl = inputHdl;
        mimePicture.mimeOut = mimeOut.Get ();
        mimePicture.inContainsMime = false;
        mimePicture.outDepth = APIColorDepth_FromSourceImage;
        err = ACAPI_Conversion_ConvertMimePicture (&mimePicture);
        outputHdl = mimePicture.outputHdl;
    }
    BMKillHandle (&inputHdl);

    if (err != NoError || outputHdl == nullptr) {
        if (outputHdl != nullptr) {
            BMKillHandle (&outputHdl);
        }
        return CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to convert the image from " + inputFormat + " to " + outputFormat + ".");
    }

    GS::UniString converted = Base64Converter::Encode (*outputHdl, (USize) BMGetHandleSize (outputHdl));
    converted.DeleteAll (GS::UniChar (char ('\n')));
    BMKillHandle (&outputHdl);

    return GS::ObjectState ("image", converted);
}

GS::ObjectState ConvertImagesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> images;
    parameters.Get ("images", images);

    GS::ObjectState response;
    const auto& convertedImages = response.AddList<GS::ObjectState> ("images");
    for (const GS::ObjectState& imageOS : images) {
        convertedImages (ConvertImage (imageOS));
    }
    return response;
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
