"""Creates a take-off 2 m from the start of a new duct route with CreateMEPTakeOffs.

Creates the duct route first; the take-off uses the current settings of the Take-Off tool of the route's domain
(no 'domain' given), faces up (a quarter turn) and has no insulation. Archicad only has Ventilation take-offs.
Requires Archicad 28 or later.
"""
import math
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Ventilation',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 3.0}, {'x': 6.0, 'y': 0.0, 'z': 3.0}]
    }]
})['elements'][0]['elementId']
route = aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': routeId}]})['routingElements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPTakeOffs', {
    'takeOffsData': [{
        'segmentId': route['segments'][0]['elementId'],
        'distanceFromBeginning': 2.0,
        'rotationAroundMainAxis': math.pi / 2,
        'length': 0.3,
        'defaults': {'removeInsulation': True}
    }]
}))
