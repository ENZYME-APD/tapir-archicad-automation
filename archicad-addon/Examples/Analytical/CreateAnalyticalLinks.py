"""Connects the end of the first curve member to the start of the second one with a rigid link using CreateAnalyticalLinks.

Needs at least two analytical curve members. Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if len (curveMembers) < 2:
    raise SystemExit ('Fewer than two analytical curve members in the current view.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateAnalyticalLinks', {
    'links': [{
        'linkSettings': {
            'geometry': 'Link1D',
            'type': 'Rigid',
            'rigidSettings': {'startConnectionType': 'RigidConnection', 'endConnectionType': 'RigidConnection'},
            'springSettings': {
                'direction': 'Global',
                'translation': {'kxType': 'Rigid', 'kyType': 'Rigid', 'kzType': 'Rigid'},
                'rotation': {'kxxType': 'Rigid', 'kyyType': 'Rigid', 'kzzType': 'Rigid'}
            }
        },
        'startElementPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'EndVertex'},
        'endElementPartRef': {'elemGuid': curveMembers[1]['elementId'], 'partRefType': 'BegVertex'}
    }]
}))
