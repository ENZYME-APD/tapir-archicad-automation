"""Prints the database (window type, id and name) that contains each of the first two walls, with GetContainingDatabases.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetContainingDatabases', {'elements': walls}))
