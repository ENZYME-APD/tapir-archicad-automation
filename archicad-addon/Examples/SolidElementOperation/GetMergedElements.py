"""Gets the elements each of the first three slabs is merged with using GetMergedElements.

Needs at least one Slab.
"""
import aclib

slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab'})['elements'][:3]
if not slabs:
    raise SystemExit ('No slab in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMergedElements', {'elements': slabs}))
