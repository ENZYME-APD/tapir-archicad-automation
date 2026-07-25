import aclib

# Requires Archicad 28 or newer.

# Create two duct routes and a pipe route
createdRoutes = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': 0.0, 'z': 2.5 },
                    { 'x': 5.0, 'y': 0.0, 'z': 2.5 },
                    { 'x': 5.0, 'y': 5.0, 'z': 2.5 }
                ],
                'crossSectionWidth': 0.4,
                'crossSectionHeight': 0.3
            },
            {
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    { 'x': 5.0, 'y': 5.0, 'z': 2.5 },
                    { 'x': 10.0, 'y': 5.0, 'z': 2.5 }
                ]
            },
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': -2.0, 'z': 1.0 },
                    { 'x': 8.0, 'y': -2.0, 'z': 1.0 }
                ]
            }
        ]
    })['elements']

routeIds = [e['elementId'] for e in createdRoutes if 'elementId' in e]

# Place a duct terminal
createdElements = aclib.RunTapirCommand ('CreateMEPElements', {
        'elementsData': [
            {
                'type': 'Terminal',
                'domain': 'Ventilation',
                'position': { 'x': -1.0, 'y': 0.0, 'z': 2.5 }
            }
        ]
    })['elements']

# Query all MEP elements
mepElements = aclib.RunTapirCommand ('GetMEPElements', {
        'elementTypes': ['RoutingElement']
    })['elements']

# Query the details of the created routes
routingDetails = aclib.RunTapirCommand ('GetMEPRoutingElements', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })['routingElements']

# Query the ports of the first route
ports = aclib.RunTapirCommand ('GetMEPPorts', {
        'elements': [{ 'elementId': routeIds[0] }]
    })['elementPorts']

# Connect the second duct route to the first one
connectionResults = aclib.RunTapirCommand ('ConnectMEPElements', {
        'connectionsData': [
            {
                'routingElementId': routeIds[1],
                'connectToId': routeIds[0]
            }
        ]
    })['connectionResults']

# Modify the pipe route cross section
aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
        'routingElementsData': [
            {
                'elementId': routeIds[2],
                'crossSectionWidth': 0.1,
                'crossSectionHeight': 0.1
            }
        ]
    })

# List the distribution systems
distributionSystems = aclib.RunTapirCommand ('GetMEPDistributionSystems')['distributionSystems']
for system in distributionSystems:
    print ('System domain: {}, element count: {}'.format (system['domain'], len (system['elements'])))
