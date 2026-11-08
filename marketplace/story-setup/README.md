# Story Setup

Sets up the stories of the project from a table: paste one from Excel or any text, or load a CSV
file. It shows what would be added, renamed, moved and deleted before anything changes.

## How to use it

1. Run the script. It opens with the current stories as a table (`index`, `name`, `level`), so
   you only edit what should change. *Copy current stories* fills it in again.
2. Edit the table, paste another one, or press *Load CSV...*. One story per row. Columns can be
   separated by tabs, `;` or `,`, and a decimal comma works unless `,` separates the columns.
   The header row is optional (`name`, `level` or `elevation`, `height`, `index`); without one,
   the second column is the level or the height, as chosen at the top of the window.
3. With heights only, say whether the rows list the top story first (as Story Settings does) or
   the bottom story first, and give the *Level of the bottom story*.
4. Press *Compare*. The list shows each story index with its current name, level and height
   next to the new ones, and the change: added, renamed, moved or deleted.
5. Press *Apply* and confirm the list of changes.

Stories are matched by their index, as Archicad's story structure is: an `index` column wins
(the table the script opens with has one); without it the row at level 0 becomes story 0;
without either, the bottom row keeps the index of the current bottom story.

## Settings

- *Without a header row the second column is the*: level. *Unit*: m (also cm and mm).
- *With heights only - rows listed*: top story first; *Level of the bottom story*: 0.

## What it changes

The stories of the project (names, levels, added and deleted stories), only after *Apply* and
its confirmation. Before it changes them it reads the stories again, and stops if they changed
in Archicad since *Compare*. It writes no files; *Load CSV...* only reads one.

## Limits

- A table that would delete a story with elements on it is refused. Whether a story has
  elements can only be checked with the Floor Plan in front, so a deletion is refused in any
  other window.
- The height of the top story cannot be set; it stays as Archicad has it.
- New stories have *Show on sections* on; the other stories keep their setting.
- Removing a row in the middle of the table shows as renames plus a deletion at the top or the
  bottom, because stories are matched by index.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2009, written with the Tapir issue bot.
