"""Creates a multi-plane roof, then raises it, changes its slope and eaves overhang with ModifyRoofs.

Only the given fields change. Angles are in radians.
"""
import aclib
import math

roof = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [{
    'level': 3.0,
    'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 8.0, 'y': 0.0}, {'x': 8.0, 'y': 6.0}, {'x': 0.0, 'y': 6.0}],
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (30)}]
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRoofs', {'roofsWithDetails': [{
    'elementId': roof['elementId'],
    'level': 3.5,
    'eavesOverhang': 0.5,
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (40)}]
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [roof], 'fields': ['details']})
