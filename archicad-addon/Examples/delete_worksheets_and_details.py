import aclib

# Independent worksheets and details are databases of the Project Map, like
# layouts, and DeleteNavigatorItems deletes them through the database API. A
# script can so clean up the worksheets it created itself, e.g. temporary
# worksheets for 2D checks or imported DWG worksheets. Given a View Map view of
# a worksheet instead, the command still deletes only the view.

worksheetName = 'Tapir Temporary Worksheet'
detailName = 'Tapir Temporary Detail'

createdWorksheets = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{ 'name': worksheetName, 'referenceId': 'TAPIR-WS' }]
})['databases']
createdDetails = aclib.RunTapirCommand ('CreateDetails', {
    'detailsData': [{ 'name': detailName, 'referenceId': 'TAPIR-DET' }]
})['databases']

createdDatabaseGuids = set ()
for database in createdWorksheets + createdDetails:
    assert 'databaseId' in database, 'Creating the worksheet or detail failed: {}'.format (database)
    createdDatabaseGuids.add (database['databaseId']['guid'])

def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems

def GetProjectMapWorksheetsAndDetails ():
    projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {
        'navigatorMapId': 'ProjectMap'
    }, debug = False)['navigatorItemTree']
    return [
        item for item in CollectItems (projectMap, [])
        if item['type'] in ('WorksheetDrawingItem', 'DetailDrawingItem')
    ]

# CreateWorksheets and CreateDetails return database ids; the navigator items
# of the new databases are the Project Map worksheets and details reporting
# those ids.
def FindProjectMapItemsOfDatabases (databaseGuids):
    candidates = GetProjectMapWorksheetsAndDetails ()
    databases = aclib.RunTapirCommand ('GetDatabaseIdFromNavigatorItemId', {
        'navigatorItemIds': [{ 'navigatorItemId': item['navigatorItemId'] } for item in candidates]
    }, debug = False)['databases']
    return [
        item for item, database in zip (candidates, databases)
        if 'databaseId' in database and database['databaseId']['guid'] in databaseGuids
    ]

itemsToDelete = FindProjectMapItemsOfDatabases (createdDatabaseGuids)
assert len (itemsToDelete) == len (createdDatabaseGuids), 'The created worksheet and detail were not found in the Project Map'
print ('Deleting from the Project Map: {}'.format (', '.join (item['name'] for item in itemsToDelete)))

executionResults = aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{ 'navigatorItemId': item['navigatorItemId'] } for item in itemsToDelete]
})['executionResults']
assert all (result['success'] for result in executionResults), 'Deleting failed: {}'.format (executionResults)

assert not FindProjectMapItemsOfDatabases (createdDatabaseGuids), 'The deleted items are still in the Project Map'
print ('The worksheet and the detail are gone from the Project Map')
