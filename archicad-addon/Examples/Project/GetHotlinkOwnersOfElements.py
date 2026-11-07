"""Prints for the first ten walls whether each comes from a hotlink, and from which, with GetHotlinkOwnersOfElements.

Needs at least one Wall. Requires Archicad 27 or later.
"""
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:10]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetHotlinkOwnersOfElements', {'elements': walls}))
