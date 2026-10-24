import os
import sys
import aclib

# Breaking and deleting hotlink nodes.
#
# This example lists the nodes of the open project with their source file,
# then shows the two ways of removing a node: BreakHotlinkNodes turns the
# elements of its placed instances into ordinary elements of the project,
# DeleteHotlinkNodes removes the node together with its instances.
#
# Set TAPIR_HOTLINK_SOURCE to the absolute path of a .mod or .pln to have the
# example create a node from it, place it, break it, then create and place it
# again and delete it. The broken instance's elements stay in the project.
# Without it nothing is broken or deleted, so the nodes of the open project
# are left as they are.

def CollectNodes (hotlinks):
    for hotlink in hotlinks:
        yield hotlink
        yield from CollectNodes (hotlink.get ('children', []))

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
for node in CollectNodes (hotlinks):
    print ('{}: {}'.format (node.get ('name', '?'), node.get ('location', 'no location')))

sourcePath = os.environ.get ('TAPIR_HOTLINK_SOURCE')
if not sourcePath:
    print ('Set TAPIR_HOTLINK_SOURCE to a .mod or .pln to see BreakHotlinkNodes and DeleteHotlinkNodes on a node created from it.')
else:
    def CreateAndPlaceNode ():
        nodes = aclib.RunTapirCommand ('CreateHotlinkNodes', {
            'hotlinkNodes': [{'sourceLocation': sourcePath}]
        })['hotlinkNodes']
        if 'hotlinkNodeId' not in nodes[0]:
            print ('Creating the node failed: {}'.format (nodes[0]))
            sys.exit (1)
        nodeId = nodes[0]['hotlinkNodeId']
        aclib.RunTapirCommand ('CreateHotlinkInstances', {
            'hotlinkInstances': [{
                'hotlinkNodeId': nodeId,
                'origin': {'x': 30.0, 'y': 0.0}
            }]
        })
        return nodeId

    # Break: the instance's elements stay as ordinary elements.
    nodeId = CreateAndPlaceNode ()
    aclib.RunTapirCommand ('BreakHotlinkNodes', {
        'hotlinkNodes': [{'hotlinkNodeId': nodeId}]
    })

    # Delete: the node and its instance go together.
    nodeId = CreateAndPlaceNode ()
    aclib.RunTapirCommand ('DeleteHotlinkNodes', {
        'hotlinkNodes': [{'hotlinkNodeId': nodeId}]
    })
