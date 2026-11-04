"""Reads the area reductions of the first zones (covered by walls, columns, fills or a low ceiling) with GetZoneReductions.

Needs at least one Zone.
"""
import aclib

zones = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Zone'})['elements'][:3]
if not zones:
    raise SystemExit ('No zone in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetZoneReductions', {'elements': zones}))
