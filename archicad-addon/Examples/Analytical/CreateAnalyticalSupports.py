"""Places a fixed point support at the start of the first curve member with CreateAnalyticalSupports.

Needs at least one analytical curve member. Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateAnalyticalSupports', {
    'supports': [{
        'supportSettings': {
            'supportType': 'SimplePoint',
            'supportValues': {
                'xType': 'Rigid', 'yType': 'Rigid', 'zType': 'Rigid',
                'rxType': 'Rigid', 'ryType': 'Rigid', 'rzType': 'Rigid'
            },
            'coordinateSystem': 'Global'
        },
        'supportedMemberPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'BegVertex'}
    }]
}))
