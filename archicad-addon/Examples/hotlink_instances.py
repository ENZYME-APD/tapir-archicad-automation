import os
import math
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

sourcePath = os.environ.get ('TAPIR_HOTLINK_SOURCE')
if sourcePath:
    aclib.RunTapirCommand ('CreateHotlinkNodes', {
        'hotlinkNodes': [{'sourceLocation': sourcePath}]
    })

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']

if len (hotlinks) == 0:
    print ('No hotlink nodes in this project - set TAPIR_HOTLINK_SOURCE to create one.')
else:
    nodeId = hotlinks[0]['hotlinkNodeId']

    created = aclib.RunTapirCommand ('CreateHotlinkInstances', {
        'hotlinkInstances': [{
            'hotlinkNodeId': nodeId,
            'origin': {'x': 10.0, 'y': 4.0},
            'rotationAngle': math.pi / 6.0
        }]
    })['elements']

    instances = [e for e in created if 'elementId' in e]

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
