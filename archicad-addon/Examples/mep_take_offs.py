import aclib
import math

# Requires Archicad 28 or newer. Archicad only has Ventilation Take-offs.

# Place a straight duct route with one segment, 10 meters long.
createdRoutes = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': -12.0, 'z': 3.0 },
                    { 'x': 10.0, 'y': -12.0, 'z': 3.0 }
                ]
            }
        ]
    })['elements']

routeIds = [e['elementId'] for e in createdRoutes if 'elementId' in e]

createdTakeOffs = []
if routeIds:
    routingElements = aclib.RunTapirCommand ('GetMEPRoutingElements', {
            'elements': [{ 'elementId': routeIds[0] }]
        })['routingElements']
    segmentId = routingElements[0]['segments'][0]['elementId']

    # Place three Take-offs on the segment: facing left (the default), up and right
    # relative to the segment's direction. The last one is 0.3 m long and has no insulation.
    createdTakeOffs = aclib.RunTapirCommand ('CreateMEPTakeOffs', {
            'takeOffsData': [
                { 'segmentId': segmentId, 'distance': 2.0 },
                { 'segmentId': segmentId, 'distance': 5.0, 'rotationAngle': math.pi / 2 },
                {
                    'segmentId': segmentId,
                    'distance': 8.0,
                    'rotationAngle': math.pi,
                    'length': 0.3,
                    'defaults': { 'removeInsulation': True }
                }
            ]
        })['elements']

    # The ports of the Take-offs, to connect a terminal or a branch route to them.
    takeOffIds = [e['elementId'] for e in createdTakeOffs if 'elementId' in e]
    if takeOffIds:
        aclib.RunTapirCommand ('GetMEPPorts', {
                'elements': [{ 'elementId': elementId } for elementId in takeOffIds]
            })

# Clean up
createdIds = [e['elementId'] for e in createdTakeOffs + createdRoutes if 'elementId' in e]
if createdIds:
    aclib.RunTapirCommand ('DeleteElements', {
            'elements': [{ 'elementId': elementId } for elementId in createdIds]
        })
