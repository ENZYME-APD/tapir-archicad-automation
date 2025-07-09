#pragma once

#include "UniString.hpp"

class UvManager
{
public:
    UvManager () = default;

    GS::UniString GetUvExecutablePath ();


private:
    bool AttemptAutomaticInstallation ();
};