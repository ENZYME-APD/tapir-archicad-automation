import aclib

# Dimension two walls with CreateAssociativeDimensions, then read the dimension back with
# GetDimensionData. Each witness point reports the parameters CreateAssociativeDimensions takes
# (line, inIndex, special, nodeType, nodeStatus, nodeId), so a dimension placed by hand in Archicad
# can be read back to learn the values an element type needs, and a dimension can be rebuilt from
# what was read.
walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': {'x': 0.0, 'y': -60.0},
            'endCoordinate': {'x': 4.0, 'y': -60.0},
            'height': 3.0,
            'thickness': 0.3
        },
        {
            'begCoordinate': {'x': 7.0, 'y': -60.0},
            'endCoordinate': {'x': 11.0, 'y': -60.0},
            'height': 3.0,
            'thickness': 0.3
        }
    ]
}, debug = False)['elements']
wallIds = [w['elementId'] for w in walls]

# inIndex 1 and 2 are the two end points of a wall's reference line.
dimensions = aclib.RunTapirCommand ('CreateAssociativeDimensions', {
    'dimensionsData': [
        {
            'referencePoint': {'x': 0.0, 'y': -58.0},
            'direction': {'x': 1.0, 'y': 0.0},
            'witnessPoints': [
                {'elementId': wallIds[0], 'inIndex': 2},
                {'elementId': wallIds[1], 'inIndex': 1}
            ]
        }
    ]
}, debug = False)['elements']

# Coordinates and values are recomputed by Archicad, so they are rounded before printing or comparing.
def Rounded (coordinate):
    return (round (coordinate['x'], 6), round (coordinate['y'], 6))

def PrintDimensionData (dimensionId):
    data = aclib.RunTapirCommand ('GetDimensionData', {'elements': [dimensionId]}, debug = False)['dimensionsData'][0]
    print ('direction={} witness points={}'.format (Rounded (data['direction']), len (data['witnessPoints'])))
    for p in data['witnessPoints']:
        # baseElementId is missing when the witness point is not attached to any element.
        baseElementId = p.get ('baseElementId')
        baseElement = next ((i for i, wallId in enumerate (wallIds) if baseElementId is not None and wallId['guid'] == baseElementId['guid']), None)
        print ('  wall {} at {} value={} line={} inIndex={} special={} nodeType={} nodeStatus={} nodeId={}'.format (
            baseElement, Rounded (p['coordinate']), round (p['dimensionValue'], 6),
            p['line'], p['inIndex'], p['special'], p['nodeType'], p['nodeStatus'], p['nodeId']))
    return data

data = PrintDimensionData (dimensions[0])

# The witness parameters read back are enough to rebuild the same dimension.
rebuilt = []
if all ('baseElementId' in p for p in data['witnessPoints']):
    rebuilt = aclib.RunTapirCommand ('CreateAssociativeDimensions', {
        'dimensionsData': [
            {
                'referencePoint': {'x': 0.0, 'y': -57.0},
                'direction': data['direction'],
                'witnessPoints': [
                    {
                        'elementId': p['baseElementId'],
                        'line': p['line'],
                        'inIndex': p['inIndex'],
                        'special': p['special'],
                        'nodeType': p['nodeType'],
                        'nodeStatus': p['nodeStatus'],
                        'nodeId': p['nodeId']
                    }
                    for p in data['witnessPoints']
                ]
            }
        ]
    }, debug = False)['elements']
    rebuiltData = PrintDimensionData (rebuilt[0])
    print ('rebuilt dimension measures the same: {}'.format (
        [Rounded (p['coordinate']) for p in rebuiltData['witnessPoints']] == [Rounded (p['coordinate']) for p in data['witnessPoints']]))
else:
    print ('not every witness point is attached to a wall, nothing to rebuild')

aclib.RunTapirCommand ('DeleteElements', {'elements': dimensions + rebuilt + walls}, debug = False)
