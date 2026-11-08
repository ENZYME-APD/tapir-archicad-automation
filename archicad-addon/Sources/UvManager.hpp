#pragma once

#include "UniString.hpp"
#include "Array.hpp"

class UvManager
{
public:
    UvManager () = default;

    GS::UniString GetUvExecutablePath ();

    // Returns {"--python", <configured interpreter>} or an empty array when
    // no interpreter is configured and 'uv' should pick one on its own.
    GS::Array<GS::UniString> GetPythonSelectionArgs ();

    // The text reported when 'uv' aborted (exit code 134) on macOS: the likely cause
    // and how to make Tapir use a uv build that runs on this macOS.
    static GS::UniString GetAbortedUvHint ();


private:
    bool AttemptAutomaticInstallation ();
};