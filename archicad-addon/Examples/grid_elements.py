import aclib

# Requires Archicad 26 or newer.

# The Grid Tool defaults: the library part and the GDL parameters a new grid element gets.
defaults = aclib.RunTapirCommand ('GetGridElementDefaults', {})
print (f"Grid Tool library part: {defaults['libraryPartName']}")
print (f"Number of GDL parameters: {len (defaults['parameters'])}")

# Two crossing grid lines, 20 m long. The length goes to the object's A size (xRatio).
newGrids = aclib.RunTapirCommand ('CreateGridElements', {
        'gridElements': [
            {
                'begCoordinate': {'x': 0.0, 'y': -2.0},
                'endCoordinate': {'x': 0.0, 'y': 18.0}
            },
            {
                'begCoordinate': {'x': -2.0, 'y': 0.0},
                'endCoordinate': {'x': 18.0, 'y': 0.0}
            }
        ]
    })
print (f"Created {len (newGrids['elements'])} grid elements, warnings: {newGrids['warnings']}")

# Every grid element of the project with its start point, direction and length.
for grid in aclib.RunTapirCommand ('GetGridElements', {})['gridElements']:
    print (f"Grid at ({grid['pos']['x']:.2f}, {grid['pos']['y']:.2f}), angle {grid['angle']:.4f} rad, length {grid['xRatio']:.2f} m")
