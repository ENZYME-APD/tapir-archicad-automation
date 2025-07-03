#include "UvManager.hpp"
#include "ACAPinc.h"
#include "Process.hpp"
#include "StringConversion.hpp"
#include "DGModule.hpp"
#include "IBinaryChannelUtilities.hpp"
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

    static const std::regex versionRegex (R"(\d+(\.\d+)*)"); // "0.7.13"
    std::smatch match;

    // We convert the GS::UniString to a standard C-style string for the regex engine
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

GS::UniString UvManager::GetUvVersionString ()
{
    try {
        GS::Process versionProcess = GS::Process::Create ("uv", { "--version" }, GS::Process::CreateNoWindow, true, false, true);
        if (!versionProcess.IsValid ()) {
            return GS::EmptyUniString;
        }

        versionProcess.WaitFor ();

        if (versionProcess.GetExitCode () != 0) {
            return GS::EmptyUniString;
        }

        GS::IBinaryChannel& channel = versionProcess.GetStandardOutputChannel ();
        GS::UniString versionOutput;

        if (channel.GetAvailable () > 0) {
            const GS::USize uSize = static_cast<GS::USize>(channel.GetAvailable ());
            std::unique_ptr<char[]> buffer (new char[uSize]);
            channel.Read (buffer.get (), uSize);
            versionOutput = GS::UniString (buffer.get (), uSize, CC_UTF8);
        }

        return versionOutput;
    } catch (...) {
        return GS::EmptyUniString;
    }
}

bool UvManager::AttemptAutomaticInstallation ()
{
    ACAPI_WriteReport ("----- Tapir Script Execution Report -----\n" "INFO: Attempting to run the official uv installer script...", false);

#ifdef WINDOWS
    const GS::UniString command = "powershell -ExecutionPolicy ByPass -c \"irm https://astral.sh/uv/install.ps1 | iex\"";
#else
    const GS::UniString command = "curl -LsSf https://astral.sh/uv/install.sh | sh";
#endif

    // Using system() will open a terminal/console window so the user can see the installer's progress.
    int result = system (command.ToCStr ().Get ());

    if (result == 0) {
        ACAPI_WriteReport ("----- Tapir Script Execution Report -----\n" "INFO: uv installer script finished successfully.", false);
        if (!GetUvVersionString ().IsEmpty ()) {
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

GS::UniString UvManager::GetUvExecutableCommand ()
{
    GS::UniString versionOutput = GetUvVersionString ();

    if (!versionOutput.IsEmpty ()) {
        std::vector<int> version = ParseVersionString (versionOutput);

        // We require at least version 0.7.0
        if (!version.empty () && (version[0] > 0 || (version[0] == 0 && version.size () > 1 && version[1] >= 7))) {
            return "uv";
        }

        versionOutput.Trim ();
        short res = DGAlert (DG_WARNING, "Unsupported 'uv' Version",
            GS::UniString::Printf ("An old version of 'uv' (%T) was found. Tapir requires version 0.7.0 or newer to support all script features.\n\n"
                "It's highly recommended to uninstall the old version and let Tapir install the latest one.", versionOutput.ToPrintf ()),
            "Continue Anyway", "Cancel");

        if (res == 2) { // User chose Cancel
            return GS::EmptyUniString;
        }
        return "uv"; // User chose to continue anyway.
    }

    short response = DGAlert (DG_INFORMATION, "Dependency Missing: 'uv'",
        "'uv' is required to run Python scripts but it was not found on your system.",
        "How would you like to proceed?",
        "Install for me", "Open Instructions", "Cancel");

    if (response == 1) { // "Install for me"
        if (AttemptAutomaticInstallation ()) {
            return "uv";
        }
    } else if (response == 2) { // "Open Instructions"
        OpenWebpage ("https://docs.astral.sh/uv/getting-started/installation/");
    }

    // Return empty if user cancelled or installation failed
    return GS::EmptyUniString;
}