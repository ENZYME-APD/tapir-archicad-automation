# Distribution Board Schedule

Lists the circuits of every distribution board with their loads, totals each board and its
phases, flags the circuits over a load limit and the boards whose phases are out of balance, and
saves the schedule as CSV or Excel.

## How to use it

1. Run the script and choose the *Source*:
   - *Archicad electrical panels and circuits* (Archicad 30): the panels and circuits of
     Archicad's electrical tools.
   - *GDL symbols, grouped by their parameters*: Objects and Lamps that carry the board, the
     circuit and the load as GDL parameters, for projects drawn with symbols. Choose *Whole
     project*, *Current story* or *Selection* under *Symbols in*. The parameter lists are filled
     from the symbols; *Read parameter names* reads them again. Pick the *Board parameter*, the
     *Circuit parameter* and the *Load parameter*, and optionally a *Phase parameter* (values A,
     B, C, or L1, L2, L3, or 1, 2, 3; a three-phase symbol names all three, for example ABC).
2. Under *Flags*, set the *Circuit load over* limit and the *Phases out of balance by more than*
   percent.
3. Press *Refresh*. The list has one row per board, with its circuits under it: description,
   phases, voltage, connected load, the load per phase, voltage drop and cable length (the last
   two only from electrical circuits). Flagged rows are red. *Apply* checks the flags again
   after you change them.
4. *Save as...* saves the schedule, with a *Total* row per board, as a CSV file or an Excel
   workbook (`.xlsx`).

A board is out of balance when the load of one of its used phases differs from their average by
more than the set percent. A single-phase board is never flagged.

## Settings

- *Source*: *Archicad electrical panels and circuits*.
- *Symbols in*: *Whole project*. The parameters: none picked.
- *Circuit load over*: empty (no limit). *Phases out of balance by more than*: 10 %.

## What it changes

Nothing in the project. *Save as...* writes one file, where you choose in the save dialog; its
default name is `Distribution boards <date> <time>.csv`.

## Limits

- Loads are shown in the unit Archicad or the load parameter uses; the script does not convert
  them.
- With GDL symbols, the load per phase is only known when every symbol of the board has a phase
  value. The voltage drop and cable length are left empty.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. The electrical panels and circuits need
  Archicad 30. In older versions, use the GDL symbols source.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2019, written with the Tapir issue bot.
