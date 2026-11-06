"""Prints the routing parameters of the first MEP route of the project and of its nodes and segments with GetMEPRoutingParameters.

Needs at least one MEP route. Requires Archicad 28 or later.
"""
import aclib

routes = aclib.RunTapirCommand ('GetMEPElements', {'elementTypes': ['RoutingElement']})['elements']
if not routes:
    raise SystemExit ('No MEP route in the project.')
routeId = routes[0]['elementId']

route = aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': routeId}]})['routingElements'][0]
ids = [routeId] + [node['elementId'] for node in route['nodes']] + [segment['elementId'] for segment in route['segments']]

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPRoutingParameters', {
    'elements': [{'elementId': id} for id in ids]
}))
