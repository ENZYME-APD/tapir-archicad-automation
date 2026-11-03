import aclib

# A U-shaped stair with a half landing: the middle edge is a landing segment between two
# landing turns. Without baseLineEdges/baseLineVertices Archicad decides the segments itself.
uShapedStair = {
    'baseLinePoints': [
        {'x': 0.0, 'y': 0.0},
        {'x': 3.455, 'y': 0.0},
        {'x': 3.455, 'y': 1.39},
        {'x': 0.0, 'y': 1.39}
    ],
    'baseLineEdges': [
        {'segmentType': 'Flight'},
        {'segmentType': 'Landing'},
        {'segmentType': 'Flight'}
    ],
    'baseLineVertices': [
        {'turnType': 'None'},
        {'turnType': 'Landing'},
        {'turnType': 'Landing'},
        {'turnType': 'None'}
    ],
    'zCoordinate': 0.0,
    'totalHeight': 2.7684,
    'riserHeight': 0.1538,
    'treadDepth': 0.24,
    'flightWidth': 1.15
}

# The same stair with a longer landing: Archicad makes the lead edge offset at least 0.6.
longerLanding = dict (uShapedStair)
longerLanding['baseLinePoints'] = [{'x': p['x'], 'y': p['y'] + 5.0} for p in uShapedStair['baseLinePoints']]
longerLanding['baseLineVertices'] = [
    {'turnType': 'None'},
    {'turnType': 'Landing', 'minLandingStartEdgeOffset': 0.6},
    {'turnType': 'Landing'},
    {'turnType': 'None'}
]

# An L-shaped stair with winders instead of a landing.
lShapedStair = {
    'baseLinePoints': [
        {'x': 10.0, 'y': 0.0},
        {'x': 13.0, 'y': 0.0},
        {'x': 13.0, 'y': 3.0}
    ],
    'baseLineVertices': [
        {'turnType': 'None'},
        {'turnType': 'Winder'},
        {'turnType': 'None'}
    ],
    'zCoordinate': 0.0,
    'totalHeight': 2.7684,
    'flightWidth': 1.15
}

createResult = aclib.RunTapirCommand (
    'CreateStairs', {
        'stairsData': [uShapedStair, longerLanding, lShapedStair]
    })

stairs = [{'elementId': e['elementId']} for e in createResult['elements'] if 'elementId' in e]
details = aclib.RunTapirCommand (
    'GetDetailsOfElements', {
        'elements': stairs
    })

for detail in details['detailsOfElements']:
    stair = detail['details']
    print ('Edges:', [edge['segmentType'] for edge in stair['baseLineEdges']])
    print ('Turns:', [(vertex['turnType'], vertex['landingStartEdgeOffset']) for vertex in stair['baseLineVertices']])
