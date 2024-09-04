#include "ApplicationCommands.hpp"
#include "ObjectState.hpp"
#include "FileSystem.hpp"
#include "AddOnVersion.hpp"
#include "MigrationHelper.hpp"

GetAddOnVersionCommand::GetAddOnVersionCommand () :
    CommandBase (CommonSchema::NotUsed)
{

}

GS::String GetAddOnVersionCommand::GetName () const
{
    return "GetAddOnVersion";
}

GS::Optional<GS::UniString> GetAddOnVersionCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> GetArchicadLocationCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> QuitArchicadCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> GetCurrentWindowTypeCommand::GetResponseSchema () const
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

GS::ObjectState GetCurrentWindowTypeCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_WindowInfo windowInfo;
    GSErrCode err = ACAPI_Window_GetCurrentWindow (&windowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current window!");
    }
    return GS::ObjectState ("currentWindowType", ConvertWindowTypeToString (windowInfo.typeID));
}