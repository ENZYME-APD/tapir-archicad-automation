"""Zooms the current 2D window to the middle of the area it shows now, with SetZoom.

Run it with a floor plan, section, worksheet or layout window in front.
"""
import aclib

# Without parameters GetView2DTransformations returns the zoom of the current window.
zoom = aclib.RunTapirCommand ('GetView2DTransformations')['transformations'][0]['zoom']
width = zoom['xMax'] - zoom['xMin']
height = zoom['yMax'] - zoom['yMin']

aclib.PrintJson (aclib.RunTapirCommand ('SetZoom', {
    'zoomBox': {
        'xMin': zoom['xMin'] + width / 4,
        'yMin': zoom['yMin'] + height / 4,
        'xMax': zoom['xMax'] - width / 4,
        'yMax': zoom['yMax'] - height / 4
    }
}))
