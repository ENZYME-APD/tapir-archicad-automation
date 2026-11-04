"""Creates a vectorial hatching Fill of parallel lines at 45 degrees with CreateFills.

Overwrites the fill of the same name when the script is run again.
"""
import math
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateFills', {
    'fillDataArray': [{
        'name': 'Tapir Example Hatching',
        'subType': 'Vector',
        'scaleWithPlan': True,
        'useForWalls': True,
        'useForDraft': True,
        'useForCover': True,
        'horizontalSpacing': 1.0,
        'verticalSpacing': 1.0,
        'angle': math.radians (45),
        'lineItems': [{
            'frequency': 0.1,
            'direction': 0.0,
            'offsetLine': 0.0,
            'offset': {'x': 0.0, 'y': 0.0}
        }]
    }],
    'overwriteExisting': True
}))
