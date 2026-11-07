"""Creates a 6 m wall, splits it in two with SplitElements along the line x = 2, then deletes both parts.

The first part is the original wall (0 to 2 m), the second a copy with the same settings (2 to 6 m).
"""
import aclib

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 6.0, 'y': 0.0}, 'height': 3.0}]
})['elements']

result = aclib.RunTapirCommand ('SplitElements', {
    'elements': walls,
    'line': {'begin': {'x': 2.0, 'y': -1.0}, 'end': {'x': 2.0, 'y': 1.0}}
})
aclib.PrintJson (result)

parts = result['results'][0].get ('elements', walls)
aclib.RunTapirCommand ('DeleteElements', {'elements': parts})
