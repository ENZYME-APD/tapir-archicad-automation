# Room Finishes Schedule

Lists the floor, wall and ceiling finish areas of every zone, one row per zone and finish, with
the wall area split by the surface of the walls around the zone, and saves the table as CSV or
Excel.

## How to use it

1. Run the script. It calculates the schedule of every zone right away.
2. Under *Zones*, choose *Whole project*, *Current story* or *Selection*, and press
   *Calculate*.
3. The list has, for each zone, a *Floor* row, one *Wall* row per surface and a *Ceiling* row,
   with the story, the zone number and name, and the area in m². The line above it gives the
   totals of the floors, walls and ceilings.
4. *Save as...* saves the table as a CSV file or an Excel workbook (`.xlsx`).

How the areas are found:

- Floor and ceiling: the zone's net area as Archicad calculates it.
- Walls: Archicad's walls surface of the zone, which uses the real height of each wall and leaves
  out the doors, windows and covered parts. It is split by the surface each wall around the zone
  shows, in proportion to the wall's length around the zone (*Length*) times the zone's height
  (*Height*), less the doors and windows that open into the zone (*Openings*, width × height of
  each).

## Settings

- *Zones*: *Whole project*.

## What it changes

Nothing in the project. *Save as...* writes one file, where you choose in the save dialog; its
default name is `Room finishes.csv`.

## Limits

- Archicad does not report which side of a wall faces the zone. A wall whose two sides have
  different surfaces is listed with both, as `<surface> / <surface> (side not reported)`.
- Lengths are in m and areas in m², whatever the project's units.
- Works in Archicad 26 to 30 with Tapir 1.8.0 or later: Archicad 25 does not report the walls
  around a zone or the zones a door or window opens into.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2014, written with the Tapir issue bot.
