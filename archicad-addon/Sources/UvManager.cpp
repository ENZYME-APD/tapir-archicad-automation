#include "UvManager.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "Process.hpp"
#include "StringConversion.hpp"
#include "DGModule.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "Config.hpp"
#include "ResourceIds.hpp"
#include <vector>
#include <string>
#include <regex>

// The uv version Tapir's installer installs. It is fixed rather than the latest release because
// newer uv builds abort at start on macOS 15.0-15.6 with "macOS 15 (1507) or later required"
// (exit code 134, issue #392). It is bumped by hand: before raising it, check that the new
// version runs on the oldest supported macOS and Windows. A uv the user already has (0.7.0 or
// newer) is used as it is; this version only matters when Tapir installs uv.
static const char* const PinnedUvVersion = "0.8.17";


// The texts of the user interface, from the localized resources (RINT/AddOn.grc, see LOCALIZATION.md)
static GS::UniString UvText (Int32 index)
{
    return RSGetIndString (ID_UV_STRINGS, index, ACAPI_GetOwnResModule ());
}

static void OpenWebpage (const GS::UniString& webpage)
{
    const GS::UniString command = GS::UniString::Printf (
        "%s %T",
#ifdef WINDOWS
        "start",
#else
        "open",
#endif
        webpage.ToPrintf ());
    system (command.ToCStr ().Get ());
}

static std::vector<int> ParseVersionString (const GS::UniString& versionOutput)
{
    std::vector<int> versionParts;
    static const std::regex versionRegex (R"(\d+(\.\d+)*)"); // e.g., "0.7.13"
    std::smatch match;

    const std::string versionStdString = versionOutput.ToCStr ().Get ();

    if (std::regex_search (versionStdString, match, versionRegex)) {
        GS::UniString versionToken (match[0].str ().c_str ());
        GS::Array<GS::UniString> parts;
        versionToken.Split (".", &parts);
        for (const auto& part : parts) {
            try {
                versionParts.push_back (std::stoi (part.ToCStr ().Get ()));
            } catch (...) {
                // Ignore conversion errors
            }
        }
    }
    return versionParts;
}


static GS::UniString GetVersionFromExecutable (const GS::UniString& executablePath)
{
    try {
        GS::Process versionProcess = GS::Process::Create (executablePath, { "--version" }, GS::Process::CreateNoWindow, true, false, true);
        if (!versionProcess.IsValid ()) { return GS::EmptyUniString; }
        versionProcess.WaitFor ();
        if (versionProcess.GetExitCode () != 0) { return GS::EmptyUniString; }

        GS::IBinaryChannel& channel = versionProcess.GetStandardOutputChannel ();
        GS::UniString output;
        if (channel.GetAvailable () > 0) {
            const GS::USize uSize = static_cast<GS::USize>(channel.GetAvailable ());
            std::unique_ptr<char[]> buffer (new char[uSize]);
            channel.Read (buffer.get (), uSize);
            output = GS::UniString (buffer.get (), uSize, CC_UTF8);
            output.ReplaceAll ("\n", "");
            output.ReplaceAll ("\r\n", "");
        }
        return output;
    } catch (...) {
        return GS::EmptyUniString;
    }
}

static bool IsVersionSufficient (const std::vector<int>& version)
{
    if (version.empty ()) {
        return false;
    }
    // Version > 0.x.x is always sufficient
    if (version[0] > 0) {
        return true;
    }
    // Version 0.y.z requires y >= 7
    if (version[0] == 0 && version.size () > 1 && version[1] >= 7) {
        return true;
    }
    return false;
}


static GS::UniString GetValidPathIfSufficient (const GS::UniString& executablePath)
{
    GS::UniString versionOutput = GetVersionFromExecutable (executablePath);
    if (versionOutput.IsEmpty ()) {
        return GS::EmptyUniString;
    }

    if (IsVersionSufficient (ParseVersionString (versionOutput))) {
        ACAPI_WriteReport (UvText (ID_UV_STRINGS_FOUND_VALID), false, executablePath.ToPrintf (), versionOutput.ToPrintf ());
        return executablePath;
    } else {
        ACAPI_WriteReport (UvText (ID_UV_STRINGS_FOUND_OUTDATED), false, executablePath.ToPrintf (), versionOutput.ToPrintf ());
        return GS::EmptyUniString;
    }
}

