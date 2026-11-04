"""Creates a white monochrome 3D style with black contours, based on the current style, with Create3DStyles.

The new style stays in the project. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('Create3DStyles', {
    'styles': [{
        'name': 'Tapir White Model',
        'monochromeEnabled': True,
        'monochromeSurfaceColor': {'red': 1.0, 'green': 1.0, 'blue': 1.0},
        'monochromeContourColor': {'red': 0.0, 'green': 0.0, 'blue': 0.0}
    }]
}))
aclib.RunTapirCommand ('Get3DStyleDetails', {'names': ['Tapir White Model']})
