import aclib

# Archicad 27 only. Archicad 27 has one kind of preference table: for each shape (Circular,
# Rectangular) a list of values with a description. Its rows are found by shape and index.
listed = aclib.RunTapirCommand ('GetMEPPreferenceTableContainers', {
        'domains': ['Piping', 'Ventilation', 'CableCarrier']
    })
for container in listed.get ('containers', []):
    if 'error' in container:
        print (f"Container: {container['error']}")
        continue
    print (f"{container['domain']}: exists {container['exists']}")
    for table in container['tables']:
        print (f"  {table['name']}: exists {table['exists']}, rows {table['sizes']}")

# Add a table to the Piping container (it stays in the project), give it a row, read, change and
# delete the row.
created = []
if 'containers' in listed:
    created = aclib.RunTapirCommand ('CreateMEPPreferenceTables', {
            'domain': 'Piping',
            'tables': [{'name': 'Tapir example'}]
        }).get ('executionResults', [])
    print (f"Create: {created}")

if created and created[0]['success']:
    tableId = created[0]['tableId']

    added = aclib.RunTapirCommand ('AddMEPPreferenceTableRows', {
            'tableId': tableId,
            'rows': [{'key': {'shape': 'Circular'}, 'values': {'value': 0.05, 'description': 'Added by Tapir'}}]
        })['executionResults']
    print (f"Add: {added}")

    if added[0]['success']:
        key = {'shape': 'Circular', 'index': added[0]['index']}

        result = aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {
                'tableId': tableId,
                'keys': [key]
            })
        print (f"Table {result['name']}, rows {result['sizes']}, row {key}: {result['rows'][0]}")

        # Setting the value returns the index the row has after it.
        changed = aclib.RunTapirCommand ('SetMEPPreferenceTableRows', {
                'tableId': tableId,
                'rows': [{'key': key, 'values': {'value': 0.063, 'description': 'Changed by Tapir'}}]
            })['executionResults']
        print (f"Set: {changed}")

        if changed[0]['success']:
            key['index'] = changed[0]['index']
        deleted = aclib.RunTapirCommand ('DeleteMEPPreferenceTableRows', {
                'tableId': tableId,
                'keys': [key]
            })
        print (f"Delete: {deleted['executionResults']}")
