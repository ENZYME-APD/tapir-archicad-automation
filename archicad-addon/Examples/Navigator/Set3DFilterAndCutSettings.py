"""Hides the walls in the 3D window with Set3DFilterAndCutSettings, then restores the original filter.

Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('Get3DFilterAndCutSettings')['settings']

# Only the listed element types change.
aclib.PrintJson (aclib.RunTapirCommand ('Set3DFilterAndCutSettings', {
    'settings': {'elemTypeFilter': [{'elementType': 'Wall', 'show': False}]},
    'rebuild': True
}))

aclib.PrintJson (aclib.RunTapirCommand ('Set3DFilterAndCutSettings', {'settings': original, 'rebuild': True}))
