import math
import aclib

# A multi-plane roof and a single-plane roof, built here so the example runs on any project,
# read back through GetDetailsOfElements: class, level, slope and pivot line for the plane
# roof; levels, eaves overhang and pivot polygon for the multi-plane one; the roof polygon
# for both. The roofs are deleted again at the end.

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

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': roofs})

aclib.RunTapirCommand ('DeleteElements', {'elements': roofs}, debug = False)
