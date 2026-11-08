# Zones to Walls and Slabs

For early design: draw the rooms as zones, select them and run the script, and it
makes the walls around them and the slabs under and over them. An edge two zones
share becomes one internal partition; every other edge gets an external wall,
placed outside the zone so the room keeps its area. Each story gets a floor slab,
and the story above the highest zones a roof slab. Where a story is set back from
the one below, the exposed part is a terrace, and its open edges get a balustrade.
It works story by story, on as many stories as you select zones on.

## How to use it

1. Select the zones, in a plan or in 3D, on one or several stories.
2. Run the script from the Tapir palette.
3. Pick the layer and the thickness of the internal partitions and of the
   external walls, the slabs' layer and thickness or a Slab favorite, and the
   gap tolerance (below). Untick **Slabs** to make walls only. Choose how the
   balustrades are made (Railing or Wall, with a favorite of that tool or a
   height), or untick **Balustrades**.
4. Press **Preview**. The window lists, per story, how many zones it read, how
   many partitions and external walls it will make and how high they will be,
   how many slabs, with their area, the terraces, with their area, and the
   length of balustrade. It draws the walls, the slab outlines (dashed), the
   terraces (tinted) and the balustrades (dotted) of the story you select in the
   list. The zones are highlighted in Archicad.
5. Press **Create**. It asks before it creates anything, then makes all the
   walls in one Undo step, all the slabs in another and all the balustrades in a
   third, and reports how many it made on each story. The new elements are
   highlighted in blue; **Clear highlight** removes the highlight.

## Settings

| Setting | Default | What it does |
| --- | --- | --- |
| Internal partitions: layer | the Wall tool's default layer | The layer of the walls on the edges two zones share. |
| Internal partitions: thickness | 0.10 m | Their thickness. The partition is centred on the shared edge (halfway between the two zones when they do not quite touch). |
| External walls: layer | the Wall tool's default layer | The layer of the walls on the other edges. |
| External walls: thickness | 0.30 m | Their thickness. The wall stands outside the zone, its inner face on the zone's edge. |
| Slabs | on | Whether slabs are made. |
| Slabs: layer | the Slab tool's default layer | The layer of the slabs, when no favorite is chosen. |
| Slabs: thickness | 0.25 m | Their thickness, when no favorite is chosen (a Basic slab). |
| Slab favorite | none | A Slab favorite of the project. Its settings (layer, structure, composite, thickness, surfaces) are used instead of the layer and thickness. |
| Balustrades | on | Whether balustrades are made around the terraces. |
| Balustrade tool | Railing | **Railing** makes one railing per run of open terrace edges (`CreateRailings`); **Wall** makes a low wall on each straight piece of it. |
| Balustrade height | 1.10 m | Their height, when no favorite is chosen. |
| Balustrades: layer, thickness | the Wall tool's default layer, 0.20 m | For the Wall tool without a favorite (a Basic wall). A railing takes its layer from the Railing tool defaults. |
| Balustrade favorite | none | A favorite of the chosen tool. Its settings, its height included, are used; only the line and the base come from the script. |
| Gap tolerance | empty: the partition thickness | Zone edges no farther apart than this count as shared, since zones rarely touch exactly, and the slab closes the gap between them. Collinear walls no farther apart than this are merged. |

## The rules

- One partition per shared edge, never two. A zone edge that is shared only in
  part gets a partition on the shared part and an external wall on the rest.
- Pieces of wall on one line are merged into one wall (the external walls of
  two rooms side by side become one wall along both).
- Walls meet at corners and T-junctions: their reference lines (in the middle
  of the wall) are joined, so Archicad connects them.
- Heights: the walls run from their story's level up to the next story's level.
  On the top story of the project they are as high as the highest selected zone
  of that story.
- Slabs: the slab of a story is the outline of the zones on that story together
  with the zones on the story below. So:
  - where the story is set back, its slab also covers the exposed part of the
    story below (the terrace);
  - where a zone reaches out past the story below (a cantilever), the story's
    own slab covers it, and nothing more is made;
  - the story above the highest zones gets the roof slab, the outline of the
    zones below it. Above the top story of the project, the roof slab is placed
    on the top story, at the top of its walls.
- The top of each slab is at its story's level (its reference plane is the top).
  Zones that touch, or are closer than the gap tolerance, give one slab; a
  courtyard between zones becomes a hole in the slab.
- Terraces: the terrace of a story is the part of the zones on the story below
  that no zone on the story covers (a setback). The roof above the highest
  zones is not a terrace. A terrace narrower than the gap tolerance is the
  misfit of zones that nearly line up, and is left out.
- Balustrades: an edge of a terrace along a zone of the story already has that
  zone's external wall, so it gets nothing more. Every other edge gets a
  balustrade, standing at the story's level and centred over the external wall
  of the story below. It ends at the outer face of the story's external walls,
  so it runs into them neither in plan nor in 3D. A terrace open all round (an
  upper story standing in the middle of the one below) gets one closed
  balustrade.

## What it changes

It creates walls, slabs and balustrades (railings or walls). The walls are Basic
walls (one building material, the Wall tool's default) with the reference line in
the middle. Without a favorite the slabs are Basic slabs of the thickness given;
everything else comes from the tools' defaults. A railing's posts, rails and
panels come from the Railing tool defaults or its favorite. It changes nothing that is already in the project:
existing walls and slabs are not looked at, so running it twice makes them twice.
It writes no files.

## Limits

- Curved zone edges get no wall; the preview counts them per story. The slabs
  follow them as a straight edge.
- The slabs follow the zones, not the outer face of the external walls, which
  stand outside the zones.
- Zones of two stories that nearly line up give a slab with a narrow step where
  they differ.
- A terrace that is narrow in one place only (a zone drawn a few centimetres
  short along part of an edge) keeps its narrow strip, and its balustrade runs
  along it.
- A balustrade wall made from a favorite keeps the favorite's thickness and is
  centred on the same line, whatever that thickness is.
- Doors and windows are not made.
- Zones farther apart than the gap tolerance each get their own external wall,
  which may overlap the other's.
- Lengths are in metres, whatever the project's units.
- Needs the Tapir Add-On 1.8.0 or later. Not yet tried in Archicad.

## Credits

Requested by @JBENEITEZ in
[#2261](https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2261),
written with the Tapir issue bot. The geometry is tested without Archicad by
`archicad-addon/Test/test_zones_to_walls_and_slabs.py`.
