"""Checks with CheckMEPPortConnectability whether the end port of one duct can be connected to the ports of another.

Creates two temporary duct routes that meet at one point and deletes them at the end.
The command itself works from Archicad 27; creating the routes requires Archicad 28 or later.
"""
import aclib

routeIds = [element['elementId'] for element in aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 5.0, 'y': 0.0, 'z': 2.5}]
        },
        {
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 5.0, 'y': 0.0, 'z': 2.5}, {'x': 5.0, 'y': 5.0, 'z': 2.5}]
        }
    ]
})['elements']]
firstPorts, secondPorts = [item['ports'] for item in aclib.RunTapirCommand ('GetMEPPorts', {
    'elements': [{'elementId': routeId} for routeId in routeIds]
})['elementPorts']]

aclib.PrintJson (aclib.RunTapirCommand ('CheckMEPPortConnectability', {
    'portPairs': [{'portId': firstPorts[-1]['portId'], 'otherPortId': port['portId']} for port in secondPorts]
}))

aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId} for routeId in routeIds]})
