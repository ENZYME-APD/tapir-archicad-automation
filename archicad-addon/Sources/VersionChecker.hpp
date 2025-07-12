#pragma once

#include "Definitions.hpp"
#include "UniString.hpp"

class VersionChecker {
public:
    static void CreateInstance (GS::UInt16 acMainVersion);

    static bool IsUsingLatestVersion ();
    static const GS::UniString& LatestVersion ();
    static const GS::UniString& LatestVersionName ();
    static const GS::UniString& LatestVersionDownloadUrl ();

private:
    VersionChecker (GS::UInt16 acMainVersionIn);

    const GS::UniString& GetVersionFromGithub ();

private:
    GS::UInt16    acMainVersion;
    GS::UniString latestVersion;
    GS::UniString latestVersionName;
    GS::UniString latestVersionDownloadUrl;
};