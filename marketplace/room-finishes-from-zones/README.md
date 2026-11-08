# Room Finishes from Zones

Adds the finishes of the selected zones: a floor finish and a ceiling for each zone, as slabs
with the zone's polygon, and a wall finish along every edge with a wall behind it. Each kind is
made from a favorite of its tool, or from a layer, a building material or composite (or, for
the wall finishes, a complex profile) and a thickness. Every wall finish gets an opening for each
door, window and opening behind it. A cove, made from a Beam or Wall favorite, can run along the
room edge at the ceiling, with the ceiling held back from the edge so that the cove sits in the
gap. A second run on the same zones replaces the finishes of the first one.

## How to use it

1. Select the zones in Archicad. They can be on several stories.
2. Run the script. For *Floor finish* and *Ceiling*, choose a *Slab favorite*, and for *Wall
   finish* a *Wall favorite*; or leave it at *(no favorite)* and choose a *Structure* (a
   building material, a composite or, for walls, a complex profile) and a *Thickness*. Choose a
   *Layer* if the finish should not be on the favorite's or the tool's layer. Untick *Create the
   floor finishes*, *Create the ceilings* or *Create the wall finishes* to leave that kind out.
3. Under *Room height*, choose *The zone's height* or type a height. Leave *Cut an opening in
   the wall finishes for every door, window and opening behind them* ticked, and type a
   *Margin around them* if their frames or linings stand proud of the wall (see below).
   For a cove, tick *Create a cove at the ceiling* under *Cove*, choose the *Tool* (Beam or
   Wall), its *Favorite* and, if you like, a *Layer*, and type the *Ceiling offset* (see below).
4. Press *Preview*. The table at the top sums up each story: the zones, the finishes to create,
   the number and total length of the wall finishes, their area with every opening deducted, the
   number of doors, windows and openings kept clear, the length of the coves, the area and the
   clashes. The list below
   gives each zone's floor finish top and ceiling bottom (absolute heights in metres), its number
   of wall finishes and notes, among them every door, window and opening it keeps clear. Select a
   zone to draw it with its levels and its wall finishes (in blue, on the room side of the edge,
   with the openings in orange) and its cove (in violet on the room edge, with the ceiling's
   moved edge dashed); double-click it to select it in Archicad. The zones are
   highlighted in Archicad: green, or orange when a structural slab clashes with one of its
   finishes.
5. Press *Create* and confirm. The new slabs, walls and coves are highlighted in blue. The palette's
   output lists, per zone, the doors, windows and openings kept clear. *Clear highlight*, or
   closing the window, ends the highlight.

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

### Doors, windows and openings

With *Cut an opening in the wall finishes…* ticked (the default), the wall finish runs on along
the edge and gets an opening for every door, window and opening (made with the Opening tool) in
the wall behind it (Archicad 29 and later):

- **Size:** the door's or window's width and height in its settings, or the size of the
  opening's polygon, plus the *Margin* on each side and at the top. Door frames and linings often
  stand proud of the wall face into the room; the width and height Archicad gives are those of
  the wall opening, so type the frame's width as the margin (default 0).
- **Height:** a door's opening starts at the finish's bottom (sill 0) and reaches the door's head
  plus the margin; a window's starts the margin below its sill. No opening reaches above the
  finish.
- **It cuts only the finish:** it is a finite opening as deep as the finish (5 mm more on each
  face), placed on the finish's room face, and after it is placed the script takes every other
  element it cuts (the structural wall behind, a slab) off its list, so the structural wall,
  which already has its door or window, and the slabs are unchanged.
- **Which ones:** the doors and windows of straight walls, and the openings of walls, that lie
  along a wall finish, in the wall right behind it (no farther from the room edge than that wall
  is thick) and between the finish's bottom and top. Doors and windows in curved or polygonal
  walls are not found.
- **It does not follow the door.** If a door or window is moved or resized later, the finish's
  opening stays where it was. Run the script again on the zone: it replaces the wall finishes,
  with their openings, and cuts them where the doors and windows are now.
- **Quantities:** Archicad deducts openings from wall areas by the project's calculation rules
  (some leave out small openings), so the wall finish's area in a schedule can differ from the
  *Wall finish area, openings deducted* in the summary, which deducts every opening.
- In Archicad 25 to 28, which cannot place such openings (PlaceOpenings needs Archicad 29), the
  wall finishes are still made, but they cover the doors and windows behind them; the preview
  says so and lists them per zone (*covers:*).

