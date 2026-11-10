#include "ScriptsPanel.hpp"
#include "ResourceIds.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "File.hpp"
#include "FileSystem.hpp"
#include "ObjectStateJSONConversion.hpp"

// The texts of the panel, from the localized resources (RINT/AddOn.grc, see LOCALIZATION.md)
static GS::UniString PanelText (Int32 index)
{
    return RSGetIndString (ID_SCRIPTS_PANEL_STRINGS, index, ACAPI_GetOwnResModule ());
}

ScriptsPanel::ScriptsPanel (DG::Palette& _palette, short expandButtonItem, short separatorItem)
    : palette (_palette)
    , expandButton (_palette, expandButtonItem)
    , separator (_palette, separatorItem)
{
    // Never narrower than the bar's controls, never lower than the bar.
    palette.SetMinClientSize (palette.GetOriginalClientWidth (), BarHeight);
}

bool ScriptsPanel::IsOpen () const
{
    return isOpen;
}

bool ScriptsPanel::IsTallerThanBar () const
{
    return palette.GetClientHeight () >= BarHeight + MinOpenGrowth;
}

void ScriptsPanel::SetOpen (bool open)
{
    // The requested state counts, not the height read back at once: the palette's client height may only follow
    // the resize later (its PanelResized event then syncs the state with Resized).
    if (open != isOpen) {
        if (open) {
            palette.SetClientHeight (openHeight);
        } else {
            const short height = palette.GetClientHeight ();
            if (height >= MinRememberedOpenHeight) {
                openHeight = height;
            }
            palette.SetClientHeight (BarHeight);
        }
        isOpen = open;
    }
    Layout ();
    SaveState ();
}

void ScriptsPanel::SyncWithSize ()
{
    isOpen = IsTallerThanBar ();
    Layout ();
}

void ScriptsPanel::Layout ()
{
    const short width = palette.GetClientWidth ();
    const bool open = isOpen;

    palette.BeginMoveResizeItems ();
    expandButton.SetPosition (static_cast<short> (width - BarHeight), 0);
    separator.SetPosition (0, BarHeight);
    separator.SetSize (width, 2);
    palette.EndMoveResizeItems ();

    separator.SetVisibility (open);
    SetExpandButtonIcon (open);
}

void ScriptsPanel::Resized ()
{
    SyncWithSize ();
    const short height = palette.GetClientHeight ();
    if (isOpen && height >= MinRememberedOpenHeight) {
        openHeight = height;
    }
    SaveState ();
}

void ScriptsPanel::SetExpandButtonIcon (bool open)
{
    if (open == expandButtonShowsOpen) {
        return;
    }
    expandButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), open ? ID_EXPAND_UP_ICON : ID_EXPAND_DOWN_ICON));
    expandButtonShowsOpen = open;
}

bool ScriptsPanel::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () != &expandButton) {
        return false;
    }
    SetOpen (!isOpen);
    return true;
}

bool ScriptsPanel::ToolTipRequested (const DG::ItemHelpEvent& ev, GS::UniString* toolTipText) const
{
    if (ev.GetSource () != &expandButton) {
        return false;
    }
    if (toolTipText != nullptr) {
        *toolTipText = PanelText (IsOpen () ? ID_SCRIPTS_PANEL_STRINGS_HIDE_PANEL : ID_SCRIPTS_PANEL_STRINGS_SHOW_PANEL);
    }
    return true;
}

// ---- The saved state ----
//
// { "width": <client width>, "openHeight": <client height with the panel open>, "open": <bool> }. A missing or
// unreadable file leaves the palette as the resources make it: the bar alone.

IO::Location ScriptsPanel::GetStateFileLocation ()
{
    IO::Location location;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &location);
    location.AppendToLocal (IO::Name ("Tapir"));
    location.AppendToLocal (IO::Name ("scripts-panel.json"));
    return location;
}

void ScriptsPanel::RestoreState ()
{
    const IO::Location location = GetStateFileLocation ();
    bool exists = false;
    if (IO::fileSystem.Contains (location, &exists) == NoError && exists) {
        GS::ObjectState os;
        IO::File file (location);
        GSErrCode err = file.GetStatus ();
        if (err == NoError) {
            err = file.Open (IO::File::OpenMode::ReadMode);
        }
        if (err == NoError) {
            err = JSON::ConvertToObjectState (file, os);
            file.Close ();
        }
        if (err == NoError) {
            Int32 width = 0;
            Int32 savedHeight = 0;
            bool open = false;
            os.Get ("width", width);
            os.Get ("openHeight", savedHeight);
            os.Get ("open", open);
            if (savedHeight >= MinRememberedOpenHeight && savedHeight <= 4000) {
                openHeight = static_cast<short> (savedHeight);
            }
            const short minWidth = palette.GetOriginalClientWidth ();
            const short newWidth = (width > minWidth && width <= 4000) ? static_cast<short> (width) : minWidth;
            const short newHeight = open ? openHeight : static_cast<short> (BarHeight);
            palette.SetClientSize (newWidth, newHeight);
            isOpen = open;
        }
    }
    Layout ();
    // The state as it is now counts as saved: opening Archicad writes nothing.
    hasSavedState = true;
    savedWidth = palette.GetClientWidth ();
    savedOpenHeight = openHeight;
    savedOpen = isOpen;
}

void ScriptsPanel::SaveState ()
{
    const short width = palette.GetClientWidth ();
    const bool open = isOpen;
    if (hasSavedState && width == savedWidth && openHeight == savedOpenHeight && open == savedOpen) {
        return;
    }

    GS::ObjectState os;
    os.Add ("width", static_cast<Int32> (width));
    os.Add ("openHeight", static_cast<Int32> (openHeight));
    os.Add ("open", open);

    const IO::Location location = GetStateFileLocation ();
    IO::Location folder = location;
    folder.DeleteLastLocalName ();
    bool folderExists = false;
    if (IO::fileSystem.Contains (folder, &folderExists) != NoError || !folderExists) {
        IO::fileSystem.CreateFolder (folder);
    }

    IO::File file (location, IO::File::OnNotFound::Create);
    GSErrCode err = file.GetStatus ();
    if (err == NoError) {
        err = file.Open (IO::File::OpenMode::WriteEmptyMode);
    }
    if (err == NoError) {
        constexpr bool prettyPrint = true;
        err = JSON::CreateFromObjectState (os, file, prettyPrint);
        file.Close ();
    }
    if (err == NoError) {
        hasSavedState = true;
        savedWidth = width;
        savedOpenHeight = openHeight;
        savedOpen = open;
    }
}
