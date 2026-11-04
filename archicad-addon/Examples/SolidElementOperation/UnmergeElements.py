"""Removes the merge between two slabs with UnmergeElements.

The slabs are created and merged by the script first, and deleted at the end.
"""
import aclib

# Two adjacent slabs, created by this script and deleted at the end.
slabs = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [
        {'level': 0.0, 'thickness': 0.3, 'polygonCoordinates': [{'x': 50.0, 'y': 50.0}, {'x': 54.0, 'y': 50.0}, {'x': 54.0, 'y': 54.0}, {'x': 50.0, 'y': 54.0}]},
        {'level': 0.0, 'thickness': 0.3, 'polygonCoordinates': [{'x': 54.0, 'y': 50.0}, {'x': 56.0, 'y': 50.0}, {'x': 56.0, 'y': 54.0}, {'x': 54.0, 'y': 54.0}]}
    ]
})['elements']

aclib.RunTapirCommand ('MergeElements', {'elements': slabs})

aclib.PrintJson (aclib.RunTapirCommand ('UnmergeElements', {
    'elementPairs': [{'elementId1': slabs[0]['elementId'], 'elementId2': slabs[1]['elementId']}]
}))

aclib.RunTapirCommand ('GetMergedElements', {'elements': slabs})

aclib.RunTapirCommand ('DeleteElements', {'elements': slabs})
