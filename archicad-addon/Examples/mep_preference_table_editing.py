import aclib

# Requires Archicad 28 or newer.

# Adds a cable carrier segment table, fills it, renames it, removes a row and deletes the table again.
response = aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
        'domain': 'CableCarrier',
        'tablesToAdd': [{'type': 'Segment', 'name': 'Tapir example'}]
    })
print (response)
addedTable = response['addedTables'][0]
if 'guid' in addedTable:
    tableGuid = addedTable['guid']

    response = aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
            'domain': 'CableCarrier',
            'tables': [{
                'guid': tableGuid,
                'name': 'Tapir example (renamed)',
                'newRows': [
                    {'value': 0.1, 'description': '100'},
                    {'value': 0.2, 'description': '200'},
                    {'value': 0.3, 'description': '300'}
                ]
            }]
        })
    print (response)

    tables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {'domain': 'CableCarrier'})['cableCarrierTables']
    table = next (table for table in tables if table['guid'] == tableGuid)
    print (table)

    # Change the description of the first row and delete the last one; the indices come from GetMEPPreferenceTables.
    response = aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
            'domain': 'CableCarrier',
            'tables': [{
                'guid': tableGuid,
                'rows': [
                    {'index': table['rows'][0]['index'], 'description': 'Small'},
                    {'index': table['rows'][-1]['index'], 'delete': True}
                ]
            }]
        })
    print (response)

    response = aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
            'domain': 'CableCarrier',
            'tablesToDelete': [tableGuid]
        })
    print (response)

# A row read by GetMEPPreferenceTables can be sent back as it is: the read-only values are ignored.
response = aclib.RunTapirCommand ('GetMEPPreferenceTables', {'domain': 'Piping'})
editableTables = [table for table in response['tables'] if table.get ('isEditable') and table['rows']]
if editableTables:
    table = editableTables[0]
    row = table['rows'][0]
    response = aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
            'domain': 'Piping',
            'tables': [{'guid': table['guid'], 'rows': [row]}]
        })
    print (response)
