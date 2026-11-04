"""Prints the polyline, segments and nodes of the first MEP routes of the project with GetMEPRoutingElements.

Needs at least one MEP route (duct, pipe or cable carrier). Requires Archicad 28 or later.
"""
import aclib

routes = aclib.RunTapirCommand ('GetMEPElements', {'elementTypes': ['RoutingElement']})['elements']
if not routes:
    raise SystemExit ('No MEP route in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPRoutingElements', {
    'elements': [{'elementId': route['elementId']} for route in routes[:3]]
}))
