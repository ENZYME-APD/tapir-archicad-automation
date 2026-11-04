"""Prints the type-specific details of the segments, elbows, branches and transitions of the project with GetMEPElementDetails.

Needs at least one MEP route. Requires Archicad 28 or later.
"""
import aclib

mepElements = aclib.RunTapirCommand ('GetMEPElements', {
    'elementTypes': ['RigidSegment', 'FlexibleSegment', 'Elbow', 'Branch', 'Transition']
})['elements']
if not mepElements:
    raise SystemExit ('No MEP segment, elbow, branch or transition in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPElementDetails', {
    'elements': [{'elementId': element['elementId']} for element in mepElements[:5]]
}))
