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


private:
    bool AttemptAutomaticInstallation ();
};