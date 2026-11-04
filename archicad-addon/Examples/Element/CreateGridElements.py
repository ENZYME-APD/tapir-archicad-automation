"""Creates two crossing 20 m long grid lines with CreateGridElements.

The library part comes from the Grid Tool defaults; parameters sets GDL parameters by name (see
GetGridElementDefaults). Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateGridElements', {'gridElements': [
    {
        'begCoordinate': {'x': 0.0, 'y': -2.0},
        'endCoordinate': {'x': 0.0, 'y': 18.0}
    },
    {
        'begCoordinate': {'x': -2.0, 'y': 0.0},
        'endCoordinate': {'x': 18.0, 'y': 0.0}
    }
]}))
