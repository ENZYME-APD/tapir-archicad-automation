"""Creates three columns with CreateColumns: one with the Column tool defaults, one round, 3 m high column with its own
cut fill pens, and one whose top is linked to the story above, 15 cm below its level.

Coordinates are in meters; z is the elevation of the column base. CreateColumns takes the same cut fill pens
(written from Archicad 27), cover fill and top link (relativeTopStory, topOffset) as ModifyColumns. A linked column
gets its height from the story levels, so it is not given a height; the story above must exist.
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
        'circleBased': True,
        'cutFillPen': {'overridden': True, 'penIndex': 5},
        'cutFillBackgroundPen': {'overridden': True, 'penIndex': 7}
    },
    {
        'coordinates': {'x': 6.0, 'y': 0.0, 'z': 0.0},
        'floorIndex': 0,
        'relativeTopStory': 1,
        'topOffset': -0.15
    }
]}))
