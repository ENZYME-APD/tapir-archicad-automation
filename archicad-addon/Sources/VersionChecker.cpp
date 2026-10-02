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
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

static std::unique_ptr<VersionChecker> Intance;

// Splits "1.5.10" (or "v1.5.10") into its numeric components, so that
// versions compare by number and not as strings ("1.5.10" > "1.5.9").
static std::vector<GS::UInt32> ParseVersion (const std::string& version)
{
    std::vector<GS::UInt32> components;
    size_t i = 0;
    while (i < version.size () && !std::isdigit (static_cast<unsigned char> (version[i]))) {
        ++i;
    }
    while (i < version.size () && std::isdigit (static_cast<unsigned char> (version[i]))) {
        GS::UInt32 component = 0;
        while (i < version.size () && std::isdigit (static_cast<unsigned char> (version[i]))) {
            component = component * 10 + static_cast<GS::UInt32> (version[i] - '0');
            ++i;
        }
        components.push_back (component);
        if (i < version.size () && version[i] == '.') {
            ++i;
        } else {
            break;
        }
    }
    return components;
}

// Returns true when version is not newer than referenceVersion.
// An unparsable version (e.g. the GitHub query failed) counts as not newer.
static bool IsVersionNotNewerThan (const std::string& version, const std::string& referenceVersion)
{
    std::vector<GS::UInt32> components = ParseVersion (version);
    if (components.empty ()) {
        return true;
    }
    std::vector<GS::UInt32> referenceComponents = ParseVersion (referenceVersion);
    const size_t count = std::max (components.size (), referenceComponents.size ());
    for (size_t i = 0; i < count; ++i) {
        const GS::UInt32 component = i < components.size () ? components[i] : 0;
        const GS::UInt32 referenceComponent = i < referenceComponents.size () ? referenceComponents[i] : 0;
        if (component != referenceComponent) {
            return component < referenceComponent;
        }
    }
    return true;
}

void VersionChecker::CreateInstance (GS::UInt16 acMainVersion)
{
    Intance.reset (new VersionChecker (acMainVersion));
}

bool VersionChecker::IsUsingLatestVersion ()
{
    if (!Intance) {
        return true;
    }

    return IsVersionNotNewerThan (Intance->latestVersion.ToCStr ().Get (), ADDON_VERSION);
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