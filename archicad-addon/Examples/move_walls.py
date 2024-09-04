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