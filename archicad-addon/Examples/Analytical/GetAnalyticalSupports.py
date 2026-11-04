"""Places a pinned support at the start of the first curve member and prints it with GetAnalyticalSupports.

Needs at least one analytical curve member. Deletes the support at the end.
Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')
supports = aclib.RunTapirCommand ('CreateAnalyticalSupports', {
    'supports': [{
        'supportSettings': {
            'supportType': 'SimplePoint',
            'supportValues': {
                'xType': 'Rigid', 'yType': 'Rigid', 'zType': 'Rigid',
                'rxType': 'Free', 'ryType': 'Free', 'rzType': 'Free'
            },
            'coordinateSystem': 'Global'
        },
        'supportedMemberPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'BegVertex'}
    }]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalSupports', {
    'elements': supports
}))

aclib.RunTapirCommand ('DeleteElements', {'elements': supports})
