"""Creates a beam, then changes its cross section and adds a round hole with ModifyBeams,
and moves it: a new reference line and its absolute elevation (zCoordinate, as in CreateBeams).

Only the given fields change; holes replaces every hole of the beam. Last, segments tapers
one segment of the beam.
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

# segments changes single segments, picked by segmentIndex: here the first one is tapered to
# half its height at its end. The other segments of a multi-segment beam stay as they are
# (see Workflows/change_one_beam_segment.py).
aclib.PrintJson (aclib.RunTapirCommand ('ModifyBeams', {'beamsWithDetails': [{
    'elementId': beam['elementId'],
    'segments': [{
        'segmentIndex': 0,
        'geometry': {'isHomogeneous': False, 'isEndWidthAndHeightLinked': False, 'endWidth': 0.3, 'endHeight': 0.3}
    }]
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [beam], 'fields': ['details']})
