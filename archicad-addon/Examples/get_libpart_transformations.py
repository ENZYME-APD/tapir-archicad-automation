import math
import aclib

# GetLibPartTransformations returns where and how a library part instance is placed, as
# Archicad computes it: the location, the rotation angle and the x/y scaling.
elements = []
for elementType in ['Window', 'Door', 'Object', 'Lamp', 'Zone']:
    elements += aclib.RunTapirCommand (
        'GetElementsByType', {
            'elementType': elementType
        }, debug = False)['elements']

# The first few elements are enough to show the shape of the response.
elements = elements[:5]

transformations = aclib.RunTapirCommand (
    'GetLibPartTransformations', {
        'elements': elements
    }, debug = False)['transformations']

for transformation in transformations:
    if 'error' in transformation:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (transformation['error'])))
        continue

    tran = transformation['transformation']
    print ('location = ({:.2f}, {:.2f}), angle = {:.1f} degrees, xRatio = {:.2f}, yRatio = {:.2f}'.format (
        tran['location']['x'], tran['location']['y'], math.degrees (tran['angle']), tran['xRatio'], tran['yRatio']))
