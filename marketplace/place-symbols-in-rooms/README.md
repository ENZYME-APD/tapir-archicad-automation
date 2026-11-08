# Place Symbols in Rooms

Places a library part (an Object or a Lamp) in the chosen zones by a rule: along the zone's
boundary at a spacing, keeping clear of the corners and the doors (for example sockets), or in a
grid with a margin from the walls (for example lights). Each new element gets the layer and the
GDL parameter values you set.

## How to use it

1. Select the zones in Archicad, or choose *Current story* or *Whole project* under *Zones*.
2. Run the script. Under *Library part*, find the part with *Search* and click it. Its
   parameters are listed under *New elements*.
3. Under *Rule*, choose *Along the boundary* or *In a grid* and set its distances in metres.
4. Under *New elements*, choose a *Layer*, and double-click a parameter's value to change it.
   Only the changed values are written.
5. Press *Preview*. The list shows how many elements each zone gets. Select a zone to see their
   positions in the drawing next to it, and double-click it to select the zone in Archicad. The
   zones are highlighted in Archicad: green with positions, orange without.
6. Press *Place* and confirm. The new elements are highlighted in blue. *Clear highlight*, or
   closing the window, ends the highlight.

Along the boundary, each element is turned to face the inside of the zone and placed at the
mounting height above the zone's base.

## Settings

- *Zones*: *Selected zones*.
- *Along the boundary* (the default rule): *Spacing* 3.0 m, *Mounting height* 0.3 m, *Distance
  from corners* 0.3 m, *Distance from doors* 0.3 m.
- *In a grid*: *Maximum spacing* 2.4 m, *Mounting height* 2.6 m, *Margin from walls* 0.6 m.
- *Layer*: the tool's default layer. Parameters: the library part's own values.

## What it changes

Nothing until you press *Place* and confirm: then it creates the new Objects or Lamps, and sets
their layer and the changed parameters. *Preview* only highlights the zones. It writes no files.

## Limits

- Only Objects and Lamps can be placed.
- The zone's polygon is its boundary. Curved edges get no elements along them.
- Distances are in metres, whatever the project's units.
- Works in Archicad 25 to 30 with Tapir 1.8.1 or later, which reads the parameters of a library
  part before it is placed. The distance from doors needs Archicad 26 or later; in Archicad 25
  no distance is kept from the doors.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2020, written with the Tapir issue bot.
