"""Creates two roofs and a skylight in the first one, then moves the skylight onto the second roof with RecreateSkylights.

Archicad does not move a skylight onto another roof or shell through ModifySkylights (ACAPI_Element_Change
keeps its owner), so RecreateSkylights creates it again on ownerId, at anchorPosition (or at its old one),
with the old skylight's settings, library part and GDL parameters, and deletes the old one. The result
gives the new skylight and lists what could not be carried over (labels, dimensions, group).
"""
import aclib

square = lambda x, y, size: [
    {'x': x, 'y': y}, {'x': x + size, 'y': y}, {'x': x + size, 'y': y + size}, {'x': x, 'y': y + size}
]

roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [
    {
        'level': 3.0,
        'polygonCoordinates': square (0.0, 20.0, 8.0),
        'pivotLine': {'begCoordinate': {'x': 0.0, 'y': 20.0}, 'endCoordinate': {'x': 8.0, 'y': 20.0}},
        'angle': 0.5
    },
    {
        'level': 3.0,
        'polygonCoordinates': square (12.0, 20.0, 8.0),
        'pivotLine': {'begCoordinate': {'x': 12.0, 'y': 20.0}, 'endCoordinate': {'x': 20.0, 'y': 20.0}},
        'angle': 0.3
    }
]})['elements']

skylight = aclib.RunTapirCommand ('CreateSkylights', {'skylightsData': [{
    'ownerId': roofs[0]['elementId'],
    'anchorPosition': {'x': 4.0, 'y': 23.0},
    'width': 0.9,
    'height': 1.1,
    'id': 'SKY-01'
}]})['elements'][0]

result = aclib.RunTapirCommand ('RecreateSkylights', {'skylights': [{
    'elementId': skylight['elementId'],
    'ownerId': roofs[1]['elementId'],
    'anchorPosition': {'x': 16.0, 'y': 23.0}
}]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][0]['elementId']}], 'fields': ['details']}))
