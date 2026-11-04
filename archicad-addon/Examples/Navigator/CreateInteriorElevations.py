"""Creates an Interior Elevation around a 5 x 4 m room with CreateInteriorElevations, then deletes it again.

Every pair of consecutive points becomes one segment with its own viewpoint; repeating the
first point at the end closes the chain.
"""
import aclib

interiorElevations = aclib.RunTapirCommand ('CreateInteriorElevations', {
    'interiorElevationsData': [{
        'nodeCoordinates': [
            {'x': 0.0, 'y': 0.0},
            {'x': 5.0, 'y': 0.0},
            {'x': 5.0, 'y': 4.0},
            {'x': 0.0, 'y': 4.0},
            {'x': 0.0, 'y': 0.0}
        ],
        'depth': 4.5,
        'name': 'Tapir Example Room'
    }]
})['elements']
aclib.PrintJson (interiorElevations)

aclib.RunTapirCommand ('DeleteElements', {'elements': [e for e in interiorElevations if 'elementId' in e]})
