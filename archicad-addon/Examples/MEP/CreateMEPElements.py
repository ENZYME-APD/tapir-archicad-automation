"""Places a duct terminal and a pipe accessory with the current tool settings using CreateMEPElements.

Coordinates are in meters. Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPElements', {
    'elementsData': [
        {
            'type': 'Terminal',
            'domain': 'Ventilation',
            'position': {'x': 0.0, 'y': 0.0, 'z': 2.5}
        },
        {
            'type': 'Accessory',
            'domain': 'Piping',
            'position': {'x': 2.0, 'y': 0.0, 'z': 1.0}
        }
    ]
}))
