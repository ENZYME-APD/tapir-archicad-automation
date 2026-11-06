"""Creates an element set of the first two Walls with CreateElementSets, then deletes it.

An element set is a hidden collection of elements; deleting it keeps the walls. Needs at least two Walls.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Needs at least two walls.')

elementSets = aclib.RunTapirCommand ('CreateElementSets', {'elementSets': [{'elements': walls}]})['elementSets']
aclib.PrintJson (elementSets)
aclib.RunTapirCommand ('GetElementSetsOfElements', {'elements': walls})

aclib.RunTapirCommand ('DeleteElementSets', {'elementSets': [s for s in elementSets if 'elementSetId' in s]})
