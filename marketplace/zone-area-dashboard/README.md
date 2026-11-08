# Zone Area Dashboard

Shows the areas of the project's zones grouped by any zone metric (zone name, number, category,
story, a custom property or a classification), as summary cards and a table, and highlights the
zones of a group in Archicad in their colour while fading the rest.

## How to use it

1. Run the script. It reads all the zones of the project, their areas, stories and categories, and
   the project's custom properties and classification systems.
2. Under *Zones*, choose *All zones* or *Selected zones* (select the zones in Archicad first), and
   the *Area*: *Measured area*, *Net area* or *Calculated area (as in the Zone Stamp)*, as
   Archicad gives them, in m² or ft². Every number in the window and in the export is that area,
   and the cards, the table and the export say which one.
3. Under *Grouping*, pick what to *Group by* and what to *Colour by*: *Zone name*, *Zone number*,
   *Zone category*, *Story*, any custom property (*Property: Group / Name*) or any classification
   system (*Classification: System version*). The values of a property or a classification are
   read the first time you pick it.
4. *Filter 1* and *Filter 2* keep only some zones, e.g. Filter 1 *Property: General / Building ID*
   with only *A* ticked, and Filter 2 *Story* with the first three stories ticked. Nothing ticked
   means no filter. The cards, the table, the highlight and the export all show the filtered zones.
5. The cards show the total area, the number of zones, the largest and the smallest group, and one
   card per group (the first 12, largest first) with its area, share of the total and number of
   zones. The table lists every group with its area, number of zones and share. Expand a group
   to see its zones. Click a column heading to sort by it.
6. Click a card or a table row to highlight its zones in Archicad, each in the colour of its
   *Colour by* value, and fade all the other elements. Ctrl-click (Cmd-click on macOS) adds a
   group or a zone to the highlight. Clicking the highlighted card or row again, or *Clear*,
   removes it. The *Total* row highlights every listed zone.
7. *Colour by parameter* colours every listed zone by its *Colour by* value without changing the
   zones. *Select in Archicad* selects the highlighted zones (every listed zone when none is),
   and a double-click on a zone row selects that zone.
8. *Refresh* reads the zones and their values again without closing the window.
9. *Export...* saves the listed zones:
   - as a CSV file: one row per zone with every metric (all custom properties and classification
     systems included) and the chosen area;
   - as an Excel workbook (`.xlsx`): the same *Zones* sheet, one sheet for the *Group by* grouping
     and, when it is different, one for the *Colour by* grouping, each with a total row.

A zone without a value for the chosen metric is in the group *(no value)*, never left out. A
zone that Archicad gives no area for counts in its group's number of zones, with no area. Its
row in the table is marked *no area*.

## Settings

- *Zones*: *All zones*; *Area*: *Measured area*; *Unit*: m².
- *Group by* and *Colour by*: *Zone category*; *Filter 1* and *Filter 2*: none.

## What it changes

Nothing in the project: the highlight and the colours are not saved, and *Select in Archicad*
only changes the selection. Closing the window removes the highlight. *Export...* writes one file,
where you choose in the save dialog. Its default name is `Zone areas <date> <time>.xlsx`, with each
zone's GUID.

## Limits

- The areas are the zone quantities Archicad's API gives: measured, net and calculated area. A
  gross area is not among them.
- Only custom (user-defined) properties are offered, not Archicad's built-in ones. The zone name,
  number, category and story are built in.
- A property's value is grouped by the text Archicad shows for it, so `12` and `12.0` are two
  groups.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. In Archicad 25 the other elements are not
  faded.
- Charts (donut, bars, a stack per story, a treemap) follow in the next version (part 2 of #2283).
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @JBENEITEZ in #2283, written with the Tapir issue bot.
