"""Prints the local coordinate system of the axis of the first curve member with GetLCSOfAnalyticalMembers.

Needs at least one analytical curve member. Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLCSOfAnalyticalMembers', {
    'memberParts': [{
        'memberPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'Edge'}
    }]
}))
