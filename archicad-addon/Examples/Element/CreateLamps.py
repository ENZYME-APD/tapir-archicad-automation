"""Places the first Lamp library part of the loaded libraries 2.5 m above the floor, switched on, with CreateLamps.

Give any loaded lamp library part name in libraryPartName, e.g. one listed by GetAvailableLibraryParts.
The lamp starts from the library part's own default size and parameters; GDL 'parameters' given by
name override them, and 'layerIndex' puts it on a layer (here the first layer of the project).
A top-level 'databaseId' (e.g. the one CreateWorksheets returns) places the lamps in that database
instead of the current one, as in the CreateObjects example.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Lamp'})['libraryParts']
if not libraryParts:
    raise SystemExit ('No Lamp library part in the loaded libraries.')
layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes']

aclib.PrintJson (aclib.RunTapirCommand ('CreateLamps', {'lampsData': [{
    'libraryPartName': libraryParts[0]['documentName'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 2.5},
    'lightIsOn': True,
    'lightColor': {'red': 1.0, 'green': 0.9, 'blue': 0.8},
    'parameters': [{'name': 'A', 'value': 0.5}],
    'layerIndex': layers[0]['index']
}]}))
