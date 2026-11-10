"""Adds a node to a pipe route, then leaves out a node where it goes straight on, with RebuildMEPRoutingElements.

Creates a temporary pipe route, rebuilds it twice, reads it back and deletes it. Requires Archicad 28 or later.
"""
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Piping',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 4.0, 'y': 0.0, 'z': 2.5}]
    }]
})['elements'][0]['elementId']

# Adds a bend point at the end: the route goes on 3 m to the north.
routeId = aclib.RunTapirCommand ('RebuildMEPRoutingElements', {
    'routingElements': [{
        'routingElementId': routeId,
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 4.0, 'y': 0.0, 'z': 2.5}, {'x': 4.0, 'y': 3.0, 'z': 2.5}]
    }]
})['elements'][0]['elementId']

# The node at x 2 lies on the straight line from x 0 to x 4, so it is left out.
response = aclib.RunTapirCommand ('RebuildMEPRoutingElements', {
    'routingElements': [{
        'routingElementId': routeId,
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 2.0, 'y': 0.0, 'z': 2.5}, {'x': 4.0, 'y': 0.0, 'z': 2.5}, {'x': 4.0, 'y': 3.0, 'z': 2.5}],
        'removeCollinearNodes': True
    }]
})
aclib.PrintJson (response)
routeId = response['elements'][0]['elementId']

aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': routeId}]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId}]})
