import json
import aclib
import os
import sys

response = aclib.RunTapirCommand ('GetSelectedElements', {})

deltaX = 1.0
deltaY = 1.0
deltaZ = 0.0

elementsWithMoveVectors = [{'elementId': {'guid': str (e['elementId']['guid'])}, 'moveVector': {'x': deltaX, 'y': deltaY, 'z': deltaZ}} for e in response['elements']]

response = aclib.RunTapirCommand ('MoveElements', { 'elementsWithMoveVectors': elementsWithMoveVectors })