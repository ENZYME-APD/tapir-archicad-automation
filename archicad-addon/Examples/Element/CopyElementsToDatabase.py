"""Copies a polyline from the floor plan into a new worksheet, moved by 1 m, with CopyElementsToDatabase.

Run it with the floor plan open. The polyline and the worksheet are created by the script and deleted at the end.
"""
import aclib

polylines = aclib.RunTapirCommand ('CreatePolylines', {
    'polylinesData': [{'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 3.0, 'y': 3.0}, {'x': 6.0, 'y': 0.0}]}]
})['elements']
worksheet = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{'name': 'Tapir Example', 'referenceId': 'TAPIR-EX'}]
})['databases'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CopyElementsToDatabase', {
    'elements': polylines,
    'targetDatabaseId': worksheet['databaseId'],
    'offset': {'x': 1.0, 'y': 0.0}
}))

aclib.RunTapirCommand ('DeleteElements', {'elements': polylines})
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': worksheet['navigatorItemId']}]})
