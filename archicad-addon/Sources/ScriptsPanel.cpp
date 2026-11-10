#include "ScriptsPanel.hpp"
#include "ResourceIds.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "File.hpp"
#include "FileSystem.hpp"
#include "ObjectStateJSONConversion.hpp"

#include <initializer_list>

// The texts of the panel, from the localized resources (RINT/AddOn.grc, see LOCALIZATION.md)
static GS::UniString PanelText (Int32 index)
{
    return RSGetIndString (ID_SCRIPTS_PANEL_STRINGS, index, ACAPI_GetOwnResModule ());
}

// The category keys of the `# /// tapir` header, in the order of the list's groups. Their display names are the
// strings from ID_SCRIPTS_PANEL_STRINGS_FIRST_CATEGORY on, in the same order; a script without one of these keys is
// in the last group, Other.
static const char* const Categories[] = {
    "documentation-layouts",
    "element-data-properties",
    "modeling-geometry",
    "zones-areas",
    "import-export",
    "quality-assurance",
    "quantities-scheduling",
    "libraries-objects",
    "navigator-views",
    "classification-attributes",
    "batch-automation",
    "utilities"
};
static constexpr UIndex CategoryCount = sizeof (Categories) / sizeof (Categories[0]);
static constexpr UIndex OtherGroup = CategoryCount;

// The category filter's items: All, the categories in the order above, Other.
static constexpr short CategoryPopUpAllItem = 1;

// The layout below the bar
static constexpr short Margin = 6;
static constexpr short SearchTop = ScriptsPanel::BarHeight + 8;
static constexpr short SearchHeight = 22;
static constexpr short CategoryWidth = 200;
static constexpr short ListTop = SearchTop + SearchHeight + 6;
static constexpr short NameHeight = 20;
static constexpr short BadgeHeight = 16;
static constexpr short DetailsHeight = 84;
static constexpr short DetailsBlockHeight = Margin + NameHeight + 2 + BadgeHeight + 2 + DetailsHeight + Margin;
static constexpr short MinListHeight = 40;
static constexpr short ListScrollBarWidth = 18;
static constexpr short SourceColumnWidth = 120;

// The list's columns (tab fields): the name, what the script does, where it comes from.
static constexpr short NameColumn = 1;
static constexpr short SummaryColumn = 2;
static constexpr short SourceColumn = 3;

static UIndex GetGroup (const ScriptInfo& info)
{
    for (UIndex i = 0; i < CategoryCount; ++i) {
        if (info.category == GS::UniString (Categories[i])) {
            return i;
        }
    }
    return OtherGroup;
}

static GS::UniString GetGroupName (UIndex group)
{
    return PanelText (static_cast<Int32> (ID_SCRIPTS_PANEL_STRINGS_FIRST_CATEGORY + group));
}

static GS::UniString LowerCased (const GS::UniString& text)
{
    GS::UniString result = text;
    result.SetToLowerCase ();
    return result;
}

ScriptsPanel::ScriptsPanel (DG::Palette& _palette, short expandButtonItem, short firstPanelItem, const ScriptsGetter& _getScripts)
    : palette (_palette)
    , expandButton (_palette, expandButtonItem)
    , separator (_palette, firstPanelItem)
    , searchEdit (_palette, static_cast<short> (firstPanelItem + 1))
    , categoryPopUp (_palette, static_cast<short> (firstPanelItem + 2))
    , scriptList (_palette, static_cast<short> (firstPanelItem + 3))
    , nameText (_palette, static_cast<short> (firstPanelItem + 4))
    , badgeText (_palette, static_cast<short> (firstPanelItem + 5))
    , detailsText (_palette, static_cast<short> (firstPanelItem + 6))
    , getScripts (_getScripts)
{
    // Never narrower than the bar's controls, never lower than the bar.
    palette.SetMinClientSize (palette.GetOriginalClientWidth (), BarHeight);

    scriptList.SetTabFieldCount (3);
    FillCategoryPopUp ();

    searchEdit.Attach (*this);
    categoryPopUp.Attach (*this);
    scriptList.Attach (*this);
}

ScriptsPanel::~ScriptsPanel ()
{
    searchEdit.Detach (*this);
    categoryPopUp.Detach (*this);
    scriptList.Detach (*this);
}

bool ScriptsPanel::IsOpen () const
{
    return palette.GetClientHeight () >= BarHeight + MinOpenGrowth;
}

void ScriptsPanel::SetOpen (bool open)
{
    if (open != IsOpen ()) {
        if (open) {
            palette.SetClientHeight (openHeight);
        } else {
            const short height = palette.GetClientHeight ();
            if (height >= MinRememberedOpenHeight) {
                openHeight = height;
            }
            palette.SetClientHeight (BarHeight);
        }
    }
    Layout ();
    SaveState ();
}

