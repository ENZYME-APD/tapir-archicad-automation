"""Creates a wall and places two openings in it with PlaceOpenings.

The first opening uses the Opening tool settings, the second gets its own size and floor plan display
(the tool settings are not changed). Requires Archicad 29 or later.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 8.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('PlaceOpenings', {'openingsData': [
    {
        'parentElementId': wall['elementId'],
        'point': {'x': 2.0, 'y': 0.0, 'z': 1.0}
    },
    {
        'parentElementId': wall['elementId'],
        'point': {'x': 5.0, 'y': 0.0, 'z': 1.0},
        'extrusionParameters': {'linkedStatus': 'NotLinked', 'width': 1.2, 'height': 0.6},
        'floorPlanParameters': {'floorPlanDisplayMode': 'Projected'}
    }
]}))
