"""Reads the single planes of the first roofs with GetRoofPlanes; a multi-plane roof comes back plane by plane.

Needs at least one Roof.
"""
import aclib

roofs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Roof'})['elements'][:3]
if not roofs:
    raise SystemExit ('No roof in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetRoofPlanes', {'elements': roofs}))
