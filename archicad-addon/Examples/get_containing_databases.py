import aclib

walls = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Wall' })['elements']

# A guid that belongs to no element shows how a failed lookup is reported.
missingElement = { 'elementId': { 'guid': '00000000-0000-0000-0000-000000000001' } }

response = aclib.RunTapirCommand ('GetContainingDatabases', {
    'elements': walls[:2] + [missingElement]
})

# Each element's database can be opened with ChangeWindow, wherever the element is,
# for example a text placed on a layout while the floor plan was expected.
for database in response['databases']:
    if 'databaseId' in database:
        aclib.RunTapirCommand ('ChangeWindow', {
            'windowType': database['windowType'],
            'databaseId': database['databaseId']
        })
        break
