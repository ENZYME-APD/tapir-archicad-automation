import json
import aclib
import os
import sys

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

moveVector = {'x': 1.0, 'y': 1.0, 'z': 0.0}
elementsWithMoveVectors = [{'elementId': w['elementId'], 'moveVector': moveVector} for w in walls]

aclib.RunTapirCommand (
    'MoveElements', {
        'elementsWithMoveVectors': elementsWithMoveVectors
    })

rotationBeginPoint = {'x': 0.0, 'y': 1.0}
rotationEndPoint = {'x': 1.0, 'y': 0.0}
rotationOrigin = {'x': 0.0, 'y': 0.0}
elementsWithRotations = [{'elementId': w['elementId'], 'rotation': {'beginPoint':rotationBeginPoint, 'endPoint':rotationEndPoint, 'origin':rotationOrigin}} for w in walls]

aclib.RunTapirCommand (
    'RotateElements', {
        'elementsWithRotations': elementsWithRotations
    })

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls})