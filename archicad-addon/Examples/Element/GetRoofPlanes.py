"""Reads the single planes of the first roofs with GetRoofPlanes; a multi-plane roof comes back plane by plane.

Needs at least one Roof.
"""
import aclib

roofs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Roof'}, debug = False)['elements'][:3]
if not roofs:
    raise SystemExit ('No roof in the project.')

aclib.RunTapirCommand ('GetRoofPlanes', {'elements': roofs})
