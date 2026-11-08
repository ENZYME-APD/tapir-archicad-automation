"""Creates a 6 m wall and a 6 m x 5 m slab with a hole, splits both with SplitElements along the line x = 2, then deletes the parts.

The wall gives two parts: the original wall (0 to 2 m) and a copy with the same settings (2 to 6 m).
The slab gives two slabs with the same settings: the original keeps the larger piece (2 to 6 m, with the hole),
the copy gets the rest.
"""
import aclib

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 6.0, 'y': 0.0}, 'height': 3.0}]
})['elements']

slabs = aclib.RunTapirCommand ('CreateSlabs', {'slabsData': [{
    'level': 0.0,
    'thickness': 0.25,
    'polygonCoordinates': [{'x': 0.0, 'y': 1.0}, {'x': 6.0, 'y': 1.0}, {'x': 6.0, 'y': 6.0}, {'x': 0.0, 'y': 6.0}],
    'holes': [{'polygonOutline': [{'x': 3.0, 'y': 3.0}, {'x': 4.0, 'y': 3.0}, {'x': 4.0, 'y': 4.0}, {'x': 3.0, 'y': 4.0}]}]
}]})['elements']

result = aclib.RunTapirCommand ('SplitElements', {
    'elements': walls + slabs,
    'line': {'begin': {'x': 2.0, 'y': -1.0}, 'end': {'x': 2.0, 'y': 1.0}}
})
aclib.PrintJson (result)

parts = []
for original, item in zip (walls + slabs, result['results']):
    parts += item.get ('elements', [original])
aclib.RunTapirCommand ('DeleteElements', {'elements': parts})
