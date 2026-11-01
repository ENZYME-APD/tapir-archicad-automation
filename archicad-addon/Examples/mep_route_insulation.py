import sys
import aclib

# Requires Archicad 28 or newer.

# A pipe route with one turn, then ModifyMEPRoutingElements insulates the whole route (its segments,
# elbow and transitions) and removes the insulation again. Archicad changes a route only when its
# layer is shown in the active window, so the example works in the floor plan.
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -8.0, 'z': 3.0},
                    {'x': 4.0, 'y': -8.0, 'z': 3.0},
                    {'x': 4.0, 'y': -5.0, 'z': 3.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or 'error' in created['elements'][0]:
    print ('Could not create the routing element, this example needs Archicad 28 or newer.')
    sys.exit ()
routeId = created['elements'][0]['elementId']

def SetInsulation (insulationThickness):
    result = aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
            'routingElementsData': [{'elementId': routeId, 'insulationThickness': insulationThickness}]
        })['executionResults'][0]
    print (f"Insulation {insulationThickness}: {'succeeded' if result['success'] else result['error']['message']}")
    routingElement = aclib.RunTapirCommand ('GetMEPRoutingElements', {
            'elements': [{'elementId': routeId}]
        })['routingElements'][0]
    for i, segment in enumerate (routingElement['segments']):
        print (f"Segment {i}: insulation {segment.get ('insulationThickness', 'none')}")

SetInsulation (0.02)
SetInsulation (0)

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': routeId}]
    })
