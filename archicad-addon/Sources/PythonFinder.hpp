#pragma once

#include "Array.hpp"
#include "UniString.hpp"
#include "Location.hpp"
#include <map>

class PythonFinder
{
public:
    PythonFinder ();

    void FindAllPythonExePath ();
    const std::map<GS::UniString, GS::UniString>& GetPythonExeMap () const
    {
        return pythonExeMap;
    }

    static bool IsValidLocation (const IO::Location& location);

private:
    GS::UniString GetPythonExePathUsingPATHEnvVariable (const GS::UniString& pythonAlias) const;
    GS::Array<GS::UniString> GetPythonExePathListFromFolder (const IO::Location& folderLoc) const;
    GS::Array<GS::UniString> GetPythonExePathListFromPyEnv () const;

    GS::UniString GetVersionOfPythonExe (const GS::UniString& pythonExe) const;

private:
    GS::Array<GS::UniString> pythonAliasesToCheck;
    GS::Array<GS::UniString> foldersToCheck;
    std::map<GS::UniString, GS::UniString> pythonExeMap;
};