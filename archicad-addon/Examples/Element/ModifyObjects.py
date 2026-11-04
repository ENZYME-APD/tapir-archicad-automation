"""Places an object, then moves it, turns it by 90 degrees and mirrors it with ModifyObjects.

The object is the first Object library part of the loaded libraries. Angles are in radians.
"""
import aclib
import math

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts']
if not libraryParts:
    raise SystemExit ('No Object library part in the loaded libraries.')

obj = aclib.RunTapirCommand ('CreateObjects', {'objectsData': [{
    'libraryPartName': libraryParts[0]['documentName'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyObjects', {'objectsWithDetails': [{
    'elementId': obj['elementId'],
    'coordinates': {'x': 2.0, 'y': 1.0, 'z': 0.0},
    'angle': math.pi / 2,
    'reflected': True
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [obj], 'fields': ['details']})
