"""Reads the hole each Skylight cuts into its host Roof or Shell with GetSkylightHolePolygons.

Needs at least one Skylight.
"""
import aclib

skylights = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Skylight'}, debug = False)['elements']
if not skylights:
    raise SystemExit ('No skylight in the project.')

aclib.RunTapirCommand ('GetSkylightHolePolygons', {'elements': skylights})
