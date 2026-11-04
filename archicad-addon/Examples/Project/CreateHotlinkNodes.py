"""Saves the first wall as a module in the temp folder, creates a hotlink node from it with CreateHotlinkNodes, then deletes the node.

Needs at least one Wall.
"""
import os
import tempfile
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
modulePath = os.path.join (tempfile.mkdtemp (), 'TapirExample.mod')
aclib.RunTapirCommand ('SaveAsModuleFile', {'moduleFilePath': modulePath, 'elements': walls})

node = aclib.RunTapirCommand ('CreateHotlinkNodes', {
    'hotlinkNodes': [
        {'sourceLocation': modulePath, 'name': 'Tapir Example', 'storyRangeType': 'AllStories'}
    ]
})['hotlinkNodes'][0]
aclib.PrintJson (node)

if 'hotlinkNodeId' in node:
    aclib.RunTapirCommand ('DeleteHotlinkNodes', {'hotlinkNodes': [{'hotlinkNodeId': node['hotlinkNodeId']}]})
