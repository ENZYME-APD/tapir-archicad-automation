import aclib

# The trace reference (ghost) of the floor plan and of every section is deleted.
# A database without a trace reference gets a failed execution result.
# Deleting a trace record cannot be undone.
walls = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Wall' })['elements']
floorPlanDatabases = [
    { 'databaseId': d['databaseId'] }
    for d in aclib.RunTapirCommand ('GetContainingDatabases', { 'elements': walls[:1] })['databases']
    if 'databaseId' in d
]

sectionDatabases = [
    { 'databaseId': d['databaseId'] }
    for d in aclib.RunTapirCommand ('GetDatabases', { 'types': ['Section'] })['databases']
]

response = aclib.RunTapirCommand ('DeleteGhostRecords', {
    'databases': floorPlanDatabases + sectionDatabases
})

for database, result in zip (floorPlanDatabases + sectionDatabases, response['executionResults']):
    print (database['databaseId']['guid'], result)
