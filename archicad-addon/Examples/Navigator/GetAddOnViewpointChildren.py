"""Lists the direct children of every add-on viewpoint root with GetAddOnViewpointChildren.

Requires Archicad 27 or later.
"""
import aclib

roots = aclib.RunTapirCommand ('GetAddOnViewpointRoots')['viewpointIds']
if not roots:
    raise SystemExit ('No add-on viewpoint in the project.')

for root in roots:
    aclib.PrintJson (aclib.RunTapirCommand ('GetAddOnViewpointChildren', {'viewpointId': root['viewpointId']}))
