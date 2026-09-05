import os
import math
import sys
import aclib

# Hotlink nodes and instances.
#
# A hotlink module is a NODE (the reference to the source file) plus any
# number of placed INSTANCES. This example lists the nodes, places an instance
# of the first one, reads its placement back, moves and mirrors it with
# ChangeHotlinkInstances, then deletes the instance again.
#
# Set TAPIR_HOTLINK_SOURCE to the absolute path of a .mod or .pln to have the
# example create a node first; without it the example works with whatever
# nodes the open project already has, and does nothing if there are none.

# GetHotlinks walks the tree of PLACED nodes, so a node that has just been
# created and not placed yet is not in it; the id CreateHotlinkNodes returns is
# what the first placement needs.
nodeId = None
sourcePath = os.environ.get ('TAPIR_HOTLINK_SOURCE')
if sourcePath:
    nodes = aclib.RunTapirCommand ('CreateHotlinkNodes', {
        'hotlinkNodes': [{'sourceLocation': sourcePath}]
    })['hotlinkNodes']
    nodeId = next ((n['hotlinkNodeId'] for n in nodes if 'hotlinkNodeId' in n), None)

if nodeId is None:
    hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
    if len (hotlinks) > 0:
        nodeId = hotlinks[0]['hotlinkNodeId']

if nodeId is None:
    print ('No hotlink nodes in this project - set TAPIR_HOTLINK_SOURCE to create one.')
else:

    created = aclib.RunTapirCommand ('CreateHotlinkInstances', {
        'hotlinkInstances': [{
            'hotlinkNodeId': nodeId,
            'origin': {'x': 10.0, 'y': 4.0},
            'rotationAngle': math.pi / 6.0
        }]
    })['elements']

    instances = [e for e in created if 'elementId' in e]
    if not instances:
        print ('Placement failed: {}'.format (created))
        sys.exit (1)

    aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': instances})

    aclib.RunTapirCommand ('ChangeHotlinkInstances', {
        'hotlinkInstances': [{
            'elementId': instances[0]['elementId'],
            'origin': {'x': 20.0, 'y': 8.0},
            'rotationAngle': math.pi / 3.0,
            'mirrored': True
        }]
    })

    aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': instances})

    aclib.RunTapirCommand ('DeleteElements', {'elements': instances})
