# Bulk View Renamer

Renames the views of the View Map or the Project Map, or of one of their folders, in one go: find
and replace, a prefix and a suffix, or a pattern with a counter. You see every new name before
anything is renamed.

## How to use it

1. Run the script. It lists the views of the View Map in the View Map's folders, with their name,
   ID and type (floor plan, section, elevation, interior elevation, worksheet, detail, schedule,
   ...). Click the arrow of a folder to close or open it; *Expand all* and *Collapse all* open or
   close every folder.
2. In *Map* choose the *View Map* (the saved views) or the *Project Map* (the viewpoints: stories,
   sections, elevations, details, worksheets, ...). The Project Map is shown in its own groups
   (Stories, Sections, Elevations, ...), which open and close the same way.
3. In *Views in* choose the whole map or one folder (its subfolders are included), and in
   *Type* all types or one type of view.
4. Choose how the names change:
   - *Find and replace*: *Find* and *Replace with*, optionally *Match case* and
     *Regular expression*.
   - *Prefix and suffix*: *Add before* and *Add after*.
   - *Pattern with counter*: a *Pattern* in which `{name}` is the current name, `{id}` the view's
     ID and `{n}` the counter, with its *Start*, *Step* and *Digits*. The counter counts in the
     map's order.
5. The table shows the new names. A *clash* is a view that would get the same name as another
   view in its folder; an empty new name is skipped. Select rows to rename only those (selecting a
   folder selects every listed view in it); *Clear selection* renames every listed view again.
6. Press *Rename...*: the script lists the changes and asks first, then renames the views and
   reads the map again.

## Settings

- *Map*: the View Map; *Views in*: the whole map; *Type*: all types. Every folder open.
- *Find and replace*, with *Match case* and *Regular expression* off.
- Pattern `{name} {n}`, *Start* 1, *Step* 1, *Digits* 2.

## What it changes

The names of the views it lists as *rename* or *clash*, after you confirm, one view at a time.
In the Project Map it renames the section, elevation, interior elevation, detail or worksheet
behind the viewpoint, so its marker and every View Map view that takes its name from it show the
new name too. Views that could not be renamed are named in the palette's output, and the first
one in the window's status line. It writes no files.

## Limits

- Only view names change, not view IDs or the views' settings.
- In the Project Map only sections, elevations, interior elevations, details and worksheets can
  be renamed. Archicad does not let a script rename the other viewpoints (stories, 3D views,
  schedules, project indexes, lists) through the Navigator, so they are listed as
  *cannot rename*, skipped and not counted. Rename a story in the Story Settings, or rename its
  saved views in the View Map.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2004, written with the Tapir issue bot. The View Map folders and the
view types were asked for by the Discord user Bot3d in #2783, and the Project Map by the same user
in #2834.
