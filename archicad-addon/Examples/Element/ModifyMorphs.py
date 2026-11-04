"""Creates a box morph, then moves it 3 m along X and turns it by 30 degrees with ModifyMorphs.

rotationDegreesZ is in degrees and is added to the current rotation.
"""
import aclib

morph = aclib.RunTapirCommand ('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 0.0, 'y': 0.0, 'z': 0.0},
    'size': {'x': 2.0, 'y': 1.0, 'z': 1.0}
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMorphs', {'morphsWithDetails': [{
    'elementId': morph['elementId'],
    'translation': {'x': 3.0, 'y': 0.0, 'z': 0.0},
    'rotationDegreesZ': 30.0,
    'castShadow': False
}]}))
