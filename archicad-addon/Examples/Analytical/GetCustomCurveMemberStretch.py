"""Prints the lengths added to the start and end of the first curve members with GetCustomCurveMemberStretch.

Needs at least one analytical curve member. Requires Archicad 29 or later.
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')

aclib.PrintJson (aclib.RunTapirCommand ('GetCustomCurveMemberStretch', {
    'elements': curveMembers[:3]
}))