After *Create*, an opening that is not square to the finish's face, or that Archicad could not
read back, is listed under *Check:* in the palette's output: look at it in 3D.

### Cove

With *Create a cove at the ceiling* ticked (off by default), every zone gets a cove along each
straight edge of its outline and of its holes, all round the room, also along an edge with no
wall behind it, because the ceiling is held back along every edge:

- **What it is:** a beam or a wall (*Tool*) from the chosen *Favorite*, which gives its size,
  material and, usually, its complex profile. Save a favorite with the cove's profile in Archicad
  first; the cove needs one.
- **Where it goes:** its reference line is on the room edge and it grows into the room, like the
  wall finishes: a wall with its reference line on its *Outside* face, not mirrored, offset 0; a
  beam anchored at its bottom left (looking along the edge, the room is on the right), offset 0.
  Its bottom is at the room height, which is the ceiling's bottom and the wall finishes' top, so
  it stands on top of the wall finishes and does not overlap them. Its height comes from the
  favorite. The pieces of two edges meet at the corner on the room edge. As for the wall
  finishes, which side a complex profile goes to depends on how it is drawn, so check it in a
  section after the first run.
- **The ceiling:** the ceiling's outline is moved into the room, and its holes out of their
  hole, by the *Ceiling offset* (default 0.10 m), the corners where the moved edges meet. Type
  the cove's width there, so that the cove sits in the gap between the ceiling and the room edge.
  The floor finish keeps the zone's polygon.
- A zone with a curved edge gets no cove, and its ceiling keeps the zone's polygon; so does a
  zone too small for the offset (an edge would vanish). The note says so.

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
`FIN-<zone number>-ceiling`, `FIN-<zone number>-wall-1`, `-wall-2`, … and
`FIN-<zone number>-cove-1`, `-cove-2`, … (spaces in the number
become `_`; a zone without a number uses the start of its guid, and two selected zones with the
same number on one story get their guid added). You can find them with Find & Select or in a
schedule.

With *Replace the finishes an earlier run made for these zones* ticked (the default), *Create*
first deletes the finishes with the same Element ID on the zone's story (for walls: every
`FIN-<zone number>-wall-…`, with the openings in them; for coves every beam and wall
`FIN-<zone number>-cove-…`), then creates the new ones. Unticked, a zone that already has a
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
- *Cut an opening in the wall finishes…*: on. *Margin around them*: 0 m.
- *Create a cove at the ceiling*: off. *Tool*: Beam. *Favorite*: the first of the tool's
  favorites. *Ceiling offset*: 0.10 m.

## What it changes

Nothing until you press *Create* and confirm: then it deletes the finishes of an earlier run
for the selected zones (when *Replace* is on), in one Undo step, and creates the new slabs, in
one more Undo step, and the new walls, in a third. Each opening in a wall finish is placed in an
Undo step of its own, and one more fits them to the finishes. The coves come last, in one more
Undo step. *Preview* only highlights the
zones. It writes no files.

## Limits

- Only the selected zones are used.
- Floors and ceilings are flat: sloped ceilings are not made. Skirting boards and tiling patterns
  are not made either.
- The cove runs along every straight edge of the zone; it is not left out at an opening to the
  next zone, and zones with a curved edge get none.
- A composite must be one that is available for slabs; otherwise Archicad refuses the slab and
  the error is shown.
- Lengths are in metres, whatever the project's units. The area in the summary is the zone's
  polygon less its holes, with curved edges counted as straight.
- The clash check compares points of a grid in plan, so a sliver of overlap narrower than about
  a sixteenth of the zone can be missed.
- In Teamwork, or on a locked layer, the finishes of an earlier run may not be deletable. Then
  nothing is created (that would add a second finish next to the old one) and the error is
  shown; reserve or unlock them and press *Preview* again.
- The openings in the wall finishes need Archicad 29 or later; before it a wall finish covers
  the doors, windows and openings behind it.
- The zone's boundaries are those Archicad gives for the zone as it is: an automatic zone that
  was not updated after its walls moved can give edges with no wall behind them.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later, which sets the Element ID and the layer of
  the new slabs and walls. The wall finishes need Archicad 28 or later (the zone boundaries come
  from Archicad's API there); in Archicad 25 to 27 the preview says so and makes only the floor
  finishes and ceilings. The openings in the wall finishes need Archicad 29 or later.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @JBENEITEZ in #2292, written with the Tapir issue bot.
