"""Adds a node to a pipe route, then leaves out a node where it goes straight on, with RebuildMEPRoutingElements.

Creates a temporary pipe route, rebuilds it twice, reads it back and deletes it. Requires Archicad 28 or later.

Every segment, node and fitting gets a new guid (see guidMap in the response); labels, dimensions and other
things attached to them are not carried over. Use ModifyMEPRoutingElements nodePositions instead when the node
count stays the same: it keeps every guid. The route keeps its guid when Archicad allows it (else the response
gives the new one), and each new segment keeps the size and settings of the old segment it lies along. Not kept:
the element IDs, classifications, property values and own renovation status of single parts, take-offs on the
route's segments, and the route's design option; the item's warnings list anything else not carried over.
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
