#include "MarketplaceInstalls.hpp"
#include "ObjectStateJSONConversion.hpp"
#include "FileSystem.hpp"
#include "File.hpp"

namespace MarketplaceInstalls {

const InstalledScript* State::Find (const GS::UniString& slug) const
{
    for (const InstalledScript& script : scripts) {
        if (script.slug == slug) {
            return &script;
        }
    }
    return nullptr;
}

InstalledScript* State::Find (const GS::UniString& slug)
{
    for (InstalledScript& script : scripts) {
        if (script.slug == slug) {
            return &script;
        }
    }
    return nullptr;
}

bool State::IsAuthorAllowed (const GS::UniString& authorKey) const
{
    for (const GS::UniString& allowed : allowedAuthors) {
        if (allowed == authorKey) {
            return true;
        }
    }
    return false;
}

IO::Location GetMarketplaceFolder ()
{
    IO::Location folder;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &folder);
    folder.AppendToLocal (IO::Name ("Tapir"));
    folder.AppendToLocal (IO::Name ("marketplace"));
    return folder;
}

IO::Location GetScriptFolder (const GS::UniString& slug)
{
    IO::Location folder = GetMarketplaceFolder ();
    folder.AppendToLocal (IO::Name (slug));
    return folder;
}

IO::Location GetVersionFolder (const GS::UniString& slug, const GS::UniString& version)
{
    IO::Location folder = GetScriptFolder (slug);
    folder.AppendToLocal (IO::Name (version));
    return folder;
}

static IO::Location GetInstalledFileLocation ()
{
    IO::Location location = GetMarketplaceFolder ();
    location.AppendToLocal (IO::Name ("installed.json"));
    return location;
}

State Load ()
{
    State state;

    const IO::Location location = GetInstalledFileLocation ();
    bool exists = false;
    if (IO::fileSystem.Contains (location, &exists) != NoError || !exists) {
        return state;
    }
    IO::File file (location);
    if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
        return state;
    }
    GS::ObjectState os;
    const GSErrCode err = JSON::ConvertToObjectState (file, os);
    file.Close ();
    if (err != NoError) {
        return state;
    }

    GS::Array<GS::ObjectState> scriptsOS;
    os.Get ("scripts", scriptsOS);
    for (const GS::ObjectState& scriptOS : scriptsOS) {
        InstalledScript script;
        scriptOS.Get ("slug", script.slug);
        scriptOS.Get ("name", script.name);
        scriptOS.Get ("version", script.version);
        scriptOS.Get ("entry", script.entry);
        scriptOS.Get ("enabled", script.enabled);
        scriptOS.Get ("authorAllowed", script.authorAllowed);
        // Only what can be a path is kept: the file may have been edited by hand.
        if (!IsSafeName (script.slug) || !IsSafeName (script.version) || !IsSafeName (script.entry) || state.Find (script.slug) != nullptr) {
            continue;
        }
        if (script.name.IsEmpty ()) {
            script.name = script.slug;
        }
        state.scripts.push_back (script);
    }
    os.Get ("allowedAuthors", state.allowedAuthors);
    return state;
}

bool Save (const State& state)
{
    GS::ObjectState os;
    os.Add ("version", static_cast<Int32> (1));
    const auto& scriptsList = os.AddList<GS::ObjectState> ("scripts");
    for (const InstalledScript& script : state.scripts) {
        scriptsList (GS::ObjectState (
            "slug", script.slug,
            "name", script.name,
            "version", script.version,
            "entry", script.entry,
            "enabled", script.enabled,
            "authorAllowed", script.authorAllowed));
    }
    os.Add ("allowedAuthors", state.allowedAuthors);

    IO::fileSystem.CreateFolderTree (GetMarketplaceFolder ());
    IO::File file (GetInstalledFileLocation (), IO::File::OnNotFound::Create);
    if (file.GetStatus () != NoError || file.Open (IO::File::WriteEmptyMode) != NoError) {
        return false;
    }
    constexpr bool prettyPrint = true;
    const GSErrCode err = JSON::CreateFromObjectState (os, file, prettyPrint);
    file.Close ();
    return err == NoError;
}

bool IsSafeName (const GS::UniString& text)
{
    if (text.IsEmpty () || text.GetLength () > 100 || text == "." || text == "..") {
        return false;
    }
    for (UIndex i = 0; i < text.GetLength (); ++i) {
        const GS::UniChar::Layout c = static_cast<GS::UniChar::Layout> (text[i]);
        const bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

// Touched on the main thread only, by the Script Manager and the Tapir Palette.
static GS::Array<GS::UniString> updatableSlugs;

void SetUpdatableSlugs (const GS::Array<GS::UniString>& slugs)
{
    updatableSlugs = slugs;
}

bool HasUpdate (const GS::UniString& slug)
{
    for (const GS::UniString& updatable : updatableSlugs) {
        if (updatable == slug) {
            return true;
        }
    }
    return false;
}

}
