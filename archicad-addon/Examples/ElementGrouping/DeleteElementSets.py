"""Creates an element set of the first two Walls and deletes it again with DeleteElementSets.

The walls are kept. Only element sets created by Tapir can be deleted. Needs at least two Walls.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Needs at least two walls.')
elementSets = aclib.RunTapirCommand ('CreateElementSets', {'elementSets': [{'elements': walls}]})['elementSets']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteElementSets', {'elementSets': [s for s in elementSets if 'elementSetId' in s]}))
