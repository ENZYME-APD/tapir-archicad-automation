"""Makes a pipe route circular and changes the elbow radius factor of its nodes, and sets the transition angle of a duct route, with SetMEPRoutingParameters.

Creates a temporary pipe route with one corner and a temporary duct route, changes them, reads the parameters back and deletes them.
The transition angle sets the length of the transitions Archicad makes later on that duct route (largest half size step x tan(angle));
a route keeps the angle it was created with otherwise. The elbow and transition parameters can be set from Archicad 29.
Requires Archicad 28 or later.
"""
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Piping',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 3.0}, {'x': 4.0, 'y': 0.0, 'z': 3.0}, {'x': 4.0, 'y': 3.0, 'z': 3.0}]
    }]
})['elements'][0]['elementId']
route = aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': routeId}]})['routingElements'][0]
nodeIds = [node['elementId'] for node in route['nodes']]

ductId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Ventilation',
        'crossSectionShape': 'Rectangular', 'crossSectionWidth': 0.3, 'crossSectionHeight': 0.2,
        'nodeCoordinates': [{'x': 0.0, 'y': -2.0, 'z': 3.0}, {'x': 4.0, 'y': -2.0, 'z': 3.0}]
    }]
})['elements'][0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('SetMEPRoutingParameters', {
    'elements': [{
        'elementId': routeId,
        'routingSegmentParameters': {'crossSectionShape': 'Circular', 'crossSectionWidth': 0.08}
    }] + [{
        'elementId': nodeId,
        'elbowParameters': {'factorRadius': 2.0}
    } for nodeId in nodeIds] + [{
        'elementId': ductId,
        'routingNodeParameters': {'transition': {'angle': 50.0}}
    }]
}))

aclib.RunTapirCommand ('GetMEPRoutingParameters', {'elements': [{'elementId': id} for id in [routeId, ductId] + nodeIds]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId}, {'elementId': ductId}]})
