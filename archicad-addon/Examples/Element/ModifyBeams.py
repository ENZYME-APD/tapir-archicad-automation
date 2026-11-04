"""Creates a beam, then changes its cross section and adds a round hole with ModifyBeams.

Only the given fields change; holes replaces every hole of the beam.
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
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [beam], 'fields': ['details']})
