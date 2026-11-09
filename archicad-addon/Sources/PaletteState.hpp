#pragma once

#include "UniString.hpp"
#include "Array.hpp"
#include "Location.hpp"

// The Tapir Palette's own state in Documents/Tapir/palette.json, next to config.json:
//   { "version": 1, "favourites": [<script path>, ...], "recent": [<script path>, ...], "selected": <script path> }
// A script is named by its file path (IO::Location::ToPath). A missing or broken file is never an error: it reads as
// empty and is written again on the next Save.
class PaletteState
{
public:
    static constexpr Int32 FileVersion = 1;
    static constexpr UIndex MaxRecentCount = 10;

    void Load ();
    void Save () const;

    const GS::Array<GS::UniString>& Favourites () const { return favourites; }
    bool IsFavourite (const GS::UniString& scriptPath) const;
    void SetFavourite (const GS::UniString& scriptPath, bool isFavourite);

    // The last MaxRecentCount scripts run, the newest first.
    const GS::Array<GS::UniString>& Recent () const { return recent; }
    void AddRecent (const GS::UniString& scriptPath);

    // The script selected in the palette's dropdown; empty when none was saved.
    const GS::UniString& Selected () const { return selected; }
    void SetSelected (const GS::UniString& scriptPath) { selected = scriptPath; }

    static IO::Location GetFileLocation ();

private:
    void Clear ();

    GS::Array<GS::UniString> favourites;
    GS::Array<GS::UniString> recent;
    GS::UniString selected;
};