void ScriptsPanel::Layout ()
{
    const short width = palette.GetClientWidth ();
    const bool open = IsOpen ();

    const short height = palette.GetClientHeight ();
    const short innerWidth = static_cast<short> (width - 2 * Margin);
    const short searchWidth = static_cast<short> (innerWidth - Margin - CategoryWidth);
    short listHeight = static_cast<short> (height - ListTop - DetailsBlockHeight);
    if (listHeight < MinListHeight) {
        listHeight = MinListHeight;
    }
    const short nameTop = static_cast<short> (ListTop + listHeight + Margin);
    const short badgeTop = static_cast<short> (nameTop + NameHeight + 2);
    const short detailsTop = static_cast<short> (badgeTop + BadgeHeight + 2);

    palette.BeginMoveResizeItems ();
    expandButton.SetPosition (static_cast<short> (width - BarHeight), 0);
    separator.SetPosition (0, BarHeight);
    separator.SetSize (width, 2);
    searchEdit.SetPosition (Margin, SearchTop);
    searchEdit.SetSize (searchWidth, SearchHeight);
    categoryPopUp.SetPosition (static_cast<short> (width - Margin - CategoryWidth), SearchTop);
    categoryPopUp.SetSize (CategoryWidth, SearchHeight);
    scriptList.SetPosition (Margin, ListTop);
    scriptList.SetSize (innerWidth, listHeight);
    nameText.SetPosition (Margin, nameTop);
    nameText.SetSize (innerWidth, NameHeight);
    badgeText.SetPosition (Margin, badgeTop);
    badgeText.SetSize (innerWidth, BadgeHeight);
    detailsText.SetPosition (Margin, detailsTop);
    detailsText.SetSize (innerWidth, DetailsHeight);
    palette.EndMoveResizeItems ();

    // The columns: the name about a third of the list, the source at the right, what the script does between them.
    const short columnsWidth = static_cast<short> (innerWidth - ListScrollBarWidth);
    const short nameEnd = static_cast<short> (columnsWidth / 3);
    const short sourceBegin = static_cast<short> (columnsWidth - SourceColumnWidth);
    scriptList.SetTabFieldProperties (NameColumn, 0, nameEnd, DG::ListBox::Left, DG::ListBox::EndTruncate);
    scriptList.SetTabFieldProperties (SummaryColumn, static_cast<short> (nameEnd + 4), static_cast<short> (sourceBegin - 4), DG::ListBox::Left, DG::ListBox::EndTruncate);
    scriptList.SetTabFieldProperties (SourceColumn, sourceBegin, columnsWidth, DG::ListBox::Left, DG::ListBox::EndTruncate);

    separator.SetVisibility (open);
    searchEdit.SetVisibility (open);
    categoryPopUp.SetVisibility (open);
    scriptList.SetVisibility (open);
    nameText.SetVisibility (open);
    badgeText.SetVisibility (open);
    detailsText.SetVisibility (open);
    SetExpandButtonIcon (open);
}

// ---- The list and the details ----

void ScriptsPanel::FillCategoryPopUp ()
{
    categoryPopUp.DeleteItem (DG_ALL_ITEMS);
    categoryPopUp.AppendItem ();
    categoryPopUp.SetItemText (DG::PopUp::BottomItem, PanelText (ID_SCRIPTS_PANEL_STRINGS_ALL));
    for (UIndex group = 0; group <= OtherGroup; ++group) {
        categoryPopUp.AppendItem ();
        categoryPopUp.SetItemText (DG::PopUp::BottomItem, GetGroupName (group));
    }
    categoryPopUp.SelectItem (CategoryPopUpAllItem);
}

void ScriptsPanel::ScriptListChanged ()
{
    scripts = getScripts ();
    FillList ();
}

// The scripts grouped by category, each group under a bold row with its name. Within a group the scripts whose name
// matches the search come first, then those whose summary matches; the search ignores case.
void ScriptsPanel::FillList ()
{
    GS::UniString search = searchEdit.GetText ();
    search.Trim ();
    search = LowerCased (search);
    const short categoryItem = categoryPopUp.GetSelectedItem ();
    const bool allCategories = categoryItem <= CategoryPopUpAllItem;
    const UIndex shownGroup = allCategories ? 0 : static_cast<UIndex> (categoryItem - CategoryPopUpAllItem - 1);

    scriptList.DisableDraw ();
    scriptList.DeleteItem (DG::ListBox::AllItems);

    const auto appendRow = [this] (const GS::UniString& name, const GS::UniString& summary, const GS::UniString& source, DGUserData value) {
        scriptList.AppendItem ();
        const short row = scriptList.GetItemCount ();
        scriptList.SetTabItemText (row, NameColumn, name);
        scriptList.SetTabItemText (row, SummaryColumn, summary);
        scriptList.SetTabItemText (row, SourceColumn, source);
        scriptList.SetItemValue (row, value);
        return row;
    };

    short rowToSelect = 0;
    for (UIndex group = 0; group <= OtherGroup; ++group) {
        if (!allCategories && group != shownGroup) {
            continue;
        }

        GS::Array<UIndex> nameMatches;
        GS::Array<UIndex> summaryMatches;
        for (UIndex i = 0; i < scripts.GetSize (); ++i) {
            const Script& script = scripts[i];
            if (GetGroup (script.info) != group) {
                continue;
            }
            if (search.IsEmpty () || LowerCased (script.name).Contains (search) || LowerCased (script.fileName).Contains (search)) {
                nameMatches.Push (i);
            } else if (LowerCased (script.info.summary).Contains (search)) {
                summaryMatches.Push (i);
            }
        }
        if (nameMatches.IsEmpty () && summaryMatches.IsEmpty ()) {
            continue;
        }

        const short headerRow = appendRow (GetGroupName (group), GS::EmptyUniString, GS::EmptyUniString, 0);
        scriptList.SetItemFontStyle (headerRow, DG::Font::Bold);
        for (const GS::Array<UIndex>* matches : { &nameMatches, &summaryMatches }) {
            for (UIndex i : *matches) {
                const Script& script = scripts[i];
                const short row = appendRow (script.name, script.info.summary, script.source, static_cast<DGUserData> (i + 1));
                if (!selectedLocation.IsEmpty () && script.fileLocation == selectedLocation) {
                    rowToSelect = row;
                }
            }
        }
    }

    if (scriptList.GetItemCount () == 0 && !scripts.IsEmpty ()) {
        appendRow (PanelText (ID_SCRIPTS_PANEL_STRINGS_NO_MATCH), GS::EmptyUniString, GS::EmptyUniString, 0);
    }
    if (rowToSelect > 0) {
        scriptList.SelectItem (rowToSelect);
    }

    scriptList.EnableDraw ();
    ShowDetails ();
}

