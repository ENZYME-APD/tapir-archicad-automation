import aclib

# Demo: Server-Side Query Projection and Filtering in Tapir
# Instead of downloading all GUIDs via GetAllElements and then downloading massive JSON
# blobs via GetDetailsOfElements, we can now execute a projected, filtered query in ONE call:

response = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'filter': {
        'types': ['Wall'],
        'floorIndices': [0]
    },
    'fields': ['elementId', 'id', 'type', 'floorIndex', 'layerIndex']
})

details = response.get ('detailsOfElements', [])
print (f'Retrieved {len (details)} elements matching filter server-side.')

for elem in details[:5]:
    guid = elem.get ('elementId', {}).get ('guid', 'N/A')
    elem_id = elem.get ('id', 'Untitled')
    elem_type = elem.get ('type', 'Unknown')
    floor = elem.get ('floorIndex', 0)
    print (f' - [{elem_type}] ID: {elem_id} | GUID: {guid} | Story: {floor}')
