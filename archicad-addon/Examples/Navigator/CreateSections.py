"""Creates a 10 m long Section on the floor plan with CreateSections, then deletes it again.

The section looks to the left of its start to end direction, here to the north.
"""
import aclib

sections = aclib.RunTapirCommand ('CreateSections', {
    'sectionsData': [{
        'startCoordinate': {'x': 0.0, 'y': -2.0},
        'endCoordinate': {'x': 10.0, 'y': -2.0},
        'depth': 5.0,
        'name': 'Tapir Example Section',
        'id': 'TX-1'
    }]
})['elements']
aclib.PrintJson (sections)

aclib.RunTapirCommand ('DeleteElements', {'elements': [s for s in sections if 'elementId' in s]})
