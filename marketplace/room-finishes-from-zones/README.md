# Room Finishes from Zones

Adds the finishes of the selected zones: a floor finish and a ceiling for each zone, as slabs
with the zone's polygon, and a wall finish along every edge with a wall behind it. Each kind is
made from a favorite of its tool, or from a layer, a building material or composite (or, for
the wall finishes, a complex profile) and a thickness. A second run on the same zones replaces
the finishes of the first one.

This version makes the floor finishes, the ceilings and the wall finishes. Openings kept clear
for doors and windows, and the cove, are coming in the next versions (#2292). Until then a wall
finish covers the doors and windows behind it.

## How to use it

1. Select the zones in Archicad. They can be on several stories.
2. Run the script. For *Floor finish* and *Ceiling*, choose a *Slab favorite*, and for *Wall
   finish* a *Wall favorite*; or leave it at *(no favorite)* and choose a *Structure* (a
   building material, a composite or, for walls, a complex profile) and a *Thickness*. Choose a
   *Layer* if the finish should not be on the favorite's or the tool's layer. Untick *Create the
   floor finishes*, *Create the ceilings* or *Create the wall finishes* to leave that kind out.
3. Under *Room height*, choose *The zone's height* or type a height.
4. Press *Preview*. The table at the top sums up each story: the zones, the finishes to create,
   the number and total length of the wall finishes, the area and the clashes. The list below
   gives each zone's floor finish top and ceiling bottom (absolute heights in metres), its number
   of wall finishes and notes. Select a zone to draw it with its levels and its wall finishes (in
   blue, on the room side of the edge); double-click it to select it in Archicad. The zones are
   highlighted in Archicad: green, or orange when a structural slab clashes with one of its
   finishes.
5. Press *Create* and confirm. The new slabs and walls are highlighted in blue. *Clear
   highlight*, or closing the window, ends the highlight.

## Where the finishes go

- **Floor finish:** its top face is at the zone's base (the story level, unless the zone is
  offset from it), so it grows downwards by its thickness.
- **Ceiling:** its bottom face is at the room height (the zone's base plus the zone's height, or
  the height you typed), so it grows upwards. A zone with no height gets no ceiling unless you
  type a height.
- The slab has the zone's polygon, with its holes and curved edges.
- **Wall finish** (Archicad 28 and later): a wall along each part of a zone edge that has a wall
  behind it, found from the zone's boundaries in Archicad. Its reference line is on the room
  edge (its *Outside* face, not mirrored, offset 0) and its thickness grows into the room. It
  runs from the zone's base, the floor finish's top, up to the room height, the ceiling's bottom.
  An edge, or the part of an edge, with no wall behind it (a virtual boundary, an opening to the
  next zone, a column) gets no finish. The finishes of two edges meet at the corner on the room
  edge, so Archicad joins them there like any two walls that meet. The edges of the zone's holes
  get a finish too, facing the room; curved edges and pieces shorter than 5 cm get none.
  The reference line location, offset and side are set by the script, whatever the favorite
  says. A complex profile has no reference line location: Archicad puts the profile's origin on
  the room edge, so which side the profile goes to depends on how it is drawn. Check it in a
  section after the first run.

### Structural slabs

The floor finish needs room under the zone's base: the structural slab's top should be at the
zone's base **minus the finish's thickness**. A script that puts the structural slab's top at the
story level (such as *Zones to Walls and Slabs*) leaves no room for it. With *Warn about
structural slabs that overlap a finish* ticked, the script lists every slab, other than its own
finishes, that overlaps a finish both in plan and in height, and moves nothing:

- in *Preview*, from the thickness typed in the window. With a favorite or a composite the
  thickness is not known yet, so only a slab that reaches the floor finish's top (or the
  ceiling's bottom) is found;
- after *Create*, again with the real thickness of each new slab. These clashes are printed to
  the palette's output.

## Element IDs and running it again

Every finish gets an Element ID from its zone's number: `FIN-<zone number>-floor`,
`FIN-<zone number>-ceiling` and `FIN-<zone number>-wall-1`, `-wall-2`, … (spaces in the number
become `_`; a zone without a number uses the start of its guid, and two selected zones with the
same number on one story get their guid added). You can find them with Find & Select or in a
schedule.

With *Replace the finishes an earlier run made for these zones* ticked (the default), *Create*
first deletes the finishes with the same Element ID on the zone's story (for walls: every
`FIN-<zone number>-wall-…`), then creates the new ones. Unticked, a zone that already has a
finish of that kind keeps it and gets no second one. Only the kinds you create are replaced:
with *Create the ceilings* unticked, the ceilings of an earlier run stay.

So when a zone changes, select it and run the script again.

If the zone was updated after the first run, so that it now ends at the wall finishes, the new
wall finishes take the lines of the earlier ones (the room edges the zone had then) instead of
the zone's edges, and the note says so. Walls added or moved since then are not seen; to start
again from the structural walls, delete the wall finishes, update the zone and run the script.

## Settings

- *Slab favorite*: none. *Layer*: the favorite's or the tool's layer. *Structure*: the tool's
  building material.
- *Thickness*: 0.05 m for the floor finish, 0.02 m for the ceiling, 0.015 m for the wall finish
  (used without a favorite, for a building material; a composite or a complex profile gives its
  own thickness).
- *Room height*: the zone's height; the typed height is 2.60 m.
- *Replace the finishes an earlier run made*: on. *Warn about structural slabs*: on.

## What it changes

Nothing until you press *Create* and confirm: then it deletes the finishes of an earlier run
for the selected zones (when *Replace* is on), in one Undo step, and creates the new slabs, in
one more Undo step, and the new walls, in a third. *Preview* only highlights the zones. It writes
no files.

## Limits

- Only the selected zones are used.
- Floors and ceilings are flat: sloped ceilings are not made.
- A composite must be one that is available for slabs; otherwise Archicad refuses the slab and
  the error is shown.
- Lengths are in metres, whatever the project's units. The area in the summary is the zone's
  polygon less its holes, with curved edges counted as straight.
- The clash check compares points of a grid in plan, so a sliver of overlap narrower than about
  a sixteenth of the zone can be missed.
- In Teamwork, or on a locked layer, the finishes of an earlier run may not be deletable. Then
  nothing is created (that would add a second finish next to the old one) and the error is
  shown; reserve or unlock them and press *Preview* again.
- A wall finish covers the doors, windows and openings behind it, for now (#2292).
- The zone's boundaries are those Archicad gives for the zone as it is: an automatic zone that
  was not updated after its walls moved can give edges with no wall behind them.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later, which sets the Element ID and the layer of
  the new slabs and walls. The wall finishes need Archicad 28 or later (the zone boundaries come
  from Archicad's API there); in Archicad 25 to 27 the preview says so and makes only the floor
  finishes and ceilings.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @JBENEITEZ in #2292. Written by the Tapir project.
