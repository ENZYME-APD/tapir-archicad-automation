# Drawing Status Report

Lists every drawing placed on the layouts of the Layout Book with the columns of Archicad's
Drawing Manager, so you see which drawings are out of date. You can export the list, or update
the drawings you choose.

## How to use it

1. Open the project and run the script. It opens each layout in turn to read its drawings
   (Archicad gives a drawing's data only from its own layout), then goes back to the window you
   were in.
2. The window lists each drawing with its layout, custom name, status (OK, Modified or
   Unknown), update type (Auto or Manual), whether it is stored in the project, its source and
   its path. *Only drawings that are not up to date* hides the drawings that are OK.
3. To update drawings, select their rows and press *Update drawings...*; with no row selected,
   every listed drawing with the status Modified is updated. The script shows which drawings it
   will update and asks first, then reads the list again.
4. *Export...* saves the listed rows as an Excel workbook (`.xlsx`) or a CSV file.
5. *Refresh* reads the drawings again.

## Settings

- *Only drawings that are not up to date*: off.

## What it changes

*Update drawings...* updates the chosen drawings, after you confirm, one drawing at a time.
Drawings it could not update are named in the palette's output, and the first one in the
window's status line. Reading opens each layout and then the window you started in; it changes
nothing in the project.

*Export...* writes one file, where you choose in the save dialog; its default name is
`Drawings <date> <time>.xlsx`, with the columns of the list and each drawing's GUID.

## Limits

- Reading takes a moment per layout in a project with many layouts.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. Updating drawings needs Archicad 27 or
  later; in Archicad 25 and 26 the list and the export work, and an update is reported as not
  supported.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #1992, written with the Tapir issue bot, based on the
`get_drawing_manager_data.py` workflow example.
