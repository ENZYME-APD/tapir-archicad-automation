"""Checks which slabs still show a skin in the Core Only Partial Structure Display mode with GetVisibleSkinStatus.

The current window is not changed. Needs at least one Slab.
"""
import aclib

slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab'})['elements']
if not slabs:
    raise SystemExit ('No slab in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetVisibleSkinStatus', {
    'elements': slabs,
    'structureDisplay': 'CoreOnly'
}))
