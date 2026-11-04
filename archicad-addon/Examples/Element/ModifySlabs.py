"""Creates a slab, then raises it, makes it thicker and replaces its outline with ModifySlabs.

Only the given fields change; polygonOutline replaces the whole polygon, holes included.
"""
import aclib

slab = aclib.RunTapirCommand ('CreateSlabs', {'slabsData': [{
    'level': 0.0,
    'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}]
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifySlabs', {'slabsWithDetails': [{
    'elementId': slab['elementId'],
    'zCoordinate': 0.5,
    'structureType': 'Basic',
    'thickness': 0.3,
    'polygonOutline': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}, {'x': 6.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}]
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [slab], 'fields': ['details']})
