"""Changes MEP ports with ModifyMEPPorts: the flow direction and outer diameter of a piping terminal's port, and
the connection size (width and height) of an air terminal.

Creates a temporary piping terminal and a ventilation terminal, modifies their ports, reads them back and
deletes them. GetMEPPorts reads the ports of MEP elements such as terminals, not of routes. Requires Archicad 28
or later.
"""
import aclib

pipeTerminalId, airTerminalId = [element['elementId'] for element in aclib.RunTapirCommand ('CreateMEPElements', {
    'elementsData': [
        {'type': 'Terminal', 'domain': 'Piping', 'position': {'x': 0.0, 'y': 0.0, 'z': 1.0}},
        {'type': 'Terminal', 'domain': 'Ventilation', 'position': {'x': 0.0, 'y': 2.0, 'z': 2.5}}
    ]
})['elements']]

ports = aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': pipeTerminalId}, {'elementId': airTerminalId}]})['elementPorts']
pipePort = ports[0]['ports'][0]
airPort = ports[1]['ports'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPorts', {
    'portsData': [
        {
            'portId': pipePort['portId'],
            'flowDirection': 'FlowIn',
            'outerDiameter': 0.05
        },
        {
            'portId': airPort['portId'],
            'shape': 'Rectangular',
            'width': 0.4,
            'height': 0.25
        }
    ]
}))

aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': pipeTerminalId}, {'elementId': airTerminalId}]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': pipeTerminalId}, {'elementId': airTerminalId}]})
