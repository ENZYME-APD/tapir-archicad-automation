# Missing Library Parts

Lists the library parts that placed Objects, Lamps, Windows and Doors use but the loaded
libraries do not have (the elements Archicad shows as dots, for example after migrating a
project), with the elements that use each one, per story. It can select those elements or
switch them to a loaded library part.

## How to use it

1. Run the script. It reads the loaded library parts and the elements, and lists each missing
   library part with its type, the stories it is on and the number of elements. Open a row to
   see its stories, and a story to see its elements by Element ID.
2. Under *Elements*, choose *Selection*, *Current story* or *Whole project*; the list is read
   again.
3. Select rows and press *Select in Archicad* (or double-click a row) to select their elements.
   With no row selected, every listed element is selected.
4. To replace a missing library part, select its rows (elements of one type), find a loaded
   library part of that type with *Search*, choose it under *Library part* and press
   *Replace...*. The confirmation lists the elements to change. A report then shows the result
   of each element, and the list is read again.

## Settings

- *Elements*: *Whole project*.
- *Keep the parameters with the same name*: on. *Keep the size (A and B)*: on.

## What it changes

Nothing until you press *Replace...* and confirm: then the chosen elements get the new library
part, and stay the same elements, with their properties and classifications. You can undo it in
Archicad. *Select
in Archicad* only changes the selection. It writes no files.

## Limits

- Only Objects, Lamps, Windows and Doors are checked, and an element can only get a library
  part of its own type.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. A Tapir version newer than 1.8.0 also
  reads the name of a missing library part from the element itself; with 1.8.0 the name in the
  element's details is used, and a part whose name cannot be read is listed as *(name not
  readable)*.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2010, written with the Tapir issue bot.
