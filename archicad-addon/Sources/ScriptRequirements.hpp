#pragma once

// The rules for the `min-tapir-version` and `archicad-versions` keys of a palette script's `# /// tapir` header
// (see ScriptInfo.hpp). Only the standard library is used, so the rules can be compiled and tested without
// Archicad: archicad-addon/Test/test_script_requirements.py does that.

#include <cstdint>
#include <string>
#include <vector>

namespace ScriptRequirements {

// Parses a version written as N.N.N (three numbers of digits only, at most 9 digits each, e.g. "1.10.0").
// Returns false, leaving parts empty, for anything else, also for "v1.8.1", "1.8" or "1.8.1-beta".
inline bool ParseVersion (const std::string& text, std::vector<std::uint32_t>& parts)
{
    parts.clear ();
    std::vector<std::uint32_t> result;
    size_t i = 0;
    while (true) {
        size_t digits = 0;
        std::uint32_t number = 0;
        while (i < text.size () && text[i] >= '0' && text[i] <= '9') {
            if (++digits > 9) {
                return false;
            }
            number = number * 10 + static_cast<std::uint32_t> (text[i] - '0');
            ++i;
        }
        if (digits == 0) {
            return false;
        }
        result.push_back (number);
        if (i == text.size ()) {
            break;
        }
        if (text[i] != '.' || result.size () == 3) {
            return false;
        }
        ++i;
    }
    if (result.size () != 3) {
        return false;
    }
    parts = result;
    return true;
}

enum class MinTapirVersionResult {
    NotGiven,     // the key is absent (empty)
    Malformed,    // not N.N.N: ignored, never blocks the script
    Satisfied,    // the installed Tapir is the same or newer
    TooOld        // the installed Tapir is older: the script does not run
};

// Compares numerically part by part, never as text: 1.10.0 is newer than 1.9.2.
// An installed version that is not N.N.N never blocks a script.
inline MinTapirVersionResult CheckMinTapirVersion (const std::string& minVersion, const std::string& installedVersion)
{
    if (minVersion.empty ()) {
        return MinTapirVersionResult::NotGiven;
    }
    std::vector<std::uint32_t> required;
    if (!ParseVersion (minVersion, required)) {
        return MinTapirVersionResult::Malformed;
    }
    std::vector<std::uint32_t> installed;
    if (!ParseVersion (installedVersion, installed)) {
        return MinTapirVersionResult::Satisfied;
    }
    for (size_t i = 0; i < required.size (); ++i) {
        if (installed[i] != required[i]) {
            return installed[i] > required[i] ? MinTapirVersionResult::Satisfied : MinTapirVersionResult::TooOld;
        }
    }
    return MinTapirVersionResult::Satisfied;
}

// False only when the list is given (not empty) and does not contain the running Archicad's main version.
// An unknown running version (0) is never reported as not listed. The list says where the author tested the
// script, not where it cannot run, so this only ever warns.
inline bool IsArchicadVersionListed (const std::vector<int>& archicadVersions, int runningMainVersion)
{
    if (archicadVersions.empty () || runningMainVersion <= 0) {
        return true;
    }
    for (int version : archicadVersions) {
        if (version == runningMainVersion) {
            return true;
        }
    }
    return false;
}

} // namespace ScriptRequirements
