"""Creates a straight stair, then changes its width, its walking line, its tread thickness and its structure with ModifyStairs.

Makes the flights monolithic with a smooth underside and numbers the steps on the risers, reads the
stair back with GetDetailsOfElements, then deletes the stair it created.
"""
import aclib

stairs = aclib.RunTapirCommand ('CreateStairs', {'stairsData': [{
    'baseLinePoints': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'totalHeight': 3.0,
    'flightWidth': 1.2
}]})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyStairs', {'stairsWithDetails': [{
    'elementId': stairs[0]['elementId'],
    'flightWidth': 1.4,
    'walkingLinePosition': 'Center',
    'numberingType': 'Risers',
    'treads': [{'role': 'Run', 'visible': True, 'thickness': 0.04}],
    'structures': [{'role': 'Run', 'side': 'UnderSide', 'visible': True, 'structureType': 'Monolith', 'bottomType': 'Smooth', 'thickness': 0.18}]
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': stairs}))

aclib.RunTapirCommand ('DeleteElements', {'elements': stairs})
