import aclib

# Read the placement of every Opening in the project with GetOpeningTransformations, without changing
# them. globalToLocal is the matrix Archicad returns; origin and the axes are in world coordinates,
# derived from its inverse (localToGlobal).
openings = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Opening'})['elements']

transformations = aclib.RunTapirCommand ('GetOpeningTransformations', {'elements': openings}, debug = False)['openingTransformations']

# Coordinates are computed by Archicad, so they are rounded before printing.
def Rounded (coordinate):
    return (round (coordinate['x'], 6), round (coordinate['y'], 6), round (coordinate['z'], 6))

print ('openings={}'.format (len (transformations)))
for transformation in transformations:
    if 'error' in transformation:
        print ('error: {}'.format (transformation['error']['message']))
        continue
    if 'origin' not in transformation:
        print ('not invertible, determinant={}'.format (round (transformation['determinant'], 6)))
        continue
    print ('origin={} xAxis={} yAxis={} zAxis={} mirrored={}'.format (
        Rounded (transformation['origin']),
        Rounded (transformation['xAxis']),
        Rounded (transformation['yAxis']),
        Rounded (transformation['zAxis']),
        transformation['isMirrored']))
