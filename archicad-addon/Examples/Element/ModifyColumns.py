"""Creates a column, then moves it, rotates it by 45 degrees and makes it 0.5 m x 0.3 m with ModifyColumns,
and changes its floor plan display: home story only, contour pen 21 and dashed hidden lines.

Only the given fields change. Angles are in radians. The display settings have the names
GetDetailsOfElements reads them under, so what it returns can be sent back.
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

lineTypes = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'})['attributes']
dashed = next ((a for a in lineTypes if 'dash' in a.get ('name', '').lower ()), lineTypes[-1])

aclib.PrintJson (aclib.RunTapirCommand ('ModifyColumns', {'columnsWithDetails': [{
    'elementId': column['elementId'],
    'isAutoOnStoryVisibility': False,
    'visibility': {'showOnHome': True, 'showAllAbove': False, 'showAllBelow': False, 'showRelAbove': 0, 'showRelBelow': 0},
    'contourPen': 21,
    'hiddenLinePen': 21,
    'hiddenLineTypeId': dashed['attributeId']
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [column], 'fields': ['details']})
