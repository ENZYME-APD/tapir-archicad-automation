"""Prints the start node and the axis curve of the first curve member with GetAnalyticalMemberPartGeometries.

Needs at least one analytical curve member. Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')
memberId = curveMembers[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalMemberPartGeometries', {
    'memberPartRefs': [
        {'elemGuid': memberId, 'partRefType': 'BegVertex'},
        {'elemGuid': memberId, 'partRefType': 'Edge'}
    ]
}))
