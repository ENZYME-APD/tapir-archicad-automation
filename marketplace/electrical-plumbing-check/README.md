# Electrical and Plumbing Check

Before drawings are issued, lists the electrical and plumbing elements that miss data: symbols
whose circuit, board or number is empty or names a board that does not exist, Archicad 30
electrical devices that are not on a circuit, Archicad 30 circuits without devices, and sanitary
objects that are outside every zone. Each group can be highlighted and selected in Archicad.

## How to use it

1. Run the script.
2. Under *Checks*, tick the checks you want.
3. Name the GDL parameters your electrical symbols use for the circuit, the board and the number.
   To pick them from a list, select one or more of your symbols in Archicad and press *Pick from
   selection*. A parameter is found by its name or by the name shown in the settings dialog;
   leave one empty to skip it.
4. Optionally limit the symbols to library parts whose name contains a text (*Symbols whose
   library part name contains*), name boards that are not Archicad 30 panels (*Boards besides the
   Archicad 30 panels*), and change the names that mark a sanitary object (*Sanitary library part
   names contain*). Lists are separated by `;`.
5. Press *Run checks*. Each check is a group in the list, with its findings under it. Select a
   group or a row to highlight its elements; double-click a row, or press *Select in Archicad*,
   to select them.

## Settings

- *Checks*: all five ticked.
- *Circuit parameter*, *Board parameter*, *Number parameter*: empty.
- *Symbols whose library part name contains*: empty (every object or lamp that has one of the
  parameters).
- *Boards besides the Archicad 30 panels*: empty.
- *Sanitary library part names contain*: `WC; Toilet; Basin; Washbasin; Sink; Shower; Bath;
  Urinal; Bidet`.

## What it changes

Nothing in the project, and it writes no files. It only highlights and selects the elements it
lists; the highlight is cleared when the window closes.

## Limits

- Symbols are objects and lamps. A number parameter set to 0 counts as empty. Hotlinked elements
  are left out.
- The board check knows the Archicad 30 electrical panels and the board names you type. With
  neither it is skipped.
- The checks of electrical devices and circuits need Archicad 30; in older versions they are
  listed as skipped.
- A sanitary object is outside every zone when its insertion point is outside the zones of its
  story. Curved zone edges are taken as straight.
- At most 2000 findings are shown per check; selecting the group selects them all.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2021, written with the Tapir issue bot.
