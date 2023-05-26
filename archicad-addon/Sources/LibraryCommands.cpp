#include "LibraryCommands.hpp"
#include "ObjectState.hpp"

GS::Optional<GS::UniString> GetLibrariesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "A list of project libraries.",
                "items": {
                    "type": "object",
                    "description": "Library",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Library name."
                        },
                        "path": {
                            "type": "string",
                            "description": "A filesystem path to library location."
                        },
                        "type": {
                            "type": "integer",
                            "description": "Library type."
                        },
                        "available": {
                            "type": "boolean",
                            "description": "Is library not missing."
                        },
                        "readOnly": {
                            "type": "boolean",
                            "description": "Is library not writable."
                        },
                        "twServerUrl": {
                            "type": "string",
                            "description": "URL address of the TeamWork server hosting the library"
                        },
                        "urlWebLibrary": {
                            "type": "string",
                            "description": "URL of the downloaded Internet library"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "type",
                        "path"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    })";
}

GetLibrariesCommand::GetLibrariesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetLibrariesCommand::GetName () const
{
    return "GetLibraries";
}

GS::ObjectState GetLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_LibraryInfo> libs;

    GSErrCode err = ACAPI_Environment (APIEnv_GetLibrariesID, &libs);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive libraries.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("libraries");

    for (UInt32 i = 0; i < libs.GetSize (); i++) {

        GS::ObjectState libraryData;
        GS::UniString   twServerUrl;
        GS::UniString   urlWebLibrary;
        switch (libs[i].libraryType) {
            case API_LibraryTypeID::API_ServerLibrary:  twServerUrl   = libs[i].twServerUrl; break;
            case API_LibraryTypeID::API_UrlLibrary:     urlWebLibrary = libs[i].twServerUrl; break;
            case API_LibraryTypeID::API_UrlOtherObject: urlWebLibrary = libs[i].twServerUrl; break;
        }

        libraryData.Add ("name", libs[i].name);
        libraryData.Add ("path", libs[i].location.ToDisplayText());
        libraryData.Add ("type", libs[i].libraryType);
        libraryData.Add ("available", libs[i].available);
        libraryData.Add ("readOnly", libs[i].readOnly);
        libraryData.Add ("twServerUrl", twServerUrl);
        libraryData.Add ("urlWebLibrary", urlWebLibrary);
        listAdder (libraryData);
    }

    return response;
}

ReloadLibrariesCommand::ReloadLibrariesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReloadLibrariesCommand::GetName () const
{
    return "ReloadLibraries";
}

GS::ObjectState ReloadLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_Automate (APIDo_ReloadLibrariesID);

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to reload libraries.");
    }

    return {};
}
