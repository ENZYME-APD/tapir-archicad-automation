"""Reads the boundaries (bounding elements, neighbour zones ...) of the first zones with GetZoneBoundaries.

Needs at least one Zone.
"""
import aclib

zones = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Zone'})['elements'][:3]
if not zones:
    raise SystemExit ('No zone in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetZoneBoundaries', {'zones': zones}))
