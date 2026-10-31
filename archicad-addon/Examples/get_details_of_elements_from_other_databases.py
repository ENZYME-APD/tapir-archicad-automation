import aclib

# Elements of the worksheets and details, read while another window (the floor plan) is current.
databases = [
    { 'databaseId': d['databaseId'] }
    for d in aclib.RunTapirCommand ('GetDatabases', { 'types': ['Worksheet', 'Detail'] })['databases']
]

elements = []
if databases:
    elements = aclib.RunTapirCommand ('GetElementsByType', {
        'elementType': 'Line',
        'databases': databases
    })['elements']

# An element that is not in the current database returns only its header fields
# (type, floorIndex, layerIndex, drawIndex) and "fromOtherDatabase": true.
details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': elements[:5]
})['detailsOfElements']

for detail in details:
    print (detail)
