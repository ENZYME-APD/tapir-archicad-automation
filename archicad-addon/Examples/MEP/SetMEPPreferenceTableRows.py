"""Changes the description of a row of an editable pipe segment preference table with SetMEPPreferenceTableRows.

Reads the row, changes it, reads it back and restores the old description.
Requires Archicad 28 or later (on Archicad 27 the key is a shape and an index).
"""
import aclib

tables = [t for t in aclib.RunTapirCommand ('GetMEPPreferenceTables', {'domain': 'Piping'})['tables']
          if t.get ('isEditable') and t['rows']]
if not tables:
    raise SystemExit ('No editable pipe segment preference table with rows in the project.')
tableId = {'guid': tables[0]['guid']}
key = {'referenceId': tables[0]['rows'][0]['referenceId']}
oldDescription = tables[0]['rows'][0].get ('description', '')

aclib.PrintJson (aclib.RunTapirCommand ('SetMEPPreferenceTableRows', {
    'tableId': tableId,
    'rows': [{'key': key, 'values': {'description': 'Changed by Tapir'}}]
}))
aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {'tableId': tableId, 'keys': [key]})

aclib.PrintJson (aclib.RunTapirCommand ('SetMEPPreferenceTableRows', {
    'tableId': tableId,
    'rows': [{'key': key, 'values': {'description': oldDescription}}]
}))
