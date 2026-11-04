import aclib

# Creates a worksheet and draws a polyline and a text in it from the floor plan, with the
# databaseId input of the 2D creation commands: the front window stays the floor plan.
# Then removes the example worksheet again.

aclib.RunTapirCommand ('ChangeWindow', { 'windowType': 'FloorPlan' })

createdWorksheets = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{ 'name': 'Tapir Database Id Example', 'referenceId': 'TAPIR-WS-DB' }]
})['databases']
worksheet = createdWorksheets[0]
assert 'databaseId' in worksheet, 'Creating the worksheet failed: {}'.format (worksheet)

aclib.PrintJson (aclib.RunTapirCommand ('CreatePolylines', {
    'databaseId': worksheet['databaseId'],
    'polylinesData': [{
        'coordinates': [
            { 'x': 0.0, 'y': 0.0 },
            { 'x': 3.0, 'y': 3.0 },
            { 'x': 6.0, 'y': 0.0 }
        ]
    }]
}))
aclib.PrintJson (aclib.RunTapirCommand ('CreateTexts', {
    'databaseId': worksheet['databaseId'],
    'textsData': [{ 'text': 'Drawn from the floor plan', 'coordinate': { 'x': 0.0, 'y': -1.0, 'z': 0.0 } }]
}))

print ('Current window: {}'.format (aclib.RunTapirCommand ('GetCurrentWindowType')['currentWindowType']))
for elementType in ['PolyLine', 'Text']:
    elements = aclib.RunTapirCommand ('GetElementsByType', {
        'elementType': elementType,
        'databases': [{ 'databaseId': worksheet['databaseId'] }]
    })['elements']
    print ('{} elements in the new worksheet: {}'.format (elementType, len (elements)))

if 'navigatorItemId' in worksheet:
    aclib.RunTapirCommand ('DeleteNavigatorItems', {
        'navigatorItemIds': [{ 'navigatorItemId': worksheet['navigatorItemId'] }]
    })
