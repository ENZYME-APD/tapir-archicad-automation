"""Places the first Lamp library part of the loaded libraries 2.5 m above the floor, switched on, with CreateLamps.

Give any loaded lamp library part name in libraryPartName, e.g. one listed by GetAvailableLibraryParts.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Lamp'})['libraryParts']
if not libraryParts:
    raise SystemExit ('No Lamp library part in the loaded libraries.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateLamps', {'lampsData': [{
    'libraryPartName': libraryParts[0]['documentName'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 2.5},
    'lightIsOn': True,
    'lightColor': {'red': 1.0, 'green': 0.9, 'blue': 0.8}
}]}))
