"""Copies the settings of a wide duct route onto a narrow one with TransferMEPSettings.

Creates two temporary duct routes, transfers the settings, reads the target back and deletes both.
Requires Archicad 28 or later.
"""
import aclib

sourceId, targetId = [element['elementId'] for element in aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 5.0, 'y': 0.0, 'z': 2.5}],
            'crossSectionWidth': 0.6,
            'crossSectionHeight': 0.4
        },
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 0.0, 'y': 2.0, 'z': 2.5}, {'x': 5.0, 'y': 2.0, 'z': 2.5}],
            'crossSectionWidth': 0.2,
            'crossSectionHeight': 0.2
        }
    ]
})['elements']]

aclib.PrintJson (aclib.RunTapirCommand ('TransferMEPSettings', {
    'sourceElementId': sourceId,
    'targetElements': [{'elementId': targetId}]
}))

aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': targetId}]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': sourceId}, {'elementId': targetId}]})
