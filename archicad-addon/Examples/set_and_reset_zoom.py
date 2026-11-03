import aclib

# Zooms the current 2D window to a box in model coordinates (meters), then steps
# back in the window's zoom history, like Archicad's Zoom Back. Run it with a floor
# plan, section, elevation, worksheet or layout window in front.

def get_zoom ():
    response = aclib.RunTapirCommand ('GetView2DTransformations', {}, debug=False)
    return response['transformations'][0]['zoom']

aclib.RunTapirCommand ('FitInWindow')
fullZoom = get_zoom ()

# Zoom to the middle half of the area the window shows now.
width = fullZoom['xMax'] - fullZoom['xMin']
height = fullZoom['yMax'] - fullZoom['yMin']
zoomBox = {
    'xMin': fullZoom['xMin'] + width / 4,
    'yMin': fullZoom['yMin'] + height / 4,
    'xMax': fullZoom['xMax'] - width / 4,
    'yMax': fullZoom['yMax'] - height / 4
}
aclib.RunTapirCommand ('SetZoom', {'zoomBox': zoomBox})
newZoom = get_zoom ()
print ('The new zoom contains the box: {}'.format (
    newZoom['xMin'] <= zoomBox['xMin'] + 1e-6 and newZoom['yMin'] <= zoomBox['yMin'] + 1e-6 and
    newZoom['xMax'] >= zoomBox['xMax'] - 1e-6 and newZoom['yMax'] >= zoomBox['yMax'] - 1e-6))
print ('The new zoom is smaller than the full one: {}'.format (
    newZoom['xMax'] - newZoom['xMin'] < width))

# An inverted box is rejected.
aclib.RunTapirCommand ('SetZoom', {'zoomBox': {'xMin': 1.0, 'yMin': 0.0, 'xMax': 0.0, 'yMax': 1.0}})

# Zoom Back once: the window shows the fitted area again.
aclib.RunTapirCommand ('ResetZoom', {'numberOfStepsBack': 1})
backZoom = get_zoom ()
print ('Zoom Back restored the full zoom: {}'.format (
    all (abs (backZoom[key] - fullZoom[key]) < 1e-6 for key in fullZoom)))
