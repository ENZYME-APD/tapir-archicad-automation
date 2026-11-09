"""Creates a beam, then changes its cross section and adds a round hole with ModifyBeams,
moves it: a new reference line and its absolute elevation (zCoordinate, as in CreateBeams),
and changes its floor plan display: home story only, cut contour pen 21 and dashed hidden lines.

Only the given fields change; holes replaces every hole of the beam. The display settings
have the names GetDetailsOfElements reads them under, so what it returns can be sent back.
"""
import aclib

beam = aclib.RunTapirCommand ('CreateBeams', {'beamsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'zCoordinate': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyBeams', {'beamsWithDetails': [{
    'elementId': beam['elementId'],
    'isWidthAndHeightLinked': False,
    'width': 0.3,
    'height': 0.6,
    'holes': [{'type': 'Circular', 'centerX': 3.0, 'centerZ': 0.3, 'width': 0.15}]
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('ModifyBeams', {'beamsWithDetails': [{
    'elementId': beam['elementId'],
    'begCoordinate': {'x': 1.0, 'y': 2.0},
    'endCoordinate': {'x': 7.0, 'y': 2.0},
    'zCoordinate': 2.5
}]}))

lineTypes = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'})['attributes']
dashed = next ((a for a in lineTypes if 'dash' in a.get ('name', '').lower ()), lineTypes[-1])

aclib.PrintJson (aclib.RunTapirCommand ('ModifyBeams', {'beamsWithDetails': [{
    'elementId': beam['elementId'],
    'isAutoOnStoryVisibility': False,
    'visibility': {'showOnHome': True, 'showAllAbove': False, 'showAllBelow': False, 'showRelAbove': 0, 'showRelBelow': 0},
    'cutContourLinePen': 21,
    'hiddenLinePen': 21,
    'hiddenLineTypeId': dashed['attributeId']
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [beam], 'fields': ['details']})
