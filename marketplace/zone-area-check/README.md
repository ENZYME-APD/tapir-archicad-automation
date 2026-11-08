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

The target property's value is read as a number, with a comma or a point as the decimal
separator.

## Settings

- *Zones*: *Whole project*; *Measured area*: *Calculated area (as in the Zone Stamp)*.
- *Target unit*: m²; *Tolerance (%)*: 5; *Only over and under*: off.

## What it changes

Nothing in the project: the highlight is not saved, and *Select in Archicad* only changes the
selection. *Export...* writes one file, where you choose in the save dialog; its default name is
`Zone area check <date> <time>.xlsx`, with each zone's GUID.

## Limits

- A zone without a target value, or with a target of 0, is listed as *No target* and not
  highlighted.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. In Archicad 25 the other zones are not
  dimmed.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2007, written with the Tapir issue bot.