static GS::UniString FindValidUvExecutablePath ()
{
    GS::Array<GS::UniString> pathsToTest;

#ifndef WINDOWS
    GS::Array<IO::Location> searchDirs;
    IO::Location homeDir;
    if (IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserHome, &homeDir) == NoError) {
        //default install path for uv v0.5.0 ++ (~/.local/bin)
        IO::Location localBinPath = homeDir;
        localBinPath.AppendToLocal (IO::Name (".local"));
        localBinPath.AppendToLocal (IO::Name ("bin"));
        searchDirs.Push (localBinPath);

        //legacy uv install path + Rust/Cargo tool path 
        IO::Location cargoPath = homeDir;
        cargoPath.AppendToLocal (IO::Name (".cargo"));
        cargoPath.AppendToLocal (IO::Name ("bin"));
        searchDirs.Push (cargoPath);
    }
    searchDirs.Push (IO::Location ("/opt/homebrew/bin")); // Apple Silicon Homebrew
    searchDirs.Push (IO::Location ("/usr/local/bin"));    // Intel Homebrew / other
    searchDirs.Push (IO::Location ("/usr/bin"));

    for (const auto& dir : searchDirs) {
        IO::Location uvExecutableLoc (dir, IO::Name ("uv"));
        bool exists = false;
        if (IO::fileSystem.Contains (uvExecutableLoc, &exists) == NoError && exists) {
            GS::UniString fullPath;
            uvExecutableLoc.ToPath (&fullPath);
            pathsToTest.Push (fullPath);
        }
    }
#endif

    pathsToTest.Push ("uv");

    for (const auto& path : pathsToTest) {
        GS::UniString validPath = GetValidPathIfSufficient (path);
        if (!validPath.IsEmpty ()) {
            return validPath;
        }
    }

    ACAPI_WriteReport (UvText (ID_UV_STRINGS_NOT_FOUND_REPORT), true);
    return GS::EmptyUniString;
}

GS::UniString UvManager::GetUvExecutablePath ()
{
    const GS::UniString& uvLocationFromConfigFile = Config::Instance ().uvLocation ();
    if (!uvLocationFromConfigFile.IsEmpty ()) {
        GS::UniString validPath = GetValidPathIfSufficient (uvLocationFromConfigFile);
        if (!validPath.IsEmpty ()) {
            return validPath;
        } else {
            static bool alertOnce = true;
            if (alertOnce) {
                DGAlert (DG_WARNING, UvText (ID_UV_STRINGS_CONFIGURED_INVALID_TITLE),
                    GS::UniString::Printf (UvText (ID_UV_STRINGS_CONFIGURED_INVALID_TEXT), uvLocationFromConfigFile.ToPrintf ()),
                    "", UvText (ID_UV_STRINGS_OK));
                alertOnce = false;
            }
        }
    }

    GS::UniString uvPath = FindValidUvExecutablePath ();
    if (!uvPath.IsEmpty ()) {
        return uvPath;
    }

    short response = DGAlert (DG_INFORMATION, UvText (ID_UV_STRINGS_MISSING_TITLE),
        UvText (ID_UV_STRINGS_MISSING_TEXT1),
        UvText (ID_UV_STRINGS_MISSING_TEXT2),
        UvText (ID_UV_STRINGS_INSTALL_FOR_ME), UvText (ID_UV_STRINGS_OPEN_INSTRUCTIONS), UvText (ID_UV_STRINGS_CANCEL));

    if (response == 1) { // "Install for me"
        if (AttemptAutomaticInstallation ()) {
            return FindValidUvExecutablePath ();
        }
    } else if (response == 2) { // "Open Instructions"
        OpenWebpage ("https://docs.astral.sh/uv/getting-started/installation/");
    }

    return GS::EmptyUniString;
}

static bool LooksLikeFilePath (const GS::UniString& pythonLocation)
{
    return pythonLocation.Contains ("/") || pythonLocation.Contains ("\\");
}

