"""Changes MEP ports with ModifyMEPPorts: the flow direction and outer diameter of a pipe port, and the
connection size (width and height) of an air terminal.

Creates a temporary pipe route and a ventilation terminal, modifies their ports, reads them back and
deletes them. Requires Archicad 28 or later.
"""
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Piping',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 1.0}, {'x': 4.0, 'y': 0.0, 'z': 1.0}]
    }]
})['elements'][0]['elementId']
terminalId = aclib.RunTapirCommand ('CreateMEPElements', {
    'elementsData': [{'type': 'Terminal', 'domain': 'Ventilation', 'position': {'x': 0.0, 'y': 2.0, 'z': 2.5}}]
})['elements'][0]['elementId']

ports = aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': routeId}, {'elementId': terminalId}]})['elementPorts']
pipePort = ports[0]['ports'][0]
terminalPort = ports[1]['ports'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPorts', {
    'portsData': [
        {
            'portId': pipePort['portId'],
            'flowDirection': 'FlowIn',
            'outerDiameter': 0.05
        },
        {
            'portId': terminalPort['portId'],
            'shape': 'Rectangular',
            'width': 0.4,
            'height': 0.25
        }
    ]
}))

aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': routeId}, {'elementId': terminalId}]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId}, {'elementId': terminalId}]})
