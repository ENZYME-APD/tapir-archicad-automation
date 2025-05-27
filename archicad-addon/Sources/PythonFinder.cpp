#include "PythonFinder.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "Process.hpp"
#include "IOBinProtocolXs.hpp"

#include <regex>

static GS::UniString GetShell ()
{
#if defined (macintosh)
    return "/bin/bash";
#else
    IO::Location systemLoc;
    GS::UniString shellPath;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::System, &systemLoc);
    systemLoc.AppendToLocal (IO::Name ("cmd.exe"));
    systemLoc.ToPath (&shellPath);
    return shellPath;
#endif
}

static GS::UniString ExecuteProcess (const GS::UniString& command, const GS::Array<GS::UniString>& args)
{
    constexpr bool redirectStandardOutput = true;
    constexpr bool redirectStandardInput = false;
    constexpr bool redirectStandardError = true;
    GS::Process process = GS::Process::Create (command, args, GS::Process::CreateFlags::CreateNoWindow, redirectStandardOutput, redirectStandardInput, redirectStandardError);
    process.GetExitCode ();
    return GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (
            process.GetStandardOutputChannel (),
            GS::GetBinIProtocolX (),
            GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);
}

static GS::UniString RunShell (const GS::Array<GS::UniString>& args)
{
    return ExecuteProcess (GetShell (), args);
}

PythonFinder::PythonFinder ()
    : pythonAliasesToCheck ({"python3", "python"})
    , foldersToCheck ({"/usr/local/bin", "/usr/bin", "/bin", "/usr/sbin", "/sbin", "/usr/local/sbin"})
{
}

bool PythonFinder::IsValidLocation (const IO::Location& location)
{
    if (location.IsEmpty ()) {
        return false;
    }

    bool exists = false;
    GSErrCode err = IO::fileSystem.Contains (location, &exists);
    return err == NoError && exists;
}

GS::UniString PythonFinder::GetPythonExePathUsingPATHEnvVariable (const GS::UniString& pythonAlias) const
{
    GS::Array<GS::UniString> args;
    args.Push ("-c");
#if defined(macintosh)
    args.Push (GS::UniString::Printf ("PATH=\"$PATH:/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin\"; source ~/.bash_profile; %T %s",
        pythonAlias.ToPrintf (), "-c \"import sys;print(sys.executable)\""));
#else
    args.Append ({"/C", pythonAlias, "-c", "import sys;print(sys.executable)"});
#endif

    GS::UniString output = RunShell (args);
#if defined (macintosh)
    output.ReplaceAll ("\n", "");
#else
    output.ReplaceAll ("\r\n", "");
#endif

    return output;
}

GS::Array<GS::UniString> PythonFinder::GetPythonExePathListFromFolder (const IO::Location& folderLoc) const
{
	GS::Array<GS::UniString> output;

	IO::Folder (folderLoc).Enumerate ([&](const IO::Name& name, bool isFolder) {
		const IO::Location loc (folderLoc, name);

        if (isFolder) {
            output.Append (GetPythonExePathListFromFolder (loc));
            return;
        }

        #if defined (macintosh)
            static const std::regex pythonExeRegex ("python[\\d.]*");
        #else
            static const std::regex pythonExeRegex ("python[\\d.]*\\.exe");
        #endif

		if (std::regex_match (name.ToString ().ToCStr ().Get (), pythonExeRegex)) {
            GS::UniString filePath;
			loc.ToPath (&filePath);
            output.Push (filePath);
		}
	});

    return output;
}

GS::Array<GS::UniString> PythonFinder::GetPythonExePathListFromPyEnv () const
{
	IO::Location homeLoc;
	IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserHome, &homeLoc);
    IO::Location pyEnvLoc (homeLoc);
    pyEnvLoc.AppendToLocal (IO::Name (".pyenv"));
#ifdef WINDOWS
    pyEnvLoc.AppendToLocal (IO::Name ("pyenv-win"));
#endif
    pyEnvLoc.AppendToLocal (IO::Name ("versions"));

    if (!IsValidLocation (pyEnvLoc)) {
        return {};
    }

    return GetPythonExePathListFromFolder (pyEnvLoc);
}

GS::UniString PythonFinder::GetVersionOfPythonExe (const GS::UniString& pythonExe) const
{
    GS::UniString output = ExecuteProcess (pythonExe, {"--version"});
#if defined (macintosh)
    output.ReplaceAll ("\n", "");
#else
    output.ReplaceAll ("\r\n", "");
#endif

    return output;
}

void PythonFinder::FindAllPythonExePath ()
{
    pythonExeMap.clear ();

    for (const auto& pythonAlias : pythonAliasesToCheck) {
        const GS::UniString output = GetPythonExePathUsingPATHEnvVariable (pythonAlias);

        if (!IsValidLocation (IO::Location (output))) {
            continue;
        }

        pythonExeMap.emplace (GetVersionOfPythonExe (output), output);
    }

    for (const auto& pythonExePath : GetPythonExePathListFromPyEnv ()) {
        pythonExeMap.emplace (GetVersionOfPythonExe (pythonExePath), pythonExePath);
    }

    for (const auto& kv : pythonExeMap) {
        ACAPI_WriteReport ("Tapir found %T at %T", false, kv.first.ToPrintf (), kv.second.ToPrintf ());
    }
}