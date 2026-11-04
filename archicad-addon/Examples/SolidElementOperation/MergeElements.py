"""Merges two adjacent slabs with MergeElements, the Merge command of Archicad, and reads the merge back.

The slabs are created by the script and deleted at the end.
"""
import aclib

# Two adjacent slabs, created by this script and deleted at the end.
slabs = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [
        {'level': 0.0, 'thickness': 0.3, 'polygonCoordinates': [{'x': 50.0, 'y': 50.0}, {'x': 54.0, 'y': 50.0}, {'x': 54.0, 'y': 54.0}, {'x': 50.0, 'y': 54.0}]},
        {'level': 0.0, 'thickness': 0.3, 'polygonCoordinates': [{'x': 54.0, 'y': 50.0}, {'x': 56.0, 'y': 50.0}, {'x': 56.0, 'y': 54.0}, {'x': 54.0, 'y': 54.0}]}
    ]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('MergeElements', {'elements': slabs}))

aclib.RunTapirCommand ('GetMergedElements', {'elements': slabs})

aclib.RunTapirCommand ('DeleteElements', {'elements': slabs})
