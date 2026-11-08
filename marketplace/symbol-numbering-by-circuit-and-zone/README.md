# Electrical and Plumbing Symbol Numbering

Numbers electrical and plumbing symbols with a pattern such as `{board}.{circuit}-{n}` or
`{zone}-{n}`, writes the numbers to the Element ID or to a GDL parameter, and can place a label
with the number at each symbol.

## How to use it

1. Run the script and choose the *Symbols*:
   - *GDL symbols (Objects and Lamps)*: symbols from a library. Pick where the circuit comes from
     (*Circuit from*: a GDL parameter or a property, and its name) and, if the pattern has
     `{board}`, where the board comes from. The lists are filled when the window opens; *Read
     parameter and property names* reads them again, for example after you change *Symbols in*.
   - *Archicad electrical devices (Archicad 30)*: the devices of Archicad's electrical tools.
     Their board is the panel and their circuit the circuit they are connected to.
2. Choose *Symbols in*: *Selection*, *Current story* or *Whole project*.
3. Set the *Pattern*, *Start at*, *Digits* and, for GDL symbols, the *Order*.
4. Choose *Write to*: the *Element ID* or a *GDL parameter*. Tick *Place a label with the number*
   to put a label on each symbol.
5. Press *Read symbols*. The list shows the new and the current number of every symbol; the list
   below it shows the symbols that cannot be numbered and why (no circuit, no board, outside every
   zone, inside more than one zone, a zone without a number, no such parameter). Nothing is
   changed yet. Changing the pattern, the start, the digits or the order updates the list.
6. Press *Write* and confirm. The status column then shows, per symbol, whether it was written and
   labelled, or why not.

### The pattern

- `{board}`: the board (distribution board, panel).
- `{circuit}`: the circuit.
- `{zone}`: the number of the zone whose outline holds the symbol's insertion point, on the
  symbol's own story.
- `{n}`: the running number. It is counted separately for every different text the rest of the
  pattern gives, so `{board}.{circuit}-{n}` counts per circuit and `{zone}-{n}` per zone.

Anything else in the pattern is kept as it is. The script warns when new Element IDs repeat.

### The order

- GDL symbols, *By position*: in rows from top to bottom on the floor plan, each row from left to
  right. A symbol joins a row when it is at most the *Row tolerance* below the row's top symbol.
- GDL symbols, *By current ID*: in the order of their current Element ID (C2 before C10).
- Electrical devices: always in circuit order, the order Archicad lists the devices of each
  circuit in.

## Settings

| Field | Default | Meaning |
| --- | --- | --- |
| *Symbols* | GDL symbols | *GDL symbols (Objects and Lamps)* or *Archicad electrical devices*. |
| *Symbols in* | Current story | *Selection*, *Current story* or *Whole project*. |
| *Circuit from* | GDL parameter, none picked | A GDL parameter or a property of the symbols that holds the circuit. |
| *Board from* | GDL parameter, none picked | The same for the board; only needed when the pattern has `{board}`. |
| *Pattern* | `{board}.{circuit}-{n}` | See *The pattern*. |
| *Start at* | 1 | The first `{n}`. |
| *Digits* | 1 | The least number of digits of `{n}`, padded with zeros (2 gives `01`). |
| *Order* | By position | For GDL symbols: *By position* or *By current ID*. |
| *Row tolerance* | 0.3 m | For *By position*. Also in `cm`, `mm`, `ft` (`'`) or `in` (`"`). |
| *Write to* | Element ID | The *Element ID*, or a *GDL parameter* you pick (a text parameter). |
| *Place a label* | off | Places a text label with the number at each numbered symbol, from the Label tool's settings or the favorite you name. |

## What it changes

After you confirm: the Element ID, or the picked GDL parameter, of the symbols whose number
changes, all in one Undo step; and, when *Place a label* is ticked, one new label per numbered
symbol, in a second Undo step. A label is placed once per symbol while the window is open;
running the script again places new ones, so delete the old labels first. It writes no files.

## Limits

- GDL symbols are Objects and Lamps. Library objects have no standard circuit parameter: pick the
  one your library uses, or a property.
- `{zone}` uses the zone's polygon on the symbol's story. A symbol on a zone's boundary may fall on
  either side. Curved zone edges are followed closely, not exactly.
- Writing to a GDL parameter needs a text (string) parameter; a number parameter is reported as not
  written.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. Electrical devices need Archicad 30; in older
  versions, use GDL symbols.
- In a Teamwork project, a symbol you have not reserved is not changed; it is shown as not written,
  with the reason.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2017, written with the Tapir issue bot.
