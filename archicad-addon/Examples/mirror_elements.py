import json
import aclib
import os
import sys

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

# Mirror every wall across the Y axis (the line through (0,0) and (0,1)).
mirrorAxis = {'beginPoint': {'x': 0.0, 'y': 0.0}, 'endPoint': {'x': 0.0, 'y': 1.0}}
elementsWithMirrorAxes = [{'elementId': w['elementId'], 'mirrorAxis': mirrorAxis} for w in walls]

aclib.RunTapirCommand (
    'MirrorElements', {
        'elementsWithMirrorAxes': elementsWithMirrorAxes
    })

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls})

# Mirror them back as copies across the same axis, so the originals stay and
# the copies land where the walls started.
elementsWithMirrorAxes = [{'elementId': w['elementId'], 'mirrorAxis': mirrorAxis, 'copy': True} for w in walls]

aclib.RunTapirCommand (
    'MirrorElements', {
        'elementsWithMirrorAxes': elementsWithMirrorAxes
    })

# An axis whose two points coincide is rejected per element.
aclib.RunTapirCommand (
    'MirrorElements', {
        'elementsWithMirrorAxes': [{
            'elementId': walls[0]['elementId'],
            'mirrorAxis': {'beginPoint': {'x': 1.0, 'y': 1.0}, 'endPoint': {'x': 1.0, 'y': 1.0}}
        }]
    })
