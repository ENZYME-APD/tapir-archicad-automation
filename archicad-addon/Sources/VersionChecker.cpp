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
#include "MessageLoopExecutor.hpp"

#include <map>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

static std::unique_ptr<VersionChecker> Intance;

// Bounds how long the checker thread waits for GitHub. The lookup does not block Archicad, so the value only
// limits how long the thread lives when GitHub accepts the connection but never answers (#798).
static constexpr int VersionCheckTimeoutMs = 15 * 1000;

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

// Runs the GitHub lookup on the checker thread, then hands the completion back to the message loop,
// like the UIUpdaterThread of the palette does for the script output.
class VersionChecker::CheckTask : public GS::Runnable {
    GS::UInt16              acMainVersion;
    std::shared_ptr<Result> result;
    std::function<void ()>  onCompleted;

    class CompletedTask : public GS::Runnable {
        std::function<void ()> onCompleted;
    public:
        explicit CompletedTask (const std::function<void ()>& onCompletedIn) : onCompleted (onCompletedIn)
        {
        }
        virtual void Run () override
        {
            onCompleted ();
        }
    };

public:
    CheckTask (GS::UInt16 acMainVersionIn, const std::shared_ptr<Result>& resultIn, const std::function<void ()>& onCompletedIn)
        : acMainVersion (acMainVersionIn)
        , result (resultIn)
        , onCompleted (onCompletedIn)
    {
    }

    virtual void Run () override
    {
        GetVersionFromGithub (acMainVersion, *result);
        result->completed.store (true, std::memory_order_release);
        if (onCompleted) {
            GS::MessageLoopExecutor ().Execute (new CompletedTask (onCompleted));
        }
    }
};

void VersionChecker::CreateInstance (GS::UInt16 acMainVersion)
{
    Intance.reset (new VersionChecker (acMainVersion));
}

void VersionChecker::StartCheck (const std::function<void ()>& onCompleted)
{
    if (!Intance || Intance->checkStarted) {
        return;
    }
    Intance->checkStarted = true;

    try {
        Intance->checkerThread = GS::Thread (new CheckTask (Intance->acMainVersion, Intance->result, onCompleted), "TapirVersionCheck");
        Intance->checkerThread.Start ();
    } catch (...) {
        // Without the thread there is no lookup: the empty result counts as "no newer version", like a failed query.
        Intance->result->completed.store (true, std::memory_order_release);
    }
}

const VersionChecker::Result* VersionChecker::GetCompletedResult ()
{
    if (!Intance || !Intance->result->completed.load (std::memory_order_acquire)) {
        return nullptr;
    }

    return Intance->result.get ();
}

bool VersionChecker::IsCheckCompleted ()
{
    return GetCompletedResult () != nullptr;
}

bool VersionChecker::IsUsingLatestVersion ()
{
    const Result* result = GetCompletedResult ();
    if (result == nullptr) {
        return true;
    }

    // Without an Add-On for this Archicad version in the latest release there is nothing to update to.
    if (result->latestVersionDownloadUrl.IsEmpty ()) {
        return true;
    }

    return IsVersionNotNewerThan (result->latestVersion.ToCStr ().Get (), ADDON_VERSION);
}

// True when the latest release is newer but has no Add-On for this Archicad version.
bool VersionChecker::IsNewerVersionWithoutAddOn ()
{
    const Result* result = GetCompletedResult ();
    if (result == nullptr || !result->latestVersionDownloadUrl.IsEmpty ()) {
        return false;
    }

    return !IsVersionNotNewerThan (result->latestVersion.ToCStr ().Get (), ADDON_VERSION);
}

const GS::UniString& VersionChecker::LatestVersion ()
{
    const Result* result = GetCompletedResult ();
    if (result == nullptr) {
        return GS::EmptyUniString;
    }

    return result->latestVersion;
}

const GS::UniString& VersionChecker::LatestVersionName ()
{
    const Result* result = GetCompletedResult ();
    if (result == nullptr) {
        return GS::EmptyUniString;
    }

    return result->latestVersionName;
}

const GS::UniString& VersionChecker::LatestInstallerDownloadUrl ()
{
    const Result* result = GetCompletedResult ();
    if (result == nullptr) {
        return GS::EmptyUniString;
    }

    return result->latestInstallerDownloadUrl;
}

GS::UInt16 VersionChecker::ArchicadMainVersion ()
{
    if (!Intance) {
        return 0;
    }

    return Intance->acMainVersion;
}

VersionChecker::VersionChecker (GS::UInt16 acMainVersionIn)
    : acMainVersion (acMainVersionIn)
    , result (std::make_shared<Result> ())
{
}

void VersionChecker::GetVersionFromGithub (GS::UInt16 acMainVersion, Result& result)
{
    GS::UniString namePostfix = "AC" + GS::ValueToUniString (acMainVersion);
#if defined (WINDOWS)
    namePostfix += "_Win";
    const GS::UniString installerName ("TapirInstaller_Win.exe");
#else
    namePostfix += "_Mac";
    const GS::UniString installerName ("TapirInstaller_Mac.zip");
#endif

    try {
        IO::URI::URI connectionUrl ("https://api.github.com");
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.SetTimeout (VersionCheckTimeoutMs);
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
            result.latestVersion = tagNameValue->Get ();

            JSON::ArrayValueRef arrayValue;
            std::map<GS::UniString, GS::UniString> nameDownloadUrlMap;
            outputObject->GetMandatoryMember ("assets", arrayValue);
            for (GS::UIndex i = 0; i < arrayValue->GetSize (); ++i) {
                JSON::ValueRef assetValue = arrayValue->Get (i);
                JSON::ObjectValueRef assetObject = GS::DynamicCast<JSON::ObjectValue> (assetValue);
                JSON::StringValueRef nameValue = GS::DynamicCast<JSON::StringValue> (assetObject->Get ("name"));

                GS::UniString name = nameValue->Get ();
                if (result.latestVersionDownloadUrl.IsEmpty () && name.Contains (namePostfix)) {
                    JSON::StringValueRef downloadUrlValue = GS::DynamicCast<JSON::StringValue> (assetObject->Get ("browser_download_url"));
                    result.latestVersionDownloadUrl = downloadUrlValue->Get ();
                    result.latestVersionName = name;
                } else if (result.latestInstallerDownloadUrl.IsEmpty () && name == installerName) {
                    JSON::StringValueRef downloadUrlValue = GS::DynamicCast<JSON::StringValue> (assetObject->Get ("browser_download_url"));
                    result.latestInstallerDownloadUrl = downloadUrlValue->Get ();
                }
                if (!result.latestVersionDownloadUrl.IsEmpty () && !result.latestInstallerDownloadUrl.IsEmpty ()) {
                    break;
                }
            }
        }

        clientConnection.Close (false);
    } catch (...) {
        // A failed or timed out query leaves the result empty, which counts as "no newer version".
    }
}
