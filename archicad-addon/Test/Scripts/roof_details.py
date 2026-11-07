import math
import aclib

# A multi-plane roof and a single-plane roof, built here so the example runs on any project,
# read back through GetDetailsOfElements. Only what the script itself sets is printed (the
# class of both roofs, the slope and pivot line of the plane roof, the planes of the
# multi-plane roof's pivot edges), so the output does not depend on the project's roof
# defaults or on new fields in the response. The roofs are deleted again at the end.

roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [
    {
        'level': 2.7, 'floorIndex': 0, 'eavesOverhang': 0.45,
        'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (25.0)}],
        'polygonCoordinates': [{'x': 100.0, 'y': 100.0}, {'x': 106.0, 'y': 100.0}, {'x': 106.0, 'y': 105.0}, {'x': 100.0, 'y': 105.0}]
    },
    {
        'level': 2.7, 'floorIndex': 0, 'angle': math.radians (20.0),
        'pivotLine': {'begCoordinate': {'x': 120.0, 'y': 100.0}, 'endCoordinate': {'x': 126.0, 'y': 100.0}},
        'polygonCoordinates': [{'x': 120.0, 'y': 100.0}, {'x': 126.0, 'y': 100.0}, {'x': 126.0, 'y': 104.0}, {'x': 120.0, 'y': 104.0}]
    }
]}, debug = False)['elements']

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': roofs}, debug = False)['detailsOfElements']
multiPlane = details[0]['details']
plane = details[1]['details']

print (multiPlane['roofClass'], plane['roofClass'])
print (round (math.degrees (plane['angle']), 6))
print (plane['pivotLine'])

# The roof planes of each edge of the multi-plane roof's pivot polygon, each with its angle and eaves
# overhang; printed here: whether each plane is sloped or a gable.
for edge in multiPlane['pivotEdges']:
    print ([level['angleType'] for level in edge['levels']])

aclib.RunTapirCommand ('DeleteElements', {'elements': roofs}, debug = False)
