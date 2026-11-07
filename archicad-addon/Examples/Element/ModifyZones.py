"""Renames and renumbers the first zone and changes its contour line pen with ModifyZones, reads it back, then restores them.

Needs at least one Zone. Only the given fields change; the polygon is kept. ModifyZones takes the
same settings as CreateZones, contourLine and coverFill included.
"""
import aclib

zones = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Zone'})['elements'][:1]
if not zones:
    raise SystemExit ('No zone in the project.')

old = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': zones, 'fields': ['details']})['detailsOfElements'][0]['details']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyZones', {'zonesWithDetails': [{
    'elementId': zones[0]['elementId'],
    'name': 'Meeting Room',
    'numberStr': 'M01',
    'contourLine': {'pen': 5}
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': zones, 'fields': ['details']})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyZones', {'zonesWithDetails': [{
    'elementId': zones[0]['elementId'],
    'name': old['name'],
    'numberStr': old['numberStr'],
    'contourLine': {'pen': old['contourLine']['pen']}
}]}))
