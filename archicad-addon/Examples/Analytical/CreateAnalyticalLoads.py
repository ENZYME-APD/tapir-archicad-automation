"""Puts a 10 kN point load and a 5 kN/m line load on the first curve member with CreateAnalyticalLoads.

Needs at least one analytical curve member and one load case; the loads go into the first load case. Forces are in
newtons, downward along the global z axis. Requires Archicad 27 or later (the command itself works from Archicad 26).
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
loadCases = [c for c in aclib.RunTapirCommand ('GetAnalyticalLoadCases', {})['loadCases'] if 'error' not in c]
if not curveMembers or not loadCases:
    raise SystemExit ('No analytical curve member in the current view or no load case in the project.')
memberId = curveMembers[0]['elementId']
loadCaseId = loadCases[0]['loadCaseId']
curve = aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {
    'elements': [{'elementId': memberId}]
})['analyticalMembers'][0]['curveMember']['curve']

aclib.PrintJson (aclib.RunTapirCommand ('CreateAnalyticalLoads', {
    'loads': [
        {
            'pointLoad': {
                'loadSettings': {
                    'loadCaseId': loadCaseId,
                    'loadForceType': 'Force',
                    'coordinateSystemType': 'Global',
                    'xValue': 0.0, 'yValue': 0.0, 'zValue': -10000.0
                },
                'loadedMemberPartRef': {'elemGuid': memberId, 'partRefType': 'BegVertex'}
            }
        },
        {
            'edgeLoad': {
                'loadSettings': {
                    'loadCaseId': loadCaseId,
                    'loadForceType': 'Force',
                    'loadDistributionType': 'Uniform',
                    'coordinateSystemType': 'Global',
                    'startValue': {'x': 0.0, 'y': 0.0, 'z': -5000.0},
                    'endValue': {'x': 0.0, 'y': 0.0, 'z': -5000.0}
                },
                'loadedMemberPartRef': {'elemGuid': memberId, 'partRefType': 'Edge'}
            },
            'customData': {'curve': [curve]}
        }
    ]
}))
