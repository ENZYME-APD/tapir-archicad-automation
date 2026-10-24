import aclib

# ModifyMorphs' rotationDegreesZ turns a Morph about the vertical axis through its origin
# (for a CreateMorphs box that is its basePoint), counterclockwise seen from above. The Morph
# stays where it is: it is not swung around the project origin, and it is not tipped over.

def GetBoundingBox (morphId):
    boundingBoxes = aclib.RunTapirCommand (
        'Get3DBoundingBoxes', {
            'elements': [{'elementId': morphId}]
        }, debug = False)
    return boundingBoxes['boundingBoxes3D'][0]['boundingBox3D']

def PrintBoundingBox (title, box):
    print ('{0}: x {1:.2f} to {2:.2f}, y {3:.2f} to {4:.2f}, z {5:.2f} to {6:.2f}'.format (
        title, box['xMin'], box['xMax'], box['yMin'], box['yMax'], box['zMin'], box['zMax']))

def ModifyMorph (morphId, details):
    details['elementId'] = morphId
    aclib.RunTapirCommand ('ModifyMorphs', {
        'morphsWithDetails': [details]
    }, debug = False)

createResult = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData': [
        {'basePoint': {'x': 10.0, 'y': 0.0, 'z': 0.0}, 'size': {'x': 1.0, 'y': 2.0, 'z': 0.5}}
    ]
}, debug = False)
morphId = createResult['elements'][0]['elementId']
PrintBoundingBox ('Created', GetBoundingBox (morphId))

# Expected: x 8 to 10, y 0 to 1, z 0 to 0.5 (local +X now points to world +Y).
ModifyMorph (morphId, {'rotationDegreesZ': 90.0})
PrintBoundingBox ('After +90', GetBoundingBox (morphId))

# Turning back gives the original box again.
ModifyMorph (morphId, {'rotationDegreesZ': -90.0})
PrintBoundingBox ('After -90', GetBoundingBox (morphId))

# A translation given together with a rotation moves the Morph by exactly that vector.
# Expected: x 9 to 11, y 0 to 1, z 0 to 0.5.
ModifyMorph (morphId, {'translation': {'x': 1.0, 'y': 0.0, 'z': 0.0}, 'rotationDegreesZ': 90.0})
PrintBoundingBox ('After moving by (1, 0, 0) and +90', GetBoundingBox (morphId))

aclib.RunTapirCommand ('DeleteElements', {
    'elements': [{'elementId': morphId}]
}, debug = False)
