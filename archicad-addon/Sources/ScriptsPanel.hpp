#pragma once

#include "DGModule.hpp"
#include "UniString.hpp"
#include "Location.hpp"

// The scripts panel of the Tapir Palette, below its bar (#2407). The palette is one palette in two sizes: the bar
// (BarHeight tall), and the bar with the panel under it. The panel opens when the palette is dragged taller than the
// bar, or with the expand arrow, the last control at the bar's right end, which opens it at its last height. The
// palette's size and whether the panel is open are kept in Documents/Tapir/scripts-panel.json; Archicad keeps its
// position with the palette's guid.
//
// The panel's controls are the items of ID_PALETTE below y BarHeight. TapirPalette creates the panel, forwards its
// resize and the panel's events to it, and changes as little as possible otherwise.
class ScriptsPanel
{
public:
    static constexpr short BarHeight = 28;
    // The palette's client height when the panel is opened with the arrow and was never open before.
    static constexpr short DefaultOpenHeight = 520;
    // A palette at least this much taller than the bar has the panel open; a lower one counts as the bar.
    static constexpr short MinOpenGrowth = 8;
    // A palette closed or left at a smaller height than this does not change the height the arrow opens it at.
    static constexpr short MinRememberedOpenHeight = BarHeight + 120;

    // The item numbers of the expand arrow and of the panel's controls in ID_PALETTE.
    ScriptsPanel (DG::Palette& palette, short expandButtonItem, short separatorItem);

    bool IsOpen () const;
    // Resizes the palette to the bar or to the panel's last height, and remembers it.
    void SetOpen (bool open);
    // Applies the saved size and open state. The palette calls it once, from its constructor.
    void RestoreState ();
    // Places the arrow and the panel's controls for the palette's current width, and shows the controls when the
    // panel is open.
    void Layout ();
    // When Archicad or the user gave the palette a size (it opened, or was resized): takes whether the panel is open
    // from the palette's height, then Layout.
    void SyncWithSize ();
    // After the palette was resized: Layout, and remember the size.
    void Resized ();

    // The palette forwards the events of every item; these return true when the item was the panel's.
    bool ButtonClicked (const DG::ButtonClickEvent& ev);
    bool ToolTipRequested (const DG::ItemHelpEvent& ev, GS::UniString* toolTipText) const;

private:
    bool IsTallerThanBar () const;
    void SetExpandButtonIcon (bool open);
    void SaveState ();
    static IO::Location GetStateFileLocation ();

    DG::Palette&   palette;
    DG::IconButton expandButton;
    DG::Separator  separator;

    short openHeight = DefaultOpenHeight;   // the client height the arrow opens the panel at
    bool  isOpen = false;                   // set by SetOpen and RestoreState, synced from the height by SyncWithSize
    bool  expandButtonShowsOpen = false;    // the arrow's icon is the up chevron

    // What SaveState wrote last, so that a resize that changed nothing writes nothing.
    bool  hasSavedState = false;
    short savedWidth = 0;
    short savedOpenHeight = 0;
    bool  savedOpen = false;
};
