# Daylight Factor Heatmap

Computes the Daylight Factor (DF) of the selected zones under a CIE overcast sky with
[Radiance](https://www.radiance-online.org/), from the model's real 3D geometry, and checks each
zone against a target (EN 17037: 2 % DF over 50 % of the floor for side-lit rooms). The zones are
coloured in Archicad by their average DF, and **Place heatmaps** draws each zone's grid in its
colours on the floor plan, with a label and a legend.

## How to use it

1. Select the zones to analyse on the floor plan (any stories).
2. Start the script from the Tapir palette. On its first run the palette installs
   [pyradiance](https://pypi.org/project/pyradiance/), which contains the Radiance programs, so
   the first start takes a little longer and needs an internet connection.
3. Check the settings and press **Run**. The status line shows the zone Radiance is working on;
   a zone can take minutes. **Stop** stops the run; the zones already done stay in the table.
4. The table lists per zone the number of grid cells counted, the average and median DF, the
   share of the floor that reaches the target DF, and whether the zone passes. The zones are
   coloured in Archicad on the scale shown above the table (dark is low, light is high; the
   target is marked), and the other elements are faded. **Colour zones** colours them again,
   **Clear** removes the colours.
5. **Place heatmaps...** places a heatmap object in each zone of the table, after saying what it
   will do (see Heatmap objects below). Run it again after a new **Run** to update the objects.
6. **Export...** writes an Excel workbook (sheets Zones, Cells and Settings) or CSV files (the
   zones, plus `<name> cells.csv` and `<name> settings.csv` next to it).

## Settings

| Setting | Default |
| --- | --- |
| Distance read around the zones: elements farther from the selected zones are left out of the scene | 30 m |
| Grid size: the cells the zones are divided into | 0.5 m |
| Work-plane height above the zone floor | 0.85 m |
| Wall band left out of the statistics (the option "Leave the wall band out", EN 17037) | 0.5 m, on |
| Glass light transmittance | 0.70 |
| Wall, floor and ceiling reflectance | 0.50, 0.20, 0.70 |
| Outside surface and ground reflectance | 0.20 |
| Target DF and the share of the floor that must reach it | 2 %, 50 % |
| Top of the colour scale | 5 % DF |
| Include objects (furniture, buildings modelled as objects) | off |
| Glass surface names: a face is glass when its surface's name contains one of these | glass, glazing, glas, verre, vitre, vetro, vidrio, … |
| Radiance options (rtrace) | `-ab 5 -ad 2048 -as 1024 -ar 256 -aa 0.1 -lw 0.0002` |
| Unit of the lengths | m (or ft) |
| Layer of the heatmap objects | Daylight Factor Heatmap (created when missing) |
| Legend under each label | on |
| Cross the cells below the target | on |
| Coloured surface at the work plane in 3D | off |

## Heatmap objects

**Place heatmaps...** writes the library part *Tapir Daylight Factor Heatmap 1* into the
project's embedded library (once, under `Tapir/`) and places one of it in each zone, on the
layer chosen in the window, at the zone's story and floor level. Each object:

- draws every cell of the zone's grid filled in its colour on the floor plan. Archicad draws
  plans with the pens of the pen table, so each of the scale's 20 colour bands is drawn with
  the pen of the pen table nearest to that colour (read when you place the objects);
- crosses the cells below the target DF, so the plan reads without colour too;
- shows the zone's name, average and median DF and the share of the floor reaching the target,
  with a legend of the scale and the target marked under it. The label stays upright; its pen
  and text size are settings of the object;
- optionally (*Coloured surface at the work plane in 3D*) shows the cells in 3D as a thin
  surface at the work-plane height, in the scale's own colours.

Placing again after a new run finds the zones' objects (by the zone they were placed for) and
updates their values, and moves them when the zone has moved, instead of placing new ones. The
top of the scale, the target and the switches above are also parameters of the object, so they
can be changed in its settings dialog afterwards.

## How it is computed

- The scene is the 3D geometry (Get3DGeometryOfElements) of the walls, slabs, roofs, shells,
  columns, beams, windows, doors, skylights, curtain walls, meshes, morphs, stairs and railings
  whose 3D box lies within the distance around the zones, from the lowest zone's floor upwards
  (upper floors, roofs, overhangs, balconies and other buildings in the model shade the zones).
- The faces of elements at a zone are walls, floors (facing up) or ceilings (facing down); the
  faces of elements away from the zones are outside surfaces. A face whose surface name contains
  one of the glass surface names is glass: one side of each pane is kept, and with several panes
  behind each other each gets its share of the glass transmittance.
- The sky is a CIE standard overcast sky with a uniform ground, scaled to 100 lx on an
  unobstructed horizontal plane, so the illuminance of a point in lux is its DF in %. No location
  or weather file is needed.
- The grid is laid along each zone's longest edge, so the same room moved or rotated gets the
  same cells. A zone smaller than the wall band counts all its cells.
- The Radiance files of each run (scene, sky, materials, grid points, results and the settings)
  are kept in `Documents/Tapir/Daylight Factor/<date and time>/`, and the settings are in the
  export, so a result can be reproduced with Radiance.

## What it changes

**Run** and **Colour zones** change nothing in the project: they only colour the zones (the
highlight, which closing the window or **Clear** removes). **Place heatmaps...**, after asking,
creates the heatmap library part in the embedded library, the layer *Daylight Factor Heatmap*
when it is chosen and missing, places the heatmap objects and updates (and moves) the ones it
placed before. Placing and updating the objects can be undone in Archicad; the library part
stays in the embedded library. Delete the objects to remove the heatmaps.
It writes the Radiance files of each run in
`Documents/Tapir/Daylight Factor/<date and time>/`, and the exported files where you save them.
It runs the Radiance programs `oconv` and `rtrace` of the pyradiance package on your computer.

## Limits

- The geometry is what Archicad's 3D model shows: elements on hidden layers or filtered out of
  the 3D view are not in the scene.
- Glass is found by the name of its surface. A window or skylight without a face whose surface
  name contains one of the glass surface names lets no light in; the status line says how many
  there are, so you can add the name.
- The materials are the reflectances of the window, not of the building materials or surfaces.
- Curved zone edges are taken as straight.
- Daylight Factor only: no annual climate-based metrics (sDA, ASE), glare or views out.
- Windows on ARM computers: pyradiance has no version for them yet, and the script says so.
- Without an internet connection, or with pypi.org blocked, the first start fails while the
  palette installs pyradiance, before the script's window opens.
- The heatmap object holds at most 12,500 cells around a zone (3,125 m² at the 0.5 m grid);
  larger zones are left out of **Place heatmaps** and need a larger grid. Its cell values are
  rounded to 0.05 % DF.
- The plan colours are the nearest pens of the pen table, so they depend on it; the 3D surface
  and the window show the exact scale.
- A heatmap object moved to another layer by hand may not be found again (with Tapir versions
  that cannot tell an object's library part), and then a new one is placed.
- Works in Archicad 25 to 30, with Tapir 1.8.0 or newer.

Requested by @JBENEITEZ in #2301.
