import aclib

square = lambda x, y, size: [
    {'x': x, 'y': y}, {'x': x + size, 'y': y}, {'x': x + size, 'y': y + size}, {'x': x, 'y': y + size}
]

roofs = aclib.RunTapirCommand (
    'CreateRoofs', {
        'roofsData': [
            {
                'level': 3.0,
                'polygonCoordinates': square (0.0, 20.0, 8.0),
                'pivotLine': {'begCoordinate': {'x': 0.0, 'y': 20.0}, 'endCoordinate': {'x': 8.0, 'y': 20.0}},
                'angle': 0.5
            },
            {
                'level': 3.0,
                'polygonCoordinates': square (12.0, 20.0, 10.0),
                'levels': [{'levelAngle': 0.6, 'levelHeight': 0.0}]
            }
        ]
    })
singlePlaneRoof, multiPlaneRoof = [element['elementId'] for element in roofs['elements']]

# One skylight in the single-plane roof, one in each of two planes of the multi-plane roof.
aclib.RunTapirCommand (
    'CreateSkylights', {
        'skylightsData': [
            {'ownerId': singlePlaneRoof, 'anchorPosition': {'x': 4.0, 'y': 23.0}},
            {'ownerId': multiPlaneRoof, 'anchorPosition': {'x': 17.0, 'y': 22.0}, 'width': 0.8, 'height': 1.2},
            {'ownerId': multiPlaneRoof, 'anchorPosition': {'x': 17.0, 'y': 28.0}, 'anchorPoint': 'TopCenter'}
        ]
    })
