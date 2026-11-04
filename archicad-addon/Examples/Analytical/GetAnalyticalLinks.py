"""Links the first two curve members with a rigid link and prints it with GetAnalyticalLinks.

Needs at least two analytical curve members. Deletes the link at the end.
Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if len (curveMembers) < 2:
    raise SystemExit ('Fewer than two analytical curve members in the current view.')
links = aclib.RunTapirCommand ('CreateAnalyticalLinks', {
    'links': [{
        'linkSettings': {
            'geometry': 'Link1D',
            'type': 'Rigid',
            'rigidSettings': {'startConnectionType': 'RigidConnection', 'endConnectionType': 'HingeConnection'},
            'springSettings': {
                'direction': 'Global',
                'translation': {'kxType': 'Rigid', 'kyType': 'Rigid', 'kzType': 'Rigid'},
                'rotation': {'kxxType': 'Rigid', 'kyyType': 'Rigid', 'kzzType': 'Rigid'}
            }
        },
        'startElementPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'EndVertex'},
        'endElementPartRef': {'elemGuid': curveMembers[1]['elementId'], 'partRefType': 'BegVertex'}
    }]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalLinks', {
    'elements': links
}))

aclib.RunTapirCommand ('DeleteElements', {'elements': links})
