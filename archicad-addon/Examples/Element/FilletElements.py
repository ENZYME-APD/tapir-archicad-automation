"""Creates a 6 m x 5 m slab, rounds its four corners with FilletElements (radius 0.5 m), prints the result and the slab's new arcs, then deletes the slab.

mode 'chamfer' would cut the corners off with straight edges instead; 'corners' (e.g. [0, 2]) changes only the given corners.
"""
import aclib

slabs = aclib.RunTapirCommand ('CreateSlabs', {'slabsData': [{
    'level': 0.0,
    'thickness': 0.25,
    'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}, {'x': 6.0, 'y': 5.0}, {'x': 0.0, 'y': 5.0}]
}]})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('FilletElements', {'elements': slabs, 'radius': 0.5, 'mode': 'fillet'}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': slabs, 'fields': ['details']})['detailsOfElements'][0]['details']
aclib.PrintJson (details.get ('polygonArcs', []))

aclib.RunTapirCommand ('DeleteElements', {'elements': slabs})
