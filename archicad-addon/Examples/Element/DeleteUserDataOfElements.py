"""Deletes the user data the Tapir Add-On stored on a wall with DeleteUserDataOfElements.

The user data of other add-ons is kept. Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('DeleteUserDataOfElements', {'elements': walls}))
