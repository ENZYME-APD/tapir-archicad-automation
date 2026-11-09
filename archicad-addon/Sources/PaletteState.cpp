#include "PaletteState.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "ObjectState.hpp"
#include "ObjectStateJSONConversion.hpp"

// Keeps the strings that are not empty, each once, in their order.
static GS::Array<GS::UniString> WithoutEmptyAndDuplicates (const GS::Array<GS::UniString>& strings)
{
    GS::Array<GS::UniString> result;
    for (const GS::UniString& str : strings) {
        if (!str.IsEmpty () && !result.Contains (str)) {
            result.Push (str);
        }
    }
    return result;
}

IO::Location PaletteState::GetFileLocation ()
{
    IO::Location fileLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &fileLoc);
    fileLoc.AppendToLocal (IO::Name ("Tapir"));
    fileLoc.AppendToLocal (IO::Name ("palette.json"));
    return fileLoc;
}

void PaletteState::Clear ()
{
    favourites.Clear ();
    recent.Clear ();
    selected.Clear ();
}

void PaletteState::Load ()
{
    Clear ();

    const IO::Location fileLoc = GetFileLocation ();
    bool exists = false;
    if (IO::fileSystem.Contains (fileLoc, &exists) != NoError || !exists) {
        return;
    }

    // Anything unexpected in the file (not JSON, a key of another type) leaves the state empty; the next Save
    // writes the file again.
    try {
        GS::ObjectState os;
        {
            IO::File file (fileLoc);
            if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
                return;
            }
            const GSErrCode err = JSON::ConvertToObjectState (file, os);
            file.Close ();
            if (err != NoError) {
                return;
            }
        }

        GS::Array<GS::UniString> readFavourites;
        GS::Array<GS::UniString> readRecent;
        GS::UniString readSelected;
        if (os.Contains ("favourites")) {
            os.Get ("favourites", readFavourites);
        }
        if (os.Contains ("recent")) {
            os.Get ("recent", readRecent);
        }
        if (os.Contains ("selected")) {
            os.Get ("selected", readSelected);
        }

        favourites = WithoutEmptyAndDuplicates (readFavourites);
        recent = WithoutEmptyAndDuplicates (readRecent);
        while (recent.GetSize () > MaxRecentCount) {
            recent.DeleteLast ();
        }
        selected = readSelected;
    } catch (...) {
        Clear ();
    }
}

void PaletteState::Save () const
{
    try {
        GS::ObjectState os;
        os.Add ("version", static_cast<Int32> (FileVersion));
        const auto& favouritesList = os.AddList<GS::UniString> ("favourites");
        for (const GS::UniString& path : favourites) {
            favouritesList (path);
        }
        const auto& recentList = os.AddList<GS::UniString> ("recent");
        for (const GS::UniString& path : recent) {
            recentList (path);
        }
        os.Add ("selected", selected);

        const IO::Location fileLoc = GetFileLocation ();
        IO::Location folderLoc = fileLoc;
        folderLoc.DeleteLastLocalName ();
        IO::fileSystem.CreateFolderTree (folderLoc);

        IO::File file (fileLoc, IO::File::OnNotFound::Create);
        if (file.GetStatus () != NoError || file.Open (IO::File::WriteEmptyMode) != NoError) {
            return;
        }
        constexpr bool prettyPrint = true;
        JSON::CreateFromObjectState (os, file, prettyPrint);
        file.Close ();
    } catch (...) {
        // The state is only a convenience: a file that cannot be written is left as it is.
    }
}

bool PaletteState::IsFavourite (const GS::UniString& scriptPath) const
{
    return !scriptPath.IsEmpty () && favourites.Contains (scriptPath);
}

void PaletteState::SetFavourite (const GS::UniString& scriptPath, bool isFavourite)
{
    if (scriptPath.IsEmpty ()) {
        return;
    }
    if (isFavourite) {
        if (!favourites.Contains (scriptPath)) {
            favourites.Push (scriptPath);
        }
    } else {
        favourites.DeleteAll (scriptPath);
    }
}

void PaletteState::AddRecent (const GS::UniString& scriptPath)
{
    if (scriptPath.IsEmpty ()) {
        return;
    }
    recent.DeleteAll (scriptPath);
    recent.Insert (0, scriptPath);
    while (recent.GetSize () > MaxRecentCount) {
        recent.DeleteLast ();
    }
}
