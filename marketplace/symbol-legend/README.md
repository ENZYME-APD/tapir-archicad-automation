# Symbol Legend

Draws a legend of the symbols used on a story, for example the electrical and plumbing symbols of
a sheet, in a new worksheet: one instance of each library part in a column, with its name (or a
description parameter) beside it, and the number placed if you want it. The worksheet can then be
placed on a layout like any other drawing.

## How to use it

1. Run the script. Under *Where*, choose *Current story*, *Selected elements*, *Whole project* or
   one of the stories. To list only some layers, select them in the layer list (Ctrl or Shift for
   several); with none selected, every layer is used.
2. Optionally type the name (or the label shown in the Object settings) of a GDL parameter that
   holds a description of the symbol. Its text, from the first placed instance, is used instead of
   the library part's name where it is not empty.
3. Press *Find symbols*. The list shows one row per library part with the number placed. Nothing
   in the project has changed yet.
4. Click a row's box (or select rows and press Space) to leave a symbol out. Double-click a row to
   change its text in the legend.
5. Set the worksheet's name and reference ID, the title, the spacing and the text height under
   *Legend*, and tick *Show the count* to add the number placed after each name.
6. Press *Create* and confirm. The worksheet is created, the symbols are placed in it in a column
   and the texts are written beside them. With *Open the worksheet afterwards* it is shown.

Each legend symbol has the size (A and B, and the height) of the first placed instance and the
library part's own default for every other parameter, at the angle 0.

## Settings

- *Where*: *Current story*. Layers: all.
- *Description parameter*: empty (the library part names are used).
- *Worksheet name* `Symbol legend`, *Worksheet reference ID* `LEG`, *Title* `Legend`.
- *Row spacing* 0.8 m (a row is taller when its symbol is), *Gap between symbol and text* 0.5 m.
  Lengths are in metres unless you add a unit: `80 cm`, `800 mm`, `2.5 ft`, `2'`, `30 in`.
- *Text height* 2.5 mm on paper, the title 1.4 times that.
- *Show the count*: off. *Open the worksheet afterwards*: on.

## What it changes

Nothing until you press *Create* and confirm. Then it creates one new worksheet in the Project Map
and places the symbols and texts in it; the placed symbols on the stories are not touched. It
writes no files.

Undo: the symbols are one Undo step (two when there are both objects and lamps) and the texts
another, as each Tapir command is one step; the worksheet is removed from the Project Map like any
other worksheet.

## Limits

- Lists Objects and Lamps. MEP elements (Archicad's MEP Modeler, and the electrical devices of
  Archicad 28 and later) and doors and windows are not listed.
- A library part is one row, whatever its parameters: two differently set instances of the same
  part share one legend symbol.
- The symbols and texts go on the Object, Lamp and Text tools' default layers; if such a layer is
  hidden in the worksheet, turn it on there.
- Works in Archicad 27 to 30 with Tapir 1.8.1 or later, which places objects in another database
  than the current one.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2018, written with the Tapir issue bot.
