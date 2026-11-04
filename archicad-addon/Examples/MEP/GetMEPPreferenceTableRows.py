"""Reads the first two rows of the first pipe segment preference table by their reference id with GetMEPPreferenceTableRows.

Requires Archicad 28 or later (on Archicad 27 the keys are a shape and an index).
"""
import aclib

tables = [t for t in aclib.RunTapirCommand ('GetMEPPreferenceTables', {'domain': 'Piping'})['tables'] if t['rows']]
if not tables:
    raise SystemExit ('No pipe segment preference table with rows in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {
    'tableId': {'guid': tables[0]['guid']},
    'keys': [{'referenceId': row['referenceId']} for row in tables[0]['rows'][:2]]
}))
