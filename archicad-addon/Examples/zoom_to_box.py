import aclib

# ZoomToBox zooms the current 2D window (floor plan, section/elevation, worksheet,
# layout etc.) to a box given in model coordinates. The box has the same shape as
# the zoom returned by GetView2DTransformations, so the two can be used together
# to save and restore the shown area.

# Fit everything in the window first, so the current zoom covers the whole model.
response = aclib.RunTapirCommand ('FitInWindow')
if not response.get ('success'):
    print ('FitInWindow failed; is a 2D window in front?')
    exit ()

response = aclib.RunTapirCommand ('GetView2DTransformations', debug = False)
fullZoom = response['transformations'][0]['zoom']

# Zoom to the middle half of it.
width = fullZoom['xMax'] - fullZoom['xMin']
height = fullZoom['yMax'] - fullZoom['yMin']
box = {
    'xMin': fullZoom['xMin'] + width / 4,
    'yMin': fullZoom['yMin'] + height / 4,
    'xMax': fullZoom['xMax'] - width / 4,
    'yMax': fullZoom['yMax'] - height / 4
}
response = aclib.RunTapirCommand ('ZoomToBox', {'zoom': box}, debug = False)
print ('ZoomToBox succeeded: {}'.format (response.get ('success')))

# The window keeps its aspect ratio, so the new zoom contains the box but may be larger.
response = aclib.RunTapirCommand ('GetView2DTransformations', debug = False)
newZoom = response['transformations'][0]['zoom']
tolerance = 1e-6
containsBox = (newZoom['xMin'] <= box['xMin'] + tolerance and newZoom['yMin'] <= box['yMin'] + tolerance and
               newZoom['xMax'] >= box['xMax'] - tolerance and newZoom['yMax'] >= box['yMax'] - tolerance)
isSmaller = (newZoom['xMax'] - newZoom['xMin'] < width - tolerance or
             newZoom['yMax'] - newZoom['yMin'] < height - tolerance)
print ('The new zoom contains the box: {}'.format (containsBox))
print ('The new zoom is smaller than the full one: {}'.format (isSmaller))

# An invalid box is rejected.
response = aclib.RunTapirCommand ('ZoomToBox', {'zoom': {'xMin': 1.0, 'yMin': 0.0, 'xMax': 0.0, 'yMax': 1.0}}, debug = False)
print ('ZoomToBox with xMin > xMax succeeded: {}'.format (response.get ('success')))

# Restore the full view.
aclib.RunTapirCommand ('FitInWindow', debug = False)
