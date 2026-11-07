# Zones to Walls and Slabs

For early design: draw the rooms as zones, select them and run the script, and it
makes the walls around them. An edge two zones share becomes one internal
partition; every other edge gets an external wall, placed outside the zone so the
room keeps its area. It works story by story, on as many stories as you select
zones on.

This version makes the walls. The slabs (a floor slab under each story's zones,
extended over the setbacks of the story below, and a roof slab) follow in the
next version.

## How to use it

1. Select the zones, in a plan or in 3D, on one or several stories.
2. Run the script from the Tapir palette.
3. Pick the layer and the thickness of the internal partitions and of the
   external walls, and the gap tolerance (below).
4. Press **Preview**. The window lists, per story, how many zones it read and
   how many partitions and external walls it will make, and how high they will
   be, and draws the walls of the story you select in the list. The zones are
   highlighted in Archicad.
5. Press **Create**. It asks before it creates anything, then makes all the
   walls in one Undo step and reports how many it made on each story. The new
   walls are highlighted in blue; **Clear highlight** removes the highlight.

## Settings

| Setting | Default | What it does |
| --- | --- | --- |
| Internal partitions: layer | the Wall tool's default layer | The layer of the walls on the edges two zones share. |
| Internal partitions: thickness | 0.10 m | Their thickness. The partition is centred on the shared edge (halfway between the two zones when they do not quite touch). |
| External walls: layer | the Wall tool's default layer | The layer of the walls on the other edges. |
| External walls: thickness | 0.30 m | Their thickness. The wall stands outside the zone, its inner face on the zone's edge. |
| Gap tolerance | empty: the partition thickness | Zone edges no farther apart than this count as shared, since zones rarely touch exactly. Collinear walls no farther apart than this are merged. |

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

## What it changes

It creates Basic walls (one building material, the Wall tool's default) with the
reference line in the middle; everything else comes from the Wall tool's
defaults. It changes nothing that is already in the project: existing walls are
not looked at, so running it twice makes the walls twice. It writes no files.

## Limits

- Curved zone edges get no wall; the preview counts them per story.
- Doors and windows are not made.
- Zones farther apart than the gap tolerance each get their own external wall,
  which may overlap the other's.
- Lengths are in metres, whatever the project's units.
- Needs the Tapir Add-On 1.8.0 or later. Not yet tried in Archicad.

## Credits

Requested by @JBENEITEZ in
[#2261](https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2261).
The geometry is tested without Archicad by
`archicad-addon/Test/test_zones_to_walls_and_slabs.py`.
