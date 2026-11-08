# Object Counter

Counts the placed objects, lamps, doors, windows and (in Archicad 30) electrical devices by
library part, per story and per zone they stand in. It can also sum one numeric GDL parameter,
for example a fixture's load, and saves the table as CSV or Excel.

## How to use it

1. Run the script.
2. Under *What to count*, tick the element types to count. Under *Elements*, choose *Whole
   project*, *Current story* or *Selection*.
3. To sum a parameter, type its GDL name or its name in the settings dialog in *Sum the GDL
   parameter*.
4. Press *Count*. The list has one row per story, zone, type and library part, with the count
   and the sum. The line above it gives the number of elements, library parts and zones with
   elements, and the total of the sum.
5. Double-click a row, or select rows and press *Select in Archicad*, to select their elements.
   With no row selected, every counted element is selected.
6. *Save as...* saves the table as a CSV file or an Excel workbook (`.xlsx`).

How the zone is found: an object or lamp is in the zone that contains its origin on its story.
A door or window is in a zone on either side of its wall. When zones overlap, the smallest one
wins.

## Settings

- *What to count*: every type ticked.
- *Elements*: *Whole project*.
- *Group by zone*: on. Off, the rows are per story, type and library part only.
- *Sum the GDL parameter*: empty (nothing summed).

## What it changes

Nothing in the project. *Select in Archicad* only changes the selection. *Save as...* writes one
file, where you choose in the save dialog; its default name is `Object count <date> <time>.csv`.

## Limits

- Curved zone edges are taken as straight lines.
- Elements whose position cannot be found are listed under *(position unknown)*, and those
  outside every zone under *(no zone)*.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. Electrical devices are counted only in
  Archicad 30. Their library part names need a Tapir version newer than 1.8.0. With 1.8.0 they
  are listed as *Electrical device (<load classification>)*.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2015, written with the Tapir issue bot.
