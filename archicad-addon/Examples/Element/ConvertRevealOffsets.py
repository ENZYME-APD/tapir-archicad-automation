"""Converts a reveal depth offset of 0 measured from the wall's core skin to the wall side reference with ConvertRevealOffsets.

Needs at least one Window or Door placed in a wall. Requires Archicad 27 or later.
"""
import aclib

openings = []
for elementType in ['Window', 'Door']:
    openings += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']
if not openings:
    raise SystemExit ('No window or door in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('ConvertRevealOffsets', {
    'openings': [{'elementId': opening['elementId'], 'coreOffset': 0.0} for opening in openings[:3]]
}))
