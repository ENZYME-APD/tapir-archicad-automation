import aclib

# A square traced with a repeated point and a "bow tie" that crosses itself: Archicad's
# regularization turns each into valid polygons, which can be passed on to CreateSlabs.
square_with_duplicate_point = {
    'polygonCoordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 4.0, 'y': 0.0},
        {'x': 4.0, 'y': 0.0},
        {'x': 4.0, 'y': 4.0},
        {'x': 0.0, 'y': 4.0}
    ],
    'holes': [
        {
            'polygonOutline': [
                {'x': 1.0, 'y': 1.0},
                {'x': 1.0, 'y': 2.0},
                {'x': 2.0, 'y': 2.0},
                {'x': 2.0, 'y': 1.0}
            ]
        }
    ]
}

bow_tie = {
    'polygonCoordinates': [
        {'x': 10.0, 'y': 0.0},
        {'x': 14.0, 'y': 4.0},
        {'x': 14.0, 'y': 0.0},
        {'x': 10.0, 'y': 4.0}
    ]
}

regularizedPolygons = aclib.RunTapirCommand(
    'RegularizePolygons', {
        'polygons': [square_with_duplicate_point, bow_tie],
        'withAncestry': True
    })

slabsData = []
for result in regularizedPolygons['results']:
    for polygon in result.get('polygons', []):
        slabsData.append({
            'level': 0.0,
            'polygonCoordinates': polygon['polygonCoordinates'],
            'polygonArcs': polygon.get('polygonArcs', []),
            'holes': polygon['holes']
        })

if slabsData:
    aclib.RunTapirCommand('CreateSlabs', {'slabsData': slabsData})

aclib.RunTapirCommand(
    'RegularizePolylines', {
        'polylines': [
            {
                'coordinates': [
                    {'x': 0.0, 'y': 10.0},
                    {'x': 2.0, 'y': 10.0},
                    {'x': 2.0, 'y': 10.0},
                    {'x': 4.0, 'y': 12.0}
                ]
            }
        ],
        'withAncestry': True
    })
