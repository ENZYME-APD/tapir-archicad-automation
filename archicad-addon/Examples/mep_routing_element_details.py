import sys
import aclib

# Requires Archicad 28 or newer.

# A duct route with one turn, then its structure as GetMEPRoutingElements reports it: the segments
# with their begin and end nodes and rigid segments, and the nodes with their incoming and outgoing
# segments, elbows and transitions.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': 0.0, 'z': 3.0},
                    {'x': 4.0, 'y': 0.0, 'z': 3.0},
                    {'x': 4.0, 'y': 3.0, 'z': 3.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or 'error' in created['elements'][0]:
    print ('Could not create the routing element, this example needs Archicad 28 or newer.')
    sys.exit ()

routingElement = aclib.RunTapirCommand ('GetMEPRoutingElements', {
        'elements': created['elements']
    })['routingElements'][0]

print (f"Offset from home story: {routingElement['offsetFromHomeStory']}")
print (f"Branch preference table: {routingElement.get ('branchPreferenceTableId')}")

nodeNumber = {node['elementId']['guid']: i for i, node in enumerate (routingElement['nodes'])}
for i, segment in enumerate (routingElement['segments']):
    print (f"Segment {i}: node {nodeNumber.get (segment['beginNodeId']['guid'])} -> node {nodeNumber.get (segment['endNodeId']['guid'])}, "
           f"anchor {segment['anchor']}, wall thickness {segment['wallThickness']}, "
           f"{len (segment['rigidSegments'])} rigid segment(s)")
for i, node in enumerate (routingElement['nodes']):
    print (f"Node {i}: incoming {'yes' if 'incomingSegmentId' in node else 'no'}, outgoing {'yes' if 'outgoingSegmentId' in node else 'no'}, "
           f"{len (node['elbows'])} elbow(s), {len (node['transitions'])} transition(s)")
