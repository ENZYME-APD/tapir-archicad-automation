"""Creates a 1.1 m high railing along three edges of a balcony with CreateRailings.

The posts, rails, handrails and infill come from the Railing tool defaults, or from favoriteName.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateRailings', {'railingsData': [{
    'coordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 0.0, 'y': 2.0},
        {'x': 5.0, 'y': 2.0},
        {'x': 5.0, 'y': 0.0}
    ],
    'floorIndex': 0,
    'bottomOffset': 0.0,
    'height': 1.1
}]}))
