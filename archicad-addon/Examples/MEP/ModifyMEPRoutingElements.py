"""Changes the cross section and insulation of a duct route with ModifyMEPRoutingElements.

Creates a temporary duct route, modifies it, reads it back and deletes it. Requires Archicad 28 or later.
"""
import aclib

routeId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
    'routingElementsData': [{
        'domain': 'Ventilation',
        'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 5.0, 'y': 0.0, 'z': 2.5}]
    }]
})['elements'][0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
    'routingElementsData': [{
        'elementId': routeId,
        'crossSectionShape': 'Rectangular',
        'crossSectionWidth': 0.5,
        'crossSectionHeight': 0.25,
        'insulationThickness': 0.05
    }]
}))

aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [{'elementId': routeId}]})
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': routeId}]})
