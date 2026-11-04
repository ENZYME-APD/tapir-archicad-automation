"""Renames the first add-on viewpoint root with ChangeAddOnViewpoints, reads it back, then restores its name.

Needs an add-on viewpoint. Roots can't be changed in a Teamwork project. Requires Archicad 27 or later.
"""
import aclib

roots = aclib.RunTapirCommand ('GetAddOnViewpointRoots')['viewpointIds']
if not roots:
    raise SystemExit ('No add-on viewpoint in the project.')
root = aclib.RunTapirCommand ('GetAddOnViewpoints', {'viewpointIds': roots[:1]})['viewpoints'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ChangeAddOnViewpoints', {
    'viewpoints': [{'viewpointId': root['viewpointId'], 'displayName': root['displayName'] + ' (Tapir)'}]
}))
aclib.RunTapirCommand ('GetAddOnViewpoints', {'viewpointIds': roots[:1]})

aclib.PrintJson (aclib.RunTapirCommand ('ChangeAddOnViewpoints', {
    'viewpoints': [{'viewpointId': root['viewpointId'], 'displayName': root['displayName']}]
}))
