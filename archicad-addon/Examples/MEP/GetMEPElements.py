"""Lists the duct and pipe routes and terminals of the project with GetMEPElements.

Leave out both filters to list every MEP element. Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPElements', {
    'elementTypes': ['RoutingElement', 'Terminal'],
    'domains': ['Ventilation', 'Piping']
}))
