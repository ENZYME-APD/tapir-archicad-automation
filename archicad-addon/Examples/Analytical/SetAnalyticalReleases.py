"""Adds a hinge (free rotation around the local y axis) at the end of the first curve member with SetAnalyticalReleases.

Needs at least one curve member. Reads the release back and restores the old one.
Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')
memberId = curveMembers[0]['elementId']
oldRelease = aclib.RunTapirCommand ('GetAnalyticalReleases', {'elements': [{'elementId': memberId}]})['releases'][0]['release']

hinge = {
    'ux': 'Rigid', 'uy': 'Rigid', 'uz': 'Rigid', 'fix': 'Rigid', 'fiy': 'Free', 'fiz': 'Rigid',
    'amPartRef': {'elemGuid': memberId, 'partRefType': 'EndVertex'}
}
aclib.PrintJson (aclib.RunTapirCommand ('SetAnalyticalReleases', {
    'releases': [{
        'elementId': memberId,
        'release': {'guid': oldRelease['guid'], 'releaseDescriptions': oldRelease['releaseDescriptions'] + [hinge]}
    }]
}))
aclib.RunTapirCommand ('GetAnalyticalReleases', {'elements': [{'elementId': memberId}]})

aclib.PrintJson (aclib.RunTapirCommand ('SetAnalyticalReleases', {
    'releases': [{'elementId': memberId, 'release': oldRelease}]
}))
