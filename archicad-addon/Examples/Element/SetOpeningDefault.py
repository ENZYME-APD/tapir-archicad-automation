"""Changes the floor plan display mode of the Opening tool defaults with SetOpeningDefault, then restores it.

Requires Archicad 29 or later.
"""
import aclib

default = aclib.RunTapirCommand ('GetOpeningDefault', {})
oldMode = default['floorPlanParameters'].get ('floorPlanDisplayMode', 'Symbolic')

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningDefault', {
    'floorPlanParameters': {'floorPlanDisplayMode': 'Projected'}
}))
aclib.RunTapirCommand ('GetOpeningDefault', {})

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningDefault', {
    'floorPlanParameters': {'floorPlanDisplayMode': oldMode}
}))
