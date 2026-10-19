import aclib

# Requires Archicad 28 or newer. The flange size is only returned from Archicad 30.

# Create a duct route and a pipe route
createdRoutes = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': 0.0, 'z': 2.5 },
                    { 'x': 5.0, 'y': 0.0, 'z': 2.5 }
                ]
            },
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': -2.0, 'z': 1.0 },
                    { 'x': 5.0, 'y': -2.0, 'z': 1.0 }
                ]
            }
        ]
    })['elements']

routeIds = [e['elementId'] for e in createdRoutes if 'elementId' in e]

# Print the domain specific data of their ports
elementPorts = aclib.RunTapirCommand ('GetMEPPorts', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })['elementPorts']
for portsOfElement in elementPorts:
    for port in portsOfElement.get ('ports', []):
        print ('{} port {}: outer diameter {}, wall thickness {}, flange size {}, connectable {}'.format (
            port['domain'],
            port['name'],
            port.get ('outerDiameter', '-'),
            port.get ('wallThickness', '-'),
            port.get ('flangeSize', '-'),
            port.get ('isPhysicallyConnectable', '-')))

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })
