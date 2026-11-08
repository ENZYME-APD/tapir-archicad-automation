"""Places the first Object library part of the loaded libraries at the origin with CreateObjects, then once more in a new worksheet.

Give any loaded library part name in libraryPartName, e.g. one listed by GetAvailableLibraryParts.
The object starts from the library part's own default size and parameters; 'dimensions' and the
GDL 'parameters' given (by name, e.g. A and B, the size) override them, and 'layerIndex' puts it
on a layer (here the first layer of the project).
With 'databaseId' (here the one CreateWorksheets returns) the object is placed in that database,
e.g. for a legend of symbols in a worksheet, and the front window stays as it is. The example
worksheet is deleted again at the end.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts']
if not libraryParts:
    raise SystemExit ('No Object library part in the loaded libraries.')
layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes']

aclib.PrintJson (aclib.RunTapirCommand ('CreateObjects', {'objectsData': [{
    'libraryPartName': libraryParts[0]['documentName'],
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0},
    'angle': 0.0,
    'parameters': [{'name': 'A', 'value': 1.2}, {'name': 'B', 'value': 0.8}],
    'layerIndex': layers[0]['index']
}]}))

worksheet = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{'name': 'Tapir CreateObjects Example', 'referenceId': 'TAPIR-OBJ'}]
})['databases'][0]
aclib.PrintJson (aclib.RunTapirCommand ('CreateObjects', {
    'databaseId': worksheet['databaseId'],
    'objectsData': [{
        'libraryPartName': libraryParts[0]['documentName'],
        'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}
    }]
}))

if 'navigatorItemId' in worksheet:
    aclib.RunTapirCommand ('DeleteNavigatorItems', {
        'navigatorItemIds': [{'navigatorItemId': worksheet['navigatorItemId']}]
    })
