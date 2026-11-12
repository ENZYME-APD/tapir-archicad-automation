"""Creates a level dimension showing a fixed level, then moves it and changes its marker and texts with ModifyLevelDimensions.

The settings have the keys CreateLevelDimensions takes and GetDetailsOfElements returns; only the
given ones are changed. The text height, pen, font and style apply to both texts of the marker.
"""
import aclib

levelDimension = aclib.RunTapirCommand ('CreateLevelDimensions', {'levelDimensionsData': [
    {'position': {'x': 2.0, 'y': 2.0}, 'level': 0.5}
]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyLevelDimensions', {'levelDimensionsWithDetails': [{
    'elementId': levelDimension['elementId'],
    'position': {'x': 3.0, 'y': 2.0},
    'pen': 3,
    'textHeight': 3.0,
    'textPen': 5,
    'markerSize': 4.0,
    'needPlus': True,
    'bold': True,
    'italic': False
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [levelDimension]}))
