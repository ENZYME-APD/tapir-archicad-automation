"""Creates two columns with CreateColumns: one with the Column tool defaults and one round, 3 m high column.

Coordinates are in meters; z is the elevation of the column base.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateColumns', {'columnsData': [
    {
        'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}
    },
    {
        'coordinates': {'x': 3.0, 'y': 0.0, 'z': 0.0},
        'height': 3.0,
        'width': 0.4,
        'circleBased': True
    }
]}))
