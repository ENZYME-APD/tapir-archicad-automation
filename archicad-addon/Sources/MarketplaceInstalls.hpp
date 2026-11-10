#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "Location.hpp"

#include <vector>

// The marketplace scripts this person installed with the Script Manager (#2408), kept in
// Documents/Tapir/marketplace/installed.json, never in the preferences blob, so an older Tapir is never affected:
//
//     {"version": 1,
//      "scripts": [{"slug", "name", "version", "entry", "enabled", "authorAllowed"}, ...],
//      "allowedAuthors": ["<author key>", ...]}
//
// An installed script's files are in Documents/Tapir/marketplace/<slug>/<version>/: its entry file, and the
// copies of builtin-scripts/aclib and builtin-scripts/utilities when it imports them. The Script Manager writes the
// file; the Tapir Palette reads it.
namespace MarketplaceInstalls {

struct InstalledScript {
    GS::UniString slug;
    GS::UniString name;
    GS::UniString version;
    GS::UniString entry;
    bool          enabled = true;          // listed in the Tapir Palette
    bool          authorAllowed = false;   // its author was allowed before it was installed
};

struct State {
    std::vector<InstalledScript>  scripts;
    GS::Array<GS::UniString>      allowedAuthors;   // see AuthorKey in ScriptManager.cpp

    const InstalledScript* Find (const GS::UniString& slug) const;
    InstalledScript*       Find (const GS::UniString& slug);
    bool                   IsAuthorAllowed (const GS::UniString& authorKey) const;
};

// Documents/Tapir/marketplace
IO::Location GetMarketplaceFolder ();
// Documents/Tapir/marketplace/<slug>
IO::Location GetScriptFolder (const GS::UniString& slug);
// Documents/Tapir/marketplace/<slug>/<version>
IO::Location GetVersionFolder (const GS::UniString& slug, const GS::UniString& version);

// Reads installed.json; an empty state when it is missing or cannot be read.
State Load ();
// Writes installed.json; false when it could not be written.
bool Save (const State& state);

// Whether text can be used as one file or folder name of an installed script: not empty, at most 100 characters,
// only ASCII letters, digits and "-", "_", "." (so no separators), and not "." or "..". The catalogue's address can
// be pointed anywhere, so a slug, a version or an entry is checked before it becomes a path.
bool IsSafeName (const GS::UniString& text);

}
