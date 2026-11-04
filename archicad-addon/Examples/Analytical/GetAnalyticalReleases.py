"""Prints the releases of the first curve members of the Structural Analytical Model with GetAnalyticalReleases.

Needs at least one curve member. Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalReleases', {
    'elements': curveMembers[:3]
}))
