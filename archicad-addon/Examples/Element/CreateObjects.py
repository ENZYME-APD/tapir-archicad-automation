"""Places the first Object library part of the loaded libraries at the origin with CreateObjects.

Give any loaded library part name in libraryPartName, e.g. one listed by GetAvailableLibraryParts.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts']
if not libraryParts:
    raise SystemExit ('No Object library part in the loaded libraries.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateObjects', {'objectsData': [{
    'libraryPartName': libraryParts[0]['documentName'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0},
    'angle': 0.0
}]}))
