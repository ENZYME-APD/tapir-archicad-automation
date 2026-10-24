import aclib

# Creates a wall with an opening in it, then changes the opening's size and position with
# ModifyOpenings and reads the result back through its 3D bounding box. ModifyOpenings takes
# the fields CreateOpenings takes: width, height and basePoint (the bottom left corner of the
# opening in world space). A new height keeps the bottom edge where it is.
#
# It needs Archicad 28 or earlier: in Archicad 29 and later the command reports that it is not
# supported there yet.

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': { 'x': 0.0, 'y': -30.0 },
            'endCoordinate': { 'x': 8.0, 'y': -30.0 },
            'height': 3.0,
            'thickness': 0.3
        }
    ]
})['elements']

openings = aclib.RunTapirCommand ('CreateOpenings', {
    'openingsData': [
        {
            'ownerElementId': walls[0]['elementId'],
            'basePoint': { 'x': 2.0, 'y': -30.0, 'z': 1.0 },
            'width': 0.8,
            'height': 0.8
        }
    ]
})['elements']

def GetBoundingBoxOf (element):
    boundingBoxes = aclib.RunTapirCommand ('Get3DBoundingBoxes', {
        'elements': [element]
    }, debug = False)['boundingBoxes3D']
    return boundingBoxes[0]['boundingBox3D']

def PrintSize (label, box):
    print ('{}: width={:.3f} height={:.3f} bottom={:.3f}'.format (
        label, box['xMax'] - box['xMin'], box['zMax'] - box['zMin'], box['zMin']))

PrintSize ('created', GetBoundingBoxOf (openings[0]))

# Resize only: the bottom edge stays at z = 1.0.
aclib.RunTapirCommand ('ModifyOpenings', {
    'openingsWithDetails': [
        {
            'elementId': openings[0]['elementId'],
            'width': 1.2,
            'height': 1.5
        }
    ]
})
PrintSize ('resized', GetBoundingBoxOf (openings[0]))

# Move: the bottom left corner goes to the new base point.
aclib.RunTapirCommand ('ModifyOpenings', {
    'openingsWithDetails': [
        {
            'elementId': openings[0]['elementId'],
            'basePoint': { 'x': 5.0, 'y': -30.0, 'z': 0.5 }
        }
    ]
})
PrintSize ('moved', GetBoundingBoxOf (openings[0]))

# An item without any field to change fails; other items of the same call are still applied.
result = aclib.RunTapirCommand ('ModifyOpenings', {
    'openingsWithDetails': [
        { 'elementId': openings[0]['elementId'] }
    ]
}, debug = False)
print ('no fields: success={}'.format (result['executionResults'][0]['success']))

aclib.RunTapirCommand ('DeleteElements', {'elements': openings + walls})
