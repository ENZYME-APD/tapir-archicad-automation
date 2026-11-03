import sys
import aclib

# Requires Archicad 28 or newer; the floor plan display settings require Archicad 29.

# A duct route with one turn, then ModifyMEPRoutingElements changes the connection style of every
# segment, the preferred transition placement of every node and the floor plan display settings
# of the routing element.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Ventilation',
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

def PrintSettings (title):
    routingElement = aclib.RunTapirCommand ('GetMEPRoutingElements', {
            'elements': [{'elementId': routeId}]
        })['routingElements'][0]
    print (title)
    for key in ['cutPlanePosition', 'floorPlanDisplayOption', 'showProjection', 'onStoryVisibility']:
        print (f"  {key}: {routingElement.get (key, 'not reported (Archicad 28)')}")
    for i, segment in enumerate (routingElement['segments']):
        print (f"  Segment {i} connection style: {segment.get ('connectionStyle', 'not reported')}")
    for i, node in enumerate (routingElement['nodes']):
        print (f"  Node {i} preferred transition placement: {node.get ('preferredTransitionPlacement', 'not reported')}")

PrintSettings ('Before:')

result = aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
        'routingElementsData': [
            {
                'elementId': routeId,
                'connectionStyle': 'Flanged',
                'preferredTransitionPlacement': 'LargeToSmall'
            }
        ]
    })['executionResults'][0]
print (f"Modify connection style and transition placement: {'succeeded' if result['success'] else result['error']['message']}")

result = aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
        'routingElementsData': [
            {
                'elementId': routeId,
                'floorPlanDisplayOption': 'Symbolic',
                'cutPlanePosition': 'ByViewSettings',
                'onStoryVisibility': 'HomeStoryOnly'
            }
        ]
    })['executionResults'][0]
print (f"Modify floor plan display settings: {'succeeded' if result['success'] else result['error']['message']}")

PrintSettings ('After:')

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': routeId}]
    })
