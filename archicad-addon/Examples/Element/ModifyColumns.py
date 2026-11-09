"""Creates a column, then moves it, rotates it by 45 degrees and makes it 0.5 m x 0.3 m with ModifyColumns,
and gives its segment its own side and end surfaces.

Only the given fields change. Angles are in radians. segments writes the surfaces (and the veneer)
of the segment given by segmentIndex (0 for a column of one segment), in the shape
GetDetailsOfElements returns them.
"""
import aclib
import math

surfaces = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Surface'})['attributes']

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

aclib.PrintJson (aclib.RunTapirCommand ('ModifyColumns', {'columnsWithDetails': [{
    'elementId': column['elementId'],
    'segments': [{
        'segmentIndex': 0,
        'materialsChained': False,
        'extrusionSurfaceMaterial': {'overridden': True, 'attributeId': surfaces[0]['attributeId']},
        'endsMaterial': {'overridden': True, 'attributeId': surfaces[1]['attributeId']}
    }]
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [column], 'fields': ['details']})
