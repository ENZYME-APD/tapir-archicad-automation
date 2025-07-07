#include "UvManager.hpp"
#include "ACAPinc.h"
#include "Process.hpp"
#include "StringConversion.hpp"
#include "DGModule.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include <vector>
#include <string>
#include <regex>


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
        ACAPI_WriteReport ("Tapir found a valid 'uv' executable at: %T (Version: %T)", false, executablePath.ToPrintf (), versionOutput.ToPrintf ());
        return executablePath;
    } else {
        ACAPI_WriteReport ("Tapir found an outdated 'uv' at: %T (Version: %T). Ignoring.", false, executablePath.ToPrintf (), versionOutput.ToPrintf ());
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

    ACAPI_WriteReport ("Tapir could not find a 'uv' installation with version 0.7.0 or newer.", true);
    return GS::EmptyUniString;
}

GS::UniString UvManager::GetUvExecutablePath ()
{
    GS::UniString uvPath = FindValidUvExecutablePath ();
    if (!uvPath.IsEmpty ()) {
        return uvPath;
    }

    short response = DGAlert (DG_INFORMATION, "Dependency Missing: 'uv'",
        "'uv' (version 0.7.0 or newer) is required to run Python scripts but it was not found on your system.",
        "How would you like to proceed?",
        "Install for me", "Open Instructions", "Cancel");

    if (response == 1) { // "Install for me"
        if (AttemptAutomaticInstallation ()) {
            return FindValidUvExecutablePath ();
        }
    } else if (response == 2) { // "Open Instructions"
        OpenWebpage ("https://docs.astral.sh/uv/getting-started/installation/");
    }

    return GS::EmptyUniString;
}

bool UvManager::AttemptAutomaticInstallation ()
{
    ACAPI_WriteReport ("----- Tapir Script Execution Report -----\n" "INFO: Attempting to run the official uv installer script...", false);

#ifdef WINDOWS
    const GS::UniString command = "powershell -ExecutionPolicy ByPass -c \"irm https://astral.sh/uv/install.ps1 | iex\"";
#else
    const GS::UniString command = "curl -LsSf https://astral.sh/uv/install.sh | sh";
#endif

    int result = system (command.ToCStr ().Get ());

    if (result == 0) {
        ACAPI_WriteReport ("----- Tapir Script Execution Report -----\n" "INFO: uv installer script finished successfully.", false);
        if (!FindValidUvExecutablePath ().IsEmpty ()) {
            DGAlert (DG_INFORMATION, "Installation Successful", "'uv' has been installed and is ready to use.", "", "OK");
            return true;
        } else {
            DGAlert (DG_WARNING, "Installation May Require a Restart",
                    "'uv' was installed, but it's not yet available in the system's PATH.\n\n"
                    "Please restart Archicad for the changes to take effect.", "", "OK");
            return false;
        }
    } else {
        ACAPI_WriteReport ("----- Tapir Script Execution Report -----\n" "ERROR: uv installer script failed with exit code: %d", false, result);
        short response = DGAlert (DG_ERROR, "Installation Failed",
                "The automatic installation failed. Please check the Report window for details.\n\n"
                "You can try installing it manually from the official website.", "", "Go to uv website", "OK");
        if (response == 1) { // "Go to uv website"
            OpenWebpage ("https://docs.astral.sh/uv/getting-started/installation/");
        }
        return false;
    }
}
