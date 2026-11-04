"""Connects a branch duct to the middle of a main duct with ConnectMEPElements, which creates a branch element.

Creates the two duct routes; the branch starts on the main route, as the command needs. Requires Archicad 28 or later.
"""
import aclib

mainId, branchId = [element['elementId'] for element in aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 8.0, 'y': 0.0, 'z': 2.5}]
        },
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 4.0, 'y': 0.0, 'z': 2.5}, {'x': 4.0, 'y': 3.0, 'z': 2.5}]
        }
    ]
})['elements']]

aclib.PrintJson (aclib.RunTapirCommand ('ConnectMEPElements', {
    'connectionsData': [{
        'routingElementId': mainId,
        'connectToId': branchId
    }]
}))
