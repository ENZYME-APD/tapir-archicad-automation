"""Creates a rectangular duct route with one corner and a circular pipe route with CreateMEPRoutingElements.

Coordinates are in meters. Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [
                {'x': 0.0, 'y': 0.0, 'z': 2.5},
                {'x': 5.0, 'y': 0.0, 'z': 2.5},
                {'x': 5.0, 'y': 4.0, 'z': 2.5}
            ],
            'crossSectionShape': 'Rectangular',
            'crossSectionWidth': 0.4,
            'crossSectionHeight': 0.3
        },
        {
            'domain': 'Piping',
            'nodeCoordinates': [
                {'x': 0.0, 'y': -2.0, 'z': 1.0},
                {'x': 6.0, 'y': -2.0, 'z': 1.0}
            ],
            'crossSectionShape': 'Circular',
            'crossSectionWidth': 0.05,
            'crossSectionHeight': 0.05
        }
    ]
}))
