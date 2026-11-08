# Property Value Importer

Reads a CSV or Excel file in the layout of the
[Property Value Exporter](../property-value-exporter/) and writes its values into the properties of
the elements, after showing what would change. Export, edit the values in Excel, import.

## How to use it

1. Run the script and press *Open file...*, then choose the `.xlsx` or `.csv` file.
2. The list shows each value that would change as *Current* → *New*, and the rows and cells that
   cannot be written, in red, with the reason: no element with that GUID or Element ID, an
   Element ID used by several elements, a property that is not available for the element.
3. Press *Write to Archicad* and confirm. The *Result* column then says *Written* or why a value
   was refused (for example, the property is read-only for that element).

## The file

A header row, then one row per element. The columns *GUID*, *Element ID*, *Type* and *Story* are
what the exporter writes; the others are properties, headed `<group>: <name>` as in
Archicad (upper and lower case do not matter). A column that is not a property of the project is
skipped and listed in the palette's output.

- A row is matched by its GUID when it has one, otherwise by its Element ID.
- An empty cell leaves the value as it is. A value equal to the current one is not listed.
- CSV: UTF-8 (Excel's "CSV UTF-8"), separated by a comma or a semicolon (found from the header
  line). Excel: the first sheet.
- Values are written as text, the way you would type them in the property's field, in the
  project's units; `.` or `,` can be the decimal separator.

## Settings

None.

## What it changes

The property values listed in the preview, in the open project, in one Undo step, and only after
you confirm. It writes no file.

## Limits

- Use a file exported without *Precise numbers*: those values are in SI units, which Archicad does
  not read back in a project with other working units.
- A value Archicad cannot convert to the property's type, or a read-only property, is refused for
  that value only; the rest is written.
- Only elements of the open project match; a GUID or Element ID from another project does not.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).
- Not yet tried in a live Archicad.

## Credits

Requested by @parikshit99 in #2006, written with the Tapir issue bot.
