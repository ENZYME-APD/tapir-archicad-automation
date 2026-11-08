# Material Take-Off

Totals the volume (m³) and the projected area (m²) of every building material used by the
walls, columns, beams, slabs, roofs and shells, per story, from the quantities Archicad
calculates for each skin, for a bill of quantities.

## How to use it

1. Run the script. It calculates the take-off of the whole project right away.
2. Under *Settings*, choose the *Elements*: *Whole project*, *Current story* or *Selection*.
   Tick *Group by element type as well* for one row per story, element type and building
   material, and untick *Only elements on visible layers* to count hidden elements too.
3. Press *Calculate*. The top shows the number of building materials, the total volume, the
   total projected area and the number of elements counted. The list has one row per story and
   building material, from the top story down, with the number of elements, the volume and the
   projected area.
4. *Copy to clipboard* copies the table to paste into Excel; *Save as...* saves it as an Excel
   workbook (`.xlsx`) or a CSV file.

## Settings

- *Elements*: *Whole project*.
- *Group by element type as well*: off. *Only elements on visible layers*: on.

## What it changes

Nothing in the project. *Save as...* writes one file, where you choose in the save dialog; its
default name is `Material take-off <date> <time>.xlsx`.

## Limits

- Walls, Columns, Beams, Slabs, Roofs and Shells are counted. Morphs and Zones are not.
- Volumes are in m³ and areas in m², whatever the project's units.
- Archicad reports no projected areas when the legacy intersection and surface calculation is on
  in the Project Preferences; the script then says so.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2013, written with the Tapir issue bot.
