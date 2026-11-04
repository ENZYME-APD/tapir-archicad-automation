"""Makes the contours of the current 3D style thicker with Change3DStyles, then restores them.

Requires Archicad 27 or later.
"""
import aclib

style = aclib.RunTapirCommand ('Get3DStyleDetails')['styles'][0]

aclib.PrintJson (aclib.RunTapirCommand ('Change3DStyles', {
    'styles': [{'name': style['name'], 'contourThickness': style['contourThickness'] + 1}]
}))
aclib.RunTapirCommand ('Get3DStyleDetails', {'names': [style['name']]})

aclib.PrintJson (aclib.RunTapirCommand ('Change3DStyles', {
    'styles': [{'name': style['name'], 'contourThickness': style['contourThickness']}]
}))
