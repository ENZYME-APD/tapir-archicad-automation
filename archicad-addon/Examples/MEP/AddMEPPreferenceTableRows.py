"""Adds a row to the first editable pipe elbow preference table with AddMEPPreferenceTableRows.

The new row gets the reference id of the first row and an angle of the angle set that this reference id has no row
for yet; it is deleted again at the end. Elbow rows can be added from Archicad 29. Requires Archicad 29 or later.
"""
import aclib

preferences = aclib.RunTapirCommand ('GetMEPPreferenceTables', {'domain': 'Piping', 'includeFittingTables': True})
elbowTables = [t for t in preferences.get ('elbowTables', []) if t.get ('isEditable') and t['rows']]
if not elbowTables:
    raise SystemExit ('No editable pipe elbow preference table with rows in the project.')
table = elbowTables[0]
referenceId = table['rows'][0]['referenceId']
usedAngles = [row.get ('angle') for row in table['rows'] if row['referenceId'] == referenceId]
freeAngles = [angle for angle in preferences['angleSet']['angles'] if angle not in usedAngles]
if not freeAngles:
    raise SystemExit ('Every angle of the angle set has a row already.')
key = {'referenceId': referenceId, 'angle': freeAngles[0]}

aclib.PrintJson (aclib.RunTapirCommand ('AddMEPPreferenceTableRows', {
    'tableId': {'guid': table['guid']},
    'rows': [{'key': key, 'values': {'description': 'Added by Tapir'}}]
}))

aclib.RunTapirCommand ('DeleteMEPPreferenceTableRows', {'tableId': {'guid': table['guid']}, 'keys': [key]})
