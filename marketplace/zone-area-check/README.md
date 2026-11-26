# Zone Area Check

Compares the area of every zone with a target area read from a property you choose, within a
tolerance in percent, and highlights the zones that are over, under and on target.

## How to use it

1. Run the script. It reads every property of the project.
2. Under *Zones*, choose *Selection*, *Current story* or *Whole project*, and the *Measured area*:
   *Calculated area (as in the Zone Stamp)*, *Net area* or *Gross area*.
3. Pick the property that holds the target area (*Search* finds it), set its *Target unit* (m² or
   ft²) and the *Tolerance (%)*, and press *Check*.
4. The list shows each zone's number, name, story, measured and target area in m², the difference
   in percent and its status: *Over*, *Under*, *On target*, *No target* or *No area*. Zones over
   are highlighted red, under blue and on target green; the other zones are dimmed.
5. Select rows to highlight only those zones. Double-click a row to select its zone in Archicad,
   or press *Select in Archicad* for the selected rows (every listed zone when none is selected).
   *Only over and under* hides the other zones.
6. *Export...* saves the list as an Excel workbook (`.xlsx`) or a CSV file. *Clear highlight*
   removes the highlight; closing the window removes it too.
7. *Dimension perimeters* places a linear dimension along each straight edge of the zones: those
   of the selected rows, or every zone in the *Zones* scope when no row is selected (you do not
   need to press *Check* first). Each dimension runs along its edge, attached to the zone's two
   corners, with its line inside the room at the *Offset from the edge*, on the zone's story.
   - Parallel edges of the same length (within 1 mm) in one zone, such as the opposite sides of a
     rectangular room, get one dimension.
   - An edge that already has a dimension (the same two points, the same direction) is skipped,
     so pressing it again adds no second set. A wall face shared by two zones is dimensioned in
     both rooms, because each room's zone ends at its own face of the wall.
   - Under *Perimeter dimensions* choose a Dimension *Favorite* and a *Layer* (otherwise the
     Dimension tool defaults are used), the *Offset from the edge*, the *Shortest edge* to
     dimension (shorter jogs are skipped) and *Also the edges of holes*.
   - The *Dimensions* column of the list and the export say what was done with each zone.

The target property's value is read as a number, with a comma or a point as the decimal
separator.

## Settings

- *Zones*: *Whole project*; *Measured area*: *Calculated area (as in the Zone Stamp)*.
- *Target unit*: m²; *Tolerance (%)*: 5; *Only over and under*: off.
- *Favorite* and *Layer*: the Dimension tool defaults; *Offset from the edge*: 0.3 m; *Shortest
  edge*: 0.1 m; *Also the edges of holes*: off.

## What it changes

*Check* changes nothing in the project: the highlight is not saved, and *Select in Archicad* only
changes the selection. *Dimension perimeters* adds Dimension elements to the project (*Edit > Undo*
removes them). *Export...* writes one file, where you choose in the save dialog; its default name
is `Zone area check <date> <time>.xlsx`, with each zone's GUID.

## Limits

- A zone without a target value, or with a target of 0, is listed as *No target* and not
  highlighted.
- *Dimension perimeters* skips curved edges, and zones whose boundary is measured to the walls'
  reference lines (it lists them in the status line and the *Dimensions* column), since their
  dimensions would run to the wall centres, not the wall faces.
- When Archicad does not attach a dimension to the zone's corners, the dimension is placed with
  fixed points instead and does not follow the zone when it changes; the status line says how
  many.
- Works in Archicad 25 to 30 with Tapir 2.0.1 or later. In Archicad 25 the other zones are not
  dimmed.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2007, written with the Tapir issue bot. *Dimension perimeters*
requested by @parikshit99 in #3074.
