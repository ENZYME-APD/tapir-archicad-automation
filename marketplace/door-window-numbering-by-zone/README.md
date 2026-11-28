# Door and Window Numbering by Zone

Gives every door and window an ID built from the zone it opens into, such as `1.04-D1` or
`1.04-W2`, numbered within each zone, and writes it to the Element ID or to a text property you
choose.

## How to use it

1. Open the floor plan and run the script. It reads the doors and windows of the current story
   and shows the new IDs next to the current values; nothing is changed yet.
2. Choose which elements to number in *Elements*, which zone side comes first in *Zone side
   first* and where the new ID goes in *Write to*, then press *Read elements*.
3. Adjust the patterns, the start number and the digits; the list updates as you type.
4. Press *Rename* and confirm. The status column shows the elements that could not be changed
   and why.

Each door or window belongs to the zone on the side chosen in *Zone side first*, or the zone on
the other side when there is none. The *Zone* column shows which side was used, `(to)` or
`(from)`. Within a zone they are numbered clockwise around the zone stamp, starting from the
north. Zones with the same number continue one count, from north to south. Doors and windows
without a zone are listed separately, with the reason, and are not renamed.

## Settings

| Field | Default | Meaning |
| --- | --- | --- |
| *Elements* | Current story | *Selection*, *Current story* or *Whole project*. |
| *Zone side first* | To zone | *To zone* or *From zone*, as Archicad's To Zone and From Zone properties name them. |
| *Doors* | `{zone}-D{n}` | The pattern of the doors' IDs. |
| *Windows* | `{zone}-W{n}` | The pattern of the windows' IDs. |
| *Start at* | 1 | The first number in each zone. |
| *Digits* | 1 | The least number of digits of `{n}`, padded with zeros (2 gives `01`). |
| *Number repeated IDs, adding* | off, `.{k}` | When ticked, an ID that more than one element would get has this added, with `{k}` counting 1, 2, ... in numbering order. Unique IDs stay as they are. |
| *Digits of {k}* | 2 | The least number of digits of `{k}`. |
| *Write to* | Element ID | The Element ID, or an editable text property of the project (custom properties first). |

In the patterns `{zone}` is the zone number, `{name}` the zone name (also used for `{zone}`
when the zone has no number), `{n}` the running number within the zone and `{id}` the Element ID
as it was when the elements were read. For example `{id}.{zone}` with *Number repeated IDs*
ticked gives `D1.1.04.01`, `D1.1.04.02` to two doors with the ID `D1` that open into zone 1.04,
and written to a property it leaves the Element ID in the door markers as it is. The script
warns when new IDs repeat.

## What it changes

The Element ID, or the chosen property, of the doors and windows whose new value differs from the
current one, after you confirm. You can undo it in Archicad. It writes no files.

## Limits

- Only doors and windows; skylights and curtain wall panels are not numbered.
- Only properties with a single text value that you can edit are offered in *Write to*. An
  element the chosen property is not available for is listed under *Without a zone* with the
  reason, and is not changed.
- A new ID built with `{id}` and written to the Element ID changes the ID it was built from:
  pressing *Read elements* again builds the next ID from the new one.
- Works in Archicad 26 to 30 with Tapir 1.8.0 or later; Archicad 25 cannot tell which zone a
  door or window opens into.
- In a Teamwork project, an element you have not reserved may not be changed; it is shown as
  not changed, with the reason.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2001, written with the Tapir issue bot. Improved as asked by
@radry in #3071.
