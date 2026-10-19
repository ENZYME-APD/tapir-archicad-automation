import aclib

# ModifyMorphs' rotationDegreesZ must turn a Morph about the vertical axis through its origin,
# counterclockwise seen from above, and leave the origin where it is. A 1 x 2 x 0.5 box is placed
# with its origin at (10, 0, 0), turned, and checked by its 3D bounding box. The box is deleted
# at the end, so the project is left as it was.

def GetBox (elementId):
    box = aclib.RunTapirCommand ('Get3DBoundingBoxes', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['boundingBoxes3D'][0]['boundingBox3D']
    return (box['xMin'], box['xMax'], box['yMin'], box['yMax'], box['zMin'], box['zMax'])

def GetOrigin (elementId):
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['detailsOfElements'][0]['details']
    return (details['origin']['x'], details['origin']['y'], details['origin']['z'])

def ModifyMorph (elementId, fields):
    return aclib.RunTapirCommand ('ModifyMorphs', {
        'morphsWithDetails': [dict ({ 'elementId': elementId }, **fields)]
    }, debug = False)['executionResults'][0]

def IsNear (values1, values2):
    return all (abs (value1 - value2) < 1e-4 for value1, value2 in zip (values1, values2))

elementId = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData': [
        { 'basePoint': { 'x': 10.0, 'y': 0.0, 'z': 0.0 }, 'size': { 'x': 1.0, 'y': 2.0, 'z': 0.5 } }
    ]
}, debug = False)['elements'][0]['elementId']
print ('Placed box:', IsNear (GetBox (elementId), (10.0, 11.0, 0.0, 2.0, 0.0, 0.5)))

# +90: the box's x side now runs along +y and its y side along -x, around the origin (10, 0, 0).
ModifyMorph (elementId, { 'rotationDegreesZ': 90.0 })
print ('Turned +90 about its origin:', IsNear (GetBox (elementId), (8.0, 10.0, 0.0, 1.0, 0.0, 0.5)))
print ('Origin kept:', IsNear (GetOrigin (elementId), (10.0, 0.0, 0.0)))

# A turn of +45 and then one of -45 leave the box as it was.
ModifyMorph (elementId, { 'rotationDegreesZ': 45.0 })
ModifyMorph (elementId, { 'rotationDegreesZ': -45.0 })
print ('+45 then -45 changes nothing:', IsNear (GetBox (elementId), (8.0, 10.0, 0.0, 1.0, 0.0, 0.5)))

# A translation given in the same call moves the origin by exactly that much; it is not turned.
ModifyMorph (elementId, { 'translation': { 'x': 1.0, 'y': 0.0, 'z': 0.0 }, 'rotationDegreesZ': -90.0 })
print ('Translated and turned back:', IsNear (GetBox (elementId), (11.0, 12.0, 0.0, 2.0, 0.0, 0.5)))
print ('Origin moved by the translation only:', IsNear (GetOrigin (elementId), (11.0, 0.0, 0.0)))

aclib.RunTapirCommand ('DeleteElements', { 'elements': [{ 'elementId': elementId }] }, debug = False)
