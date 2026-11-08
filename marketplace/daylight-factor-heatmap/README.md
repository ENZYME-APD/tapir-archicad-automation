# Daylight Factor Heatmap

Computes the Daylight Factor (DF) of the selected zones under a CIE overcast sky with
[Radiance](https://www.radiance-online.org/), from the model's real 3D geometry, and checks each
zone against a target (EN 17037: 2 % DF over 50 % of the floor for side-lit rooms). The zones are
coloured in Archicad by their average DF.

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
5. **Export...** writes an Excel workbook (sheets Zones, Cells and Settings) or CSV files (the
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

Nothing in the project: it only colours the zones (the highlight, which closing the window or
**Clear** removes). It writes the Radiance files of each run in
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
- Works in Archicad 25 to 30, with Tapir 1.8.0 or newer.

Requested by @JBENEITEZ in #2301.
