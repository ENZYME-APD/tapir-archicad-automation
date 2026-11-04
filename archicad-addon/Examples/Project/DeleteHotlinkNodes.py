"""Places a hotlink made from the first wall and deletes its node, with the instance, with DeleteHotlinkNodes.

The node's source is the first wall, saved as a module in the temp folder. Needs at least one Wall.
"""
import os
import tempfile
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
modulePath = os.path.join (tempfile.mkdtemp (), 'TapirExample.mod')
aclib.RunTapirCommand ('SaveAsModuleFile', {'moduleFilePath': modulePath, 'elements': walls})
nodeId = aclib.RunTapirCommand ('CreateHotlinkNodes', {'hotlinkNodes': [{'sourceLocation': modulePath}]})['hotlinkNodes'][0]['hotlinkNodeId']
aclib.RunTapirCommand ('CreateHotlinkInstances', {'hotlinkInstances': [{'hotlinkNodeId': nodeId, 'origin': {'x': 50.0, 'y': 0.0}}]})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteHotlinkNodes', {
    'hotlinkNodes': [{'hotlinkNodeId': nodeId}]
}))
