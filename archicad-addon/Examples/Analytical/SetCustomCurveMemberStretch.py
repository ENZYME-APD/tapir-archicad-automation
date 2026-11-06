"""Stretches the first curve member by 0.1 m at both ends with SetCustomCurveMemberStretch.

Needs at least one analytical curve member. Reads the stretch back and restores the old one.
Requires Archicad 29 or later.
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')
memberId = curveMembers[0]['elementId']
old = aclib.RunTapirCommand ('GetCustomCurveMemberStretch', {'elements': [{'elementId': memberId}]})['stretches'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetCustomCurveMemberStretch', {
    'stretches': [{'elementId': memberId, 'begAdditionalLength': 0.1, 'endAdditionalLength': 0.1}]
}))
aclib.RunTapirCommand ('GetCustomCurveMemberStretch', {'elements': [{'elementId': memberId}]})

aclib.PrintJson (aclib.RunTapirCommand ('SetCustomCurveMemberStretch', {
    'stretches': [{
        'elementId': memberId,
        'begAdditionalLength': old['begAdditionalLength'],
        'endAdditionalLength': old['endAdditionalLength']
    }]
}))
