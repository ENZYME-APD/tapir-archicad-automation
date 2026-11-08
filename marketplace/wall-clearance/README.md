# Wall Clearance

Finds the doors, windows and objects that are too close to a wall, column or curtain wall, and
moves them just enough to keep a margin. A door or window slides along its own wall; an object
slides along the wall it stands against, or moves straight away from the obstruction when it
stands against none.

## How to use it

1. Open the floor plan. Select the doors, windows and objects to check, or choose *Current story*.
2. Run the script. Under *Elements*, tick *Doors*, *Windows* and *Objects* as needed, and
   optionally type a text that the library part name must contain.
3. Set the margins: one per kind, or per library part in the table (*Add...*, `*` and `?` match
   any text, the first matching row wins), or from a property you pick, whose number is used for
   each element that has one.
4. Press *Check*. The list shows each element that is too close: the obstruction, the side it is
   on, the gap now (negative when they overlap), the margin needed and the move. Elements to move
   are highlighted orange, elements that cannot be moved red. Double-click a row, or press
   *Select in Archicad*, to select it.
5. Press *Apply*. It asks first, then moves the elements; moved elements turn blue. Doors,
   windows and objects are each one Undo step.

How it measures: an object's space is the outline of its floor plan symbol. A door's or window's
space is its opening in the wall, its width across the wall's thickness; the swing of a door is not
part of it. The obstructions are the walls, columns and curtain walls cut on its story, except the
wall a door or window sits in and the wall an object stands against.

## Settings

- *Elements*: *Selection*; doors, windows and objects ticked; no library part text.
- Margins: doors and windows 0.10 m, objects 0.20 m.
- *Take the margin from a property*: *(none)*.
- The per library part table: empty.

The settings and the table are remembered between runs.

## What it changes

*Apply* moves the doors and windows along their wall and moves the objects. Nothing changes before
you confirm. It writes its settings to `Documents/Tapir/WallClearance.json`; *Save...* under the
table writes the margins and the table to a file you choose (default name
`Wall Clearance.json`), so an office can share its standards.

## Limits

- Distances are in metres; property values are read in metres.
- Doors and windows in curved walls, objects inside a wall or column, and objects whose floor
  plan symbol cannot be read are listed as not checked.
- A door or window is not moved past the end of its wall or past the next door or window in it;
  an object is not moved into another checked object, and one that stands against no wall moves
  at most 1 m. Such elements are listed red and left as they are.
- A wall without a floor plan outline is measured from its reference line and thickness.
- Works in Archicad 25 to 30 with Tapir 1.8.1 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2146, written with the Tapir issue bot. Measuring doors and windows by
their opening in the wall comes from @Cri5chan's pull request #2158.
