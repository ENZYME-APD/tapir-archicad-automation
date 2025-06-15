#include "VersionChecker.hpp"
#include "AddOnVersion.hpp"

#include "HTTP/Client/ClientConnection.hpp"
#include "HTTP/Client/Request.hpp"
#include "HTTP/Client/Response.hpp"
#include "JSON/Value.hpp"
#include "JSON/JDOMParser.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "IOBinProtocolXs.hpp"
#include "IChannelX.hpp"
#include "StringConversion.hpp"

#include <map>

static std::unique_ptr<VersionChecker> Intance;

void VersionChecker::CreateInstance (GS::UInt16 acMainVersion)
{
    Intance.reset (new VersionChecker (acMainVersion));
}

bool VersionChecker::IsUsingLatestVersion ()
{
    if (!Intance) {
        return true;
    }

    return Intance->latestVersion <= ADDON_VERSION;
}

const GS::UniString& VersionChecker::LatestVersion ()
{
    if (!Intance) {
        return GS::EmptyUniString;
    }

    return Intance->latestVersion;
}

const GS::UniString& VersionChecker::LatestVersionName ()
{
    if (!Intance) {
        return GS::EmptyUniString;
    }

    return Intance->latestVersionName;
}

const GS::UniString& VersionChecker::LatestVersionDownloadUrl ()
{
    if (!Intance) {
        return GS::EmptyUniString;
    }

    return Intance->latestVersionDownloadUrl;
}

VersionChecker::VersionChecker (GS::UInt16 acMainVersionIn)
    : acMainVersion (acMainVersionIn)
{
    GetVersionFromGithub ();
}

const GS::UniString& VersionChecker::GetVersionFromGithub ()
{
    GS::UniString namePostfix = "AC" + GS::ValueToUniString (acMainVersion);
#if defined (WINDOWS)
    namePostfix += "_Win";
#else
    namePostfix += "_Mac";
#endif

    try {
        IO::URI::URI connectionUrl ("https://api.github.com");
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.Connect ();

        HTTP::Client::Request request (HTTP::MessageHeader::Method::Get, "/repos/ENZYME-APD/tapir-archicad-automation/releases/latest");
        request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
                "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36");
        clientConnection.Send (request);

        HTTP::Client::Response response;
        JSON::JDOMParser parser;
        JSON::ValueRef parsed = parser.Parse (clientConnection.BeginReceive (response));

        if (response.GetStatusCode () == HTTP::MessageHeader::StatusCode::OK) {
            JSON::ObjectValueRef outputObject = GS::DynamicCast<JSON::ObjectValue> (parsed);
            JSON::StringValueRef tagNameValue = GS::DynamicCast<JSON::StringValue> (outputObject->Get ("tag_name"));
            latestVersion = tagNameValue->Get ();

            JSON::ArrayValueRef arrayValue;
            std::map<GS::UniString, GS::UniString> nameDownloadUrlMap;
            outputObject->GetMandatoryMember ("assets", arrayValue);
            for (GS::UIndex i = 0; i < arrayValue->GetSize (); ++i) {
                JSON::ValueRef assetValue = arrayValue->Get (i);
                JSON::ObjectValueRef assetObject = GS::DynamicCast<JSON::ObjectValue> (assetValue);
                JSON::StringValueRef nameValue = GS::DynamicCast<JSON::StringValue> (assetObject->Get ("name"));

                GS::UniString name = nameValue->Get ();
                if (name.Contains (namePostfix)) {
                    JSON::StringValueRef downloadUrlValue = GS::DynamicCast<JSON::StringValue> (assetObject->Get ("browser_download_url"));
                    latestVersionDownloadUrl = downloadUrlValue->Get ();
                    latestVersionName = name;
                    break;
                }
            }
        }

        clientConnection.Close (false);
    } catch (...) {
    }

    return latestVersion;
}