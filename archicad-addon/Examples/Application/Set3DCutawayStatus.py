"""Turns 3D Cutaway on with Set3DCutawayStatus, reads it back, then restores the original state.

In Archicad 25-27 the 3D window may need to be open.
"""
import aclib

original = aclib.RunTapirCommand ('Get3DCutawayStatus')

aclib.PrintJson (aclib.RunTapirCommand ('Set3DCutawayStatus', {'cutawayEnabled': True, 'cuttingPlanesVisible': True}))
aclib.RunTapirCommand ('Get3DCutawayStatus')

aclib.PrintJson (aclib.RunTapirCommand ('Set3DCutawayStatus', {
    'cutawayEnabled': original['cutawayEnabled'],
    'cuttingPlanesVisible': original['cuttingPlanesVisible']
}))
