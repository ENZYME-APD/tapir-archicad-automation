import sys
import json
import aclib

# Requires Archicad 28 or newer; the routing node, elbow and transition parameters can be set from Archicad 29.

# A pipe route with one turn. GetMEPRoutingParameters reads the routing parameters of the route, of its
# nodes and of its segments, then SetMEPRoutingParameters changes the routing segment parameters of the
# route and the elbow radius factor of its nodes.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -12.0, 'z': 3.0},
                    {'x': 4.0, 'y': -12.0, 'z': 3.0},
                    {'x': 4.0, 'y': -9.0, 'z': 3.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or 'error' in created['elements'][0]:
    print ('Could not create the routing element, this example needs Archicad 28 or newer.')
    sys.exit ()
routeId = created['elements'][0]['elementId']

routingElement = aclib.RunTapirCommand ('GetMEPRoutingElements', {
        'elements': [{'elementId': routeId}]
    })['routingElements'][0]
nodeIds = [node['elementId'] for node in routingElement['nodes']]
segmentIds = [segment['elementId'] for segment in routingElement['segments']]

def PrintRoutingParameters (title):
    parameters = aclib.RunTapirCommand ('GetMEPRoutingParameters', {
            'elements': [{'elementId': id} for id in [routeId] + nodeIds + segmentIds]
        })['routingParameters']
    print (title)
    print (json.dumps (parameters, indent = 4))

PrintRoutingParameters ('Before:')

modifications = [
    {
        'elementId': routeId,
        'routingSegmentParameters': {
            'crossSectionShape': 'Circular',
            'crossSectionWidth': 0.08
        }
    }
]
modifications += [
    {
        'elementId': nodeId,
        'elbowParameters': {
            'factorRadius': 2.0
        }
    } for nodeId in nodeIds
]
results = aclib.RunTapirCommand ('SetMEPRoutingParameters', {
        'elements': modifications
    })['executionResults']
for modification, result in zip (modifications, results):
    print (f"Set on {modification['elementId']['guid']}: {'succeeded' if result['success'] else result['error']['message']}")

PrintRoutingParameters ('After:')

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': routeId}]
    })
