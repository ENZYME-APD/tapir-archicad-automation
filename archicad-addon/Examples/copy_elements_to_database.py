import aclib

# Two new worksheets: a polyline and a text are drawn into the first one and then copied into
# the second one, moved by 10 m. CopyElementsToDatabase opens a worksheet whose window was never
# opened once, so the new worksheets need no ChangeWindow before the copy.
worksheets = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [
        {'name': 'Tapir Example Source', 'referenceId': 'TAPIR-SRC'},
        {'name': 'Tapir Example Target', 'referenceId': 'TAPIR-TGT'}
    ]
})['databases']
source, target = worksheets

aclib.RunTapirCommand ('ChangeWindow', {'databaseId': source['databaseId']})
polylines = aclib.RunTapirCommand ('CreatePolylines', {
    'polylinesData': [{'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 3.0, 'y': 3.0}, {'x': 6.0, 'y': 0.0}]}]
})['elements']
texts = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [{'coordinate': {'x': 0.0, 'y': 5.0, 'z': 0.0}, 'text': 'Copied', 'height': 2.5, 'angle': 0.0, 'justification': 'Left'}]
})['elements']
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

copies = aclib.RunTapirCommand ('CopyElementsToDatabase', {
    'elements': polylines + texts,
    'sourceDatabaseId': source['databaseId'],
    'targetDatabaseId': target['databaseId'],
    'offset': {'x': 10.0, 'y': 0.0}
})['elements']

for elementType in ('PolyLine', 'Text'):
    inTarget = aclib.RunTapirCommand ('GetElementsByType', {
        'elementType': elementType,
        'databases': [{'databaseId': target['databaseId']}]
    })['elements']
    print ('{} in the target worksheet: {}'.format (elementType, len (inTarget)))

aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': worksheet['navigatorItemId']} for worksheet in worksheets]
})
