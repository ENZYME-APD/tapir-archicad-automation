import aclib

# CreateWorksheets returns the databaseId and the Project Map navigatorItemId of
# each new worksheet. 2D creation commands create in the current window, so the
# new worksheet is opened with ChangeWindow first: by its databaseId (which opens
# its Project Map item, as its window was never opened) or by its navigatorItemId.

createdWorksheets = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{ 'name': 'Tapir Example Worksheet', 'referenceId': 'TAPIR-WS-EX' }]
})['databases']
worksheet = createdWorksheets[0]
assert 'databaseId' in worksheet, 'Creating the worksheet failed: {}'.format (worksheet)
assert 'navigatorItemId' in worksheet, 'The new worksheet has no Project Map item: {}'.format (worksheet)

result = aclib.RunTapirCommand ('ChangeWindow', { 'databaseId': worksheet['databaseId'] })
assert result['success'], 'Opening the new worksheet failed: {}'.format (result)
print ('Current window: {}'.format (aclib.RunTapirCommand ('GetCurrentWindowType', debug = False)['currentWindowType']))

aclib.RunTapirCommand ('CreatePolylines', {
    'polylinesData': [
        {
            'coordinates': [
                { 'x': 0.0, 'y': 0.0 },
                { 'x': 3.0, 'y': 3.0 },
                { 'x': 6.0, 'y': 0.0 }
            ]
        }
    ]
})

polylines = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'PolyLine',
    'databases': [{ 'databaseId': worksheet['databaseId'] }]
})['elements']
print ('Polylines in the new worksheet: {}'.format (len (polylines)))

# Back to the floor plan, then remove the example worksheet again.
aclib.RunTapirCommand ('ChangeWindow', { 'windowType': 'FloorPlan' })
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{ 'navigatorItemId': worksheet['navigatorItemId'] }]
})
