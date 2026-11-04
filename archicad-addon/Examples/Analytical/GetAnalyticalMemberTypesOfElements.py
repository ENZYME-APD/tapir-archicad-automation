"""Prints the analytical member type of the first walls, beams and columns of the project with GetAnalyticalMemberTypesOfElements.

Needs at least one wall, beam or column.
"""
import aclib

elements = []
for elementType in ['Wall', 'Beam', 'Column']:
    elements += aclib.RunCommand ('API.GetElementsByType', {'elementType': elementType})['elements'][:2]
if not elements:
    raise SystemExit ('No wall, beam or column in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalMemberTypesOfElements', {
    'elements': elements
}))
