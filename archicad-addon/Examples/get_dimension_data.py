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

def PrintDimensionData (dimensionId):
    data = aclib.RunTapirCommand ('GetDimensionData', {'elements': [dimensionId]}, debug = False)['dimensionsData'][0]
    print ('direction=({}, {}) witness points={}'.format (data['direction']['x'], data['direction']['y'], len (data['witnessPoints'])))
    for p in data['witnessPoints']:
        baseElement = next ((i for i, wallId in enumerate (wallIds) if wallId['guid'] == p['baseElementId']['guid']), None)
        print ('  wall {} at ({}, {}) value={} line={} inIndex={} special={} nodeType={} nodeStatus={} nodeId={}'.format (
            baseElement, p['coordinate']['x'], p['coordinate']['y'], p['dimensionValue'],
            p['line'], p['inIndex'], p['special'], p['nodeType'], p['nodeStatus'], p['nodeId']))
    return data

data = PrintDimensionData (dimensions[0])

# The witness parameters read back are enough to rebuild the same dimension.
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
    [p['coordinate'] for p in rebuiltData['witnessPoints']] == [p['coordinate'] for p in data['witnessPoints']]))

aclib.RunTapirCommand ('DeleteElements', {'elements': dimensions + rebuilt + walls}, debug = False)
