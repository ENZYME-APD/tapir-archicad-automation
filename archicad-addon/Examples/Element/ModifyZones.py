"""Renames and renumbers the first zone with ModifyZones, reads it back, then restores its name and number.

Needs at least one Zone. Only the given fields change; the polygon is kept.
"""
import aclib

zones = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Zone'})['elements'][:1]
if not zones:
    raise SystemExit ('No zone in the project.')

old = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': zones, 'fields': ['details']})['detailsOfElements'][0]['details']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyZones', {'zonesWithDetails': [{
    'elementId': zones[0]['elementId'],
    'name': 'Meeting Room',
    'numberStr': 'M01'
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': zones, 'fields': ['details']})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyZones', {'zonesWithDetails': [{
    'elementId': zones[0]['elementId'],
    'name': old['name'],
    'numberStr': old['numberStr']
}]}))
