"""Creates an element set of the first two Walls, reads its elements and custom data with GetElementSetData, then deletes it.

Deleting the set keeps the walls. Needs at least two Walls.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Needs at least two walls.')
elementSets = aclib.RunTapirCommand ('CreateElementSets', {'elementSets': [{'elements': walls}]})['elementSets']
elementSets = [s for s in elementSets if 'elementSetId' in s]

aclib.PrintJson (aclib.RunTapirCommand ('GetElementSetData', {'elementSets': elementSets}))

aclib.RunTapirCommand ('DeleteElementSets', {'elementSets': elementSets})
