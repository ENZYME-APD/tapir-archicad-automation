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

GS::ObjectState QuitArchicadCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#if defined (ServerMainVers_2800)
    GSErrCode err = ACAPI_ProjectOperation_Quit ();
#else
    Int32 magicCode = 1234;
    GSErrCode err = ACAPI_ProjectOperation_Quit (magicCode);
#endif
    if (err != NoError) {
        return CreateErrorResponse (APIERR_COMMANDFAILED, "Failed to quit Archicad!");
    }

    return {};
}
