#pragma once

#include "UniString.hpp"

class UvManager
{
public:
    UvManager () = default;

    GS::UniString GetUvExecutableCommand ();

private:
    GS::UniString GetUvVersionString ();
    bool AttemptAutomaticInstallation ();
};