const ScriptsPanel::Script* ScriptsPanel::GetSelectedScript () const
{
    const short row = scriptList.GetSelectedItem ();
    if (row <= 0 || row > scriptList.GetItemCount ()) {
        return nullptr;
    }
    const DGUserData value = scriptList.GetItemValue (row);
    if (value <= 0 || static_cast<UIndex> (value) > scripts.GetSize ()) {
        return nullptr;
    }
    return &scripts[static_cast<UIndex> (value - 1)];
}

// The name, the badge of the `changes` key, and below them the summary, the file and where it comes from, the author
// and the version when the header gives them, and the Tapir and Archicad versions it needs, worded as in the bar's
// tooltip.
void ScriptsPanel::ShowDetails ()
{
    const Script* script = GetSelectedScript ();
    if (script == nullptr) {
        nameText.SetText (GS::EmptyUniString);
        badgeText.SetText (GS::EmptyUniString);
        detailsText.SetText (scripts.IsEmpty () ? GS::EmptyUniString : PanelText (ID_SCRIPTS_PANEL_STRINGS_SELECT_SCRIPT));
        return;
    }

    GS::UniString details;
    const auto addLine = [&details] (const GS::UniString& line) {
        if (line.IsEmpty ()) {
            return;
        }
        if (!details.IsEmpty ()) {
            details += "\n";
        }
        details += line;
    };
    addLine (script->info.summary);
    addLine (script->source.IsEmpty () ? script->fileName : script->fileName + " (" + script->source + ")");
    GS::UniString authorAndVersion;
    if (!script->info.author.IsEmpty ()) {
        authorAndVersion = GS::UniString::Printf (PanelText (ID_SCRIPTS_PANEL_STRINGS_BY_AUTHOR), script->info.author.ToPrintf ());
    }
    if (!script->info.version.IsEmpty ()) {
        if (!authorAndVersion.IsEmpty ()) {
            authorAndVersion += ", ";
        }
        authorAndVersion += GS::UniString::Printf (PanelText (ID_SCRIPTS_PANEL_STRINGS_VERSION), script->info.version.ToPrintf ());
    }
    addLine (authorAndVersion);
    addLine (script->requirements);

    nameText.SetText (script->name);
    badgeText.SetText (script->badge);
    detailsText.SetText (details);
}

void ScriptsPanel::TextEditChanged (const DG::TextEditChangeEvent& ev)
{
    if (ev.GetSource () == &searchEdit) {
        FillList ();
    }
}

void ScriptsPanel::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    if (ev.GetSource () == &categoryPopUp) {
        FillList ();
    }
}

void ScriptsPanel::ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev)
{
    if (ev.GetSource () != &scriptList) {
        return;
    }
    const Script* script = GetSelectedScript ();
    // A group's row or the "no match" row keeps the last script, so that it is selected again when it shows again.
    if (script != nullptr) {
        selectedLocation = script->fileLocation;
    }
    ShowDetails ();
}

void ScriptsPanel::Resized ()
{
    Layout ();
    const short height = palette.GetClientHeight ();
    if (IsOpen () && height >= MinRememberedOpenHeight) {
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
    SetOpen (!IsOpen ());
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
        }
    }
    Layout ();
    // The state as it is now counts as saved: opening Archicad writes nothing.
    hasSavedState = true;
    savedWidth = palette.GetClientWidth ();
    savedOpenHeight = openHeight;
    savedOpen = IsOpen ();
}

void ScriptsPanel::SaveState ()
{
    const short width = palette.GetClientWidth ();
    const bool open = IsOpen ();
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
