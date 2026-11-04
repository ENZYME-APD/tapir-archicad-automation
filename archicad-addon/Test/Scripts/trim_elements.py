import math
import aclib

# Two walls under a multi-plane roof, built here so the example runs on any project: the
# walls and the roof go into one TrimElements call and the roof does the trimming; the trims
# are read back, then removed, then the elements are deleted again.

walls = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [
    {'begCoordinate': {'x': 100.0, 'y': 100.0}, 'endCoordinate': {'x': 106.0, 'y': 100.0}, 'height': 2.7, 'thickness': 0.2, 'floorIndex': 0},
    {'begCoordinate': {'x': 106.0, 'y': 100.0}, 'endCoordinate': {'x': 106.0, 'y': 105.0}, 'height': 2.7, 'thickness': 0.2, 'floorIndex': 0}
]}, debug = False)['elements']

roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [{
    'level': 2.7, 'floorIndex': 0, 'eavesOverhang': 0.45,
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (25.0)}],
    'polygonCoordinates': [{'x': 100.0, 'y': 100.0}, {'x': 106.0, 'y': 100.0}, {'x': 106.0, 'y': 105.0}, {'x': 100.0, 'y': 105.0}]
}]}, debug = False)['elements']

aclib.RunTapirCommand ('TrimElements', {'elements': walls + roofs})

trims = aclib.RunTapirCommand ('GetElementTrims', {'elements': walls})['elementTrims']

pairs = []
for wall, item in zip (walls, trims):
    for t in item['trimmedBy']:
        pairs.append ({'elementId': wall['elementId'], 'trimmingElementId': t['elementId']})
if len (pairs) > 0:
    aclib.RunTapirCommand ('RemoveElementTrims', {'elementPairs': pairs})

aclib.RunTapirCommand ('DeleteElements', {'elements': walls + roofs}, debug = False)
