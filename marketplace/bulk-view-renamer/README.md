# Bulk View Renamer

Renames the views of the View Map, or of one of its folders, in one go: find and replace, a
prefix and a suffix, or a pattern with a counter. You see every new name before anything is
renamed.

## How to use it

1. Run the script. It lists the views of the View Map with their folder, ID and name.
2. In *Views in* choose the whole View Map or one folder (its subfolders are included).
3. Choose how the names change:
   - *Find and replace*: *Find* and *Replace with*, optionally *Match case* and
     *Regular expression*.
   - *Prefix and suffix*: *Add before* and *Add after*.
   - *Pattern with counter*: a *Pattern* in which `{name}` is the current name, `{id}` the view's
     ID and `{n}` the counter, with its *Start*, *Step* and *Digits*. The counter counts in View
     Map order.
4. The table shows the new names. A *clash* is a view that would get the same name as another
   view in its folder; an empty new name is skipped. Select rows to rename only those; *Clear
   selection* renames every listed view again.
5. Press *Rename...*: the script lists the changes and asks first, then renames the views and
   reads the View Map again.

## Settings

- *Views in*: the whole View Map.
- *Find and replace*, with *Match case* and *Regular expression* off.
- Pattern `{name} {n}`, *Start* 1, *Step* 1, *Digits* 2.

## What it changes

The names of the views it lists as *rename* or *clash*, after you confirm, one view at a time.
Views that could not be renamed are named in the palette's output, and the first one in the
window's status line. It writes no files.

## Limits

- Only view names change, not view IDs or the views' settings.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2004, written with the Tapir issue bot.
