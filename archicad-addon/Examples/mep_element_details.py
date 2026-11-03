import aclib

# Requires Archicad 28 or newer.

# Create a duct route with a corner, so it gets rigid segments and an elbow
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
            }
        ]
    })['elements']

routeIds = [e['elementId'] for e in createdRoutes if 'elementId' in e]

# Query the parts of the routes that have type-specific details
mepElements = aclib.RunTapirCommand ('GetMEPElements', {
        'elementTypes': ['RigidSegment', 'FlexibleSegment', 'Elbow', 'Branch', 'Transition']
    })['elements']

elementDetails = aclib.RunTapirCommand ('GetMEPElementDetails', {
        'elements': [{ 'elementId': element['elementId'] } for element in mepElements]
    })['elementDetails']

for details in elementDetails:
    if 'error' in details:
        print ('Error: {}'.format (details['error']['message']))
        continue
    print ('{} ({}): {}'.format (details['type'], details['domain'],
        ', '.join ('{}={}'.format (key, value) for key, value in details.items () if key not in ('elementId', 'type', 'domain'))))

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })
