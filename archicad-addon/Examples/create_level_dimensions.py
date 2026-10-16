import aclib

# Places an associative Level Dimension on every Mesh of the project: it shows the
# height of the mesh at the marker position and follows the mesh when it changes.
# The marker is put at the centre of the mesh's 2D bounding box, so for a concave
# mesh it may fall outside of the mesh - then the dimension measures the floor.

meshes = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Mesh'})['elements']

boxes = aclib.RunCommand ('API.Get2DBoundingBoxes', {'elements': meshes})['boundingBoxes2D']

levelDimensionsData = []
for mesh, box in zip (meshes, boxes):
    bb = box['boundingBox2D']
    levelDimensionsData.append ({
        'parentElementId': mesh['elementId'],
        'position': {'x': (bb['xMin'] + bb['xMax']) / 2.0, 'y': (bb['yMin'] + bb['yMax']) / 2.0},
        'dimForm': 0,
        'textHeight': 1.8
    })

created = aclib.RunTapirCommand ('CreateLevelDimensions', {'levelDimensionsData': levelDimensionsData})['elements']

details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': [e for e in created if 'elementId' in e]
})['detailsOfElements']

for detail in details:
    print (detail['details']['typeSpecificDetails'])
