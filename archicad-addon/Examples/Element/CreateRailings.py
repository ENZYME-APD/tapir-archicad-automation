"""Creates a 1.1 m high railing along three edges of a balcony with CreateRailings, then a second one with its settings.

The posts, rails, handrails and infill come from the Railing tool defaults, or from favoriteName. The second railing
takes every setting of the first through templateElementId; the reference line of a railing cannot be changed, so
this is also how a railing gets a new one (create it again with itself as the template, then delete the old one).
"""
import aclib

balcony = aclib.RunTapirCommand ('CreateRailings', {'railingsData': [{
    'coordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 0.0, 'y': 2.0},
        {'x': 5.0, 'y': 2.0},
        {'x': 5.0, 'y': 0.0}
    ],
    'floorIndex': 0,
    'bottomOffset': 0.0,
    'height': 1.1
}]})
aclib.PrintJson (balcony)

aclib.PrintJson (aclib.RunTapirCommand ('CreateRailings', {'railingsData': [{
    'coordinates': [
        {'x': 8.0, 'y': 0.0},
        {'x': 8.0, 'y': 2.0},
        {'x': 11.0, 'y': 2.0}
    ],
    'templateElementId': balcony['elements'][0]['elementId']
}]}))
