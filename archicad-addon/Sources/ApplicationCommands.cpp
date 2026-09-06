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

    GS::UniString windowTypeStr;
    if (!parameters.Get ("windowType", windowTypeStr)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing parameter: windowType.");
    }

    API_WindowInfo windowInfo = {};
    windowInfo.typeID = ConvertStringToWindowType (windowTypeStr);
    if (windowInfo.typeID == API_ZombieWindowID) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: windowType.");
    }

    Int32 storyIndex = 0;
    const bool hasStoryIndex = parameters.Get ("storyIndex", storyIndex);
    if (hasStoryIndex) {
        if (windowInfo.typeID != APIWind_FloorPlanID) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "storyIndex is only valid when windowType is 'FloorPlan'.");
        }
        windowInfo.index = storyIndex;
        // GetDatabaseInfo fills in the GUID and all fields from typeID+index,
        // which are needed to change the current database (used by ACAPI element creation).
        if (ACAPI_Window_GetDatabaseInfo (&windowInfo) == NoError) {
            ACAPI_Database_ChangeCurrentDatabase (&windowInfo);
        }
        const GSErrCode err = ACAPI_Window_ChangeWindow (&windowInfo);
        return err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to change active story.");
    }

    const GS::ObjectState* databaseId = parameters.Get ("databaseId");
    if (databaseId != nullptr) {
        API_DatabaseInfo targetDatabase = DatabaseIdResolver::Instance ().GetDatabaseWithId (GetGuidFromObjectState (*databaseId));
        GSErrCode err = ACAPI_Window_GetDatabaseInfo (&targetDatabase);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to resolve target database.");
        }

        if (targetDatabase.typeID != windowInfo.typeID) {
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

