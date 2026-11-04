"""Prints the first curve and surface member of the Structural Analytical Model of the current view with GetAnalyticalMemberDetails.

Needs at least one analytical member. Reading the members of the current view requires Archicad 27 or later.
"""
import aclib

members = aclib.RunTapirCommand ('GetAnalyticalMembers', {})
elements = members['curveMembers'][:1] + members['surfaceMembers'][:1]
if not elements:
    raise SystemExit ('No analytical member in the current view.')

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {
    'elements': elements
}))
