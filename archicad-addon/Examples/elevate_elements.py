import json
import aclib
import os
import sys

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

# Elevate every wall by 1 m (a negative value would move them down), like Edit > Move > Elevate.
elementsWithElevations = [{'elementId': w['elementId'], 'elevation': 1.0} for w in walls]

aclib.RunTapirCommand (
    'ElevateElements', {
        'elementsWithElevations': elementsWithElevations
    })

# Elevate a copy of the first wall by another 0.5 m; the original stays where it is.
if walls:
    aclib.RunTapirCommand (
        'ElevateElements', {
            'elementsWithElevations': [{'elementId': walls[0]['elementId'], 'elevation': 0.5, 'copy': True}]
        })

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls})
