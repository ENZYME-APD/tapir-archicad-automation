"""Prints the key-value pairs that scripts stored on the first ten walls (SetElementUserData) with GetElementUserData.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:10]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetElementUserData', {'elements': walls}))