GS::Array<GS::UniString> UvManager::GetPythonSelectionArgs ()
{
    const GS::UniString& pythonLocationFromConfigFile = Config::Instance ().pythonLocation ();
    if (pythonLocationFromConfigFile.IsEmpty ()) {
        return {};
    }

    // The configured value is forwarded to 'uv' as-is, so it can be either a path to
    // an interpreter or a request like "3.12" or "python3.12". Only paths are checked here.
    if (LooksLikeFilePath (pythonLocationFromConfigFile)) {
        bool exists = false;
        if (IO::fileSystem.Contains (IO::Location (pythonLocationFromConfigFile), &exists) != NoError || !exists) {
            static bool alertOnce = true;
            if (alertOnce) {
                DGAlert (DG_WARNING, UvText (ID_UV_STRINGS_PYTHON_NOT_FOUND_TITLE),
                    GS::UniString::Printf (UvText (ID_UV_STRINGS_PYTHON_NOT_FOUND_TEXT), pythonLocationFromConfigFile.ToPrintf ()),
                    "", UvText (ID_UV_STRINGS_OK));
                alertOnce = false;
            }
            return {};
        }
    }

    ACAPI_WriteReport (UvText (ID_UV_STRINGS_USING_PYTHON), false, pythonLocationFromConfigFile.ToPrintf ());
    return { "--python", pythonLocationFromConfigFile };
}

GS::UniString UvManager::GetAbortedUvHint ()
{
    return GS::UniString::Printf (UvText (ID_UV_STRINGS_ABORTED_HINT), PinnedUvVersion);
}

// "<the report header>\n<prefix> <message>", the way TapirPalette::WriteReport writes into the Report window
static GS::UniString GetScriptReportText (Int32 prefixIndex, Int32 messageIndex)
{
    return RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_SCRIPT_REPORT_HEADER, ACAPI_GetOwnResModule ()) + "\n" +
        RSGetIndString (ID_REPORT_STRINGS, prefixIndex, ACAPI_GetOwnResModule ()) + " " +
        UvText (messageIndex);
}

bool UvManager::AttemptAutomaticInstallation ()
{
    ACAPI_WriteReport (GetScriptReportText (ID_REPORT_STRINGS_INFO_PREFIX, ID_UV_STRINGS_INSTALL_STARTING), false, PinnedUvVersion);

#ifdef WINDOWS
    const GS::UniString command = GS::UniString::Printf ("powershell -ExecutionPolicy ByPass -c \"irm https://astral.sh/uv/%s/install.ps1 | iex\"", PinnedUvVersion);
#else
    const GS::UniString command = GS::UniString::Printf ("curl -LsSf https://astral.sh/uv/%s/install.sh | sh", PinnedUvVersion);
#endif

    int result = system (command.ToCStr ().Get ());

    if (result == 0) {
        ACAPI_WriteReport (GetScriptReportText (ID_REPORT_STRINGS_INFO_PREFIX, ID_UV_STRINGS_INSTALL_FINISHED), false);
        if (!FindValidUvExecutablePath ().IsEmpty ()) {
            DGAlert (DG_INFORMATION, UvText (ID_UV_STRINGS_INSTALL_SUCCESS_TITLE), UvText (ID_UV_STRINGS_INSTALL_SUCCESS_TEXT), "", UvText (ID_UV_STRINGS_OK));
            return true;
        } else {
            DGAlert (DG_WARNING, UvText (ID_UV_STRINGS_INSTALL_RESTART_TITLE),
                    UvText (ID_UV_STRINGS_INSTALL_RESTART_TEXT), "", UvText (ID_UV_STRINGS_OK));
            return false;
        }
    } else {
        ACAPI_WriteReport (GetScriptReportText (ID_REPORT_STRINGS_ERROR_PREFIX, ID_UV_STRINGS_INSTALL_FAILED_REPORT), false, result);
        short response = DGAlert (DG_ERROR, UvText (ID_UV_STRINGS_INSTALL_FAILED_TITLE),
                UvText (ID_UV_STRINGS_INSTALL_FAILED_TEXT), "", UvText (ID_UV_STRINGS_GO_TO_UV_WEBSITE), UvText (ID_UV_STRINGS_OK));
        if (response == 1) { // "Go to uv website"
            OpenWebpage ("https://docs.astral.sh/uv/getting-started/installation/");
        }
        return false;
    }
}
