# Copy Settings to Selection

Copies the settings of one element to the selected elements of the same type, without their
geometry: the eyedropper and syringe for many elements at once.

## How to use it

1. Run the script and press *Pick source...*, then click the element to copy the settings from in
   Archicad (Escape cancels).
2. Select the elements to change in Archicad and press *Read selection*. The list shows which
   elements will change; elements of another type than the source are skipped.
3. Under *Apply*, choose what to copy: *Settings*, *Classifications*, *Categories* and
   *Properties*.
4. Under *Keep on the selected elements*, choose what each element keeps of its own: *Element ID*,
   *Layer*, *Height and top link* (Walls, Columns), *Size and sill height* (Windows, Doors,
   Skylights) and *Reference line location* (Walls).
5. Press *Apply...*. The script reads the selection again, lists the elements it will change and
   what they keep, and asks first.

The script saves the source's settings as a temporary Favorite named `Tapir temporary <code>`,
applies it to the elements and deletes it again.

## Settings

- *Apply*: everything ticked.
- *Keep*: *Element ID*, *Height and top link*, *Size and sill height* and *Reference line
  location* ticked; *Layer* not ticked.

## What it changes

The settings of the selected elements of the source's type, after you confirm. Their position
and story, and what you chose to keep, stay as they are. You can undo it in Archicad; saving and
deleting the temporary Favorite are Undo steps of their own. Elements that could not be changed
are named in the palette's output. If the temporary Favorite could not be deleted, the status line
says so; delete it by hand. It writes no files.

## Limits

- Only elements of the source's type are changed.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- In a Teamwork project, an element you have not reserved may not be changed; the first error is
  shown in the status line.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2008, written with the Tapir issue bot.
