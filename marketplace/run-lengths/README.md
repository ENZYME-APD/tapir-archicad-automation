# Run Lengths

Totals the lengths of pipe, duct and wiring runs in two ways, side by side: the MEP Modeler
routes and flexible segments per domain, MEP system and size, and the 2D lines, polylines, arcs
and circles drawn for them, per layer, pen or line type. In Archicad 30 it also lists each
electrical circuit's estimated cable length. The table can be copied or saved as CSV or Excel.

## How to use it

1. Run the script. It calculates the lengths of the whole project right away.
2. Under *Elements*, choose a *Story* or *All stories*. Tick *Only the selected elements* to
   measure the selection only, and *Split MEP runs by connected network* to list each connected
   network separately.
3. Under *Group 2D lines by*, choose *Layer*, *Pen*, *Line type* or *Layer, pen and line type*.
   *Only groups containing* keeps the 2D groups whose name contains the text you type.
4. Press *Calculate*. The *MEP Modeler* list has the routes and flexible segments, and the
   circuits in Archicad 30, with their count and length. The *2D lines* list has the groups. The
   line above them gives the totals.
5. *Copy to clipboard* copies both lists as a table you can paste into Excel. *Save as...* saves
   them as a CSV file or an Excel workbook (`.xlsx`).

How the lengths are measured: a MEP route along its centre line from node to node, elbows
included; a flexible segment along its control points; 2D arcs and polyline arcs along the arc.

## Settings

- *Story*: *All stories*.
- *Only the selected elements*: off. *Split MEP runs by connected network*: off.
- *Group 2D lines by*: *Layer*. *Only groups containing*: empty.

## What it changes

Nothing in the project. *Save as...* writes one file, where you choose in the save dialog; its
default name is `RunLengths.csv`.

## Limits

- Lengths are in m and MEP sizes in mm, whatever the project's units.
- Flexible segments have no story, so they are only counted with *All stories*.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. The MEP routes need Archicad 28 or
  later and the electrical circuits Archicad 30. In older versions only the 2D lines are
  measured.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2016, written with the Tapir issue bot.
