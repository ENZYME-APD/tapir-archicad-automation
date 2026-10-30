import aclib

# Requires Archicad 28 or newer for creating the routes; CheckMEPPortConnectability itself works from Archicad 27.

# Create two duct routes that end at the same point
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
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    { 'x': 5.0, 'y': 0.0, 'z': 2.5 },
                    { 'x': 5.0, 'y': 5.0, 'z': 2.5 }
                ]
            }
        ]
    })['elements']

routeIds = [e['elementId'] for e in createdRoutes if 'elementId' in e]

# The ports of the routes, each with the MEP element it belongs to
elementPorts = aclib.RunTapirCommand ('GetMEPPorts', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })['elementPorts']
for routeId, item in zip (routeIds, elementPorts):
    for port in item.get ('ports', []):
        print ('Port {} belongs to its route: {}'.format (port['name'], port['mepElementId'] == routeId))

# Can the first port of the first route be connected to the ports of the second route?
if len (elementPorts) == 2 and 'ports' in elementPorts[0] and 'ports' in elementPorts[1] and elementPorts[0]['ports']:
    firstPortId = elementPorts[0]['ports'][0]['portId']
    results = aclib.RunTapirCommand ('CheckMEPPortConnectability', {
            'portPairs': [{ 'portId': firstPortId, 'otherPortId': port['portId'] } for port in elementPorts[1]['ports']]
        })['results']
    print (results)

# The segments and nodes report the routing element they belong to
routingDetails = aclib.RunTapirCommand ('GetMEPRoutingElements', {
        'elements': [{ 'elementId': routeIds[0] }]
    })['routingElements']

# The ports in the distribution systems graph
ports = aclib.RunTapirCommand ('GetMEPDistributionSystems')['ports']
print ('Ports in the distribution systems graph: {}'.format (len (ports)))

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })
