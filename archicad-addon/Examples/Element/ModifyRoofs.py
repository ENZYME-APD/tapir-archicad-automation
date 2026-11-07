"""Creates a multi-plane and a single-plane roof, then changes both with ModifyRoofs.

The multi-plane roof is raised and gets a new slope and eaves overhang. The single-plane roof gets a
steeper angle, a new pivot line and a wider outline. Only the given fields change. Angles are in radians.
"""
import aclib
import math

roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [{
    'level': 3.0,
    'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 8.0, 'y': 0.0}, {'x': 8.0, 'y': 6.0}, {'x': 0.0, 'y': 6.0}],
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (30)}]
}, {
    'level': 3.0,
    'polygonCoordinates': [{'x': 12.0, 'y': 0.0}, {'x': 18.0, 'y': 0.0}, {'x': 18.0, 'y': 4.0}, {'x': 12.0, 'y': 4.0}],
    'pivotLine': {'begCoordinate': {'x': 12.0, 'y': 0.0}, 'endCoordinate': {'x': 18.0, 'y': 0.0}},
    'angle': math.radians (15)
}]})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRoofs', {'roofsWithDetails': [{
    'elementId': roofs[0]['elementId'],
    'level': 3.5,
    'eavesOverhang': 0.5,
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (40)}]
}, {
    'elementId': roofs[1]['elementId'],
    'angle': math.radians (25),
    'pivotLine': {'begCoordinate': {'x': 12.0, 'y': -1.0}, 'endCoordinate': {'x': 18.0, 'y': -1.0}},
    'polygonOutline': [{'x': 12.0, 'y': -1.0}, {'x': 18.0, 'y': -1.0}, {'x': 18.0, 'y': 5.0}, {'x': 12.0, 'y': 5.0}]
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': roofs, 'fields': ['details']})
