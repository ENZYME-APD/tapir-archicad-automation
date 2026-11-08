# Door and Window Numbering by Zone

Gives every door and window an Element ID built from the zone it opens into, such as
`1.04-D1` or `1.04-W2`, numbered within each zone.

## How to use it

1. Open the floor plan and run the script. It reads the doors and windows of the current story
   and shows the new IDs next to the current ones; nothing is changed yet.
2. Choose which elements to number in *Elements*, then press *Read elements*.
3. Adjust the patterns, the start number and the digits; the list updates as you type.
4. Press *Rename* and confirm. The status column shows the elements that could not be changed
   and why.

Each door or window belongs to the zone on its "to" side, or the zone on its "from" side when
there is none. Within a zone they are numbered clockwise around the zone stamp, starting from
the north. Zones with the same number continue one count, from north to south. Doors and
windows without a zone are listed separately, with the reason, and are not renamed.

## Settings

| Field | Default | Meaning |
| --- | --- | --- |
| *Elements* | Current story | *Selection*, *Current story* or *Whole project*. |
| *Doors* | `{zone}-D{n}` | The pattern of the doors' IDs. |
| *Windows* | `{zone}-W{n}` | The pattern of the windows' IDs. |
| *Start at* | 1 | The first number in each zone. |
| *Digits* | 1 | The least number of digits of `{n}`, padded with zeros (2 gives `01`). |

In the patterns `{zone}` is the zone number, `{name}` the zone name (also used for `{zone}`
when the zone has no number) and `{n}` the running number within the zone. The script warns
when new IDs repeat.

## What it changes

The Element ID of the doors and windows whose new ID differs from the current one, after you
confirm. You can undo it in Archicad. It writes no files.

## Limits

- Only doors and windows; skylights and curtain wall panels are not numbered.
- Works in Archicad 26 to 30 with Tapir 1.8.0 or later; Archicad 25 cannot tell which zone a
  door or window opens into.
- In a Teamwork project, an element you have not reserved may not be changed; it is shown as
  not changed, with the reason.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2001, written with the Tapir issue bot.
