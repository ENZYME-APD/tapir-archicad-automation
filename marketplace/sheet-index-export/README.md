# Sheet Index Export

Lists every layout of the Layout Book with its subset, number, name, paper size, current revision
and the changes of that revision, so you can save a sheet index as a spreadsheet.

## How to use it

1. Open the project and run the script. It reads the Layout Book and the revisions and lists one
   row per layout, in Layout Book order; master layouts are left out.
2. The columns are *Subset* (the path of the subsets the layout is in), *Number*, *Name*, *Size*
   (the paper size, named when it is an ISO A or B size, e.g. `A1 landscape (841 × 594 mm)`),
   *Revision* and *Status* of the layout's current revision, and *Changes* (the IDs and
   descriptions of that revision's changes).
3. *Save as...* saves the list as an Excel workbook (`.xlsx`) or a CSV file.
4. *Copy to clipboard* copies the list with its headings, to paste into a spreadsheet.
5. *Read again* reads the Layout Book again.

The current revision of a layout is its revision that is not issued yet, or else the last one
issued. Without Revision Management the revision columns stay empty.

## Settings

None.

## What it changes

Nothing in the project. *Save as...* writes one file, where you choose in the save dialog; its
default name is `Sheet index <date>.xlsx`.

## Limits

- Layouts that cannot be read are named in the palette's output, and the first one in the
  window's status line.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2003, written with the Tapir issue bot.
