"""Creates a column, then moves it, rotates it by 45 degrees and makes it 0.5 m x 0.3 m with ModifyColumns.

Only the given fields change. Angles are in radians.
"""
import aclib
import math

column = aclib.RunTapirCommand ('CreateColumns', {'columnsData': [{
    'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyColumns', {'columnsWithDetails': [{
    'elementId': column['elementId'],
    'origin': {'x': 2.0, 'y': 1.0},
    'axisRotationAngle': math.radians (45),
    'circleBased': False,
    'isWidthAndHeightLinked': False,
    'width': 0.5,
    'depth': 0.3
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [column], 'fields': ['details']})
