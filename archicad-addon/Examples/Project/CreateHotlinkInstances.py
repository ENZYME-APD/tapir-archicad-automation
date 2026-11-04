"""Places an instance of a hotlink node at (50, 0), rotated by 30 degrees, with CreateHotlinkInstances.

The node is made from the first wall, saved as a module in the temp folder; at the end the node is
deleted with its instance. Needs at least one Wall.
"""
import math
import os
import tempfile
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
modulePath = os.path.join (tempfile.mkdtemp (), 'TapirExample.mod')
aclib.RunTapirCommand ('SaveAsModuleFile', {'moduleFilePath': modulePath, 'elements': walls})
nodeId = aclib.RunTapirCommand ('CreateHotlinkNodes', {'hotlinkNodes': [{'sourceLocation': modulePath}]})['hotlinkNodes'][0]['hotlinkNodeId']

aclib.PrintJson (aclib.RunTapirCommand ('CreateHotlinkInstances', {
    'hotlinkInstances': [{
        'hotlinkNodeId': nodeId,
        'origin': {'x': 50.0, 'y': 0.0},
        'rotationAngle': math.radians (30),
        'mirrored': False
    }]
}))

aclib.RunTapirCommand ('DeleteHotlinkNodes', {'hotlinkNodes': [{'hotlinkNodeId': nodeId}]})
