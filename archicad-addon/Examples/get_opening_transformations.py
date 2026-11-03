import aclib

# Read the placement of every Opening and Window in the project with GetOpeningTransformations, without
# changing them. origin and the axes are in world coordinates, from localToGlobal: for a Window (or Door,
# Skylight) the matrix Archicad returns, for an Opening the one built from its geometry, with the origin
# at its anchor point. globalToLocal is the inverse.
openings = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Opening'})['elements']
openings += aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Window'})['elements']

transformations = aclib.RunTapirCommand ('GetOpeningTransformations', {'elements': openings}, debug = False)['openingTransformations']

# Coordinates are computed by Archicad, so they are rounded before printing.
def Rounded (coordinate):
    return (round (coordinate['x'], 6), round (coordinate['y'], 6), round (coordinate['z'], 6))

print ('openings={}'.format (len (transformations)))
for transformation in transformations:
    if 'error' in transformation:
        print ('error: {}'.format (transformation['error']['message']))
        continue
    print ('origin={} xAxis={} yAxis={} zAxis={} mirrored={}'.format (
        Rounded (transformation['origin']),
        Rounded (transformation['xAxis']),
        Rounded (transformation['yAxis']),
        Rounded (transformation['zAxis']),
        transformation['isMirrored']))
