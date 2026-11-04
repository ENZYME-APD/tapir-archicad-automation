"""Converts a shortest link to the axis of a curve member into a link to a fixed point with ConvertAnalyticalLinksToNonShortest.

Needs at least two analytical curve members. Creates the shortest link from the end of the first member to the axis
(edge) of the second one, then converts it; the converted link stays in the project.
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
            'rigidSettings': {'startConnectionType': 'RigidConnection', 'endConnectionType': 'RigidConnection'},
            'springSettings': {
                'direction': 'Global',
                'translation': {'kxType': 'Rigid', 'kyType': 'Rigid', 'kzType': 'Rigid'},
                'rotation': {'kxxType': 'Rigid', 'kyyType': 'Rigid', 'kzzType': 'Rigid'}
            }
        },
        'startElementPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'EndVertex'},
        'endElementPartRef': {'elemGuid': curveMembers[1]['elementId'], 'partRefType': 'Edge'}
    }]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('ConvertAnalyticalLinksToNonShortest', {
    'elements': links
}))
