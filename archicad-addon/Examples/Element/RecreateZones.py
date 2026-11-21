"""Builds a room of four walls with an automatic zone in it, then turns the zone into a manual zone with RecreateZones.

Archicad does not make an automatic zone manual through ModifyZones (ACAPI_Element_Change keeps it
automatic), so RecreateZones creates a manual zone with the polygon the zone has now and its category,
name, number, stamp and other settings, Element ID, classifications and property values, and deletes the
old zone. The manual zone no longer follows its walls when the zones are updated. The result gives the
new zone and lists what could not be carried over (labels, dimensions, group).
"""
import aclib

corners = [(0.0, 0.0), (6.0, 0.0), (6.0, 5.0), (0.0, 5.0)]
aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': corners[i][0], 'y': corners[i][1]},
    'endCoordinate': {'x': corners[(i + 1) % 4][0], 'y': corners[(i + 1) % 4][1]},
    'height': 3.0,
    'thickness': 0.2
} for i in range (4)]})

zone = aclib.RunTapirCommand ('CreateZones', {'zonesData': [{
    'name': 'Office',
    'numberStr': '001',
    'geometry': {'referencePosition': {'x': 3.0, 'y': 2.5}}
}]})['elements'][0]

result = aclib.RunTapirCommand ('RecreateZones', {'zones': [{
    'elementId': zone['elementId'],
    'makeManual': True
}]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][0]['elementId']}], 'fields': ['details']}))
