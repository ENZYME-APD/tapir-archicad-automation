# Property Value Exporter

Writes the values of the properties you choose into an Excel or CSV file, one row per element,
without building an interactive schedule first.

## How to use it

1. Run the script. It reads every property of the project.
2. Under *Elements*, choose *Selection*, *Current story* or *Whole project*, and if you want
   *Only elements on visible layers*. Tick the element types to export (*All types* and *No
   types* tick or untick every one).
3. Under *Properties*, find properties with *Search* and move them to the *Exported* list with
   *Add →* or a double-click. *Up* and *Down* set the order of the columns; *← Remove* takes one
   out again.
4. Tick *Precise numbers* to export numbers in SI units (m, m², m³, degrees) instead of the
   values as Archicad displays them.
5. Press *Export...* and choose the file.

The file has the columns *GUID*, *Element ID*, *Type* and *Story*, then one column per property,
headed `<group>: <name>`. Rows are sorted by type, story from the bottom up and Element ID. A
property an element does not have gives an empty cell.

## Settings

- *Elements*: *Whole project*; *Only elements on visible layers*: off; no type ticked.
- *Precise numbers*: off.

## What it changes

Nothing in the project. *Export...* writes one file, where you choose in the save dialog; its
default name is `Property values <date> <time>.xlsx`.

## Limits

- The element types are Wall, Column, Beam, Slab, Roof, Shell, Morph, Mesh, Window, Door,
  Skylight, Opening, Object, Lamp, Zone, Curtain Wall, Stair and Railing.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. *Precise numbers* needs a Tapir version
  newer than 1.8.0; with an older one the displayed values are exported, and the palette's
  output says so.
- The file cannot be written while it is open in Excel.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2005, written with the Tapir issue bot.
