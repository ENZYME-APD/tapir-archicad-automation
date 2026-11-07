# ContourMesh

Creates one Archicad Mesh, a 3D terrain, from a DXF file with 3D contour lines.
Every contour becomes a ridge line of the mesh at its own height, so the mesh
follows the ground, and the mesh gets a solid skirt below it so the terrain has
a thickness.

## How to use it

1. Open the project. The mesh goes on the story the Mesh tool's defaults use.
2. Run the script from the Tapir palette.
3. Pick the DXF file (*Browse...*). It should hold only the contour lines.
4. Choose the units, the placement and the thickness (below), then press
   *Create mesh*.
5. The script says how many contour lines it found and asks before it creates
   the mesh. Press *F* (Zoom Full Drawing) afterwards to see it.

## Settings

| Setting | Default | What it does |
| --- | --- | --- |
| Units | `auto` | The unit the DXF coordinates are in. `auto` reads the DXF header (`$INSUNITS`; meters when it is missing). The header is often wrong, so pick meters, millimeters, centimeters, feet, inches or kilometers when the mesh comes out at the wrong size. |
| Move the mesh's lower left corner to the project origin | off | Moves the mesh so the lower left corner of its outline is at X 0, Y 0. The heights stay as in the DXF. |
| Target location | empty | `X, Y` or `X, Y, Z` in meters. Moves the lower left corner of the outline to X, Y and adds Z to every height. Overrides the origin option. Empty: the DXF coordinates are used as they are. |
| Thickness below lowest contour | 1.0 m | The depth of the skirt below the mesh, in meters, 0 or more. When contours lie below 0, the skirt is made that much deeper. |

## What it changes

It creates one Mesh, *Solid Body with Skirt* with its ridge lines shown (with a
thickness of 0 no skirt is set, so the skirt type is the Mesh tool's default); the
rest (story, layer, building material) comes from the Mesh tool's defaults. Nothing else in the project is changed, and it writes no
files. It is one Undo step.

## How it reads the DXF

- It reads `LWPOLYLINE` and old-style `POLYLINE` (with `VERTEX` points)
  entities. Lines drawn as separate `LINE` segments are not read.
- Only lines that are flat (the same height at every point) count as contours;
  other 3D lines, e.g. fences or eaves, are left out.
- Contours with no height of their own take the polyline's elevation.
- Very small closed contours (under 1 m²) and short open ones (under 1 m) are
  left out.
- The outline of the mesh is the convex hull of all the points (the shape a
  rubber band around them would make). Its heights are those of the nearest
  contour point.

## Limits

- Tried in Archicad 30. It needs the Tapir Add-On 1.5.0 or later.
- The target location and the thickness are in meters, whatever the units of the
  project or the DXF.
- One mesh per run, from one DXF file. Very large files (many thousands of
  contour points) take a while to read.
- The outline is convex: a site with an L-shaped boundary gets a mesh over the
  whole hull.

## Credits

Original by Paul K (`ArchPrime`), shared in
[#1913](https://github.com/ENZYME-APD/tapir-archicad-automation/pull/1913) under
the MIT licence. Adapted for the marketplace for
[#2169](https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2169).
