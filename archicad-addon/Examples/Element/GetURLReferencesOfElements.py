"""Prints the web link (URL and description) of every wall with GetURLReferencesOfElements.

Walls without a link get an APIERR_NOUSERDATA error item.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetURLReferencesOfElements', {'elements': walls}))
