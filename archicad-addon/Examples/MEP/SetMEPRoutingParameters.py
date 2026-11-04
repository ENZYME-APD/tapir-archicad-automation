"""Makes a pipe route circular and changes the elbow radius factor of its nodes with SetMEPRoutingParameters.

Creates a temporary pipe route with one corner, changes it, reads the parameters back and deletes it.
The elbow parameters can be set from Archicad 29. Requires Archicad 28 or later.
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

aclib.PrintJson (aclib.RunTapirCommand ('SetMEPRoutingParameters', {
    'elements': [{
        'elementId': routeId,
        'routingSegmentParameters': {'crossSectionShape': 'Circular', 'crossSectionWidth': 0.08}
    }] + [{
        'elementId': nodeId,
        'elbowParameters': {'factorRadius': 2.0}
    } for nodeId in nodeIds]
}))

aclib.RunTapirCommand ('GetMEPRoutingParameters', {'elements': [{'elementId': id} for id in [routeId] + nodeIds]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId}]})
