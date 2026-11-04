"""Adds a cable carrier segment table, renames it and fills it with rows using ModifyMEPPreferenceTables, then deletes it.

Requires Archicad 28 or later.
"""
import aclib

tableGuid = aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
    'domain': 'CableCarrier',
    'tablesToAdd': [{'type': 'Segment', 'name': 'Tapir example'}]
})['addedTables'][0]['guid']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
    'domain': 'CableCarrier',
    'tables': [{
        'guid': tableGuid,
        'name': 'Tapir example (renamed)',
        'newRows': [
            {'value': 0.1, 'description': '100'},
            {'value': 0.2, 'description': '200'}
        ]
    }]
}))

tables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {'domain': 'CableCarrier'})['cableCarrierTables']
print (next (table for table in tables if table['guid'] == tableGuid))

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPreferenceTables', {
    'domain': 'CableCarrier',
    'tablesToDelete': [tableGuid]
}))
