"""Changes the flow direction and outer diameter of a pipe port with ModifyMEPPorts.

Creates a temporary pipe route, modifies the port at its start, reads it back and deletes the route.
Requires Archicad 28 or later.
"""
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Piping',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 1.0}, {'x': 4.0, 'y': 0.0, 'z': 1.0}]
    }]
})['elements'][0]['elementId']
port = aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': routeId}]})['elementPorts'][0]['ports'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPorts', {
    'portsData': [{
        'portId': port['portId'],
        'flowDirection': 'FlowIn',
        'outerDiameter': 0.05
    }]
}))

aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': routeId}]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId}]})
