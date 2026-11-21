"""Removes one direction of the trim between two roofs that trim each other with RemoveElementTrimDirection.

The two overlapping roofs and their trims are created by the script; the roofs are deleted at the end.
"""
import math

import aclib

# Two overlapping hip roofs, created by this script and deleted at the end.
roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [
    {
        'level': 2.7, 'floorIndex': 0,
        'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (25.0)}],
        'polygonCoordinates': [{'x': 100.0, 'y': 100.0}, {'x': 106.0, 'y': 100.0}, {'x': 106.0, 'y': 105.0}, {'x': 100.0, 'y': 105.0}]
    },
    {
        'level': 2.7, 'floorIndex': 0,
        'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (25.0)}],
        'polygonCoordinates': [{'x': 103.0, 'y': 102.0}, {'x': 109.0, 'y': 102.0}, {'x': 109.0, 'y': 107.0}, {'x': 103.0, 'y': 107.0}]
    }
]})['elements']
roofA, roofB = roofs[0]['elementId'], roofs[1]['elementId']

# The roofs trim each other: A is trimmed by B, and B by A.
aclib.RunTapirCommand ('TrimElements', {'elements': [{'elementId': roofA}], 'trimmingElement': roofB, 'trimType': 'KeepInside'})
aclib.RunTapirCommand ('TrimElements', {'elements': [{'elementId': roofB}], 'trimmingElement': roofA, 'trimType': 'KeepOutside'})

# Only the trim of A by B goes; B stays trimmed by A, with its KeepOutside trim type.
aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementTrimDirection', {
    'elementPairs': [{'elementId': roofA, 'trimmingElementId': roofB}]
}))
aclib.PrintJson (aclib.RunTapirCommand ('GetElementTrims', {'elements': roofs}))

aclib.RunTapirCommand ('DeleteElements', {'elements': roofs})
