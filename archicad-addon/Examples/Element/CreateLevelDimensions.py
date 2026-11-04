"""Creates a slab and puts an associative level dimension on it with CreateLevelDimensions, plus one showing a fixed level.

The associative one shows the slab's level at its position and follows the slab when it changes;
level (in meters, from the project zero) is only used without parentElementId.
"""
import aclib

slab = aclib.RunTapirCommand ('CreateSlabs', {'slabsData': [{
    'level': 1.2,
    'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}]
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateLevelDimensions', {'levelDimensionsData': [
    {'position': {'x': 2.0, 'y': 2.0}, 'parentElementId': slab['elementId']},
    {'position': {'x': 6.0, 'y': 2.0}, 'level': 0.5}
]}))
