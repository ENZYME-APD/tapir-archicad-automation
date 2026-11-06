"""Prints the physical systems the first MEP elements of the project belong to with GetMEPPhysicalSystemsOfElements.

Needs at least one MEP element. Requires Archicad 29 or later.
"""
import aclib

mepElements = aclib.RunTapirCommand ('GetMEPElements', {})['elements']
if not mepElements:
    raise SystemExit ('No MEP element in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPPhysicalSystemsOfElements', {
    'elements': [{'elementId': element['elementId']} for element in mepElements[:5]]
}))
