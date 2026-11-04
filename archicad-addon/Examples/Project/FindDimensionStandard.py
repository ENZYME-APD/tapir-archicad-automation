"""Finds the first dimension standard by its name with FindDimensionStandard and prints its index and guid.

Requires Archicad 27 or later.
"""
import aclib

standards = aclib.RunTapirCommand ('GetDimensionStandards', {}, debug = False)['dimensionStandards']
if not standards:
    raise SystemExit ('No dimension standard in the project.')

aclib.RunTapirCommand ('FindDimensionStandard', {'name': standards[0]['name']})
