"""Places a lamp, then raises it, switches it off and makes its light blue with ModifyLamps.

The lamp is the first Lamp library part of the loaded libraries. Colors are RGB values between 0 and 1.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Lamp'})['libraryParts']
if not libraryParts:
    raise SystemExit ('No Lamp library part in the loaded libraries.')

lamp = aclib.RunTapirCommand ('CreateLamps', {'lampsData': [{
    'libraryPartName': libraryParts[0]['documentName'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 2.0}
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyLamps', {'lampsWithDetails': [{
    'elementId': lamp['elementId'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 2.5},
    'lightIsOn': False,
    'lightColor': {'red': 0.6, 'green': 0.8, 'blue': 1.0}
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [lamp], 'fields': ['details']})
