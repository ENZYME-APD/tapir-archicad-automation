"""Puts a 10 kN point load on the start of the first curve member and prints it with GetAnalyticalLoads.

Needs at least one analytical curve member and one load case. Deletes the load at the end.
Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
loadCases = [c for c in aclib.RunTapirCommand ('GetAnalyticalLoadCases', {})['loadCases'] if 'error' not in c]
if not curveMembers or not loadCases:
    raise SystemExit ('No analytical curve member in the current view or no load case in the project.')
loads = aclib.RunTapirCommand ('CreateAnalyticalLoads', {
    'loads': [{
        'pointLoad': {
            'loadSettings': {
                'loadCaseId': loadCases[0]['loadCaseId'],
                'loadForceType': 'Force',
                'coordinateSystemType': 'Global',
                'xValue': 0.0, 'yValue': 0.0, 'zValue': -10000.0
            },
            'loadedMemberPartRef': {'elemGuid': curveMembers[0]['elementId'], 'partRefType': 'BegVertex'}
        }
    }]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalLoads', {
    'elements': loads
}))

aclib.RunTapirCommand ('DeleteElements', {'elements': loads})
