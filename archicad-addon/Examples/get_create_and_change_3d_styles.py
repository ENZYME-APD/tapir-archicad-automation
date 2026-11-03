import aclib

# The settings of the current 3D style.
current = aclib.RunTapirCommand ('Get3DStyleDetails')['styles'][0]

# A "white model, black contours" style for exports: starts from the current style,
# only the given fields differ from it.
styleName = 'Tapir White Model'
white = {'red': 1.0, 'green': 1.0, 'blue': 1.0}
black = {'red': 0.0, 'green': 0.0, 'blue': 0.0}
aclib.RunTapirCommand ('Create3DStyles', {
    'styles': [{
        'name': styleName,
        'monochromeEnabled': True,
        'monochromeSurfaceColor': white,
        'monochromeContourColor': black
    }]
})

# Make its contours thicker.
aclib.RunTapirCommand ('Change3DStyles', {
    'styles': [{
        'name': styleName,
        'contourThickness': current['contourThickness'] + 1
    }]
})

aclib.RunTapirCommand ('Get3DStyleDetails', {'names': [styleName, current['name']]})
