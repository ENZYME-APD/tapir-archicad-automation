"""Places a take-off 2 m from the start of a new pipe route with PlaceMEPTakeOffs.

Creates the pipe route first; the take-off uses the current settings of the Take-Off tool. Requires Archicad 28 or later.
"""
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Piping',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 1.0}, {'x': 6.0, 'y': 0.0, 'z': 1.0}]
    }]
})['elements'][0]['elementId']
route = aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': routeId}]})['routingElements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('PlaceMEPTakeOffs', {
    'takeOffsData': [{
        'domain': 'Piping',
        'segmentId': route['segments'][0]['elementId'],
        'distanceFromBeginning': 2.0,
        'rotationAroundMainAxis': 0.0,
        'length': 0.3
    }]
}))
